/*
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

*/

#include <LittleFS.h>
#include <ArduinoJson.h>
#include <AirGradient.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "SSD1306Wire.h"

AirGradient ag = AirGradient();

SSD1306Wire display(0x3c, SDA, SCL);
#define DISPLAY_DELAY 2000

// set sensors that you do not use to false
boolean hasPM=true;
boolean hasCO2=true;
boolean hasSHT=true;

// set to true if you want to connect to wifi. The display will show values only when the sensor has wifi connection
boolean connectWIFI=true;

WiFiClient client;
PubSubClient mqtt_client(client);

char mqtt_srvr[40];
char mqtt_user[40];
char mqtt_pass[40];
char mqtt_locn[40];
char mqtt_room[40];

//flag for saving data
bool shouldSaveConfig = false;

//callback function telling us we need to save the config
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setup(){
  Serial.begin(9600);

  display.init();
  display.flipScreenVertically();
  showTextRectangle("Init", String(ESP.getChipId(),HEX),true);

  if (hasPM) ag.PMS_Init();
  if (hasCO2) ag.CO2_Init();
  if (hasSHT) ag.TMP_RH_Init(0x44);

  //Start LittleFS
  if(LittleFS.begin()){
    Serial.println("Mounted LittleFS");
    readConfig("/config.json");  
  } else {
    Serial.println("An Error has occurred while mounting LittleFS");
  }

  if (connectWIFI) connectToWifi();
  delay(2000);

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  showTextRectangle(mqtt_locn,mqtt_room,true);
  delay(DISPLAY_DELAY);
}

void loop(){
  ArduinoOTA.handle();

  if (connectWIFI){
    mqtt_connect();
    
    if (hasPM) {
      //int PM2 = ag.getPM2_Raw();
      const char *pm2 = ag.getPM2();
      showTextRectangle("PM2",pm2,false);
      mqtt_publish("pm02",pm2);
      delay(DISPLAY_DELAY);
    }

    if (hasCO2) {
      //int CO2 = ag.getCO2_Raw();
      const char *co2 = ag.getCO2();
      showTextRectangle("CO2",co2,false);
      mqtt_publish("co2",co2);      
      delay(DISPLAY_DELAY);
    }

    if (hasSHT) {
      TMP_RH result = ag.periodicFetchData();
      int tempF=(result.t*1.8)+32;
      showTextRectangle(String(tempF)+"F",String(result.rh)+"%",false);      
      char rhstr[10]; itoa(result.rh,rhstr,10);
      mqtt_publish("rh",rhstr); 
      char tempstr[10];
      itoa(tempF,tempstr,10);
      mqtt_publish("temp",tempstr); 
      delay(DISPLAY_DELAY);
    }

    mqtt_client.loop();
  }
}

// DISPLAY
void showTextRectangle(String ln1, String ln2, boolean small) {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  if (small) {
    display.setFont(ArialMT_Plain_16);
  } else {
    display.setFont(ArialMT_Plain_24);
  }
  display.drawString(32, 16, ln1);
  display.drawString(32, 36, ln2);
  display.display();
}

// Wifi Manager
void connectToWifi(){
  WiFiManager wifiManager;
  //WiFi.disconnect(); //to delete previous saved hotspot

  wifiManager.setSaveConfigCallback(saveConfigCallback);
  
  // id/name, placeholder/prompt, default, length
  WiFiManagerParameter custom_mqtt_srvr("srvr", "MQTT Server", "", 40);
  WiFiManagerParameter custom_mqtt_user("user", "MQTT Username", "", 40);
  WiFiManagerParameter custom_mqtt_pass("pass", "MQTT Password", "", 40);
  WiFiManagerParameter custom_mqtt_locn("locn", "MQTT Location", "", 40);
  WiFiManagerParameter custom_mqtt_room("room", "MQTT Room", "", 40);
  
  wifiManager.addParameter(&custom_mqtt_srvr);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_pass);
  wifiManager.addParameter(&custom_mqtt_locn);
  wifiManager.addParameter(&custom_mqtt_room);
  
  String HOTSPOT = "AIRGRADIENT-"+String(ESP.getChipId(),HEX);
  wifiManager.setTimeout(120);
  if(!wifiManager.autoConnect((const char*)HOTSPOT.c_str())) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      ESP.restart();
      delay(5000);
  }

  if (shouldSaveConfig) {
    strcpy(mqtt_srvr, custom_mqtt_srvr.getValue());
    strcpy(mqtt_user, custom_mqtt_user.getValue());
    strcpy(mqtt_pass, custom_mqtt_pass.getValue());
    strcpy(mqtt_locn, custom_mqtt_locn.getValue());
    strcpy(mqtt_room, custom_mqtt_room.getValue());
    saveConfig("/config.json");
  }
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void mqtt_connect() {
  int8_t ret;

  // Stop if already connected.
  //if (mqtt_client.connected() == MQTT_CONNECTED) {
  if (mqtt_client.state() == MQTT_CONNECTED) {
    Serial.println("MQTT Already connected");
    return;
  }

  Serial.print(F("Connecting to MQTT... "));

  String CID = String(ESP.getChipId(),HEX);

  mqtt_client.setServer(mqtt_srvr,1883);

  uint8_t retries = 3;
  while ((ret = mqtt_client.connect((char *)CID.c_str(),mqtt_user,mqtt_pass)) == 0) { // connect will return 0 for not connected   
    showTextRectangle("MQTT", "Failed "+String(mqtt_client.state()),true);
    Serial.println(mqtt_client.state());
    Serial.println(F("Retrying MQTT connection in 5 seconds..."));
    delay(5000);  // wait 5 seconds
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      while (1);
    }
  }
  Serial.println(F("MQTT Connected!"));
}

void mqtt_publish(char *sub_topic, const char *payload) {
  char mqtt_topic[80];
  strcpy(mqtt_topic,mqtt_locn);
  strcat(mqtt_topic,"/");
  strcat(mqtt_topic,mqtt_room);
  strcat(mqtt_topic,"/");
  strcat(mqtt_topic,sub_topic);
      
  mqtt_client.publish(mqtt_topic,payload);
}

void readConfig(char *cFilename) {
  if (LittleFS.exists(cFilename)) {
    Serial.println("Reading config file");
    File configFile = LittleFS.open(cFilename,"r");
    if (configFile) {
      String configData;
      while (configFile.available()){
        configData += char(configFile.read());
      }
      DynamicJsonDocument json(1024);
      auto deserializeError = deserializeJson(json, configData);
      serializeJson(json, Serial);
      if (! deserializeError) {
        Serial.println("\nParsed json");
        strcpy(mqtt_srvr, json["mqtt_srvr"]); 
        strcpy(mqtt_user, json["mqtt_user"]);
        strcpy(mqtt_pass, json["mqtt_pass"]);
        strcpy(mqtt_locn, json["mqtt_locn"]);
        strcpy(mqtt_room, json["mqtt_room"]);
      } else {
        Serial.println("\nFailed to parse json");
      }
      configFile.close();
    } else {
      Serial.println("Failed to open config file");
    }
  } else {
    Serial.println("No config file to read");
  }
}

void saveConfig(char *cFilename) {
  Serial.println("Saving config");
  
  DynamicJsonDocument json(1024);
  json["mqtt_srvr"] = mqtt_srvr;
  json["mqtt_user"] = mqtt_user;
  json["mqtt_pass"] = mqtt_pass;
  json["mqtt_locn"] = mqtt_locn;
  json["mqtt_room"] = mqtt_room;
  File configFile = LittleFS.open(cFilename,"w");
  if (configFile) {
    serializeJson(json, Serial);
    serializeJson(json, configFile);
    configFile.close();      
  } else {
    Serial.println("Failed to open config file for writing");
  }
}
