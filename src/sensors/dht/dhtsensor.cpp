#include "dhtsensor.h"

#include <DHT.h>

namespace {
constexpr uint8_t DHTPIN = 2;
constexpr uint8_t DHTTYPE = DHT11;
constexpr unsigned long DHT_INTERVAL_MS = 2000;

DHT dht(DHTPIN, DHTTYPE);
unsigned long lastDhtRead = 0;
}  // namespace

void dhtInit() {
  pinMode(DHTPIN, INPUT_PULLUP);
  dht.begin();
  delay(2000);  // give the sensor time to stabilize on startup
}

void dhtUpdate(unsigned long now) {
  if (now - lastDhtRead < DHT_INTERVAL_MS) {
    return;
  }

  lastDhtRead = now;
  const float humidity = dht.readHumidity();
  const float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println(F("DHT read failed"));
    return;
  }

  Serial.print(F("Humidity: "));
  Serial.print(humidity);
  Serial.print(F("%  |  Temperature: "));
  Serial.print(temperature);
  Serial.println(F(" C"));
}
