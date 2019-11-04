#ifndef __UTILS_h__
#define __UTILS_h__

#include <ArduinoJson.h>

bool readJSONFile(const char *filename, DynamicJsonDocument &json);

#endif