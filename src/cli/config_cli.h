#pragma once

#include <Arduino.h>

void configCliInit();
void configCliEnterMode();
void configCliExitMode();
void configCliUpdate(unsigned long now);
bool configCliShouldExit(unsigned long now);
