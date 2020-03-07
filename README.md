# Chicken-Coop-Central-Control
At its essence an Arduino based controller to open and close the chicken coop door automatically at sunrise and sunset respectively.
In reality it will also be a weather station, climate control for the chicken coop, and function as it’s own solar battery charge controller.

 Created by: Joshua Gilmore
 GitHub: djsounddog
 
 NOT YET FUNCTIONAL:
 -Serial communications between ESP8266 & ATmega328P
 -OTA update functionality for ATmega328P via ESP8266

 Serial port communications function codes (to be reconfigured to bytecode commands???):
 Door status:
 D#
 1=open 0=closed/safe 2=ajar 3=in motion

 Door commands:
 O#
 0=stop 1=open 2=close

 Light status/command:
 L#
 where 0=off 1=on

 Environmental:
 Temperature = T#####
 Humidity = H#####
 Sunlight Level = S#####

 Soil Moisture = M#####
 Rain Sensor = R#####

 Refresh command = REF

 Begin code = OK

 Awake time = A######

 Coop internal LED:

 const String TEMP = "T";
 const String HUMID = "H";
 const String COOP_LIGHT_ON = "L1";
 const String COOP_LIGHT_OFF = "L0";
 const String COOP_LIGHT_DIM_UP = "L+";
 const String COOP_LIGHT_DIM_DOWN = "L-";
 const String STOP_DOOR = "O0";
 const String OPEN_DOOR = "O1";
 const String CLOSE_DOOR = "O2";
 const String REFRESH_COMMAND = "REF";
 const String LIGHT_TRIGGER_ON = "R1";
 const String LIGHT_TRIGGER_OFF = "R0";
