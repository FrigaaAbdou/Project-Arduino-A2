#pragma once

#include <Arduino.h>
#include <RTClib.h>

bool rtcInit();
void rtcUpdate(unsigned long now);
bool rtcIsReady();
bool rtcHasValidTime();
DateTime rtcGetLastDateTime();
unsigned long rtcGetLastUpdateMillis();
bool rtcSetTime(uint8_t hour, uint8_t minute, uint8_t second);
bool rtcSetDate(uint8_t month, uint8_t day, uint16_t year);
bool rtcAdjustDateTime(const DateTime &dt);
bool rtcAdjustDayOfWeek(uint8_t dayOfWeek);
