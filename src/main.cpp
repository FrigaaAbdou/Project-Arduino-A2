#include <Arduino.h>

#include "actuators/rgb/rgbled.h"
#include "cli/config_cli.h"
#include "config/config_manager.h"
#include "controls/button_manager.h"
#include "modes/mode_manager.h"
#include "sensors/bh1750/bh1750sensor.h"
#include "sensors/dht/dhtsensor.h"
#include "sensors/gps/gpssensor.h"
#include "sensors/rtc/rtcsensor.h"
#include "status/status_manager.h"
#include "storage/sd/sdlogger.h"

void setup() {
  Serial.begin(9600);
  configInit();
  configCliInit();
  statusManagerInit();
  rgbInit();
  buttonManagerInit();
  const bool startInConfig = buttonManagerIsPressed(ButtonId::Red);
  modeManagerInit(startInConfig ? OperatingMode::Configuration
                                : OperatingMode::Standard);
  rgbSetState(modeManagerLedState());
  if (startInConfig) {
    configCliEnterMode();
  }
  dhtInit();
  bh1750Init();
  gpsInit();
  rtcInit();
  sdLoggerInit();
}

void loop() {
  const unsigned long now = millis();
  buttonManagerUpdate(now);
  ButtonEvent event;
  bool modeChanged = false;
  while (buttonManagerGetEvent(event)) {
    modeChanged |= modeManagerHandleEvent(event);
  }
  OperatingMode mode = modeManagerCurrentMode();
  static OperatingMode lastMode = mode;

  if (modeChanged || mode != lastMode) {
    if (mode == OperatingMode::Configuration &&
        lastMode != OperatingMode::Configuration) {
      configCliEnterMode();
    }
    if (lastMode == OperatingMode::Configuration &&
        mode != OperatingMode::Configuration) {
      configCliExitMode();
      sdLoggerResetDailyState();
    }
    if (mode == OperatingMode::Maintenance &&
        lastMode != OperatingMode::Maintenance) {
      Serial.println(F("=== MAINTENANCE MODE (logging paused) ==="));
    }
    if (lastMode == OperatingMode::Maintenance &&
        mode != OperatingMode::Maintenance) {
      Serial.println(F("=== Resuming normal logging ==="));
    }
    if (mode == OperatingMode::Economic &&
        lastMode != OperatingMode::Economic) {
      Serial.println(F("=== ECONOMIC MODE (reduced GPS frequency) ==="));
    }
    if (lastMode == OperatingMode::Economic &&
        mode != OperatingMode::Economic) {
      Serial.println(F("=== Returning to standard GPS cadence ==="));
    }
    lastMode = mode;
  }

  rgbUpdate(now);
  rtcUpdate(now);

  const Config &config = configGet();
  unsigned long timeoutMs = static_cast<unsigned long>(config.timeoutSeconds) * 1000UL;
  if (timeoutMs < 1000UL) {
    timeoutMs = 1000UL;
  }

  statusManagerSetError(SystemError::Rtc,
                        !(rtcIsReady() && rtcHasValidTime()));

  auto updateLed = [&]() {
    RgbLedState target = statusManagerActiveLedState();
    if (target == RgbLedState::Off) {
      target = modeManagerLedState();
    }
    static RgbLedState lastLed = RgbLedState::Off;
    if (target != lastLed) {
      rgbSetState(target);
      lastLed = target;
    }
  };

  if (mode == OperatingMode::Configuration) {
    configCliUpdate(now);
    if (configCliShouldExit(now)) {
      modeManagerSetMode(OperatingMode::Standard);
      mode = modeManagerCurrentMode();
      configCliExitMode();
      sdLoggerResetDailyState();
      lastMode = mode;
    }
    updateLed();
    return;
  }

  static unsigned long lastMaintenancePrint = 0;

  if (config.tempAirEnabled || config.humidityEnabled) {
    dhtUpdate(now);
  }
  if (config.luminEnabled) {
    bh1750Update(now);
  }
  gpsUpdate(now, mode == OperatingMode::Economic);

  bool sensorAccessError = false;
  bool sensorIncoherent = false;

  if ((config.tempAirEnabled || config.humidityEnabled)) {
    const unsigned long lastRead = dhtGetLastReadMillis();
    const bool timedOut =
        (timeoutMs > 0) &&
        ((lastRead == 0 && now > timeoutMs) ||
         (lastRead != 0 && now - lastRead > timeoutMs));
    sensorAccessError |= timedOut;
    if (dhtHasValidReading()) {
      const float temperature = dhtGetLastTemperature();
      const float humidity = dhtGetLastHumidity();
      if (config.tempAirEnabled &&
          (temperature < config.minTempAir || temperature > config.maxTempAir)) {
        sensorIncoherent = true;
      }
      if (config.humidityEnabled &&
          (humidity < config.minHumidity || humidity > config.maxHumidity)) {
        sensorIncoherent = true;
      }
    }
  }

  if (config.luminEnabled) {
    const unsigned long lastRead = bh1750GetLastReadMillis();
    const bool timedOut =
        (timeoutMs > 0) &&
        ((lastRead == 0 && now > timeoutMs) ||
         (lastRead != 0 && now - lastRead > timeoutMs));
    if (!bh1750IsReady()) {
      sensorAccessError = true;
    } else {
      sensorAccessError |= timedOut;
      if (bh1750HasReading()) {
        const float lux = bh1750GetLastLux();
        if (lux < config.luminLow || lux > config.luminHigh) {
          sensorIncoherent = true;
        }
      }
    }
  }

  const unsigned long gpsLast = gpsGetLastUpdateMillis();
  const bool gpsStale =
      (timeoutMs > 0) && (now - gpsLast > timeoutMs);
  statusManagerSetError(SystemError::Gps, gpsStale);

  statusManagerSetError(SystemError::SensorAccess, sensorAccessError);
  statusManagerSetError(SystemError::SensorIncoherent, sensorIncoherent);

  updateLed();

  if (mode == OperatingMode::Maintenance) {
    if (now - lastMaintenancePrint >= 2000) {
      lastMaintenancePrint = now;
      Serial.print(F("MAINT | T="));
      if (dhtHasValidReading()) {
        Serial.print(dhtGetLastTemperature(), 1);
        Serial.print(F("C H="));
        Serial.print(dhtGetLastHumidity(), 1);
        Serial.print(F("%"));
      } else {
        Serial.print(F("NA"));
      }
      Serial.print(F(" Lux="));
      if (bh1750IsReady() && bh1750HasReading()) {
        Serial.print(bh1750GetLastLux(), 1);
      } else {
        Serial.print(F("NA"));
      }
      Serial.print(F(" GPS="));
      Serial.print(gpsHasFix() ? F("FIX ") : F("NOFIX "));
      if (gpsHasFix()) {
        Serial.print(gpsGetLatitude(), 6);
        Serial.print(F(","));
        Serial.print(gpsGetLongitude(), 6);
      }
      Serial.println();
    }
    return;
  }

  sdLoggerUpdate(now, mode);
}
