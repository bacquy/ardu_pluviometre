// Arduino code for Wemos rain gauge and MQTT
// I used this 3d print project https://www.thingiverse.com/thing:4434857

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid = "Wifi_SSID";
const char* password = "Wifi_password";
const char* mqttServer = "MQTT_IP_or_HOST";
const int mqttPort = 1883;
const char* mqttUser = "mqtt_user"; // Nom d'utilisateur MQTT
const char* mqttPassword = "mqtt_password"; // Mot de passe MQTT
const char* mqttTopicCount = "maison/jardin/pluviometre/compte_bascule";  // switch of the scale account
const char* mqttTopicMl = "maison/jardin/pluviometre/ml"; // total ml account
const char* mqttTopicMm = "maison/jardin/pluviometre/mm"; // total mm/m² account
const char* mqttTopicAverse = "maison/jardin/pluviometre/averse"; // total downpour ( or rain instance? not sure about translation )
const char* mqttTopicMlAverse = "maison/jardin/pluviometre/mlAverse"; // actual Downpour ml
const char* mqttTopicMmAverse = "maison/jardin/pluviometre/mmAverse"; // actual Downpour mm/m²
const char* mqttTopicCountAverse = "maison/jardin/pluviometre/compte_bascule_averse"; // actual Downpour switch of the scale account

// variables that you hav to define in you MQTT in retain True
const char* mqttTopicSurface = "maison/jardin/pluviometre/vars/surfaceEntonnoirCm2"; // horizontal surface of funnel variable /!\ Not used by the code actually it's a point to fix
const char* mqttTopicTimeout = "maison/jardin/pluviometre/vars/averseTimeout"; // time that define a new downpour
const char* mqttTopicMlPerBascule = "maison/jardin/pluviometre/vars/mlParBascule"; // ml per switch of the scale

const int pinD6 = D6; // d6 pin on wemos
volatile int stateD6 = LOW;
volatile int countD6 = 0;
volatile unsigned long lastDebounceTime = 0;
volatile unsigned long debounceDelay = 50;
float totalMl = 0.0;
float totalMmPerM2 = 0.0;
float surfaceEntonnoirCm2 = 0.0;
unsigned long lastChangeTime = 0;
bool isNewAverse = false;
unsigned long averseTimeout = 0;
float mlParBascule = 0.0;

// pour l'Averse en cours
float mlAverse = 0.0;
float mmAverse = 0.0;
int cptAverse = 0; // nombre de bascules par averse
int nbAverse = 0; // nombre d'averse

WiFiClient espClient;
PubSubClient client(espClient);


void publishAverse() {
  mlAverse = 0.0;
  mmAverse = 0.0;
  cptAverse = 0;
  nbAverse += 1;
  
  String aversePayload = String(nbAverse);
  client.publish(mqttTopicAverse, aversePayload.c_str(), true);
}

void ICACHE_RAM_ATTR onChangeD6() {
  if (millis() - lastDebounceTime > debounceDelay) {
        // Vérifier le timeout pour déterminer si une nouvelle averse a commencé
      if (isNewAverse && millis() - lastChangeTime > averseTimeout * 60000 || nbAverse == 0){
        isNewAverse = false;
        Serial.println("nouvelle averse");
        publishAverse();
      }
      
      countD6++;
      
      // Une bascule (changement d'état) correspond à mlParBascule ml
      totalMl += mlParBascule;

      // Conversion en millimètres par mètre carré (mm/m²)
      totalMmPerM2 = (totalMl / (surfaceEntonnoirCm2 / 10));

      // Envoi du nombre de changements d'état au topic MQTT avec retain=true
      String countPayload = String(countD6);
      client.publish(mqttTopicCount, countPayload.c_str(), true);

      // Envoi de la quantité d'eau en millilitres (ml) au topic MQTT avec retain=true
      String mlPayload = String(totalMl);
      client.publish(mqttTopicMl, mlPayload.c_str(), true);

      // Envoi des données en millimètres par mètre carré (mm/m²) au topic MQTT avec retain=true
      String mmPerM2Payload = String(totalMmPerM2);
      client.publish(mqttTopicMm, mmPerM2Payload.c_str(), true);


      Serial.print("Total des millimètres par mètre carré : ");
      Serial.println(totalMmPerM2);
      Serial.print("Nombre de changements d'état : ");
      Serial.println(countD6);
      Serial.print("Quantité d'eau en millilitres : ");
      Serial.println(totalMl);
      Serial.print("Nombre d'averses : ");
      Serial.println(nbAverse);

// PAR AVERSE


      // Ajouter la quantité d'eau à l'averse actuelle
      mlAverse += mlParBascule;

      // Ajouter mm à l'averse actuelle
      mmAverse = (mlAverse/(surfaceEntonnoirCm2/10));

      // Ajouter coompte de bascule à l'averse actuelle
      cptAverse += 1;

     
      // Envoi ml pour l'averse en cours au topic MQTT avec retain=true
      String mlAversePayload = String(mlAverse);
      client.publish(mqttTopicMlAverse, mlAversePayload.c_str(), true);

      // Envoi du mm de l'averse en cours au topic MQTT avec retain=true
      String mmAversePayload = String(mmAverse);
      client.publish(mqttTopicMmAverse, mmAversePayload.c_str(), true);

      // Envoi du compte de bascule pour l'averse en cours au topic MQTT avec retain=true
      String cptAversePayload = String(cptAverse);
      client.publish(mqttTopicCountAverse, cptAversePayload.c_str(), true);

      Serial.print("Total des millimètres par mètre carré pour l'averse en cours: ");
      Serial.println(mmAverse);
      Serial.print("Nombre de changements d'état pour l'averse en cours : ");
      Serial.println(cptAverse);
      Serial.print("Quantité d'eau en millilitres pour l'averse en cours : ");
      Serial.println(mlAverse);

      // Réinitialiser le timeout et marquer comme nouvelle averse
      isNewAverse = true;
      lastChangeTime = millis();
    }
    lastDebounceTime = millis();
  }

void setup() {
  Serial.begin(9600);

  pinMode(pinD6, INPUT_PULLUP);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);


  attachInterrupt(digitalPinToInterrupt(pinD6), onChangeD6, CHANGE);

  if (!client.connected()) {
    reconnect();
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();


}

void callback(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, mqttTopicSurface) == 0) {
    char buffer[length + 1];
    memcpy(buffer, payload, length);
    buffer[length] = '\0';
    surfaceEntonnoirCm2 = atof(buffer);
  } else if (strcmp(topic, mqttTopicTimeout) == 0) {
    char buffer[length + 1];
    memcpy(buffer, payload, length);
    buffer[length] = '\0';
    averseTimeout = atol(buffer);
  } else if (strcmp(topic, mqttTopicMlPerBascule) == 0) {
    char buffer[length + 1];
    memcpy(buffer, payload, length);
    buffer[length] = '\0';
    mlParBascule = atof(buffer);
  } else if (strcmp(topic, mqttTopicCount) == 0) {
    char buffer[length + 1];
    memcpy(buffer, payload, length);
    buffer[length] = '\0';
    countD6 = atoi(buffer);
  } else if (strcmp(topic, mqttTopicMl) == 0) {
    char buffer[length + 1];
    memcpy(buffer, payload, length);
    buffer[length] = '\0';
    totalMl = atof(buffer);
  } else if (strcmp(topic, mqttTopicMm) == 0) {
    char buffer[length + 1];
    memcpy(buffer, payload, length);
    buffer[length] = '\0';
    totalMmPerM2 = atof(buffer);
  } else if (strcmp(topic, mqttTopicAverse) == 0) {
    char buffer[length + 1];
    memcpy(buffer, payload, length);
    buffer[length] = '\0';
    nbAverse = atoi(buffer);
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect(mqttClientID, mqttUser, mqttPassword)) {
      Serial.println("Connected to MQTT");

      // Lecture des valeurs existantes des topics MQTT au démarrage
      client.subscribe(mqttTopicCount);
      client.subscribe(mqttTopicMl);
      client.subscribe(mqttTopicMm);
      client.subscribe(mqttTopicAverse);
      client.subscribe(mqttTopicSurface);
      client.subscribe(mqttTopicTimeout);
      client.subscribe(mqttTopicMlPerBascule);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}
