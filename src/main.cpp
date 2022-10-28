/*
  Titre      : Test
  Auteur     : Crepin Vardin Fouelefack
  Date       : 27/10/2022
  Description: description
  Version    : 0.0.1
*/

#include <Arduino.h>
#include <SPI.h>

// Include pour la carte SD
#include <SD.h>
// Include pour le capteur AHTX0
#include <Adafruit_AHTX0.h>
// 03 Includes pour faire fonctionner le rtc
#include "RTClib.h"
// Librairie de branchement
#include "WIFIConnector_MKR1000.h"

//bool Status = false;
String Status = "";

#include "MQTTConnector.h"


const int CHIPSELECT = 4;

//Broches pour le chauffage et la climatisation
int PinChauffage     = 2; 
int PinClimatisation = 3;

RTC_DS3231 rtc;
Adafruit_AHTX0 aht;

String buffer;
String valHumidite, valTemperature, unixTime = "";

int espace1, espace2 = 0;

/*unsigned int const fileWriteTime = 5000; // 5 secondes
unsigned int const timeToSend = 20000;   // 20 secondes*/

unsigned int const fileWriteTime = 4000; // 4 secondes
unsigned int const timeToSend = 10000;   // 10 secondes

#include <Arduino.h>

boolean runEveryShort(unsigned long interval)

{
    static unsigned long previousMillis = 0;

    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval)
    {
        previousMillis = currentMillis;

        return true;
    }

    return false;
}

boolean runEveryLong(unsigned long interval)

{
    static unsigned long previousMillis = 0;

    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval)
    {
        previousMillis = currentMillis;

        return true;
    }

    return false;
}

void dataToSend(String chaine)
{
    // Retrouver la position des 2 espaces vide dans une chaine de caracteres
    for (int i = 0; i < chaine.length(); i++)
    {
        if (chaine.charAt(i) == ' ')
        {
            if (espace1 == 0)
            {
                espace1 = i;
            }
            else
            {
                espace2 = i;
            }
        }
    }

    // Obtenir le 1er char dans un string
    char firstCharOfString = chaine.charAt(0);

    // Obtenier l'index  du 1er char dans un string
    int firstCharIndexOfString = chaine.indexOf(firstCharOfString);

    // Extraction de  l'unixTime
    unixTime = chaine.substring(firstCharIndexOfString, espace1);

    // Extraction de  la temperature
    valTemperature = chaine.substring(espace1 + 1, espace2);

    // Extraction de  l'hunidite
    valHumidite = chaine.substring(espace2, chaine.length());
}

void setup()
{
    wifiConnect(); 
    MQTTConnect(); 

    //OUTPUT pour actionner les broches
    pinMode(PinChauffage, OUTPUT);
    pinMode(PinClimatisation, OUTPUT);

    // Open serial communications and wait for port to open:
    Serial.begin(9600);

    // see if the card is present and can be initialized:
    if (!SD.begin(CHIPSELECT))
    {
        Serial.println("Card failed, or not present");
        // don't do anything more:
        while (1)
            ;
    }

    //AHT20
    if (!aht.begin())
    {
        Serial.println("Impossible de trouver l'AHT ? Vérifiez le câblage");
        while (1)
            delay(10);
    }
    Serial.println(" AHT20 trouve");

    //RTC
    if (!rtc.begin())
    {
        Serial.println("Couldn't find RTC");
        Serial.flush();
        while (1)
            delay(10);
    }

    if (rtc.lostPower())
    {
        Serial.println("RTC lost power, let's set the time!");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    File dataFile = SD.open("datalog.txt", FILE_WRITE | O_TRUNC);
    dataFile.close();
}

void loop()
{
    // put your main code here, to run repeatedly:

    //Ecout des messages de broker MQTT
    ClientMQTT.loop();

    //rtc
    DateTime now = rtc.now();

    //AHT20
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);

    String stringToWrite = String(now.unixtime()) + " " + temp.temperature + " " + humidity.relative_humidity + "\n";


    if (runEveryShort(fileWriteTime))
    {
        File dataFile = SD.open("datalog.txt", FILE_WRITE);
        // if the file is available, write to it:
        if (dataFile)
        {
            //Sauvegarde sur la carte SD
            dataFile.print(stringToWrite);
            //delai de 5 secondes
            delay(5000);

            //Impression de la chaine contenue dans la carte SD
            Serial.print(stringToWrite);

            dataFile.close();
        }
        else
        {
            Serial.println("ouverture d'erreur datalog.txt");
        }
    }

    if (runEveryLong(timeToSend))
    {
        //Ouverture du fichier en mode lecture
        File dataFile = SD.open("datalog.txt", FILE_READ); 

        if (dataFile)
        {
            // read from the file until there's nothing else in it:
            while (dataFile.available())
            {
                buffer = dataFile.readStringUntil('\n');
                Serial.println(buffer);
                dataToSend(buffer);

                //Creation de la chaine a envoyee
                unixTime = unixTime + "000";
                appendUnixTime(unixTime.toFloat()+10800);
                appendPayload("Temp", valTemperature.toFloat());
                appendPayload("Humd", valHumidite.toFloat());
                //Envoie de la chaine
                sendPayload();
                //Bref attente pour recevoir le RPC
                delay(200);
            }
            
            dataFile.close();
            SD.remove("datalog.txt");
        }
        else
        {
            // if the file didn't open, print an error:
            Serial.println("ouverture d'erreur datalog.txt");
        }
    }

    //Alumage de la climatisation a une certaine condition
    if (Status == "trueClim")
    {
      digitalWrite(PinClimatisation,HIGH);
      digitalWrite(PinChauffage,LOW);
    }

    //Alumage du chauffage a une certaine condition
    if (Status == "trueCh")
    {
      digitalWrite(PinChauffage,HIGH);
      digitalWrite(PinClimatisation,LOW);
    }

    //Affichage du status actuel dans le moniteur serie pour debogage
    Serial.println(Status);

    
}