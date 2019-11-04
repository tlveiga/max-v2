#ifndef __WEBCONFIG_H__
#define __WEBCONFIG_H__

#include <ESP8266WebServer.h>

class WebConfig {
private:
  char _info_id[8];
  char _info_name[64];
  char _info_update_server[256];
  uint8_t _info_auto_update;

public:
  WebConfig();
  void begin(ESP8266WebServer &server);

  const char *getInfoId();
  const char *getInfoName();
  const char *getUpdateServer();
  const uint8_t getAutoUpdate();

  ~WebConfig();
};

#endif