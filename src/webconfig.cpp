#include "webconfig.h"
#include "constants.h"

#include "utils.h"
#include <ArduinoJson.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#include "progmem.h"

WebConfig::WebConfig() {}

void WebConfig::begin(ESP8266WebServer &server)
{
  _lastUpdateLoop = -UPDATELOOPTIMESPAN;
  WiFi.disconnect();

  char info_id[10];
  sprintf(info_id, "%X\0", ESP.getChipId());
  _cfg[opts::info_id] = String(info_id);

  createIfNotFound("/index.html");
  createIfNotFound("/main.js");

  server.serveStatic("/", SPIFFS, "/index.html");
  server.serveStatic("/main.js", SPIFFS, "/main.js");

  setupServer(server);

  beginInfo(server);
  beginMQTT(server);
  beginStatus(server);
  beginWifi(server);

  server.on("/ping", HTTP_GET, [&]() { server.send(200); });

  /* Web interface uploads */
  server.on("/web-interface-upload/index", HTTP_POST,
            [&]() { handleUploadResult(server); },
            [&]() { handleFileUpload("/index.html", server); });

  server.on("/web-interface-upload/main", HTTP_POST,
            [&]() { handleUploadResult(server); },
            [&]() { handleFileUpload("/main.js", server); });

  server.on("/web-interface-upload/version", HTTP_POST,
            [&]() { handleUploadResult(server); },
            [&]() { handleFileUpload(UIVERSION, server); });
  /* END Web interface uploads */

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

void WebConfig::beginInfo(ESP8266WebServer &server)
{

  /* Reading config values or using defaults */
  const size_t capacity = JSON_OBJECT_SIZE(7) + 2048; // change to real values
  DynamicJsonDocument doc(capacity);

  if (readJSONFile(UIVERSION, doc))
  {
    _cfg[opts::ui_version] = doc["version"].as<String>();
    _cfg[opts::ui_date] = doc["date"].as<String>();
  }

  if (_cfg[opts::ui_version].length() == 0)
    _cfg[opts::ui_version] = String("none");
  if (_cfg[opts::ui_date].length() == 0)
    _cfg[opts::ui_date] = String("0");

  if (readJSONFile(INFOFILE, doc))
  {
    _cfg[opts::info_name] = doc["name"].as<String>();
    _cfg[opts::info_update_server] = doc["update_server"].as<String>();
    _info_auto_update = doc["auto_update"].as<bool>();
  }

  if (_cfg[opts::info_name].length() == 0)
  {
    char info_name[32];
    sprintf(info_name, "%s-%s\0", FWCODE, _cfg[opts::info_id].c_str());
    _cfg[opts::info_name] = String(info_name);
  }

  if (_cfg[opts::info_update_server].length() == 0)
    _cfg[opts::info_update_server] = String(DEFAULTUPDATESERVER);

  WiFi.hostname(_cfg[opts::info_name]);
  WiFi.mode(_wifi_networks.size() > 0 ? WIFI_STA : WIFI_AP);
  _lastmode = WIFI_OFF;

  /* INFO */
  server.on("/info", HTTP_GET, [&]() {
    Serial.println("Call /info");
    char *buf = (char *)malloc(5120);
    sprintf(buf,
            "{\"id\":\"%s\",\"code\":\"%s\",\"name\":\"%s\",\"fw_version\":"
            "\"%"
            "s\",\"fw_date\":%d,\"ui_version\":\"%s\",\"ui_date\":%s, "
            "\"update_server\":\"%s\",\"auto_update\":%s}",
            _cfg[opts::info_id].c_str(), FWCODE, _cfg[opts::info_name].c_str(),
            FWVERSION, FWDATE, _cfg[opts::ui_version].c_str(),
            _cfg[opts::ui_date].c_str(), _cfg[opts::info_update_server].c_str(),
            _info_auto_update ? "true" : "false");
    server.send(200, "application/json", buf);
    free(buf);
  });

  server.on("/info", HTTP_POST, [&]() {
    Serial.println("Post /info");
    const size_t capacity = JSON_OBJECT_SIZE(7) + 2048;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, server.arg("plain").c_str());

    _cfg[opts::info_name] = doc["name"].as<String>();
    _cfg[opts::info_update_server] = doc["update_server"].as<String>();
    _info_auto_update = doc["auto_update"].as<bool>();

    WiFi.hostname(_cfg[opts::info_name]);

    writeJSONFile(INFOFILE, doc);
    server.send(200, "application/json", R_OK);
  });
}
void WebConfig::beginMQTT(ESP8266WebServer &server)
{
  /* Reading config values or using defaults */
  const size_t capacity = JSON_OBJECT_SIZE(4) + 512; // change to real values
  DynamicJsonDocument doc(capacity);
  if (readJSONFile(MQTTFILE, doc))
  {
    _cfg[opts::mqtt_server] = doc["server"].as<String>();
    _cfg[opts::mqtt_in_topic] = doc["in_topic"].as<String>();
    _cfg[opts::mqtt_out_topic] = doc["out_topic"].as<String>();
    _mqtt_active = doc["active"].as<bool>();
  }

  if (_cfg[opts::mqtt_server].length() == 0)
    _cfg[opts::mqtt_server] = String(DEFAULTMQTTSERVER);

  if (_cfg[opts::mqtt_in_topic].length() == 0)
  {
    char topic[16];
    sprintf(topic, "%s/in\0", FWCODE);
    _cfg[opts::mqtt_in_topic] = String(topic);
  }

  if (_cfg[opts::mqtt_out_topic].length() == 0)
  {
    char topic[16];
    sprintf(topic, "%s/out\0", FWCODE);
    _cfg[opts::mqtt_out_topic] = String(topic);
  }

  server.on("/mqtt", HTTP_GET, [&]() {
    Serial.println("Call /mqtt");
    char *buf = (char *)malloc(5120);
    sprintf(buf,
            "{\"server\":\"%s\",\"in_topic\":\"%s\",\"out_topic\":\"%s\","
            "\"active\":%s}",
            _cfg[opts::mqtt_server].c_str(), _cfg[opts::mqtt_in_topic].c_str(),
            _cfg[opts::mqtt_out_topic].c_str(),
            _mqtt_active ? "true" : "false");
    server.send(200, "text/plain", buf);
    free(buf);
  });

  server.on("/mqtt", HTTP_POST, [&]() {
    Serial.println("Post /mqtt");
    const size_t capacity = JSON_OBJECT_SIZE(7) + 2048;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, server.arg("plain").c_str());

    _cfg[opts::mqtt_server] = doc["server"].as<String>();
    _cfg[opts::mqtt_in_topic] = doc["in_topic"].as<String>();
    _cfg[opts::mqtt_out_topic] = doc["out_topic"].as<String>();
    _mqtt_active = doc["active"].as<bool>();

    writeJSONFile(MQTTFILE, doc);
    server.send(200, "application/json", R_OK);
  });
}

void WebConfig::handleFileUpload(const char *filename,
                                 ESP8266WebServer &server)
{
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START)
  {
    String path = String(filename);
    if (!path.startsWith("/"))
    {
      path = "/" + path;
    }
    _uploadFile = SPIFFS.open(path, "w");

    if (_uploadFile)
    {
      Serial.println("upload begin started!");
      _validSPIFFSUpdate = true;
    }
    else
    {
      Serial.println("upload begin failed!");
      _validSPIFFSUpdate = false;
    }
  }
  else if (upload.status == UPLOAD_FILE_WRITE)
  {
    if (_uploadFile)
    {
      _uploadFile.write(upload.buf, upload.currentSize);
    }
  }
  else if (upload.status == UPLOAD_FILE_END)
  {
    if (_uploadFile)
    {
      _uploadFile.close();
      Serial.print("handleFileUpload Size: ");
      Serial.println(upload.totalSize);
      if (upload.totalSize == 0)
        _validSPIFFSUpdate = false;
    }
  }
}

void WebConfig::handleUploadResult(ESP8266WebServer &server)
{
  Serial.println(_validSPIFFSUpdate ? "OK" : "NOK");
  if (_validSPIFFSUpdate)
    server.send_P(200, PSTR("text/html"), NOHANDLER_upload_success_html);
  else
    server.send_P(200, PSTR("text/html"), NOHANDLER_upload_error_html);
}

void WebConfig::beginStatus(ESP8266WebServer &server) {}
void WebConfig::beginWifi(ESP8266WebServer &server)
{
  // read config;
  server.on("/wifi", HTTP_GET, [&]() {
    String value = String("{ \"networks\": [");

    char buf[128];
    int n = WiFi.scanNetworks();
    std::map<String, bool> ssids;
    for (int i = 0; i < n; ++i)
    {
      if (ssids.count(WiFi.SSID(i)) > 0)
        continue;
      if (ssids.size() > 0)
        value += ",";
      ssids[WiFi.SSID(i)] = true;
      std::map<String, network_status>::iterator it = _wifi_networks.find(WiFi.SSID(i));
      sprintf(buf, "{\"ssid\":\"%s\",\"rssi\":%d,\"open\": %s, \"saved\":%s}\0",
              WiFi.SSID(i).c_str(), WiFi.RSSI(i),
              (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? "true" : "false",
              it != _wifi_networks.end() ? "true" : "false");
      value += buf;
    }
    value +=
        "], \"max_number_networks\":" + String(WL_NETWORKS_LIST_MAXNUM) + "}";
    server.send(200, "application/json", value);
  });

  server.on("/wifi", HTTP_POST, [&]() {
    Serial.println("Post /wifi");
    const size_t capacity = JSON_OBJECT_SIZE(2) + 512;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, server.arg("plain").c_str());

    String ssid = doc["ssid"].as<String>();
    String password = doc["password"].as<String>();

    std::map<String, network_status>::iterator it = _wifi_networks.find(ssid);
    if (it != _wifi_networks.end())
      Serial.printf("Exists");
    _wifi_networks[ssid] = {password, WL_IDLE_STATUS, 0};

    server.send(200, "application/json", R_OK);
  });

  server.on("/wifi/forget", HTTP_POST, [&]() {
    Serial.println("Post /wifi");
    const size_t capacity = JSON_OBJECT_SIZE(1) + 512;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, server.arg("plain").c_str());

    String ssid = doc["ssid"].as<String>();

    std::map<String, network_status>::iterator it = _wifi_networks.find(ssid);
    if (it != _wifi_networks.end())
      _wifi_networks.erase(ssid);
    server.send(200, "application/json", R_OK);
  });

  server.on("/wifi/status", HTTP_GET, [&]() {
    String mode;
    switch (WiFi.getMode())
    {
    case WIFI_STA:
      mode = "station";
      break;
    case WIFI_AP:
      mode = "ap";
      break;
    case WIFI_AP_STA:
      mode = "ap & station";
      break;
    default:
      mode = "unknown";
      break;
    }

    String value = "{";
    value += "\"mode\":\"" + mode + "\",";
    value += "\"hostname\":\"" + WiFi.hostname() + "\",";
    value += "\"ssid\":\"" + WiFi.SSID() + "\",";
    value += "\"rssi\":" + String(WiFi.RSSI()) + ",";
    value += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    value += "\"dns\":\"" + WiFi.dnsIP().toString() + "\",";
    value += "\"gateway\":\"" + WiFi.gatewayIP().toString() + "\",";
    value += "\"mac\":\"" + WiFi.macAddress() + "\"";
    value += "}";

    server.send(200, "application/json", value);
  });
}

void WebConfig::createIfNotFound(const char *filename)
{
  if (!SPIFFS.exists(filename))
  {
    Serial.print(filename);
    Serial.println(" not found.");
    File file = SPIFFS.open(filename, "w");
    if (!file)
      Serial.println(F("failed: creating file"));
    else
      file.close();
  }
}

void WebConfig::update()
{
  unsigned long elapsed = millis() - _lastUpdateLoop;
  if (elapsed > UPDATELOOPTIMESPAN)
  {
    auto mode = WiFi.getMode();
    if (mode == WiFiMode_t::WIFI_STA)
      updateSTAMode();
    else
      updateAPMode();

    _lastUpdateLoop = millis();
    _lastmode = WiFi.getMode();
  }
}

void WebConfig::updateSTAMode()
{
  Serial.println("update STA Mode");
  wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED)
  {
    int32_t rssi = WiFi.RSSI();
    if (rssi < MINRSSILEVEL)
    {
      WiFi.disconnect();
      status = WL_DISCONNECTED;
      delay(10);
    }
  }

  if (status != WL_CONNECTED)
  {
  }

  /*

    WL_NO_SHIELD        = 255,   // for compatibility with WiFi Shield library
    WL_IDLE_STATUS      = 0,
    WL_NO_SSID_AVAIL    = 1,
    WL_SCAN_COMPLETED   = 2,
    WL_CONNECTED        = 3,
    WL_CONNECT_FAILED   = 4,
    WL_CONNECTION_LOST  = 5,
    WL_DISCONNECTED     = 6

*/
  Serial.println(status);
}

void WebConfig::updateAPMode()
{
  if (_lastmode != WIFI_AP && _lastmode != WIFI_AP_STA)
  {
    String ssid = _cfg[opts::info_name].length() == 0 ? String(FWCODE) : _cfg[opts::info_name];
    WiFi.softAP(ssid);
    Serial.print("Access Point \"");
    Serial.print(ssid);
    Serial.println("\" started");

    Serial.print("IP address:\t");
    Serial.println(WiFi.softAPIP());
  }
}

const String WebConfig::getConfig(const opts opt) { return _cfg[opt]; }
const bool WebConfig::getAutoUpdate() { return _info_auto_update; }
const bool WebConfig::getMQTTActive() { return _mqtt_active; };

WebConfig::~WebConfig() {}