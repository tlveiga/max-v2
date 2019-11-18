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

typedef enum
{
  WM_INIT,
  WM_STA,
  WM_AP
} wifi_mode;
typedef enum
{
  WS_READY, // ready to connect 
  WS_CONNECTED, // connected
  WS_SUSPENDED, // failed to connect but will retry
  WS_FAILED // failed to connect "wrong password"
} wifi_status;

typedef struct
{
  String password;
  wifi_status status;
  unsigned long lastupdate;
} network_status;

class WebConfig
{
private:
  unsigned long _lastUpdateLoop;
  wifi_mode _lastmode;

  bool _info_auto_update;
  bool _mqtt_active;

  std::map<String, network_status> _wifi_networks;
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

  wifi_mode updateSTAMode();
  wifi_mode updateAPMode();

  bool saveNetworks();
  String getBestNetwork();

  wifi_status connectToBestNetwork();
  wifi_status connect(String ssid, String password);

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