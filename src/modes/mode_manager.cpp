#include "mode_manager.h"

namespace {
OperatingMode currentMode = OperatingMode::Standard;
OperatingMode previousMode = OperatingMode::Standard;

RgbLedState ledStateFor(OperatingMode mode) {
  switch (mode) {
    case OperatingMode::Standard:
      return RgbLedState::SolidGreen;
    case OperatingMode::Configuration:
      return RgbLedState::SolidYellow;
    case OperatingMode::Maintenance:
      return RgbLedState::SolidOrange;
    case OperatingMode::Economic:
      return RgbLedState::SolidBlue;
  }
  return RgbLedState::Off;
}
}  // namespace

void modeManagerInit(OperatingMode initialMode) {
  currentMode = initialMode;
  previousMode = OperatingMode::Standard;
}

void modeManagerSetMode(OperatingMode mode) {
  if (mode == currentMode) {
    return;
  }
  if (mode == OperatingMode::Maintenance &&
      currentMode != OperatingMode::Maintenance) {
    previousMode = currentMode;
  }
  currentMode = mode;
}

bool modeManagerHandleEvent(const ButtonEvent &event) {
  if (event.type != ButtonEventType::LongPress) {
    return false;
  }

  const OperatingMode before = currentMode;

  switch (currentMode) {
    case OperatingMode::Standard:
      if (event.button == ButtonId::Green) {
        currentMode = OperatingMode::Economic;
      } else if (event.button == ButtonId::Red) {
        previousMode = OperatingMode::Standard;
        currentMode = OperatingMode::Maintenance;
      }
      break;

    case OperatingMode::Economic:
      if (event.button == ButtonId::Red) {
        currentMode = OperatingMode::Standard;
      }
      break;

    case OperatingMode::Maintenance:
      if (event.button == ButtonId::Red) {
        currentMode = previousMode;
      }
      break;

    case OperatingMode::Configuration:
      break;
  }

  if (before != currentMode) {
    if (currentMode == OperatingMode::Maintenance &&
        before != OperatingMode::Maintenance) {
      previousMode = before;
    }
    return true;
  }
  return false;
}

OperatingMode modeManagerCurrentMode() {
  return currentMode;
}

RgbLedState modeManagerLedState() {
  return ledStateFor(currentMode);
}
