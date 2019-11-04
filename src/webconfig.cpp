#include "webconfig.h"
#include "constants.h"
#include <FS.h>

#include "utils.h"

WebConfig::WebConfig() {}

void WebConfig::begin(ESP8266WebServer &server) {

  sprintf(_info_id, "%X\0", ESP.getChipId());
  /* Reading config values or using defaults */
  const size_t capacity = JSON_OBJECT_SIZE(7) + 2048; // change to real values
  DynamicJsonDocument doc(capacity);

  if (readJSONFile("/info.json", doc)) {
    sprintf(_info_name, doc["name"].as<const char *>());
    sprintf(_info_update_server, doc["update_server"].as<const char *>());
  } else {
    sprintf(_info_name, "%s-%s\0", FWCODE, _info_id);
    sprintf(_info_update_server, "%s\0", DEFAULTMQTT);
    _info_auto_update = 0;
  }

  server.serveStatic("/", SPIFFS, "/index.html");
  server.serveStatic("/main.js", SPIFFS, "/main.js");

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

  server.on("/test", HTTP_GET, [&]() {
    FSInfo fs_info;
    SPIFFS.info(fs_info);

    Serial.println("Call /test");
    char *buf = (char *)malloc(5120);
    sprintf(
        buf,
        "{\"totalBytes\":%d,\"usedBytes\":%d,\"blocksize\":%d,\"pageSize\":%d, "
        "\"maxOpenFiles\":%d,\"maxPathLength\":%d}",
        fs_info.totalBytes, fs_info.usedBytes, fs_info.blockSize,
        fs_info.pageSize, fs_info.maxOpenFiles, fs_info.maxPathLength);
    server.send(200, "application/json", buf);
    free(buf);
  });
}

const char *WebConfig::getInfoId() { return _info_id; };
const char *WebConfig::getInfoName() { return _info_name; };
const char *WebConfig::getUpdateServer() { return _info_update_server; };
const uint8_t WebConfig::getAutoUpdate() { return _info_auto_update; };

WebConfig::~WebConfig() {}