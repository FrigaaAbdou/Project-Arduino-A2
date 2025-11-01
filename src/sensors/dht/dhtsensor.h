#pragma once

#include <Arduino.h>

void dhtInit();
void dhtUpdate(unsigned long now);
bool dhtHasValidReading();
float dhtGetLastHumidity();
float dhtGetLastTemperature();
unsigned long dhtGetLastReadMillis();
