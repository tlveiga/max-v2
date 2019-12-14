#ifndef __ALARM_H__
#define __ALARM_H__

#include <TimeLib.h>

// AlarmRaw - 32bits
// 7bits - dow
// 5bit - hours
// 6bit - minutes
// 1bit - active
// 1bit - repeat
// 12 bits - action

typedef uint32_t AlarmRaw;

typedef struct AlarmStruct
{
  int id;
  uint8_t dow;
  uint8_t hours;
  uint8_t minutes;
  uint8_t repeat;
  uint8_t active;
  uint16_t action;
} AlarmStruct;

typedef void (*AlarmCallback)(AlarmStruct alarm);

class Alarm
{
private:
  AlarmCallback _callback;
  AlarmRaw *_alarms;
  int _lastcheckminute;
  size_t _nalarms;
  size_t _arraysize;

public:
  Alarm(const AlarmCallback callback);
  ~Alarm();

  bool addAlarm(const AlarmStruct &alarm); // ignora o id
  bool addAlarm(const AlarmRaw raw);
  bool updateAlarm(const AlarmStruct &alarm);

  size_t count();
  AlarmStruct getAlarm(int alarmid);

  size_t getRawData(AlarmRaw *buf, size_t count);

  void loop();
};

#endif
