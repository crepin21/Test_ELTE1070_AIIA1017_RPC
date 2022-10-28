/*
  Titre      : MQTTConnector.h
  Auteur     : André Roussel
  Date       : 05/11/2021
  Description: Utilitaire de branchement a un Broker MQTT une fois une connexion établisa l'internet via WIFI, GSM, etc...
  Droits     : Reproduction permise pour usage pédagogique
  Version    : 0.0.1
*/

//Code modifier de Andre Roussel

// MQTT by Joel Gaehwiler
#include <MQTT.h>

// Constante nécessaire pour le branchement au Broker MQTT

const char MQTT_SERVER[] = SECRET_MQTT_SERVER_IP;     // Adresse IP du broker MQTT
const int MQTT_SERVER_PORT = SECRET_MQTT_SERVER_PORT; // Port du broker MQTT

// Pramètre d'identification de l'objet
const char TOKEN[] = SECRET_TOKEN;         // Token d'accès de l'objet
const char DEVICE_ID[] = SECRET_DEVICE_ID; // Numéro d'indentification de l'objet provenant de Thingsboard

MQTTClient ClientMQTT; // Création d'un client MQTT pour l'échange de donnée entre l'objet IDO et le broker MQTT

String Payload = "{"; // Chaine de caractère qui contiendra le message envoyer de l'objet vers thingsboard
bool first = false;

void messageReceived(String &topic, String &payload)
{

  Serial.println("Message Recu");
  Serial.println(payload);
  Serial.println(topic);

  Serial.println(payload.substring(11, 12));
  Serial.println(payload.substring(28, 29));
  Serial.println("Status");

  if (payload.substring(28, 29) == "t")
  {
    Serial.println("HighClim");
    Status = "trueClim";
  }
  if(payload.substring(33, 34) == "c")
  {
    Serial.println("HighChauff");
    Status = "trueCh";
  }
}

// Fonctionnalité de branchement utilisant le protocole MQTT

void MQTTConnect()
{

  ClientMQTT.begin(MQTT_SERVER, MQTT_SERVER_PORT, ClientWIFI);
  ClientMQTT.onMessage(messageReceived);

  while (!ClientMQTT.connect(DEVICE_ID, TOKEN, ""))
  {
    Serial.print(".");
    delay(1000);
  }

  // traitement des message recu

  Serial.println("\nBranché au broker MQTT!\n");
  delay(1000);

  ClientMQTT.subscribe("v1/devices/me/rpc/request/+");
}

// Affiche la payload qui est envoyé ainsi que sa longeur

/*
  Fonctionnalité qui permet de créer la chaine de données a envoyé au broker MQTT
  Pour le moment, cette chaine ne doit pas dépassé un maximum de 100 caractère
*/


void appendPayload(String Name, float Val)
{

  if (first)
  {
    Payload += ",";
  }

  Payload += "\"";
  Payload += Name;
  Payload += "\": ";
  Payload += Val;
  first = true;
}

//Construction de la chaine pour le UnixTime
void appendUnixTime(float value)
{
  first = false;
  Payload = "{\"ts\":";
  Payload += value;
  Payload += ",\"values\":{";
}

/*
  Fonctionnalité qui permet l'envoie de la chaine de caractères sous la forme de paires
  Nom associé a la donnée, Valeur de al donnée
*/

void sendPayload()
{
  char attributes[200];
  Payload += "}}";
  Payload.toCharArray(attributes, 200);
  ClientMQTT.publish("v1/devices/me/telemetry", attributes);

  Serial.print("Payload -> ");
  Serial.println(Payload);
  Serial.print("Payload length -> ");
  Serial.println(Payload.length());

}







