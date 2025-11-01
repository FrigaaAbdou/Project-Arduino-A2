#include "rtcsensor.h"

#include <Wire.h>

namespace {
RTC_DS1307 rtc;
bool rtcReady = false;
bool timeValid = false;
DateTime lastDateTime;
unsigned long lastUpdate = 0;
constexpr unsigned long UPDATE_INTERVAL_MS = 1000;
bool statusPrinted = false;
}  // namespace

bool rtcInit() {
  Wire.begin();

  rtcReady = rtc.begin();
  if (!rtcReady) {
    Serial.println(F("RTC: DS1307 not detected"));
    return false;
  }

  if (!rtc.isrunning()) {
    Serial.println(F("RTC: clock not running, adjust via host if needed"));
  } else {
    timeValid = true;
    lastDateTime = rtc.now();
    lastUpdate = millis();
  }

  Serial.println(F("RTC: initialised"));
  return true;
}

void rtcUpdate(unsigned long now) {
  if (!rtcReady) {
    return;
  }

  if (now - lastUpdate < UPDATE_INTERVAL_MS) {
    return;
  }
  lastUpdate = now;

  DateTime current = rtc.now();
  lastDateTime = current;
  timeValid = rtc.isrunning();

  if (!statusPrinted) {
    Serial.print(F("RTC: "));
    Serial.print(current.year());
    Serial.print('-');
    Serial.print(current.month());
    Serial.print('-');
    Serial.print(current.day());
    Serial.print(' ');
    Serial.print(current.hour());
    Serial.print(':');
    Serial.print(current.minute());
    Serial.print(':');
    Serial.println(current.second());
    statusPrinted = true;
  }
}

bool rtcIsReady() {
  return rtcReady;
}

bool rtcHasValidTime() {
  return rtcReady && timeValid;
}

DateTime rtcGetLastDateTime() {
  return lastDateTime;
}

unsigned long rtcGetLastUpdateMillis() {
  return lastUpdate;
}

bool rtcAdjustDateTime(const DateTime &dt) {
  if (!rtcReady) {
    return false;
  }
  rtc.adjust(dt);
  lastDateTime = dt;
  lastUpdate = millis();
  timeValid = true;
  return true;
}

bool rtcAdjustDayOfWeek(uint8_t dayOfWeek) {
  if (!rtcReady || dayOfWeek > 6) {
    return false;
  }
  DateTime current = rtcHasValidTime() ? rtc.now() : DateTime(2000, 1, 1, 0, 0, 0);
  uint8_t currentDow = current.dayOfTheWeek();
  int diff = static_cast<int>(dayOfWeek) - static_cast<int>(currentDow);
  if (diff == 0) {
    return true;
  }
  DateTime adjusted = current + TimeSpan(diff, 0, 0, 0);
  return rtcAdjustDateTime(adjusted);
}

bool rtcSetTime(uint8_t hour, uint8_t minute, uint8_t second) {
  if (!rtcReady) {
    return false;
  }
  DateTime current = rtcHasValidTime() ? rtc.now() : DateTime(2000, 1, 1, 0, 0, 0);
  DateTime updated(current.year(), current.month(), current.day(), hour, minute,
                   second);
  return rtcAdjustDateTime(updated);
}

bool rtcSetDate(uint8_t month, uint8_t day, uint16_t year) {
  if (!rtcReady) {
    return false;
  }
  DateTime current = rtcHasValidTime() ? rtc.now() : DateTime(year, month, day, 0, 0, 0);
  DateTime updated(year, month, day, current.hour(), current.minute(),
                   current.second());
  return rtcAdjustDateTime(updated);
}
