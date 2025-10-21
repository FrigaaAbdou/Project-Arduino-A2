#include "gpssensor.h"

#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>

namespace {
constexpr uint8_t GPS_RX_PIN = 4;  // Arduino reads from GPS TX
constexpr uint8_t GPS_TX_PIN = 3;  // Arduino writes to GPS RX (optional)
constexpr uint32_t GPS_BAUD = 9600;

SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);
TinyGPSPlus gps;
unsigned long lastPrint = 0;
constexpr unsigned long PRINT_INTERVAL_MS = 1000;
}  // namespace

void gpsInit() {
  gpsSerial.begin(GPS_BAUD);
  Serial.println(F("Waiting for GPS... (go outside for first fix)"));
}

void gpsUpdate(unsigned long now) {
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  if (now - lastPrint < PRINT_INTERVAL_MS) {
    return;
  }
  lastPrint = now;

  Serial.print(F("Sat: "));
  if (gps.satellites.isValid()) {
    Serial.print(gps.satellites.value());
  } else {
    Serial.print(F("---"));
  }

  Serial.print(F("  HDOP: "));
  if (gps.hdop.isValid()) {
    Serial.print(gps.hdop.value() / 100.0, 2);
  } else {
    Serial.print(F("---"));
  }

  Serial.print(F("  Time(UTC): "));
  if (gps.time.isValid()) {
    Serial.print(gps.time.hour());
    Serial.print(':');
    Serial.print(gps.time.minute());
    Serial.print(':');
    Serial.print(gps.time.second());
  } else {
    Serial.print(F("---"));
  }

  Serial.print(F("  Fix: "));
  Serial.println(gps.location.isValid() ? F("YES") : F("NO"));
}
