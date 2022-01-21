# airgradient-mqtt
## Implementation of the AirGradient DIY Air Quality Sensor publishing to an MQTT broker

This is the code for the AirGradient DIY Air Quality Sensor with an ESP8266 Microcontroller.
It is a high quality sensor showing PM2.5, CO2, Temperature and Humidity on a small display and can send data over Wifi to an MQTT broker.

WiFi and MQTT parameters are entered via a captive WiFi portal and saved to flash using LittleFS.  OTA updates supported.

For build instructions please visit https://www.airgradient.com/diy/

Based on the original code @ https://github.com/airgradienthq/arduino/blob/master/examples/C02_PM_SHT_OLED_WIFI/C02_PM_SHT_OLED_WIFI.ino published under MIT License.

Compatible with the following sensors:
  * Plantower PMS5003 (Fine Particle Sensor)
  * SenseAir S8 (CO2 Sensor)
  * SHT30/31 (Temperature/Humidity Sensor)

Please install ESP8266 board manager (tested with version 3.0.0)

The following libraries need to be installed:

WifiManager by tzapu, tablatronix
  https://github.com/tzapu/WiFiManager
PubSubClient by Nick O'Leary
  https://pubsubclient.knolleary.net/
  https://github.com/knolleary/pubsubclient
ArduinoJson by Benoît Blanchon
  https://arduinojson.org/
  https://github.com/bblanchon/ArduinoJson
ESP8266 and ESP32 OLED driver for SSD1306 displays by ThingPulse, Fabrice Weinberg
  https://github.com/ThingPulse/esp8266-oled-ssd1306

Configuration:
Please set in the code below which sensor you are using and if you want to connect it to WiFi.
