#ifndef __UTILS_h__
#define __UTILS_h__

#include <ArduinoJson.h>

bool readJSONFile(const char *filename, DynamicJsonDocument &json);
bool writeJSONFile(const char *filename, const DynamicJsonDocument json);

bool readFile(const char *filename, String &file);
bool writeFile(const char *filename, String file);

bool readJSONFromSPI(uint32_t addr, uint32_t size, DynamicJsonDocument &json);
bool writeJSONToSPI(uint32_t addr, uint32_t size,
                    const DynamicJsonDocument json);

#endif