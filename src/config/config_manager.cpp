#include "config_manager.h"

#include <EEPROM.h>

namespace {
constexpr uint8_t CONFIG_VERSION = 2;

struct PersistedConfig {
  uint8_t version;
  Config config;
  uint8_t checksum;
};

PersistedConfig persisted;
bool initialised = false;

Config defaultConfig() {
  Config config{};
  config.logIntervalMinutes = 10;
  config.fileMaxSizeBytes = 2048;
  config.timeoutSeconds = 30;
  config.luminEnabled = true;
  config.tempAirEnabled = true;
  config.humidityEnabled = true;
  config.pressureEnabled = false;
  config.luminLow = 0;
  config.luminHigh = 2000;
  config.minTempAir = -20;
  config.maxTempAir = 60;
  config.minHumidity = 0;
  config.maxHumidity = 100;
  return config;
}

uint8_t computeChecksum(const Config &config) {
  const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&config);
  uint8_t checksum = 0;
  for (size_t i = 0; i < sizeof(Config); ++i) {
    checksum ^= bytes[i];
  }
  return checksum;
}

void writePersisted(const PersistedConfig &data) {
  EEPROM.put(0, data);
}

void loadPersisted(PersistedConfig &data) {
  EEPROM.get(0, data);
}

void ensureInitialised() {
  if (!initialised) {
    configInit();
  }
}
}  // namespace

void configInit() {
  loadPersisted(persisted);
  if (persisted.version != CONFIG_VERSION ||
      computeChecksum(persisted.config) != persisted.checksum) {
    persisted.version = CONFIG_VERSION;
    persisted.config = defaultConfig();
    persisted.checksum = computeChecksum(persisted.config);
    writePersisted(persisted);
  }
  initialised = true;
}

const Config &configGet() {
  ensureInitialised();
  return persisted.config;
}

void configSave(const Config &config) {
  ensureInitialised();
  persisted.config = config;
  persisted.checksum = computeChecksum(persisted.config);
  persisted.version = CONFIG_VERSION;
  writePersisted(persisted);
}

void configReset() {
  ensureInitialised();
  persisted.config = defaultConfig();
  persisted.checksum = computeChecksum(persisted.config);
  persisted.version = CONFIG_VERSION;
  writePersisted(persisted);
}
