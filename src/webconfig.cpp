#include "webconfig.h"
#include "constants.h"

#include "utils.h"
#include <ArduinoJson.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClient.h>

#include "../setup.h"

#include "progmem.h"

#include "version.h"
#include <FS.h>

struct saved_networks_struct {
  uint8_t enc;
  int32_t rssi;
  bool saved;
};

WebConfig::WebConfig(const char *root) {
  _root = String(root == NULL ? "/" : root);
  if (_root.charAt(_root.length() - 1) != '/')
    _root.concat("/");
}

void WebConfig::begin(ESP8266WebServer &server) {
  _lastUpdateLoop = -UPDATELOOPTIMESPAN;
  _lastFWCheck = -FWCHECKTIMESPAN;
  _lastMqttReconnect = -MQTTRECONNECTTIMESPAN;
  WiFi.disconnect();

  _mqttClient.setClient(_espClient);

  char info_id[10];
  sprintf(info_id, "%X\0", ESP.getChipId());
  _cfg[opts::info_id] = String(info_id);

  // PROGMEM init
  setupServer(server, _root.c_str());
  server.on(_root.c_str(), HTTP_GET, [&]() { handle_index_html(server); });

  beginInfo(server);
  beginMQTT(server);
  beginStatus(server);
  beginWifi(server);

  server.on("/ping", HTTP_GET, [&]() { server.send(200); });

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

  server.on("/update", HTTP_POST, [&]() {
    Serial.println("Call /update");
    server.send(200, "application/json", R_OK);
    updateNewFirmware();
  });
}

void WebConfig::beginInfo(ESP8266WebServer &server) {

  /* Reading config values or using defaults */
  const size_t capacity = JSON_OBJECT_SIZE(7) + 2048; // change to real values
  DynamicJsonDocument doc(capacity);

  if (readJSONFile(INFOFILE, doc)) {
    _cfg[opts::info_name] = doc["name"].as<String>();
    _cfg[opts::info_update_server] = doc["update_server"].as<String>();
    _info_auto_update = doc["auto_update"].as<bool>();
  }

  if (_cfg[opts::info_name].length() == 0) {
    char info_name[32];
    sprintf(info_name, "%s-%s\0", FWCODE, _cfg[opts::info_id].c_str());
    _cfg[opts::info_name] = String(info_name);
  }

  if (_cfg[opts::info_update_server].length() == 0)
    _cfg[opts::info_update_server] = String(DEFAULTUPDATESERVER);

  /* INFO */
  server.on("/info", HTTP_GET, [&]() {
    Serial.println("Call /info");
    char *buf = (char *)malloc(5120);
    sprintf(buf,
            "{\"id\":\"%s\",\"code\":\"%s\",\"name\":\"%s\",\"fw_version\":"
            "\"%s\",\"update_server\":\"%s\",\"auto_update\":%s}",
            _cfg[opts::info_id].c_str(), FWCODE, _cfg[opts::info_name].c_str(),
            FWVERSION, _cfg[opts::info_update_server].c_str(),
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
void WebConfig::beginMQTT(ESP8266WebServer &server) {
  /* Reading config values or using defaults */
  const size_t capacity = JSON_OBJECT_SIZE(4) + 512; // change to real values
  DynamicJsonDocument doc(capacity);
  if (readJSONFile(MQTTFILE, doc)) {
    _cfg[opts::mqtt_server] = doc["server"].as<String>();
    _cfg[opts::mqtt_in_topic] = doc["in_topic"].as<String>();
    _cfg[opts::mqtt_out_topic] = doc["out_topic"].as<String>();
    _mqtt_active = doc["active"].as<bool>();
  }

  if (_cfg[opts::mqtt_server].length() == 0)
    _cfg[opts::mqtt_server] = String(DEFAULTMQTTSERVER);

  if (_cfg[opts::mqtt_in_topic].length() == 0)
    _cfg[opts::mqtt_in_topic] = String("in");

  if (_cfg[opts::mqtt_out_topic].length() == 0)
    _cfg[opts::mqtt_out_topic] = String("out");

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

  server.on("/mqtt/test", HTTP_POST, [&]() {
    if (_mqttClient.connected()) {
      char msg[32];
      snprintf(msg, 50, "hello world #%x", millis());
      Serial.print("Publish message: ");
      Serial.println(msg);
      _mqttClient.publish("outTopic", msg);
      server.send(200, "application/json", R_SUCCESS);
    }
    else
      server.send(200, "application/json", R_FAILED);
  });
}

void WebConfig::beginStatus(ESP8266WebServer &server) {}
void WebConfig::beginWifi(ESP8266WebServer &server) {
  /* Reading config values or using defaults */
  const size_t capacity =
      JSON_OBJECT_SIZE(1) + 128 * JSON_OBJECT_SIZE(WL_NETWORKS_LIST_MAXNUM);
  DynamicJsonDocument doc(capacity);

  if (readJSONFile(WIFIFILE, doc)) {
    JsonObject networks = doc["networks"].as<JsonObject>();
    if (!networks.isNull()) {
      unsigned long now = millis();
      for (JsonPair kv : networks) {
        Serial.println(kv.key().c_str());
        Serial.println(kv.value().as<char *>());
        _wifi_networks[String(kv.key().c_str())] = {kv.value().as<String>(),
                                                    wifi_status::ready, now};
      }
    }
  }

  // for test only
  _wifi_networks[String(CFG_SSID)] = {String(CFG_PASSWORD), wifi_status::ready,
                                      0};

  WiFi.hostname(_cfg[opts::info_name]);
  _lastmode = _wifi_networks.size() > 0 ? wifi_mode::sta : wifi_mode::init;

  server.on("/wifi", HTTP_GET, [&]() {
    Serial.println("Call /wifi");
    std::map<String, struct saved_networks_struct> ssids;
    for (std::pair<String, network_status> nt : _wifi_networks)
      ssids[nt.first] = {ENC_TYPE_NONE, MINRSSILEVEL, true};

    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; ++i) {
      String ssid = WiFi.SSID(i);
      uint8_t enc = WiFi.encryptionType(i);
      int32_t rssi = WiFi.RSSI(i);
      std::map<String, struct saved_networks_struct>::iterator it =
          ssids.find(ssid);
      if (it != ssids.end()) {
        it->second.enc = enc;
        it->second.rssi = rssi;
      } else {
        ssids[ssid] = {enc, rssi, false};
      }
    }
    WiFi.scanDelete();

    String value = String("{ \"networks\": [");
    char buf[128];
    bool first = true;
    for (auto val : ssids) {
      if (!first)
        value += ",";
      first = false;
      sprintf(buf, "{\"ssid\":\"%s\",\"rssi\":%d,\"open\": %s,\"saved\":%s}\0",
              val.first.c_str(), val.second.rssi,
              val.second.enc == ENC_TYPE_NONE ? "true" : "false",
              val.second.saved ? "true" : "false");
      value += buf;
    }
    value +=
        "], \"max_number_networks\":" + String(WL_NETWORKS_LIST_MAXNUM) + "}";
    server.send(200, "application/json", value);
  });

  server.on("/wifi", HTTP_POST, [&]() {
    Serial.println("Post /wifi");
    const size_t post_capacity = JSON_OBJECT_SIZE(2) + 512;
    DynamicJsonDocument post_doc(capacity);
    deserializeJson(post_doc, server.arg("plain").c_str());

    String ssid = post_doc["ssid"].as<String>();
    String password = post_doc["password"].as<String>();
    _wifi_networks[ssid] = {password, wifi_status::ready, 0};

    saveNetworks();

    server.send(200, "application/json", R_OK);
    connect(ssid, password);
  });

  server.on("/wifi/connect", HTTP_POST, [&]() {
    Serial.println("Post /wifi/connect");
    const size_t capacity = JSON_OBJECT_SIZE(2) + 512;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, server.arg("plain").c_str());

    String ssid = doc["ssid"].as<String>();

    std::map<String, network_status>::iterator it = _wifi_networks.find(ssid);
    if (it != _wifi_networks.end()) {
      server.send(200, "application/json", R_OK);
      connect(it->first, it->second.password);
    } else {
      server.send(200, "application/json", R_NOK);
    }
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

    if (WiFi.SSID() == ssid)
      WiFi.disconnect();
    saveNetworks();

    server.send(200, "application/json", R_OK);
  });

  server.on("/wifi/status", HTTP_GET, [&]() {
    String mode;
    switch (WiFi.getMode()) {
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

void WebConfig::createIfNotFound(const char *filename) {
  if (!SPIFFS.exists(filename)) {
    Serial.print(filename);
    Serial.println(" not found.");
    File file = SPIFFS.open(filename, "w");
    if (!file)
      Serial.println(F("failed: creating file"));
    else
      file.close();
  }
}

void WebConfig::update() {
  // FALTA VERIFICAR SE HÁ ALGUEM A PINGAR O SITE
  // SE HOUVER DEVE ALTERAR O ESTADO DA REDE AUTOMATICAMENTE

  unsigned long elapsed = millis() - _lastUpdateLoop;
  if (elapsed > UPDATELOOPTIMESPAN) {
    if (_lastmode == wifi_mode::sta)
      _lastmode = updateSTAMode();
    else
      _lastmode = updateAPMode();

    _lastUpdateLoop = millis();
  }

  if (_info_auto_update && _lastmode == wifi_mode::sta) {
    elapsed = millis() - _lastFWCheck;
    if (elapsed > FWCHECKTIMESPAN)
      updateNewFirmware();

    _lastFWCheck = millis();
  }

  updateMqtt();
}

wifi_mode WebConfig::updateSTAMode() {
  wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED) {
    int32_t rssi = WiFi.RSSI();
    if (rssi < MINRSSILEVEL) {
      Serial.println("Poor signal strength");
      WiFi.disconnect();
      status = WL_DISCONNECTED;
      delay(10);
    }
  }

  if (status != WL_CONNECTED) {
    std::list<SSID_RSSI_pair> inrange = getNetworksInRange();
    for (SSID_RSSI_pair pair : inrange) {
      std::map<String, network_status>::iterator it =
          _wifi_networks.find(pair.first);
      status =
          connect(pair.first, it->second.password) == wifi_status::connected
              ? WL_CONNECTED
              : WL_DISCONNECTED;
      it->second.lastupdate = millis();
      if (status == WL_CONNECTED)
        break;
    }
  }

  return status == WL_CONNECTED ? wifi_mode::sta : wifi_mode::init;
}

wifi_mode WebConfig::updateAPMode() {
  if (_lastmode == wifi_mode::init) {
    String ssid = _cfg[opts::info_name].length() == 0 ? String(FWCODE)
                                                      : _cfg[opts::info_name];
    WiFi.softAP(ssid);
    Serial.print("Access Point \"");
    Serial.print(ssid);
    Serial.println("\" started");

    Serial.print("IP address:\t");
    Serial.println(WiFi.softAPIP());
    return wifi_mode::ap;
  } else {
    std::list<SSID_RSSI_pair> inrange = getNetworksInRange();
    return inrange.size() > 0 ? wifi_mode::sta : wifi_mode::ap;
  }
}

void WebConfig::updateMqtt() {
  if (!_mqtt_active)
    return;
  if (_lastmode == wifi_mode::sta && WiFi.status() == WL_CONNECTED &&
      !_mqttClient.connected() &&
      millis() - _lastMqttReconnect > MQTTRECONNECTTIMESPAN) {

    _mqttClient.setServer(_cfg[opts::mqtt_server].c_str(), 1883);

    Serial.print("Connecting to MQTT server: ");
    Serial.print(_cfg[opts::mqtt_server].c_str());
    Serial.print(" ");
    String willTopic = String(_cfg[opts::info_name]);
    willTopic.concat("/offline");
    const char *willMessage = "goodbye";

    if (_mqttClient.connect(_cfg[opts::info_name].c_str(), willTopic.c_str(), 1,
                            false, willMessage)) {
      Serial.println("connected");
      String helloTopic = String(_cfg[opts::info_name]);
      helloTopic.concat("/online");
      _mqttClient.publish(helloTopic.c_str(), "hello");
      String inTopic = String(_cfg[opts::info_name]);
      inTopic.concat("/");
      inTopic.concat(_cfg[opts::mqtt_in_topic]);
      Serial.printf("Subscribed to: %s\n", inTopic.c_str());
      _mqttClient.subscribe(inTopic.c_str());
    } else {
      Serial.println("failed");
    }
    _lastMqttReconnect = millis();
  }

  if (_mqttClient.connected())
    _mqttClient.loop();
}

void WebConfig::setMQTTCallback(MQTT_CALLBACK_SIGNATURE) {
  _mqttClient.setCallback(callback);
}

bool WebConfig::saveNetworks() {

  Serial.println("Saving networks");

  File file = SPIFFS.open(WIFIFILE, "w");
  if (!file) {
    Serial.println(F("failed: creating file"));
    return false;
  }
  file.print("{\"networks\": {");
  bool first = true;
  for (std::pair<const String, network_status> &pair : _wifi_networks) {
    if (!first)
      file.print(",");
    first = false;

    file.write('"');
    for (int i = 0; i < pair.first.length(); i++) {
      char a = pair.first.charAt(i);
      if (a == '"') {
        file.write('\\');
        file.write('"');
      } else
        file.write(a);
    }
    file.print("\":\"");
    for (int i = 0; i < pair.second.password.length(); i++) {
      char a = pair.second.password.charAt(i);
      if (a == '"') {
        file.write('\\');
        file.write('"');
      } else
        file.write(a);
    }
    file.write('"');
  }
  file.printf("}}");
  file.close();
  return true;
}

std::list<SSID_RSSI_pair> WebConfig::getNetworksInRange() {
  std::list<SSID_RSSI_pair> list;
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    String ssid = WiFi.SSID(i);
    std::map<String, network_status>::iterator it = _wifi_networks.find(ssid);
    if (it != _wifi_networks.end() &&
        it->second.status != wifi_status::failed) {
      int32_t rssi = WiFi.RSSI(i);
      if (rssi > MINRSSILEVEL) {
        Serial.printf("Found network: %s  ->  rssi: %d\n", ssid.c_str(), rssi);
        auto pair = std::make_pair(ssid, rssi);
        list.push_back(pair);
      }
    }
  }
  WiFi.scanDelete();

  list.sort([](const SSID_RSSI_pair &a, const SSID_RSSI_pair &b) {
    return a.second > b.second ? true : false;
  });

  return list;
}

wifi_status WebConfig::connect(String ssid, String password) {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.disconnect();
  delay(10);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  for (int i = 0; i < CONNECTIONRETRIES && WiFi.status() != WL_CONNECTED; i++) {
    Serial.print(".");
    delay(200);
  }

  Serial.println(WiFi.status() == WL_CONNECTED ? "success" : "failed");
  if (WiFi.status() == WL_CONNECTED)
    Serial.println(WiFi.localIP().toString());

  return WiFi.status() == WL_CONNECTED ? wifi_status::connected
                                       : wifi_status::failed;
}

void WebConfig::updateNewFirmware() {
  Serial.println("Checking for updates");
  WiFiClient client;
  HTTPClient http;
  String url = _cfg[opts::info_update_server];
  if (url.length() == 0) {
    Serial.println("Server url empty");
    return;
  }
  if (url.charAt(url.length() - 1) != '/')
    url += '/';
  if (http.begin(client, url + String(FWCODE) + String(".json"))) {
    int httpCode = http.GET();
    // httpCode will be negative on error
    if (httpCode > 0) {
      const size_t capacity = JSON_OBJECT_SIZE(4) + 512;
      DynamicJsonDocument doc(capacity);
      deserializeJson(doc, client);
      const char *fw_version = doc["fw_version"].as<const char *>();
      auto server_version = Version(fw_version);
      auto my_version = Version(FWVERSION);
      if (my_version < server_version) {
        String filename = doc["fw_file"].as<String>();
        ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
        Serial.print("FW update available ");
        Serial.println(url + filename);
        t_httpUpdate_return ret = ESPhttpUpdate.update(url + filename);
        switch (ret) {
        case HTTP_UPDATE_FAILED:
          Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n",
                        ESPhttpUpdate.getLastError(),
                        ESPhttpUpdate.getLastErrorString().c_str());
          break;

        case HTTP_UPDATE_NO_UPDATES:
          Serial.println("HTTP_UPDATE_NO_UPDATES");
          break;

        case HTTP_UPDATE_OK:
          Serial.println("HTTP_UPDATE_OK");
          break;
        }
      } else {
        Serial.println("FW up to date");
      }
    } else
      Serial.println("error getting file");
  } else
    Serial.println("server unreachable");
}

const String WebConfig::getConfig(const opts opt) { return _cfg[opt]; }
const bool WebConfig::getAutoUpdate() { return _info_auto_update; }
const bool WebConfig::getMQTTActive() { return _mqtt_active; };

WebConfig::~WebConfig() {}