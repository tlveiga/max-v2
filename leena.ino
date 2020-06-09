
#include "setup.h"
#include "src/constants.h"
#include "src/utils.h"
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>

#include <Bounce2.h>

#include "src/webconfig.h"

#include <TimeLib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#define BUTTON_PIN 14
#define LONGPRESS 1000

#define STRIPPIN 2
#define STRIPLEN 60

ESP8266WebServer server(80);
WebConfig cfginterface;
Bounce debouncer = Bounce();

Adafruit_NeoPixel strip = Adafruit_NeoPixel(STRIPLEN, STRIPPIN, NEO_GRB + NEO_KHZ800);

/* STRIP PART */
uint8_t hours[24];
uint8_t minutes[60];

unsigned long currentvalues[STRIPLEN];
unsigned long values[STRIPLEN];

uint8_t currentmode = random(8);
uint8_t lastmode = -1;
unsigned long changemode = 0;

unsigned long last = millis();

unsigned long addToColor(unsigned long color, int value)
{
  unsigned long newcolor = 0;
  for (int i = 0; i < 0; i++)
  {
    uint8_t c = (color >> (8 * i)) & 0xFF;
    int newc = c + value;
    if (newc > 255)
      newc = 255;
    else if (newc < 0)
      newc = 0;
    newcolor += (newc << (8 * i));
  }

  return newcolor;
}

unsigned long _lastfadeupdate = millis();
void fadeToColor(int stepvalon = 2, int stepvaloff = 2)
{
  if (millis() - _lastfadeupdate > 20) {
    for (int i = 0; i < strip.numPixels(); i++)
    {
      /*
        uint8_t greater = values[i] > currentvalues[i];
        unsigned long newcolor = addToColor(currentvalues[i], greater ? stepvalon : -stepvaloff);
        if ((greater && newcolor > values[i]) || (!greater && newcolor < values[i]))
        newcolor = values[i];

        currentvalues[i] = newcolor;*/

      for (int j = 0; j < 3; j++)
      {
        uint8_t cpv = 0xFF & (currentvalues[i] >> (8 * j));
        uint8_t pv = 0xFF & (values[i] >> (8 * j));

        if (cpv < pv)
        { //ON
          currentvalues[i] += min(stepvalon, pv - cpv) << (8 * j);
        }
        else if (cpv > pv)
        { //OFF
          currentvalues[i] -= min(stepvaloff, cpv - pv) << (8 * j);
        }
      }

      strip.setPixelColor(i, currentvalues[i]);
    }

    strip.show();
    _lastfadeupdate = millis();
  }
}
void randomShit()
{
  if (millis() - last > 10)
  {
    for (int i = 0; i < strip.numPixels(); i++)
    {
      unsigned long val = values[i];
      if (values[i] == currentvalues[i])
        values[i] = (random(0, 255) << 16) + (random(0, 255) << 8) + random(0, 255);
    }
    last = millis();
  }

  fadeToColor(20, 10);
}

void randomArms()
{
  if (millis() - last > 500)
  {
    for (int i = 0; i < strip.numPixels(); i++)
    {
      if (i < 12 && values[47 + i] == 0)
      {
        values[i] = (random(100, 255) << 16) + (random(100, 255) << 8) + random(100, 255);
      }
      else
      {
        unsigned long val = i < 12 ? values[47 + i] : values[i - 12];
        unsigned long newval = 0;

        for (int j = 0; j < 3; j++)
        {
          uint8_t c = (val >> (j * 8)) & 0xff;
          if (c < 5)
            c = 0;
          else
            c -= 5;
          newval += (c << (j * 8));
        }

        values[i] = newval;
      }
    }
    last = millis();
  }
  fadeToColor();
}

void randomArmsDecay()
{
  if (millis() - last > 100)
  {
    for (int i = 0; i < 12; i++)
    {
      if (values[i] == currentvalues[i])
      {
        unsigned long val = (random(100, 255) << 16) + (random(100, 255) << 8) + random(100, 255);
        values[i] = val;
        uint8_t r = (val >> 16) & 0xFF;
        uint8_t g = (val >> 8) & 0xFF;
        uint8_t b = val & 0xFF;

        for (int p = 0; p < 5; p++)
        {
          unsigned long newval = ((r / (p + 2)) << 16) + ((g / (p + 2)) << 8) + (b / (p + 2));
          values[i + p * 12] = newval;
        }
      }
    }
    last = millis();
  }
  fadeToColor();
}

int followadd = 1;
void followme()
{

  if (millis() - last > 100)
  {
    int valuescount = 0;
    for (int i = 0; i < strip.numPixels(); i++)
    {
      if (values[i] > 0)
        valuescount++;
    }

    if (followadd && valuescount == strip.numPixels())
      followadd = 0;
    else if (followadd == 0 && valuescount == 0)
      followadd = 1;

    for (int i = strip.numPixels() - 1; i >= 0; i--)
    {
      unsigned long val = i == 0 ? values[strip.numPixels() - 1] : values[i - 1];
      values[i] = val;
    }

    values[random(60)] = followadd ? (random(0, 255) << 16) + (random(0, 255) << 8) + random(0, 255) : 0;

    last = millis();
  }

  fadeToColor(255, 3);
}

unsigned long lastbeat = 0;
unsigned long beatcolors[12];
uint8_t beatcount = 0;
void heartbeat(int splitarms)
{
  unsigned long now = millis();

  if (now - lastbeat > 300)
  {
    if (beatcount == 0)
    {
      unsigned long color = (random(50, 255) << 16) + (random(50, 255) << 8) + random(50, 255);
      for (int i = 0; i < 12; i++)
      {
        if (splitarms)
          color = (random(50, 255) << 16) + (random(50, 255) << 8) + random(50, 255);
        beatcolors[i] = color;
        values[i] = color;
      }
    }
    else
    {
      for (int i = 0; i < 12; i++)
      {
        values[i + (12 * beatcount)] = beatcolors[i];
      }
    }

    beatcount++;
    if (beatcount == 5)
      beatcount = 0;
    lastbeat = now;
  }

  if (now - last > 300)
  {
    for (int i = 0; i < strip.numPixels(); i++)
    {
      if (values[i] == currentvalues[i])
        values[i] = addToColor(values[i], -5);
    }
    last = now;
  }

  fadeToColor(10, 5);
}

uint8_t tailcount = 0;
void tailbite()
{
  if (lastmode != currentmode)
  {
    tailcount = 0;
    for (int i = 0; i < strip.numPixels(); i++)
    {
      if (i < 12 && i % 2 == 0)
      {
        values[i] = (random(50, 255) << 16) + (random(50, 255) << 8) + random(50, 255);
        currentvalues[i] = values[i];
      }
      else
      {
        values[i] = 0;
        currentvalues[i] = 0;
      }
    }
  }
  else
  {
    /*0 -> 59; 12 -> 0; 24 -> 12 36; -> 24; 48 -> 36;
      1 -> 13; 13 -> 25; 25 -> 37; 37 -> 49; 49 -> 48*/
    if (millis() - last > 100)
    {
      if (tailcount == 70)
      {
        tailcount = 0;
        values[0] = (random(50, 255) << 16) + (random(50, 255) << 8) + random(50, 255);
      }
      tailcount++;

      unsigned long copy[60];
      for (int i = 0; i < strip.numPixels(); i++)
      {
        copy[i] = values[i];
        values[i] = 0;
      }

      for (int i = 0; i < strip.numPixels(); i++)
      {
        uint8_t arm = i % 12;
        if (arm % 2 == 0)
        {
          if (i == 0)
            values[i] = copy[11];
          else if (i < 12)
            values[i] = copy[i - 1];
          else
            values[i] = copy[i - 12];
        }
        else
        {
          if (i >= 49)
          {
            values[i] = copy[i - 1];
          }
          else
          {
            values[i] = copy[i + 12];
          }
        }
      }
      last = millis();
    }
  }
  fadeToColor(255, 5);
}

/** NTP **/
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 0, 60000);

long timeSync()
{
  return timeClient.getEpochTime();
}

uint8_t _lastsecond = 0;
void galaxyclock()
{
  uint8_t vsecond = second();
  if (_lastsecond != vsecond) {
    _lastsecond = vsecond;

    uint8_t vminute = minute();
    uint8_t vhour = hour();

    for (int i = 0; i < 60; i++)
      values[i] = 0;

    values[minutes[vsecond % 60]] = 255;
    values[minutes[vminute % 60]] +=  255 << 8;
    values[hours[vhour % 24]] += 255 << 16;
  }

  fadeToColor(2, 2);
}


void setup()
{
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.println("Booting...");

  Serial.printf("Project: %s\nVersion: %s\n", FWCODE, FWVERSION);
  SPIFFS.begin();
  server.begin();
  cfginterface.begin(server);

  Serial.println(cfginterface.getConfig(opts::mqtt_server).c_str());

  debouncer.attach(BUTTON_PIN);
  debouncer.interval(10); // interval in ms

  server.on("/restart", HTTP_POST, []() {
    server.send(200, "application/json", R_SUCCESS);
    delay(500);
    ESP.restart();
  });

  server.on("/vis", []() {
    if (server.arg("mode") != "") {
      int w = atoi(server.arg("mode").c_str());
      currentmode = w;
      changemode = millis();

      for (int i = 0; i < strip.numPixels(); i++) {
        values[i] = 0;
        currentvalues[i] = 0;
      }

    }
    server.send(200);
  });

  cfginterface.setMQTTCallback(
  [ = ](char *topic, uint8_t *message, unsigned int len) {
    if (len > 0)
    {
      Serial.print("Received message: ");
      Serial.println(message[0]);
    }
  });

  strip.begin();
  strip.setBrightness(50);
  strip.clear();
  strip.show();

  for (int i = 0; i < 60; i++) {
    minutes[i] = 0;
    values[i] = 0;
    currentvalues[i] = 0;
  }
  // mapeia os leds para os valores
  for (int i = 1; i <= 12; i++) {
    hours[12 - i] = i % 12;
    hours[24 - i] = i % 12;

    int min = (12 - i) * 5;
    minutes[min] = i % 12;

    for (int j = 1; j <= 4; j++) {
      minutes[min + j] = i % 12 + j * 12;
    }

  }

  timeClient.begin();
  setSyncProvider(timeSync);
  setSyncInterval(30);

  if (timeClient.update())
    setTime(timeClient.getEpochTime());
}

bool _readytopublish = false;
unsigned long _lastFell = 0;
void loop()
{
  cfginterface.update();
  server.handleClient();
  timeClient.update();

  debouncer.update();

  if (debouncer.fell())
  {
    Serial.println("fell");
    _lastFell = millis();
    _readytopublish = true;
  }

  if (debouncer.read() == 0 && _readytopublish && (millis() - _lastFell) > LONGPRESS)
  {
    _readytopublish = false;
    Serial.println("longpress");
    cfginterface.publish("{ \"type\": \"long\"}", false);
  }

  if (debouncer.rose() && _readytopublish)
  {
    Serial.println("press");
    cfginterface.publish("{ \"type\": \"press\"}", false);
    _readytopublish = false;
  }

  if (currentmode == 0) {
    galaxyclock();
  }
  else if (currentmode == 1) {
    randomShit();
  }
  else if (currentmode == 2) {
    randomArms();
  }
  else if (currentmode == 3) {
    randomArmsDecay();
  }
  else if (currentmode == 4) {
    followme();
  }
  else if (currentmode == 5) {
    heartbeat(0);
  }
  else if (currentmode == 6) {
    heartbeat(1);
  }
  else if (currentmode == 7) {
    tailbite();
  }

  lastmode = currentmode;

}
