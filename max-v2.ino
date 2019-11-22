
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

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  // Loop until we're reconnected
  if (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    Serial.println(clientId);
    // Attempt to connect
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      mqttClient.publish("outTopic", "hello world");
      // ... and resubscribe
      mqttClient.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
    }
  }
}

unsigned long last = 0;
void setup() {
  Serial.begin(115200);
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

  server.on("/mqtt/text", HTTP_POST, []() {
    char msg[32];
    snprintf(msg, 50, "hello world #%x", millis());
    Serial.print("Publish message: ");
    Serial.println(msg);
    mqttClient.publish("outTopic", msg);
    server.send(200, "application/json", R_SUCCESS);
  });
}

void loop() {
  setupServer.update();
  server.handleClient();

  if (millis() - last > 10000) {
    if (!mqttClient.connected() && WiFi.getMode() == WIFI_STA) {
      mqttClient.setServer(setupServer.getConfig(opts::mqtt_server).c_str(),
                           1883);
      mqttClient.setCallback(callback);
      reconnect();
    }
    last = millis();
  }
  mqttClient.loop();
}
