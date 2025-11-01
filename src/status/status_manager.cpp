#include "status_manager.h"

#include <stddef.h>

namespace {
struct ErrorState {
  SystemError error;
  RgbLedState ledState;
  bool active;
};

ErrorState errorStates[] = {
    {SystemError::Rtc, RgbLedState::ErrorRtc, false},
    {SystemError::Gps, RgbLedState::ErrorGps, false},
    {SystemError::SensorAccess, RgbLedState::ErrorSensorAccess, false},
    {SystemError::SensorIncoherent, RgbLedState::ErrorSensorIncoherent, false},
    {SystemError::SdFull, RgbLedState::ErrorSdFull, false},
    {SystemError::SdAccess, RgbLedState::ErrorSdAccess, false},
};
}  // namespace

void statusManagerInit() {
  for (size_t i = 0; i < sizeof(errorStates) / sizeof(errorStates[0]); ++i) {
    errorStates[i].active = false;
  }
}

void statusManagerSetError(SystemError error, bool active) {
  for (size_t i = 0; i < sizeof(errorStates) / sizeof(errorStates[0]); ++i) {
    if (errorStates[i].error == error) {
      errorStates[i].active = active;
      break;
    }
  }
}

bool statusManagerHasError(SystemError error) {
  for (size_t i = 0; i < sizeof(errorStates) / sizeof(errorStates[0]); ++i) {
    if (errorStates[i].error == error) {
      return errorStates[i].active;
    }
  }
  return false;
}

RgbLedState statusManagerActiveLedState() {
  for (size_t i = 0; i < sizeof(errorStates) / sizeof(errorStates[0]); ++i) {
    if (errorStates[i].active) {
      return errorStates[i].ledState;
    }
  }
  return RgbLedState::Off;
}
