#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>
#include <FS.h>

ESP8266WebServer server(80);
void prinScanResult(int networksFound)
{
  Serial.printf("%d network(s) found\n", networksFound);
  for (int i = 0; i < networksFound; i++)
  {
    Serial.printf("%d: %s, Ch:%d (%ddBm) %s\n", i + 1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i) == ENC_TYPE_NONE ? "open" : "");
  }
}


void setup()
{
  SPIFFS.begin();

  Serial.begin(115200);
  Serial.println("Booting...");

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

  server.on("/", HTTP_GET, []() {
    char *buf = (char *)malloc(1014);
    char json[] = "{\"sensor\":\"gps\",\"time\":1351824120,\"data\":[48.756080,2.302038]}";
    Serial.println("Call /");
    server.send(200, "application/json", json);
    free(buf);
  });

  server.on("/info", HTTP_GET, []() {
    Serial.println("Call /info");
    char *buf = (char *)malloc(1014);
    server.send(200, "text/plain", "{\"name\": \"MAX\"}");
    free(buf);
  });

  server.on("/wifi", HTTP_GET, []() {
    int count = WiFi.scanNetworks();
    Serial.print("Found network:");
    Serial.println(count);
    char *buf = (char *)malloc(5120); // falta verificar se o bus não enche
    char *pos = buf;
    pos += sprintf(pos, "{\"networks\": [");
    for (int i = 0; i < count; i++) {
      pos += sprintf(pos, "{\"ssid\":\"%s\",\"signal\":%d,\"saved\":%i,\"enc\":\"%d\"},", WiFi.SSID(i).c_str(), WiFi.RSSI(i), 0, WiFi.encryptionType(i));
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
