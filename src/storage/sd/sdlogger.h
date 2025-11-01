#pragma once

#include <Arduino.h>

#include "modes/mode_manager.h"

bool sdLoggerInit();
void sdLoggerUpdate(unsigned long now, OperatingMode mode);
void sdLoggerResetDailyState();
