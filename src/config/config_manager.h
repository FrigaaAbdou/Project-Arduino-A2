#pragma once

#include <Arduino.h>

struct Config {
  uint16_t logIntervalMinutes;
  uint16_t fileMaxSizeBytes;
  uint16_t timeoutSeconds;
  bool luminEnabled;
  bool tempAirEnabled;
  bool humidityEnabled;
  bool pressureEnabled;
  uint16_t luminLow;
  uint16_t luminHigh;
  int16_t minTempAir;
  int16_t maxTempAir;
  uint16_t minHumidity;
  uint16_t maxHumidity;
};

void configInit();
const Config &configGet();
void configSave(const Config &config);
void configReset();
