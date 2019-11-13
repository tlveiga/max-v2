#ifndef __WEBCONFIG_H__
#define __WEBCONFIG_H__

#include <ESP8266WebServer.h>
#include <FS.h>
#include <map>

enum class opts {
  info_id,
  info_name,
  info_update_server,
  mqtt_server,
  mqtt_in_topic,
  mqtt_out_topic,
  ui_version,
  ui_date
};

class WebConfig {
private:
  unsigned long _lastUpdateLoop;

  bool _info_auto_update;
  bool _mqtt_active;

  std::map<String, String> _wifi_networks;
  std::map<opts, String> _cfg;

  bool _validSPIFFSUpdate;
  File _uploadFile;

  void beginInfo(ESP8266WebServer &server);
  void beginMQTT(ESP8266WebServer &server);
  void beginStatus(ESP8266WebServer &server);
  void beginWifi(ESP8266WebServer &server);

  void handleFileUpload(const char *filename, ESP8266WebServer &server);
  void handleUploadResult(ESP8266WebServer &server);

  void createIfNotFound(const char *filename);

  void updateSTAMode();
  void updateAPMode();

public:
  WebConfig();
  void begin(ESP8266WebServer &server);

  const String getConfig(const opts opt);
  const bool getAutoUpdate();
  const bool getMQTTActive();

  void update();

  ~WebConfig();
};

#endif