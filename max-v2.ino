
#include "setup.h"
#include "src/constants.h"
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <PubSubClient.h>

#include "src/webconfig.h"

ESP8266WebServer server(80);
WebConfig setupServer;

/* Config variables */
char _info_id[8];
char _info_name[64];
char _info_update_server[256];
uint8 _info_auto_update;

void setup() {
  Serial.begin(115200);
  Serial.println("Booting...");

  SPIFFS.begin();

  delay(10);
  WiFi.mode(WIFI_STA);
  WiFi.begin(CFG_SSID, CFG_PASSWORD);
  WiFi.hostname("V2");

  for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("NOT connected to WiFi... Reboot.");
    delay(1000);
    ESP.restart();
  }

  Serial.println("Connected to WiFi.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.begin();
  setupServer.begin(server);

  // CHANGE TO /info

  server.on("/restart", HTTP_POST, []() {
    server.send(200, "application/json", R_SUCCESS);
    delay(500);
    ESP.restart();
  });

  void loop() { server.handleClient(); }
