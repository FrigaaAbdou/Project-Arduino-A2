#pragma once

#include <Arduino.h>

bool bh1750Init();
void bh1750Update(unsigned long now);
bool bh1750IsReady();
bool bh1750HasReading();
float bh1750GetLastLux();
unsigned long bh1750GetLastReadMillis();
