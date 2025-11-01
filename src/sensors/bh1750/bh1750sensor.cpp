#include "bh1750sensor.h"

#include <BH1750.h>
#include <Wire.h>
#include <math.h>

namespace {
constexpr unsigned long READ_INTERVAL_MS = 1000;
constexpr uint8_t DEFAULT_I2C_ADDRESS = 0x23;
constexpr uint8_t ALTERNATE_I2C_ADDRESS = 0x5C;

BH1750 lightMeter;
unsigned long lastRead = 0;
bool sensorReady = false;
uint8_t activeAddress = DEFAULT_I2C_ADDRESS;
float lastLux = NAN;
unsigned long lastValidRead = 0;
bool hasReading = false;
}  // namespace

bool bh1750Init() {
  Wire.begin();

  sensorReady = lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, DEFAULT_I2C_ADDRESS);
  if (!sensorReady) {
    sensorReady = lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, ALTERNATE_I2C_ADDRESS);
    if (sensorReady) {
      activeAddress = ALTERNATE_I2C_ADDRESS;
    }
  }

  if (sensorReady) {
    Serial.print(F("BH1750 initialised at address 0x"));
    Serial.println(activeAddress, HEX);
  } else {
    Serial.println(F("BH1750 failed to initialise. Check wiring/power."));
  }

  lastRead = millis();
  return sensorReady;
}

void bh1750Update(unsigned long now) {
  if (!sensorReady || now - lastRead < READ_INTERVAL_MS) {
    if (!sensorReady) {
      bh1750Init();
    }
    return;
  }
  lastRead = now;

  const float lux = lightMeter.readLightLevel();
  if (lux < 0) {
    Serial.println(F("BH1750 read failed"));
    hasReading = false;
    return;
  }

  lastLux = lux;
  hasReading = true;
  lastValidRead = now;

  Serial.print(F("Light: "));
  Serial.print(lux, 1);
  Serial.println(F(" lx"));
}

bool bh1750IsReady() {
  return sensorReady;
}

bool bh1750HasReading() {
  return hasReading;
}

float bh1750GetLastLux() {
  return lastLux;
}

unsigned long bh1750GetLastReadMillis() {
  return lastValidRead;
}
