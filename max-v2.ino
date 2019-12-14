
#include "setup.h"
#include "src/constants.h"
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <FS.h>

#include "src/webconfig.h"
#include <Adafruit_NeoPixel.h>

ESP8266WebServer server(80);
WebConfig setupServer;
Adafruit_NeoPixel pixels(1, 4, NEO_GRB + NEO_KHZ800);

unsigned long last = 0;
void setup()
{
  Serial.begin(115200);
  pinMode(14, OUTPUT);

  Serial.println("Booting...");

  Serial.printf("Project: %s\nVersion: %s\n", FWCODE, FWVERSION);

  SPIFFS.begin();

  server.begin();
  setupServer.begin(server);
  pixels.begin();

  delay(50);
  digitalWrite(14, LOW);
  pixels.setPixelColor(0, pixels.Color(150, 150, 150));
  pixels.show();

  Serial.println(setupServer.getConfig(opts::mqtt_server).c_str());

  server.on("/restart", HTTP_POST, []() {
    server.send(200, "application/json", R_SUCCESS);
    delay(500);
    ESP.restart();
  });

  server.on("/mqtt/text", HTTP_POST, []() {
    char msg[32];
    snprintf(msg, 50, "hello world #%x", millis());
    Serial.print("Publish message: ");
    Serial.println(msg);
    // mqttClient.publish("outTopic", msg);
    server.send(200, "application/json", R_SUCCESS);
  });

  setupServer.setMQTTCallback(
      [=](char *topic, uint8_t *message, unsigned int len) {
        if (len > 0)
        {
          Serial.print("Received message: ");
          Serial.println(message[0]);
          digitalWrite(14, message[0] == '1' ? HIGH : LOW);
          if (message[0] == '1')
          {
            pixels.setPixelColor(0, pixels.Color(0, 150, 0));
          }
          else
          {
            pixels.setPixelColor(0, pixels.Color(150, 0, 0));
          }
          pixels.show();
        }
      });
}

void loop()
{
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
