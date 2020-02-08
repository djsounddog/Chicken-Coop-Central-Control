//For some reason the motor triggers aren't working
//Can't call ClimateSensorTask from refresh function... perhaps combuine refresh int sensor loop?

#include <Scheduler.h>
#include <dht.h>
#include <uno_wifi.h>

#define SERIAL_BAUD 115200
uno_wifi board(0);

//Light sensor
const int LIGHT_SENSOR = A0;

//Hall effect sensors
const int DOOR_OPEN_SENSOR = 7;
const int DOOR_CLOSED_SENSOR = 8;

//Motor control
const int MOTOR_SPEED_PIN = 3;
const int MOTOR_FORWARD_PIN = 2;
const int MOTOR_BACKWARD_PIN = 4;

//Room light for short days
const int COOP_LIGHT_PIN = 5;

//Temperature & Humidity
dht DHT;
#define DHT11_PIN 6

//Control values
int changeDoorState = 0;    //Toggle door position, 0=stop, 1=open, 2=close
int doorState = 0;          //0=closed, 1=open, 2=open slightly
int brightness = 0;         //Coop internal LED brightness value
int previousBrightness = 0;
unsigned long sunset = 0;     //sets time at sunset
long lightLevel = 0;        //Intensity of daylight from Photosensitive LDR Sensor Module
long twilightLightLevel = 1010; //Intensity from Photosensitive LDR Sensor Module that will trigger the door to open and close
unsigned long ms = 0;           //Records time in millis
unsigned long previousms = 0;   //records time that sensor loop was previously run
unsigned long SENSOR_INTERVAL = 1800000;
bool lightTrigger = 1;      //Determines if the door will automatically open and close depending on daylight condition
unsigned long lightFadeDuration = 1800000;      //Fade time duration of the coop room LED in ms
//const String COOP_LIGHT_ON = "L1";
//const String COOP_LIGHT_OFF = "L0";
//const String COOP_LIGHT_DIM_UP = "L+";
//const String COOP_LIGHT_DIM_DOWN = "L-";
//const String STOP_DOOR = "O0";
//const String OPEN_DOOR = "O1";
//const String CLOSE_DOOR = "O2";
//const String REFRESH_COMMAND = "REF";
//const String LIGHT_TRIGGER_ON = "R1";
//const String LIGHT_TRIGGER_OFF = "R0";
const unsigned long NIGHT_DURATION = 32400000;
const unsigned long AWAKE_TIME = 50400000;
const String AWAKE_MESSAGE = "A";
unsigned long daylightHours = 0;

void SensorLoop();
void SerialLoop();
void DaylightCheck();
void OpenDoor();
void CloseDoor();
void StopDoor();
int DoorState();
void CoopLightSwitch(unsigned long fadeDuration = 0, int dimmerValue = 0);
void Wait();
void SendDoorState();
void RefreshSensorData();
void ClimateSensorTask();

void setup() {

  board.begin(SERIAL_BAUD);

  Serial.print("Arduino Started, waiting for ");
  board.serialSend(board.CONFIRMATION);

  bool started = false;
  while (!started) {          //wait until we've started
    String recv = board.serialGet();
    if (recv.equals(board.CONFIRMATION)) {  //Await WIFI board startup to continue Uno script
      Serial.println("Sucess");
      started = true;
    }
  }

  pinMode(DOOR_OPEN_SENSOR, INPUT);
  pinMode(DOOR_CLOSED_SENSOR, INPUT);
  pinMode(MOTOR_SPEED_PIN, OUTPUT);
  pinMode(MOTOR_FORWARD_PIN, OUTPUT);
  pinMode(MOTOR_BACKWARD_PIN, OUTPUT);
  pinMode(COOP_LIGHT_PIN, OUTPUT);

  //Load sensor and communications tasks
  Scheduler.startLoop(SensorLoop);
  Scheduler.startLoop(SerialLoop);
}

void loop() {
  ms = millis();
  yield();
}

//Runs light and climate sensors as per specified SENSOR_INTERVAL
void SensorLoop() {
  ClimateSensorTask();
  SendDoorState();
  if (lightTrigger) {
    DaylightCheck();
  }
  Wait();
}

void Wait() {
  await(ms - previousms >= SENSOR_INTERVAL);
  previousms = ms;
}

//Sends temperature and humidity data to the wifi module
void ClimateSensorTask() {
  int chk = DHT.read11(DHT11_PIN);
  switch (chk) {
    case DHTLIB_OK:
      String temp = "T";
      temp.concat(DHT.temperature);
      String humid = "H";
      humid.concat(DHT.humidity);
      board.serialSend(temp);
      board.serialSend(humid);
      break;
    default:
      break;
  }
  delay(1);
  yield();
}

void SerialLoop() {
  //run uno_wifi.serialGet from library : returns String
  String recv = board.serialGet();
  if (!recv.equals("")) {
    if (recv.equals("SHUTDOWN")) {
      //Serial.println(":: Shutdown unprocessed");
    } else if (recv.equals("L1")) {
      board.serialSend(board.CONFIRMATION);
      brightness = 255;
      CoopLightSwitch(1000);
    } else if (recv.equals("L0")) {
      board.serialSend(board.CONFIRMATION);
      brightness = 0;
      CoopLightSwitch(1000);
    } else if (recv.equals("L+")) {
      board.serialSend(board.CONFIRMATION);
      if (brightness < 255)
        CoopLightSwitch(1000, 15);
    } else if (recv.equals("L-")) {
      board.serialSend(board.CONFIRMATION);
      if (brightness >= 15)
        CoopLightSwitch(1000, -15);
    } else if (recv.equals("O0")) {
      board.serialSend(board.CONFIRMATION);
      StopDoor();
    } else if (recv.equals("O1")) {
      board.serialSend(board.CONFIRMATION);
      OpenDoor();
    } else if (recv.equals("O2")) {
      board.serialSend(board.CONFIRMATION);
      CloseDoor();
    } else if (recv.equals("REF")) {
      //refresh all sensor statuses
      board.serialSend(board.CONFIRMATION);
      RefreshSensorData();
    } else if (recv.equals("R1")) {
      board.serialSend(board.CONFIRMATION);
      lightTrigger = 1;
    } else if (recv.equals("R0")) {
      board.serialSend(board.CONFIRMATION);
      lightTrigger = 0;
    } else {
      //Do nothing with incorrect input
    }
  }
  //  delay(1);
  yield();
}


//Changing door state over and over even when already closed/open!
void DaylightCheck() {
  doorState = DoorState();
  lightLevel = analogRead(LIGHT_SENSOR);
  if (!(doorState == 0)) {
    if (lightLevel > twilightLightLevel) {
      Wait();
      if (lightLevel > twilightLightLevel) {
        changeDoorState = 2;
        CloseDoor();
        daylightHours = millis() - daylightHours;
        sunset = millis();
      }
    }
  } else if (!(doorState == 1)) {
    if (lightLevel <= twilightLightLevel) {
      Wait();
      if (lightLevel <= twilightLightLevel) {
        changeDoorState = 1;
        OpenDoor();
        daylightHours = millis();
        sunset = 0;
        brightness = 0;
        CoopLightSwitch();
      }
    }
  }
  if (doorState == 0) {
    if (daylightHours < AWAKE_TIME) {
      if ((millis() - sunset) > NIGHT_DURATION) {
        String msg = AWAKE_MESSAGE + daylightHours;
        board.serialSend(msg);
        //        delay(1);
        brightness = 255;
        CoopLightSwitch(lightFadeDuration);
      }
    }
  }
}

void SendDoorState() {
  String message = "D";
  message.concat(DoorState());
  board.serialSend(message);
}

void OpenDoor() {
  doorState = DoorState();
  if (!(doorState == 1) && changeDoorState == 1) {
    board.serialSend("D3");
    //    delay(1);
    digitalWrite(MOTOR_SPEED_PIN, HIGH);
    digitalWrite(MOTOR_FORWARD_PIN, HIGH);
    digitalWrite(MOTOR_BACKWARD_PIN, LOW);
  }
  while (!(doorState == 1) && changeDoorState == 1) {
    doorState = DoorState();
    delay(100);
    yield();
  }
  StopDoor();
  //  delay(1);
}

void CloseDoor() {
  doorState = DoorState();
  if (!(doorState == 0) && changeDoorState == 2) { //if the door is open or ajar start the motor to close the door
    board.serialSend("D3");
    delay(1);
    digitalWrite(MOTOR_SPEED_PIN, HIGH);
    digitalWrite(MOTOR_FORWARD_PIN, LOW);
    digitalWrite(MOTOR_BACKWARD_PIN, HIGH);
  }
  while (!(doorState == 0) && changeDoorState == 2) {
    doorState = DoorState();
    delay(100);
    yield();
  }
  StopDoor();
  delay(1);
}

void StopDoor() {
  digitalWrite(MOTOR_SPEED_PIN, LOW);
  digitalWrite(MOTOR_FORWARD_PIN, LOW);
  digitalWrite(MOTOR_BACKWARD_PIN, LOW);
  changeDoorState = 0;
  SendDoorState();
  //  delay(1);
}

int DoorState() {
  if (digitalRead(DOOR_OPEN_SENSOR) == LOW && digitalRead(DOOR_CLOSED_SENSOR) == HIGH)
    return 1; //If door open
  if (digitalRead(DOOR_CLOSED_SENSOR) == LOW && digitalRead(DOOR_OPEN_SENSOR) == HIGH)
    return 0; //If door closed
  if (digitalRead(DOOR_OPEN_SENSOR) == HIGH && (digitalRead(DOOR_CLOSED_SENSOR) == HIGH))
    return 2; //If door ajar or in motion
}

void CoopLightSwitch(unsigned long fadeDuration = 0, int dimmerValue = 0) {
  brightness = brightness + dimmerValue;
  if (fadeDuration) {
    long increment = fadeDuration / brightness;
    increment = abs(increment);
    //Fade up slowly over duration of entire period
    if (brightness > previousBrightness) {
      while (previousBrightness < brightness) {
        previousBrightness++;
        analogWrite(COOP_LIGHT_PIN, previousBrightness);
        delay(increment);
        yield();
      }
    } else if (brightness < previousBrightness) {
      while (previousBrightness > brightness) {
        previousBrightness--;
        analogWrite(COOP_LIGHT_PIN, previousBrightness);
        delay(increment);
        yield();
      }
    }
  } else {
    analogWrite(COOP_LIGHT_PIN, brightness);
    yield();
  }
  return;
}

void RefreshSensorData() {
//  ClimateSensorTask();
  delay(1);
  SendDoorState();
  delay(1);
  String msg = "L";
  msg.concat(lightTrigger);
  String sunLight = "S";
  lightLevel = analogRead(LIGHT_SENSOR);
  delay(1);
  sunLight.concat(lightLevel);
  String l = "L";
  l.concat(brightness);
  board.serialSend(l);
  delay(1);
}
