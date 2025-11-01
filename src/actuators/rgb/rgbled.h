#pragma once

#include <Arduino.h>

enum class RgbLedState {
  Off,
  SolidGreen,
  SolidYellow,
  SolidBlue,
  SolidOrange,
  ErrorRtc,
  ErrorGps,
  ErrorSensorAccess,
  ErrorSensorIncoherent,
  ErrorSdFull,
  ErrorSdAccess
};

void rgbInit();
void rgbSetState(RgbLedState state);
RgbLedState rgbCurrentState();
void rgbUpdate(unsigned long now);
