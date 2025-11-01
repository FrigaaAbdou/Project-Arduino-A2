#pragma once

#include <Arduino.h>

void gpsInit();
void gpsUpdate(unsigned long now, bool slowMode);
bool gpsHasFix();
float gpsGetLatitude();
float gpsGetLongitude();
int gpsGetSatelliteCount();
float gpsGetHdop();
float gpsGetSpeedKmph();
float gpsGetAltitudeMeters();
bool gpsTimeIsValid();
uint8_t gpsGetHour();
uint8_t gpsGetMinute();
uint8_t gpsGetSecond();
unsigned long gpsGetLastUpdateMillis();
unsigned long gpsGetLastFixMillis();
