#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <uno_wifi.h>

#define SERIAL_BAUD 115200

#include <ArduinoOTA.h>
#include <Arduino.h>
#include <ESP_Scheduler.h>
#include <Task.h>
#include <pins_arduino.h>

String codeVersion = "Version 1.1 Feb 2020 by Gilmore";

// Create the ESP Web Server on port 80
WiFiServer WebServer(80);
// Web Client
WiFiClient client;

//Initiate the uno wifi comms library
uno_wifi esp(0);

//Control values
short int temperature             = 0;        //From DHT sensor inside coop
short int humidity                = 0;           //From DHT sensor inside coop
short int changeDoorState         = 0;    //Toggle door position, 0=stop, 1=open, 2=close
short int doorState               = 0;          //0=closed, 1=open, 2=open slightly
short int brightness              = 0;         //Coop internal LED brightness value
short int previousBrightness      = 0;
unsigned long sunset              = 0;     //sets time at sunset
short int lightLevel              = 0;        //Intensity of daylight from Photosensitive LDR Sensor Module
long twilightLightLevel           = 1010; //Intensity from Photosensitive LDR Sensor Module that will trigger the door to open and close
unsigned long ms                  = 0;           //Records time in millis
unsigned long previousms          = 0;   //records time that sensor loop was previously run
unsigned long SENSOR_INTERVAL     = 1800000;
bool lightTrigger                 = 1;      //Determines if the door will automatically open and close depending on daylight condition
unsigned long lightFadeDuration   = 1800000;      //Fade time duration of the coop room LED in ms
const char SUNLIGHT_LEVEL         = 'S';
const String COOP_LIGHT_ON        = "L1";
const String COOP_LIGHT_OFF       = "L0";
const String COOP_LIGHT_DIM_UP    = "L+";
const String COOP_LIGHT_DIM_DOWN  = "L-";
String coopLight                  = COOP_LIGHT_OFF;
const String STOP_DOOR            = "O0";
const String OPEN_DOOR            = "O1";
const String CLOSE_DOOR           = "O2";
const String DOOR_CLOSED          = "O0";
const String DOOR_OPEN            = "O1";
const String DOOR_AJAR            = "O2";
const String REFRESH              = "REF";
const String LIGHT_TRIGGER_ON     = "R1";
const String LIGHT_TRIGGER_OFF    = "R0";
const char TEMPERATURE            = 'T';
const char HUMIDITY               = 'H';
const unsigned int NIGHT_DURATION = 32400000;
const unsigned int AWAKE_TIME     = 50400000;
const char AWAKE_MESSAGE          = 'A';
unsigned long daylightHours       = 0;



class ServerTask : public Task {
  private:
    void processRequest(String request) {
      if (request.indexOf("/stopDoor") != -1) {
        esp.serialSend(STOP_DOOR);
      }
      if (request.indexOf("/OpenDoor") != -1) {
        esp.serialSend(OPEN_DOOR);
      }
      if (request.indexOf("/closeDoor") != -1) {
        esp.serialSend(CLOSE_DOOR);
      }
      if (request.indexOf("/refresh") != -1) {
        esp.serialSend(REFRESH);
      }
      if (request.indexOf("/disableLightSensor") != -1) {
        esp.serialSend(LIGHT_TRIGGER_OFF);
      }
      if (request.indexOf("/enableLightSensor") != -1) {
        esp.serialSend(LIGHT_TRIGGER_ON);
      }
      if (request.indexOf("/lightOn") != -1) {
        esp.serialSend(COOP_LIGHT_ON);
      }
      if (request.indexOf("/lightOff") != -1) {
        esp.serialSend(COOP_LIGHT_OFF);
      }
      if (request.indexOf("/lightOn") != -1) {
        esp.serialSend(COOP_LIGHT_ON);
      }
      if (request.indexOf("/lightUp") != -1) {
        esp.serialSend(COOP_LIGHT_DIM_UP);
      }
      if (request.indexOf("/lightDn") != -1) {
        esp.serialSend("COOP_LIGHT_DIM_DOWN");
      }
    }

  public:
    void loop() {

      // Check if a user has connected
      client = WebServer.available();
      if (!client) {//restart loop
        yield();
        return;
      }

      // Wait until the user sends some data
      while (!client.available()) {
        yield();
        delay(1);
      }

      // Read the first line of the request
      String request = client.readStringUntil('\r\n');
      Serial.println(request);      ///-------------------------------TEST------------------------------------------------
      client.flush();

      // Process the request:
      processRequest(request);

      // Return the response
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html; charset=UTF-8");
      client.println("");
      client.println("<!DOCTYPE HTML>");
      client.println("<html>");
      client.println("<head>");
      client.println("<title>Chicken Central</title>");
      client.println("</head>");
      client.println("<body>");
      //<form action=\"/wifi\" method=\"get\"><button>Configure WiFi</button></form>
      client.println("<h1>Welcome to Chicken Central</h1></br>");
      //check the Door status
      switch (doorState) {
        case 1:
          client.print("<h2>Door is: OPEN</h2>");
          client.println("<form action='/' method=GET><button type=\"button\" onClick=\"window.location.reload();\">Close Door</button></form></br></br>");
          break;
        case 0:
          client.print("<h2>Door is: CLOSED</h2>");
          client.println("<form action='/' method=GET><INPUT TYPE=SUBMIT NAME='/openDoor' value='Open Door'></INPUT></form></br>");   //Do all the rest like this!!!!
          break;
        case 2:
          client.print("<h2>Door is: <strong>AJAR!</strong></h2>");
          client.println("<form action=\"/openDoor\" method=\"get\"><button type=\"button\" onClick=\"window.location.reload();\">Open Door</button></form></br>");
          client.println("<form action=\"/closeDoor\" method=\"get\"><button type=\"button\" onClick=\"window.location.reload();\">Close Door</button></form></br></br>");
          break;
      }
      client.println("<form action=\"/stopDoor\" method=\"get\"><button type=\"button\" onClick=\"window.location.reload();\">Stop Door</button></form></br>");
      if (coopLight == COOP_LIGHT_OFF) {
        client.println("<h2>The internal coop light is <strong>OFF</strong></h2></br><form action=\"/lightOn\" method=\"get\"><button type=\"button\" onClick=\"window.location.reload();\">Turn Coop Light On</button></form></br>");
      } else if (coopLight == COOP_LIGHT_ON) {
        client.println("<h2>The internal coop light is <strong>ON</strong></h2></br><form action=\"/lightOff\" method=\"get\"><button type=\"button\" onClick=\"window.location.reload();\">Turn Coop Light Off</button></form></br>");
      }
      client.println("<form action=\"/lightUp\" method=\"get\"><button type=\"button\" style=\"height:20px;width:20px\" onClick=\"window.location.reload();\">+</button></form>");
      client.println("<form action=\"/lightDn\" method=\"get\"><button type=\"button\" style=\"height:20px;width:20px\" onClick=\"window.location.reload();\">-</button></form></br>");
      client.println("</br>");
      client.println("The coop temperature is: ");
      client.println(temperature);
      client.println("&#176C</br>");
      client.println("The coop relative humidity is: ");
      client.println(humidity);
      client.println("</br><form action=\"/refresh\" method=\"get\"><button type=\"button\" onClick=\"window.location.reload();\">Refresh Sensor Data</button></form></br></br>");
      client.println("%</br></br>");
      client.println("Light level is at: :");
      client.println(lightLevel);
      client.println("</br><form action=\"/disableLightSensor\" method=\"get\"><button type=\"button\" onClick=\"window.location.reload();\">Disable Light Sensor</button></form></br>");
      client.println("</br>");
      client.println("</body>");
      client.println("</html>");

      delay(1);
      yield();
    }
} server_task;


class SerialTask : public Task {
  protected:
    void setup() {
      delay(6000);
      esp.serialSend("OK");
    }
    void loop()  {
      String recv = esp.serialGet();
      const char recvFirst = recv[0];

      if (recv.equals("SHUTDOWN")) {
        Serial.println(":: Shutdown unprocessed");
      } else if (recv.equals(COOP_LIGHT_ON)) {
        coopLight = COOP_LIGHT_ON;
      } else if (recv.equals(COOP_LIGHT_OFF)) {
        coopLight = COOP_LIGHT_OFF;
      } else if (recv.equals(DOOR_CLOSED)) {
        doorState = 0;
      } else if (recv.equals(DOOR_OPEN)) {
        doorState = 1;
      } else if (recv.equals(DOOR_AJAR)) {
        doorState = 2;
      } else if (recv.equals(LIGHT_TRIGGER_ON)) {
        lightTrigger = LIGHT_TRIGGER_ON;
      } else if (recv.equals(LIGHT_TRIGGER_OFF)) {
        lightTrigger = LIGHT_TRIGGER_OFF;
      } else if (recvFirst == AWAKE_MESSAGE) {
        recv.remove(0);
        daylightHours = recv.toInt();
      } else if (recvFirst == SUNLIGHT_LEVEL) {
        recv.remove(0);
        lightLevel = recv.toInt();
      } else if (recvFirst == TEMPERATURE) {
        recv.remove(0);
        temperature = recv.toFloat();
      } else if (recvFirst == HUMIDITY) {
        recv.remove(0);
        humidity = recv.toFloat();
      }
      yield();
    }
} serial_task;

class OTA_Task : public Task {
  protected:
    void loop()  {
      ArduinoOTA.handle();
      yield();
    }
} ota_task;

void setup() {
  esp.begin(SERIAL_BAUD);

  //Initialise WiFiManager library
  WiFiManager wifiManager;
  //  wifiManager.resetSettings();

  //first parameter is name of access point, second is the password
  if (!wifiManager.autoConnect("ChickenCentral")) {
    ESP.restart();
  }

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("Chicken_Central");

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // No authentication by default
  //   ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  // Start the Web Server
  WebServer.begin();

  //Begin task scheduler
  ESPScheduler.start(&ota_task);
  ESPScheduler.start(&server_task);
  ESPScheduler.start(&serial_task);
  ESPScheduler.begin();

}

void loop() {

}
