
#include "setup.h"
#include "src/constants.h"
#include "src/utils.h"
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <TimeLib.h>
#include <NTPClient.h>

#include "src/webconfig.h"
#include "src/opt_progmem.h"
#include "src/Alarm.h"
#include <Adafruit_NeoPixel.h>

#include <WebSocketsServer.h>

#define RELAY 14
#define RELAY_ON HIGH
#define RELAY_OFF LOW

#define ALARMSFILE "/alarms.dat"
#define STATUSFILE "/status.dat"
#define NUMALARMS 11

void broadcastStatus(const uint8_t num, const uint8_t val);
void broadcastTime(const uint8_t num);
void broadcastAlarms(const uint8_t num);
long timeSync();
void alarmcb(AlarmStruct alarm);
void readAlarms();
void writeAlarms();

ESP8266WebServer server(80);
Adafruit_NeoPixel pixels(1, 4, NEO_GRB + NEO_KHZ800);
WebConfig cfginterface("/cfg");
WebSocketsServer webSocket = WebSocketsServer(81);
Alarm alarms(alarmcb);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 0, 60000);

uint8_t config_changed = 0;
unsigned long last_change = 0;

/* Colors */
unsigned long last_color_update = 0;
uint8_t color_updated = 0;
const uint32_t C_boot_on = pixels.Color(0, 255, 255);
const uint32_t C_boot_off = pixels.Color(153, 102, 255);
const uint32_t C_STA_on = pixels.Color(0, 255, 0);
const uint32_t C_STA_off = pixels.Color(255, 0, 0);
const uint32_t C_AP_on = pixels.Color(0, 255, 102);
const uint32_t C_AP_off = pixels.Color(255, 0, 102);

/* END Colors */

void setRelay(int val)
{
  digitalWrite(RELAY, val ? RELAY_ON : RELAY_OFF);

  config_changed = 1;
  last_change = millis();

  color_updated = 1;
  last_color_update = millis() - 10000;

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

  File file = SPIFFS.open(STATUSFILE, "r");
  if (!file || file.size() == 0)
  {
    pixels.setPixelColor(0, C_boot_off);
    digitalWrite(RELAY, RELAY_OFF);
  }
  else
  {
    int val = file.read();
    digitalWrite(RELAY, val ? RELAY_ON : RELAY_OFF);
    pixels.setPixelColor(0, val ? C_boot_on : C_boot_off);
  }
  if (file)
    file.close();

  pixels.show();

  Serial.println(cfginterface.getConfig(opts::mqtt_server).c_str());

  createIfNotFound(ALARMSFILE);
  readAlarms();

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

  server.on("/alarms", HTTP_POST, []() {
    if (server.hasArg("id"))
    {
      int id = atoi(server.arg("id").c_str());
      AlarmStruct alm = alarms.getAlarm(id);
      if (alm.id >= 0)
      {
        uint8_t dow = atoi(server.arg("dow").c_str());
        uint8_t hours = atoi(server.arg("hours").c_str());
        uint8_t minutes = atoi(server.arg("minutes").c_str());
        uint8_t active = atoi(server.arg("active").c_str());
        uint8_t action = atoi(server.arg("action").c_str());
        uint8_t repeat = atoi(server.arg("repeat").c_str());

        alm.dow = dow;
        alm.hours = constrain(hours, 0, 23);
        alm.minutes = constrain(minutes, 0, 59);
        alm.active = active;
        alm.repeat = repeat;
        alm.action = action;

        alarms.updateAlarm(alm);

        config_changed = 1;
        last_change = millis();
      }
      broadcastAlarms(0);
    }
    server.send(200);
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

void alarmcb(AlarmStruct alarm)
{
  setRelay(alarm.action);
  broadcastAlarms(0);
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
  alarms.loop();

  int minutes = minute();
  if (minutes != _lastcheckminute)
  {
    _lastcheckminute = minutes;
    broadcastTime(0);
  }

  if (color_updated && (millis() - last_color_update) > 3000)
  {
    uint32_t color;
    if (WiFi.getMode() == WIFI_STA) {
      color = getRelay() ? C_STA_on : C_STA_off;
    }
    else {
      color = getRelay() ? C_AP_on : C_AP_off;
    }

    pixels.setPixelColor(0, color);
    pixels.show();

    last_color_update = millis();
  }

  if (config_changed && (millis() - last_change) > 60000)
  {
    config_changed = 0;
    writeAlarms();
    Serial.println("Alarms saved to memory.");
    File file = SPIFFS.open(STATUSFILE, "w");
    if (file)
    {
      file.write(getRelay() ? 1 : 0);
      file.close();
    }
  }
}

/* Websocket messages */

void broadcastAlarms(const uint8_t num)
{
  char *buf = (char *)malloc(1024);
  char *pos = buf;
  pos += sprintf(pos, "{\"action\": \"alarms\", \"value\": [");
  for (int i = 0; i < alarms.count(); i++)
  {
    AlarmStruct a = alarms.getAlarm(i);
    pos += sprintf(pos, "{\"id\":%i,\"dow\":%i,\"hours\":%i,\"minutes\":%i,\"active\":%i, \"action\":%i, \"repeat\": %i},", a.id, a.dow, a.hours, a.minutes, a.active, a.action, a.repeat);
  }
  if (alarms.count() > 1)
    pos--; // retirar aa virgula
  *pos++ = ']';
  *pos++ = '}';
  *pos = 0;

  if (num > 0)
    webSocket.sendTXT(num, buf);
  else
    webSocket.broadcastTXT(buf);
  free(buf);
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

void readAlarms()
{
  AlarmRaw raw[NUMALARMS];
  File f = SPIFFS.open(ALARMSFILE, "r");
  if (f)
  {
    Serial.printf("Alarms file exists with size: %d\n", f.size());
    const size_t alarmSize = sizeof(AlarmRaw);
    AlarmRaw alarm;
    for (int i = 0; i < NUMALARMS; i++)
    {
      size_t szread = f.read((uint8_t *)&alarm, alarmSize);
      Serial.printf("Alarm %d: %x -> %d = %d\n", i, alarm, szread, alarmSize);
      if (szread == alarmSize)
        alarms.addAlarm(alarm);
      else
        alarms.addAlarm(0);
    }
    f.close();
  }
  else
  {
    for (int i = 0; i < NUMALARMS; i++)
    {
      alarms.addAlarm(0);
    }
  }
}

void writeAlarms()
{
  AlarmRaw raw[NUMALARMS];
  alarms.getRawData(raw, NUMALARMS);

  File f = SPIFFS.open(ALARMSFILE, "w");
  if (f)
  {
    const size_t alarmSize = sizeof(AlarmRaw);
    AlarmRaw alarm;
    for (int i = 0; i < NUMALARMS; i++)
    {
      alarm = raw[i];
      f.write((uint8_t *)&alarm, alarmSize);
    }
    f.close();
  }

  size_t size = sizeof(AlarmRaw) * NUMALARMS;
  char *ptr = (char *)raw;

  //writeEEPROM(ptr, ALARMSADDRESS, size);
}