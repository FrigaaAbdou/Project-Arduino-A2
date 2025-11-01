#pragma once

#include "actuators/rgb/rgbled.h"

enum class SystemError {
  None = 0,
  Rtc,
  Gps,
  SensorAccess,
  SensorIncoherent,
  SdFull,
  SdAccess
};

void statusManagerInit();
void statusManagerSetError(SystemError error, bool active);
bool statusManagerHasError(SystemError error);
RgbLedState statusManagerActiveLedState();
