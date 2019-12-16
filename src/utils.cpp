#include "utils.h"
#include "constants.h"
#include <FS.h>

bool readJSONFile(const char *filename, DynamicJsonDocument &json)
{
  Serial.print(F("Loading file: "));
  Serial.println(filename);
  File file = SPIFFS.open(filename, "r");
  if (!file)
  {
    Serial.println(F("failed: open file"));
    return false;
  }

  size_t size = file.size();

  if (size > MAXJSONFILESIZE)
  {
    Serial.println(F("failed: file too big"));
    file.close();
    return false;
  }

  Serial.print(F("Deserializing from file. Size = "));
  Serial.println(size);

  char *buf = (char *)malloc(sizeof(char) * (size + 1));
  file.readBytes(buf, size);
  buf[size] = 0;
  DeserializationError error = deserializeJson(
      json, (const char *)buf); // the cast forces ArduinoJson to make a copy
  free(buf);
  file.close();
  if (error)
  {
    Serial.println(F("failed: deserializeJson"));
    Serial.println(error.c_str());
    return false;
  }
  return true;
};

bool writeJSONFile(const char *filename, const DynamicJsonDocument json)
{
  Serial.print(F("Saving file: "));
  Serial.println(filename);

  File file = SPIFFS.open(filename, "w");
  if (!file)
  {
    Serial.println(F("failed: creating file"));
    return false;
  }

  Serial.print(F("serializing to file. Size = "));
  uint16 size = serializeJson(json, file);
  Serial.println(size);

  file.close();

  return false;
};

bool readJSONFromSPI(uint32_t addr, uint32_t size, DynamicJsonDocument &json)
{
  return false;
}
bool writeJSONToSPI(uint32_t addr, uint32_t size,
                    const DynamicJsonDocument json)
{
  return false;
}

bool writeFile(const char *filename, String str)
{
  Serial.print(F("Saving file: "));
  Serial.println(filename);

  File file = SPIFFS.open(filename, "w");
  if (!file)
  {
    Serial.println(F("failed: creating file"));
    return false;
  }
  Serial.printf("Writing %d bytes", str.length());

  file.close();

  return true;
}

bool createIfNotFound(const char *filename)
{
  if (!SPIFFS.exists(filename))
  {
    Serial.print(filename);
    Serial.println(" not found.");
    File file = SPIFFS.open(filename, "w");
    if (!file)
    {
      Serial.println(F("failed: creating file"));
      return false;
    }
    else
    {
      file.close();
      return true;
    }
  }

  return true;
}