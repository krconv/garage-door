## Garage Door Controller

This code uses a NodeMCU Arduino device to power garage door sensors and controllers for two garage doors. This requests/data is transfered over MQTT, which Home Assistant can be set up to subscribe/publish to in order to controll the garage doors.

The code can be compiled by the Arduino IDE, with the following libraries installed:

- WiFi
- ArduinoOTA (for wirelessly updating the program)
- PubSubClient (for MQTT communication)
