
#include "setup.h"
#include "src/constants.h"
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <PubSubClient.h>


#include "src/webconfig.h";

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
  server.on("/info/update", HTTP_POST, []() {
    Serial.println("Post /info");
    const size_t capacity = JSON_OBJECT_SIZE(7) + 2048;
    DynamicJsonDocument doc(capacity);

    deserializeJson(doc, server.arg("plain").c_str());

    const char *name = doc["name"];
    const char *update_server = doc["update_server"];
    bool auto_update = doc["auto_update"];

    sprintf(_info_name, name);
    sprintf(_info_update_server, update_server);

    File f = SPIFFS.open("/info", "w");
    f.println(_info_name);
    f.println(_info_update_server);
    f.close();
    server.send(200);
  });

  server.on("/wifi", HTTP_GET, []() {
    int count = WiFi.scanNetworks();
    Serial.print("Found network:");
    Serial.println(count);
    char *buf = (char *)malloc(5120); // falta verificar se o bus não enche
    char *pos = buf;
    pos += sprintf(pos, "{\"networks\": [");
    for (int i = 0; i < count; i++) {
      pos += sprintf(
          pos, "{\"ssid\":\"%s\",\"rssi\":%d,\"saved\":%i,\"enc\":\"%d\"},",
          WiFi.SSID(i).c_str(), WiFi.RSSI(i), 0, WiFi.encryptionType(i));
    }
    if (count > 0)
      pos--; // retirar aa virgula
    *pos++ = ']';
    *pos++ = '}';
    *pos = 0;
    server.send(200, "application/json", buf);
    WiFi.scanDelete();
    free(buf);
  });
}

void loop() { server.handleClient(); }
