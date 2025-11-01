#pragma once

#include "actuators/rgb/rgbled.h"
#include "controls/button_manager.h"

enum class OperatingMode {
  Standard,
  Configuration,
  Maintenance,
  Economic
};

void modeManagerInit(OperatingMode initialMode);
bool modeManagerHandleEvent(const ButtonEvent &event);
OperatingMode modeManagerCurrentMode();
RgbLedState modeManagerLedState();
void modeManagerSetMode(OperatingMode mode);
