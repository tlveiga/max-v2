
#include "setup.h"
#include "src/constants.h"
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <TimeLib.h>
#include <NTPClient.h>

#include "src/webconfig.h"
#include "src/opt_progmem.h"
#include <Adafruit_NeoPixel.h>

#include <WebSocketsServer.h>

#define RELAY 14
#define RELAY_ON HIGH
#define RELAY_OFF LOW

void broadcastStatus(const uint8_t num, const uint8_t val);
void broadcastTime(const uint8_t num);
void broadcastAlarms(const uint8_t num);
long timeSync();

ESP8266WebServer server(80);
Adafruit_NeoPixel pixels(1, 4, NEO_GRB + NEO_KHZ800);
WebConfig cfginterface("/cfg");
WebSocketsServer webSocket = WebSocketsServer(81);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 0, 60000);

void setRelay(int val)
{
  digitalWrite(RELAY, val ? RELAY_ON : RELAY_OFF);

  if (val)
  {
    pixels.setPixelColor(0, pixels.Color(0, 150, 0));
  }
  else
  {
    pixels.setPixelColor(0, pixels.Color(150, 0, 0));
  }
  pixels.show();
  // EEPROM.write(CONFIGADDRESS, val);

  // config_changed = 1;
  // last_change = millis();

  // if (client.connected())
  //   client.publish(pub_topic, val ? "1" : "0"); // TESTAR SE ISTO NÃO PROVOCA UM COMPORTAMENTO ESTRANHO
  cfginterface.publish(val ? "1" : "0", true);
  broadcastStatus(0, val);
}

int getRelay()
{
  return digitalRead(RELAY) == RELAY_ON;
}

const char *alarms_msg = "alarms";
const char *status_msg = "status";
const char *time_msg = "time";
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  if (type == WStype_TEXT)
  {
    if (strcmp((const char *)payload, alarms_msg) == 0)
    {
      broadcastAlarms(num);
    }
    else if (strcmp((const char *)payload, status_msg) == 0)
    {
      broadcastStatus(num, getRelay());
    }
    else if (strcmp((const char *)payload, time_msg) == 0)
    {
      broadcastTime(num);
    }
  }
}

unsigned long last = 0;
void setup()
{
  Serial.begin(115200);
  pinMode(RELAY, OUTPUT);

  Serial.println("Booting...");

  Serial.printf("Project: %s\nVersion: %s\n", FWCODE, FWVERSION);

  SPIFFS.begin();

  server.begin();
  cfginterface.begin(server);
  pixels.begin();

  delay(50);

  pixels.setPixelColor(0, pixels.Color(150, 150, 150));
  pixels.show();

  Serial.println(cfginterface.getConfig(opts::mqtt_server).c_str());

  server.on("/restart", HTTP_POST, []() {
    server.send(200, "application/json", R_SUCCESS);
    delay(500);
    ESP.restart();
  });

  server.on("/value", HTTP_POST, []() {
    int value;
    if (server.arg("plain")[0] == 't')
      value = !getRelay();
    else
      value = server.arg("plain")[0] == '1';

    setRelay(value);
    server.send(200);
  });

  server.on("/value", HTTP_GET, []() {
    server.send(200, "text/plain", String(getRelay() ? '1' : '0'));
  });

  server.on("/", HTTP_GET, [&]() { handle_NOEXT_index_html(server); });

  server.on("/epoch", HTTP_GET, []() {
    Serial.println(timeClient.getFormattedTime());
    Serial.println(timeClient.getEpochTime());
    server.send(200, "text/plain", String(now()));
  });

  cfginterface.setMQTTCallback(
      [=](char *topic, uint8_t *message, unsigned int len) {
        if (len > 0)
        {
          Serial.print("Received message: ");
          Serial.println(message[0]);
          int value;
          if (message[0] == 't')
            value = !getRelay();
          else
            value = message[0] == '1';
          setRelay(value);
        }
      });

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  timeClient.begin();
  setSyncProvider(timeSync);
  setSyncInterval(30);

  if (timeClient.update())
    setTime(timeClient.getEpochTime());
}

long timeSync()
{
  return timeClient.getEpochTime();
}

int _lastcheckminute = -1;
void loop()
{
  cfginterface.update();
  server.handleClient();
  webSocket.loop();
  timeClient.update();

  int minutes = minute();
  if (minutes != _lastcheckminute)
  {
    _lastcheckminute = minutes;
    broadcastTime(0);
  }

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

/* Websocket messages */

void broadcastAlarms(const uint8_t num)
{
  // char *buf = (char *)malloc(1024);
  // char *pos = buf;
  // pos += sprintf(pos, "{\"action\": \"alarms\", \"value\": [");
  // for (int i = 0; i < alarms.count(); i++)
  // {
  //   AlarmStruct a = alarms.getAlarm(i);
  //   pos += sprintf(pos, "{\"id\":%i,\"dow\":%i,\"hours\":%i,\"minutes\":%i,\"active\":%i, \"action\":%i, \"repeat\": %i},", a.id, a.dow, a.hours, a.minutes, a.active, a.action, a.repeat);
  // }
  // if (pos - buf > 1)
  //   pos--; // retirar aa virgula
  // *pos++ = ']';
  // *pos++ = '}';
  // *pos = 0;

  // if (num > 0)
  //   webSocket.sendTXT(num, buf);
  // else
  //   webSocket.broadcastTXT(buf);
  // free(buf);
}

void broadcastStatus(const uint8_t num, const uint8_t val)
{
  char *buf = (char *)malloc(64);
  sprintf(buf, "{\"action\": \"status\", \"value\": %i}", val);
  if (num > 0)
    webSocket.sendTXT(num, buf);
  else
    webSocket.broadcastTXT(buf);
  free(buf);
}

void broadcastTime(const uint8_t num)
{
  char *buf = (char *)malloc(64);
  sprintf(buf, "{\"action\": \"time\", \"value\": %i}", now());
  if (num > 0)
    webSocket.sendTXT(num, buf);
  else
    webSocket.broadcastTXT(buf);
  free(buf);
}

/* Websockets messages end */