#include <ESP8266WiFi.h>

#define SERIAL_BAUD 115200

#include <Arduino.h>
#include <Scheduler.h>
#include <Task.h>
#include <pins_arduino.h>

String codeVersion = "Version 1.0 Dec 2019 by Gilmore";

// WiFi Router Login - change these to your router settings
const char* SSID = "SSID";
const char* password = "Password";

// Create the ESP Web Server on port 80
WiFiServer WebServer(80);
// Web Client
WiFiClient client;

//Control values
char changeDoorState = 0;    //Toggle door position, 0=stop, 1=open, 2=close
char doorState = 'o';       //c=closed, o=open, a=ajar, n=opening, s=closing
int lightLevel = 0;         //
int temperature = 0;        //From DHT sensor inside coop
int humidity = 0;           //From DHT sensor inside coop
bool roomLight = "0";    //Is internal light on(1) or off(0)

const char* STOP_DOOR = "D0", OPEN_DOOR = "D1", CLOSE_DOOR = "D2";
const char 

String recv = "";
bool data_in = false;

class ServerTask : public Task {
  public:
    void loop() {

      // Check if a user has connected
      client = WebServer.available();
      if (!client) {//restart loop
        return;
      }

      // Wait until the user sends some data
      while (!client.available()) {
        delay(1);
      }

      // Read the first line of the request
      String request = client.readStringUntil('\r\n');
      client.flush();

      // Process the request:
      if (request.indexOf("/changeDoorState=0") != -1) {
        //stop door
        Serial.println(STOP_DOOR);
      }
      if (request.indexOf("/changeDoorState=1") != -1) {
        //open door
        Serial.println(OPEN_DOOR);
      }
      if (request.indexOf("/changeDoorStatee=2") != -1) {
        //close door
        Serial.println(CLOSE_DOOR);
      }

      // Return the response
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html; charset=UTF-8");
      client.println("");
      client.println("<!DOCTYPE HTML>");
      client.println("<html>");
      client.println("<head>");
      client.println("<title>Chicken Coop Control Centre</title>");
      client.println("</head>");
      client.println("<body>");
      client.println("<a href=\"/\">Refresh Status</a>");
      client.println("</br></br>");

      //check the Door status
      switch (doorState) {
o:
          client.print("Door is: OPEN</br>");
          client.println("<a href=\"/changeDoorState=0\">Close the door</a></br></br>");
          break;
c:
          client.print("Door is: CLOSED</br>");
          client.println("<a href=\"/changeDoorState=1\">Open the door</a></br></br>");
          break;
a:
          client.print("Door is: <strong>AJAR!</strong></br>");
          client.println("<a href=\"/changeDoorState=1\">Open the door</a></br>");
          client.println("<a href=\"/changeDoorState=2\">Close the door</a></br>");
          break;
n:
          client.print("Door is: <strong>OPENING</strong></br>");
          client.println("<a href=\"/changeDoorState=0\">Stop the door</a></br></br>");
          break;
s:        client.print("Door is: <strong>CLOSING</strong></br>");
          client.println("<a href=\"/changeDoorState=0\">Stop the door</a></br></br>");
          break;
      }

      client.println("</br></br>");
      client.println("The coop temperature is: ");
      client.println(temperature);
      client.println("&#176C</br>");
      client.println("The coop relative humidity is: ");
      client.println(humidity);
      client.println("%</br></br>");
      client.println("Light level is at: :");
      client.println(lightLevel);

      client.println("</br>");
      client.println("</body>");
      client.println("</html>");

      delay(1);
    }
} server_task;

class SerialTask : public Task {
  protected:
    void loop()  {
 while (Serial.available()) {    //while / if serial data
        char c = Serial.read();     //read it

        if (c == '~') {       //start recv on ~
          data_in = true;
          recv = "";
          continue;         //read again

        } else if (data_in && c == '|') { //finish recv on  |
          data_in = false;

          if (recv.equals("SHUTDOWN")) {
            Serial.println(":: Shutdown unprocessed");

          } else if (recv.equals("LEDON")) {
            roomLight = "ON";
            Serial.println("Turning led on pin 13 on");

          } else if (recv.equals("LEDOFF")) {
            roomLight = "OFF";
            Serial.println("Turning led on pin 13 off");

          } else {
            Serial.println(":: Command unknown");
            Serial.print(":: ");
            Serial.println(recv);
          }
        } else if (data_in) {
          recv += c;
        } else {
          //do nothing, as we don't care unless it's between ~ and |
        }
      }
    }
} serial_task;

void setup() {
  Serial.begin(SERIAL_BAUD);

  // Connect to WiFi network
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, password);

  // Start the Web Server
  WebServer.begin();

  //Begin task scheduler
  Scheduler.start(&server_task);
  Scheduler.start(&serial_task);
  Scheduler.begin();

}

void loop() {

}

/*Checks outdoor light level and determines if door should be opened or closed
   RETURN: (Global) Boolean if true (1) returns advice to close the door; if false (0) to open the door.
*/
/*void CheckLight() {
  lightLevel = analogRead(lightSensor); //Read analogue signal from light sensor
  if (lightLevel < 100) {
    closeDoor = 1;
  }
  else {
    if (lightLevel > 100) {
      closeDoor = 0;
    }
  }
  }

  /*Checks position of door to determine when to stop the motor at the fully open or fully closed position
   RETURN: NA
*/
/*void OpenCloseDoor() {
  doorOpen = digitalRead(hallSensor1);
  doorClosed = digitalRead(hallSensor2);

  //If the door is open and the closeDoor value is true (1) run the motor to close the door
  if (doorClosed == HIGH && closeDoor) {
    Serial.println("Closing");
    CloseDoor();  //Run motor to close the door
    do {
      doorClosed = digitalRead(hallSensor2);
    } while (doorClosed == HIGH);
    Serial.println("Closed");
    StopMotor(); //Stop the motor
    doorOpen = HIGH;
    doorClosed = LOW;
  }
  else {
    //If the door is closed and the closeDoor value is false (0) run the motor to open the door
    if (doorOpen == HIGH && ! closeDoor) {
      Serial.println("Opening");
      OpenDoor();  //Run motor to open the door
      do {
        doorOpen = digitalRead(hallSensor1);
      } while (doorOpen == HIGH);
      Serial.println("Open");
      StopMotor(); //Stop the motor
      doorOpen = LOW;
      doorClosed = HIGH;
    }
  }
  }

  void CloseDoor() {
  digitalWrite(motorSpeed, 255);
  digitalWrite(motorForward, LOW);
  digitalWrite(motorBackward, HIGH);
  }

  void StopMotor() {
  digitalWrite(motorSpeed, 0);
  digitalWrite(motorForward, LOW);
  digitalWrite(motorBackward, LOW);
  }

  void OpenDoor() {
  digitalWrite(motorSpeed, 255);
  digitalWrite(motorForward, HIGH);
  digitalWrite(motorBackward, LOW);
  }

  /*Manual button control to open and close door???
   Uses hall effect sensors to ensure door won't be broken by opening too high or closing too far
   RETURN: NA
*/
