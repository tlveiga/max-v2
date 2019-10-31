#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <ArduinoJson.h>

#define FWCODE "max"
#define FWVERSION "0.0.1"
#define FWDATE 20191030

ESP8266WebServer server(80);


/* Config variables */
char _info_id[8];
char _info_name[64];
char _info_update_server[256];
uint8 _info_auto_update;

void setup()
{
  Serial.begin(115200);
  Serial.println("Booting...");

  SPIFFS.begin();
  sprintf(_info_id, "%X\0", ESP.getChipId());
  /* Reading config values or using defaults */
  if (SPIFFS.exists("/info")) {
    File f = SPIFFS.open("/info", "r");
    f.readBytesUntil('\n', _info_name, 64);
    f.readBytesUntil('\n', _info_update_server, 256);
    f.close();
  }
  else {
    sprintf(_info_name, "%s-%s\0", FWCODE, _info_id);
    sprintf(_info_update_server, "%s\0", "http://tlv-ubuntu.westeurope.cloudapp.azure.com/fw");
    _info_auto_update = 0;
  }


  delay(10);
  WiFi.mode(WIFI_STA);
  WiFi.begin("OlhaLua", "BATMAN00");
  WiFi.hostname("V2");

  for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++)
  {
    delay(500);
  }
  Serial.println("Connected to WiFi.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.begin();

  server.serveStatic("/", SPIFFS, "/index.html");
  server.serveStatic("/main.js", SPIFFS, "/main.js");

  server.on("/info", HTTP_GET, []() {
    Serial.println("Call /info");
    char *buf = (char *)malloc(5120);
    sprintf(buf, "{\"id\":\"%s\",\"code\":\"%s\",\"name\":\"%s\",\"fw_version\":\"%s\",\"fw_date\":%d,\"update_server\":\"%s\",\"auto_update\":%s}",
            _info_id, FWCODE, _info_name, FWVERSION, FWDATE, _info_update_server, _info_auto_update ? "true" : "false");
    server.send(200, "text/plain", buf);
    free(buf);
  });

  // CHANGE TO /info
  server.on("/info/update", HTTP_POST, []() {
    Serial.println("Post /info");
    const size_t capacity = JSON_OBJECT_SIZE(7) + 2048;
    DynamicJsonDocument doc(capacity);

    deserializeJson(doc, server.arg("plain").c_str());

    const char* name = doc["name"];
    const char* update_server = doc["update_server"];
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
      pos += sprintf(pos, "{\"ssid\":\"%s\",\"rssi\":%d,\"saved\":%i,\"enc\":\"%d\"},", WiFi.SSID(i).c_str(), WiFi.RSSI(i), 0, WiFi.encryptionType(i));
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

void loop()
{
  server.handleClient();
}
