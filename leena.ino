
#include "setup.h"
#include "src/constants.h"
#include "src/utils.h"
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>

#include <Bounce2.h>

#include "src/webconfig.h"

#include <RCSwitch.h>
RCSwitch mySwitch = RCSwitch();

#define BUTTON_PIN 14
#define LONGPRESS 1000

#define RFCODEDEBOUNCE 3000
#define RFCODEHEADER 0x1978

ESP8266WebServer server(80);
WebConfig cfginterface;
Bounce debouncer = Bounce();

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

  cfginterface.setMQTTCallback(
      [=](char *topic, uint8_t *message, unsigned int len) {
        if (len > 0)
        {
          Serial.print("Received message: ");
          Serial.println(message[0]);
        }
      });

  Serial.begin(115200);
  mySwitch.enableReceive(12); //D6
}

bool _readytopublish = false;
unsigned long _lastFell = 0;
unsigned long _lastRFCode = 0;
unsigned long _lastRFCodeSent = 0;
unsigned long _lastRFMessage = 0;
void loop()
{
  cfginterface.update();
  server.handleClient();

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

  if (mySwitch.available())
  {
    unsigned long received = mySwitch.getReceivedValue();
    unsigned int header = received >> 8;
    if (header == RFCODEHEADER)
    {
      _lastRFCode = 0x0000FF & received;
      _lastRFMessage = millis();
    }

    char buf[256];
    sprintf(buf, "%X\0", mySwitch.getReceivedValue());
    Serial.print(buf);
    Serial.println();

    mySwitch.resetAvailable();
  }

  if (_lastRFCode != _lastRFCodeSent)
  {
    char buf[64];
    sprintf(buf, "{ \"type\": \"rf\", \"code\":%i}\0", _lastRFCode);
    cfginterface.publish(buf, false);
    _lastRFCodeSent = _lastRFCode;
    Serial.println(!"RF CODE SENT");
  }

  if ((millis() - _lastRFMessage) > RFCODEDEBOUNCE) {
    _lastRFCode = 0;
    _lastRFCodeSent = 0;
  }
}
