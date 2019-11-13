#ifndef __WEBCONFIG_H__
#define __WEBCONFIG_H__

#include <ESP8266WebServer.h>
#include <FS.h>
#include <map>

enum class opts
{
  info_id,
  info_name,
  info_update_server,
  mqtt_server,
  mqtt_in_topic,
  mqtt_out_topic,
  ui_version,
  ui_date
};

class WebConfig
{
private:
  char _ui_version[16];
  char _ui_date[12];

  char _info_id[8];
  char _info_name[64];
  char _info_update_server[256];
  bool _info_auto_update;
  char _mqtt_server[256];
  char _mqtt_in_topic[256];
  char _mqtt_out_topic[256];
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

public:
  WebConfig();
  void begin(ESP8266WebServer &server);

  const char *getUIVersion();
  const char *getUIDate();

  const char *getInfoId();
  const char *getInfoName();
  const char *getUpdateServer();
  const bool getAutoUpdate();

  const char *getMQTTServer();
  const char *getMQTTInTopic();
  const char *getMQTTOutTopic();
  const bool getMQTTActive();

  ~WebConfig();
};

#endif