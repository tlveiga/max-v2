#ifndef __WEBCONFIG_H__
#define __WEBCONFIG_H__

#include <ESP8266WebServer.h>
#include <FS.h>
#include <PubSubClient.h>
#include <list>
#include <map>
#include <utility>

enum class opts : uint8_t
{
  info_id,
  info_name,
  info_update_server,
  mqtt_server,
  mqtt_in_topic,
  mqtt_out_topic,
  mqtt_username,
  mqtt_password
};

enum class wifi_mode : uint8_t
{
  init,
  sta,
  ap
};

enum class wifi_status : uint8_t
{
  ready,     // ready to connect
  connected, // connected
  suspended, // failed to connect but will retry
  failed     // failed to connect "wrong password"
};

typedef struct
{
  String password;
  wifi_status status;
  unsigned long lastupdate;
} network_status;

typedef std::pair<String, int32_t> SSID_RSSI_pair;

class WebConfig
{
private:
  String _root;
  WiFiClient _espClient;
  PubSubClient _mqttClient;

  unsigned long _lastUpdateLoop;
  wifi_mode _lastmode;
  unsigned long _lastFWCheck;
  unsigned long _lastMqttReconnect;

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

  wifi_mode updateSTAMode();
  wifi_mode updateAPMode();
  void updateMqtt();

  bool saveNetworks();
  std::list<SSID_RSSI_pair> getNetworksInRange();
  wifi_status connect(String ssid, String password);

public:
  WebConfig(const char *root = NULL);
  void begin(ESP8266WebServer &server);
  void updateNewFirmware();

  void setMQTTCallback(MQTT_CALLBACK_SIGNATURE);

  const String getConfig(const opts opt);
  const bool getAutoUpdate();
  const bool getMQTTActive();

  boolean publish(const char *payload, boolean retained);
  boolean publish(const char *topic, const char *payload, boolean retained);

  void update();

  ~WebConfig();
};

#endif