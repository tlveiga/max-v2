
#include "setup.h"
#include "src/constants.h"
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <FS.h>

#include "src/webconfig.h"

ESP8266WebServer server(80);
WebConfig setupServer("/cfg");

unsigned long last = 0;
void setup() {
  Serial.begin(115200);
  pinMode(14, OUTPUT);
  Serial.println("Booting...");

  Serial.printf("Project: %s\nVersion: %s\n", FWCODE, FWVERSION);

  SPIFFS.begin();

  server.begin();
  setupServer.begin(server);

  Serial.println(setupServer.getConfig(opts::mqtt_server).c_str());

  server.on("/restart", HTTP_POST, []() {
    server.send(200, "application/json", R_SUCCESS);
    delay(500);
    ESP.restart();
  });

  setupServer.setMQTTCallback(
      [=](char *topic, uint8_t *message, unsigned int len) {
        Serial.print("Message arrived [");
        Serial.print(topic);
        Serial.print("] ");
        for (int i = 0; i < len; i++) {
          Serial.print((char)message[i]);
        }
        Serial.println();
      });
}

void loop() {
  setupServer.update();
  server.handleClient();

  // if (millis() - last > 10000) {
  //   if (!mqttClient.connected() && WiFi.getMode() == WIFI_STA) {
  //     mqttClient.setServer(setupServer.getConfig(opts::mqtt_server).c_str(),
  //                          1883);
  //     mqttClient.setCallback(callback);
  //     reconnect();
  //   }
  //   last = millis();
  // }
  // mqttClient.loop();
}
