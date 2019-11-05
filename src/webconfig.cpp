#include "webconfig.h"
#include "constants.h"
#include <FS.h>

#include "utils.h"

#include <Arduino.h>
#include <ArduinoJson.h>

WebConfig::WebConfig() {}

void WebConfig::begin(ESP8266WebServer &server) {

  sprintf(_info_id, "%X\0", ESP.getChipId());

  server.serveStatic("/", SPIFFS, "/index.html");
  server.serveStatic("/main.js", SPIFFS, "/main.js");

  beginInfo(server);
  beginMQTT(server);
  beginStatus(server);
  beginWifi(server);

  /* Upload SPIFFS */
  server.on("/web-interface-upload", HTTP_GET, [&]() {
    server.send(200, "text/html",
                "<html><body><form method=\"post\" "
                "enctype=\"multipart/form-data\"><input type=\"file\" "
                "name=\"name\"><input class=\"button\" type=\"submit\" "
                "value=\"Upload\"></form></html>");
  });

  server.on("/web-interface-upload", HTTP_POST,
            [&]() {
              Serial.println("uploaded");
              server.send(200, "text/plain", _validSPIFFSUpdate ? "OK" : "NOK");
            },
            [&]() {
              HTTPUpload &upload = server.upload();
              if (upload.status == UPLOAD_FILE_START) {
                FSInfo fs_info;
                SPIFFS.info(fs_info);

                if (!Update.begin(fs_info.totalBytes, U_SPIFFS, -1, 0)) {
                  Serial.println("Update.begin failed!");
                  _validSPIFFSUpdate = false;
                } else {
                  _validSPIFFSUpdate = true;
                }
              } else if (upload.status == UPLOAD_FILE_WRITE) {
                if (_validSPIFFSUpdate) {
                  Update.write(upload.buf, upload.currentSize);
                }
              } else if (upload.status == UPLOAD_FILE_END) {
                if (_validSPIFFSUpdate) {
                  if (!Update.end(true)) {
                    Serial.println("Update.end failed!");
                  }
                }
              }
            });
  /* END Upload SPIFFS */

  server.on("/test", HTTP_GET, [&]() {
    FSInfo fs_info;
    SPIFFS.info(fs_info);

    Serial.println("Call /test");
    char *buf = (char *)malloc(5120);
    sprintf(buf,
            "{\"totalBytes\":%d,\"usedBytes\":%d,\"blocksize\":%d,"
            "\"pageSize\":%d, "
            "\"maxOpenFiles\":%d,\"maxPathLength\":%d}",
            fs_info.totalBytes, fs_info.usedBytes, fs_info.blockSize,
            fs_info.pageSize, fs_info.maxOpenFiles, fs_info.maxPathLength);
    server.send(200, "application/json", buf);
    free(buf);
  });
}

void WebConfig::beginInfo(ESP8266WebServer &server) {
  /* Reading config values or using defaults */
  const size_t capacity = JSON_OBJECT_SIZE(7) + 2048; // change to real values
  DynamicJsonDocument doc(capacity);
  if (readJSONFile(INFOFILE, doc)) {
    sprintf(_info_name, doc["name"].as<const char *>());
    sprintf(_info_update_server, doc["update_server"].as<const char *>());
    _info_auto_update = doc["auto_update"].as<bool>();
  } else {
    sprintf(_info_name, "%s-%s\0", FWCODE, _info_id);
    sprintf(_info_update_server, "%s\0", DEFAULTUPDATESERVER);
    _info_auto_update = false;
  }

  /* INFO */
  server.on("/info", HTTP_GET, [&]() {
    Serial.println("Call /info");
    char *buf = (char *)malloc(5120);
    sprintf(buf,
            "{\"id\":\"%s\",\"code\":\"%s\",\"name\":\"%s\",\"fw_version\":"
            "\"%"
            "s\",\"fw_date\":%d,\"update_server\":\"%s\",\"auto_update\":%s}",
            _info_id, FWCODE, _info_name, FWVERSION, FWDATE,
            _info_update_server, _info_auto_update ? "true" : "false");
    server.send(200, "text/plain", buf);
    free(buf);
  });

  server.on("/info", HTTP_POST, [&]() {
    Serial.println("Post /info");
    const size_t capacity = JSON_OBJECT_SIZE(7) + 2048;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, server.arg("plain").c_str());

    writeJSONFile(INFOFILE, doc);
    server.send(200, "application/json", "{\"result\": \"ok\"}");
  });
}
void WebConfig::beginMQTT(ESP8266WebServer &server) {
  /* Reading config values or using defaults */
  const size_t capacity = JSON_OBJECT_SIZE(7) + 2048; // change to real values
  DynamicJsonDocument doc(capacity);
  if (readJSONFile(MQTTFILE, doc)) {
    sprintf(_mqtt_server, doc["server"].as<const char *>());
    sprintf(_mqtt_in_topic, doc["in_topic"].as<const char *>());
    sprintf(_mqtt_out_topic, doc["out_topic"].as<const char *>());
    _mqtt_active = doc["active"].as<bool>();
  } else {
    sprintf(_mqtt_server, "%s\0", DEFAULTMQTTSERVER);
    sprintf(_mqtt_in_topic, "%s/in\0", FWCODE);
    sprintf(_mqtt_out_topic, "%s/out\0", FWCODE);
    _mqtt_active = false;
  }

  server.on("/mqtt", HTTP_GET, [&]() {
    Serial.println("Call /mqtt");
    char *buf = (char *)malloc(5120);
    sprintf(buf,
            "{\"server\":\"%s\",\"in_topic\":\"%s\",\"out_topic\":\"%s\","
            "\"active\":%s}",
            _mqtt_server, _mqtt_in_topic, _mqtt_out_topic,
            _mqtt_active ? "true" : "false");
    server.send(200, "text/plain", buf);
    free(buf);
  });

  server.on("/mqtt", HTTP_POST, [&]() {
    Serial.println("Post /mqtt");
    const size_t capacity = JSON_OBJECT_SIZE(7) + 2048;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, server.arg("plain").c_str());

    writeJSONFile(MQTTFILE, doc);
    server.send(200, "application/json", "{\"result\": \"ok\"}");
  });
}
void WebConfig::beginStatus(ESP8266WebServer &server) {}
void WebConfig::beginWifi(ESP8266WebServer &server) {}

const char *WebConfig::getInfoId() { return _info_id; };
const char *WebConfig::getInfoName() { return _info_name; };
const char *WebConfig::getUpdateServer() { return _info_update_server; };
const bool WebConfig::getAutoUpdate() { return _info_auto_update; };
const char *WebConfig::getMQTTServer() { return _mqtt_server; };
const char *WebConfig::getMQTTInTopic() { return _mqtt_in_topic; };
const char *WebConfig::getMQTTOutTopic() { return _mqtt_out_topic; };
const bool WebConfig::getMQTTActive() { return _mqtt_active; };

WebConfig::~WebConfig() {}