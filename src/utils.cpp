#include "utils.h"
#include <FS.h>

bool readJSONFile(const char *filename, DynamicJsonDocument &json) {
  File file = SPIFFS.open(filename, "r");

  if (!file) {
    Serial.print("Could not open file: ");
    Serial.println(filename);
    return false;
  }

  DeserializationError error = deserializeJson(json, file);
  if (error) {
    Serial.print("Failed to parse file: ");
    Serial.println(filename);
    return false;
  }
  file.close();

  return true;
}
