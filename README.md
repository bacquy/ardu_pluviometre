# ardu_pluviometre
Pluviometre electronique avec Wemos et MQTT

Modèle 3D: https://www.thingiverse.com/make:1116022

Ce code envoie les donnée vers un broker MQTT (mosquito dans mon cas)
Il permet de collecter le nombre de bascule, le nombre de ml passé dans l'entonnoir ainsi que le nombre de mm/m² (le plus important)
Il envoie le total de ces variables et par averse il compte aussi le nombre d'averse.

Le code est un peu crado je vous laisse le corriger si besoin.

ATTENTION aux variables MQTT et la surface de l'entonnoir est hardcodé et son topic MQTT n'est pas exploité (to be fix)

--------------
Electronique Rain Gauge with Wemos and MQTT

3D print model: https://www.thingiverse.com/make:1116022

This code sends the data to an MQTT broker (Mosquitto in my case). 
It collects the number of switch toggles, the amount of milliliters passed through the funnel, as well as the number of mm/m² (the most important). 
It sends the total of these variables, and additionally counts the number of downpours.

The code is a bit messy, so I'll let you clean it up if necessary.

CAUTION !!! MQTT variables and the funnel surface is hardcoded, and its MQTT topic is not utilized (to be fixed)

La bise.

Johann.
