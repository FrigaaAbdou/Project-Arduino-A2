#include "bh1750sensor.h"

#include <BH1750.h>
#include <Wire.h>

namespace {
BH1750 lightMeter;
bool sensorReady = false;
constexpr unsigned long READ_INTERVAL_MS = 1000;
unsigned long lastRead = 0;
}  // namespace

void bh1750Init() {
  Wire.begin();
  sensorReady = lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
  if (!sensorReady) {
    Serial.println(F("BH1750 init failed"));
  }
}

void bh1750Update(unsigned long now) {
  if (!sensorReady) {
    return;
  }

  if (now - lastRead < READ_INTERVAL_MS) {
    return;
  }
  lastRead = now;

  const float lux = lightMeter.readLightLevel();
  if (lux < 0) {
    Serial.println(F("BH1750 read failed"));
    return;
  }

  Serial.print(F("Light: "));
  Serial.print(lux);
  Serial.println(F(" lx"));
}
