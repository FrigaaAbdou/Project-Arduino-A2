#include <Arduino.h>

#include "sensors/bh1750/bh1750sensor.h"
#include "sensors/dht/dhtsensor.h"
#include "sensors/gps/gpssensor.h"
#include "actuators/rgb/rgbled.h"

void setup() {
  Serial.begin(9600);
  rgbInit();
  dhtInit();
  bh1750Init();
  gpsInit();
}

void loop() {
  const unsigned long now = millis();
  rgbUpdate(now);
  dhtUpdate(now);
  bh1750Update(now);
  gpsUpdate(now);
}
