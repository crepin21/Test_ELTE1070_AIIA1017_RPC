#include <Arduino.h>
#include <Adafruit_AHTX0.h>
#include "WIFIConnector_MKR1000.h"
#include "MQTTConnector.h"
//#include <Arduino_MKRMEM.h>

//AHT20
Adafruit_AHTX0 aht;
//Bouclier memoire(carte sd) ET Rtc
#include <SPI.h>                  
#include <SD.h>

#include "RTClib.h"                   //Rtc uniquement
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

const int chipSelect = 4; //Broche pour initialiser la carte SD

  // Fonction pour ecrire les donnees sur la carte SD A CHQ 5sec
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

  // Fonction Pour envoyer sur thingsboard a chq 1min
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

void setup() {
  Serial.begin(9600);
 while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
//wifiConnect();                  //Branchement au réseau WIFI
//MQTTConnect();                  //Branchement au broker MQTT
#ifndef ESP8266
  while (!Serial); // wait for serial port to connect. Needed for native USB
#endif

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  Serial.println("Adafruit AHT10/AHT20 demo!");

  Serial.print("Initializing SD card...");
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1);
  }
  Serial.println("card initialized.");


//Verifie la connection du AHT20
  if (! aht.begin()) {
    Serial.println("Could not find AHT? Check wiring");
    while (1) delay(10);
  }
  //Si AHT20 branche correctement
  Serial.println("AHT10 or AHT20 found");

}

void loop() 
{
void sendMQTTData(String dataFile)

{
    File myFile = SD.open(dataFile,FILE_READ);
    digitalWrite(LED_BUILTIN,HIGH);
    wifiConnect();
    MQTTConnect();

    if(myFile)
    {
      String myString = myFile.readString();
      int stringLength = myString.length();
      char dataString[stringLength];
      myString.toCharArray(dataString, stringLength);
      int count =0;
      char * token = strtok(dataString, "\n");

      // loop through the string to extract all other tokens
      while( token != NULL ) {
        //String dummyString =  token; //printing each token
        Serial.println(count++);
        String dummyString = token;
        sendMQTTStringAsPayload(dummyString);
        Serial.println(dummyString);
        token = strtok(NULL, "\n");

      }
      Serial.println("end");
      myFile.close();
      removeFile("datalog.txt");
      status=WL_IDLE_STATUS;

 

      WiFi.disconnect();

      WiFi.end();

      digitalWrite(LED_BUILTIN,LOW);

    }

 

}

  // Suppression de tous les donnees(et du fichier) a chaque debut de la loop donc apres chaque envoie sur Things
/* if(SD.exists("datalog.txt")){
  SD.remove("datalog.txt");
 }  */
  //Chaine ecrite dans la carte sd
  String dataString = "";

  if(runEveryShort(5000)){   //5 sec

    DateTime now = rtc.now();

  //Declaration d'objets pour la temp et l'H%
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data
    
    // Lecture des broches Analog 0,1 et 2 et ecriture dans la chaine dataString:
      dataString = "temp";
      dataString += temp.temperature ;
      dataString += "-Humd";
      dataString += humidity.relative_humidity ;
      dataString += "--";
      dataString += now.year() ;
      dataString += "-";
      dataString += now.month() ;
      dataString += "-";
      dataString += now.day() ;
      dataString += "--";
      dataString += now.hour() ;
      dataString += "-";
      dataString += now.minute() ;
      dataString += "-";
      dataString += now.second() ;

    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    File dataFile = SD.open("datalog.txt", FILE_WRITE);        //Creation du fichier datalog

    // if the file is available, write to it:
    if (dataFile) {
      dataFile.println(dataString);
      dataFile.close();
      // print to the serial port too:
      Serial.println(dataString);
    }
    // if the file isn't open, pop up an error:
    else {
      Serial.println("error opening datalog.txt");
    }
  }

  if(runEveryLong(60000)){  //1 min

    appendPayload("Temperature", dataString);  //Ajout de la donnée température au message MQTT
    sendPayload();
  }

}