#include "sdlogger.h"

#include <SPI.h>
#include <SD.h>
#include <math.h>

#include "config/config_manager.h"
#include "sensors/bh1750/bh1750sensor.h"
#include "sensors/dht/dhtsensor.h"
#include "sensors/gps/gpssensor.h"
#include "sensors/rtc/rtcsensor.h"
#include "status/status_manager.h"

namespace {
constexpr uint8_t SD_CS_PIN = 10;
constexpr unsigned long MIN_LOG_INTERVAL_MS = 1000;
constexpr uint16_t MIN_FILE_SIZE_BYTES = 256;
constexpr char HEADER[] =
    "timestamp,tempC,humidity,lux,pressure,fix,latitude,longitude,sats,hdop,"
    "speed_kmph,altitude_m";

bool sdReady = false;
unsigned long lastLogMillis = 0;
char currentDateCode[7] = "";
bool dateCodeValid = false;

void formatDateCode(const DateTime &dt, char *buffer, size_t length) {
  snprintf(buffer, length, "%02d%02d%02d", dt.year() % 100, dt.month(),
           dt.day());
}

void buildLogPaths(const char *dateCode, char *path0, char *path1) {
  snprintf(path0, 16, "%s_0.LOG", dateCode);
  snprintf(path1, 16, "%s_1.LOG", dateCode);
}

void writeHeader(const char *path) {
  File file = SD.open(path, FILE_WRITE);
  if (!file) {
    Serial.println(F("SD: failed to create log file header"));
    statusManagerSetError(SystemError::SdAccess, true);
    return;
  }
  file.println(HEADER);
  file.close();
  statusManagerSetError(SystemError::SdAccess, false);
}

void rotateLogsIfNeeded(const char *path0, const char *path1,
                        uint16_t maxBytes) {
  File file = SD.open(path0, FILE_WRITE);
  if (!file) {
    return;
  }
  const uint32_t size = file.size();
  file.close();

  if (size < maxBytes) {
    statusManagerSetError(SystemError::SdFull, false);
    return;
  }

  statusManagerSetError(SystemError::SdFull, true);
  SD.remove(path1);

  File src = SD.open(path0, FILE_READ);
  if (!src) {
    Serial.println(F("SD: rotation open failed"));
    statusManagerSetError(SystemError::SdAccess, true);
    return;
  }

  File dst = SD.open(path1, FILE_WRITE);
  if (!dst) {
    src.close();
    Serial.println(F("SD: rotation destination open failed"));
    statusManagerSetError(SystemError::SdAccess, true);
    return;
  }

  while (src.available()) {
    int c = src.read();
    if (c < 0) {
      break;
    }
    dst.write(static_cast<uint8_t>(c));
  }
  dst.close();
  src.close();

  statusManagerSetError(SystemError::SdAccess, false);
  SD.remove(path0);
  writeHeader(path0);
  statusManagerSetError(SystemError::SdFull, false);
}

void ensureLogFile(const char *path0) {
  if (!SD.exists(path0)) {
    writeHeader(path0);
  }
}

unsigned long effectiveIntervalMs(const Config &config, OperatingMode mode) {
  unsigned long intervalMs =
      static_cast<unsigned long>(config.logIntervalMinutes) * 60000UL;
  if (intervalMs == 0) {
    intervalMs = MIN_LOG_INTERVAL_MS;
  }
  if (mode == OperatingMode::Economic) {
    intervalMs *= 2;
  }
  if (intervalMs < MIN_LOG_INTERVAL_MS) {
    intervalMs = MIN_LOG_INTERVAL_MS;
  }
  return intervalMs;
}

const char *resolveDateCode() {
  if (!rtcHasValidTime()) {
    strcpy(currentDateCode, "000000");
    dateCodeValid = true;
    return currentDateCode;
  }

  DateTime dt = rtcGetLastDateTime();
  char newCode[7];
  formatDateCode(dt, newCode, sizeof(newCode));
  if (!dateCodeValid || strcmp(newCode, currentDateCode) != 0) {
    strncpy(currentDateCode, newCode, sizeof(currentDateCode));
    currentDateCode[sizeof(currentDateCode) - 1] = '\0';
    dateCodeValid = true;
  }
  return currentDateCode;
}
}  // namespace

bool sdLoggerInit() {
  pinMode(SD_CS_PIN, OUTPUT);

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println(F("SD: initialisation failed"));
    sdReady = false;
    statusManagerSetError(SystemError::SdAccess, true);
    return false;
  }

  Serial.println(F("SD: card initialised"));
  sdReady = true;
  statusManagerSetError(SystemError::SdAccess, false);
  lastLogMillis = 0;
  dateCodeValid = false;
  return true;
}

void sdLoggerResetDailyState() {
  dateCodeValid = false;
}

void sdLoggerUpdate(unsigned long now, OperatingMode mode) {
  if (!sdReady) {
    return;
  }

  if (mode == OperatingMode::Configuration || mode == OperatingMode::Maintenance) {
    return;
  }

  const Config &config = configGet();
  const unsigned long intervalMs = effectiveIntervalMs(config, mode);

  if (now - lastLogMillis < intervalMs) {
    return;
  }
  lastLogMillis = now;

  const char *dateCode = resolveDateCode();
  char path0[16];
  char path1[16];
  buildLogPaths(dateCode, path0, path1);

  ensureLogFile(path0);
  uint16_t rotateLimit = config.fileMaxSizeBytes;
  if (rotateLimit < MIN_FILE_SIZE_BYTES) {
    rotateLimit = MIN_FILE_SIZE_BYTES;
  }
  rotateLogsIfNeeded(path0, path1, rotateLimit);

  const bool hasRtc = rtcHasValidTime();
  DateTime dt = hasRtc ? rtcGetLastDateTime() : DateTime(2000, 1, 1, 0, 0, 0);

  const float humidity = config.humidityEnabled && dhtHasValidReading()
                             ? dhtGetLastHumidity()
                             : NAN;
  const float temperature = config.tempAirEnabled && dhtHasValidReading()
                                ? dhtGetLastTemperature()
                                : NAN;
  const float lux = config.luminEnabled && bh1750IsReady() && bh1750HasReading()
                        ? bh1750GetLastLux()
                        : NAN;
  const double pressure = NAN;

  const bool gpsFix = gpsHasFix();
  const double latitude = gpsGetLatitude();
  const double longitude = gpsGetLongitude();
  const int satellites = gpsGetSatelliteCount();
  const double hdop = gpsGetHdop();
  const double speed = gpsGetSpeedKmph();
  const double altitude = gpsGetAltitudeMeters();

  File logFile = SD.open(path0, FILE_WRITE);
  if (!logFile) {
    Serial.println(F("SD: failed to open log file"));
    statusManagerSetError(SystemError::SdAccess, true);
    return;
  }
  statusManagerSetError(SystemError::SdAccess, false);

  char timestamp[20];
  if (hasRtc) {
    snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d",
             dt.year(), dt.month(), dt.day(), dt.hour(), dt.minute(), dt.second());
  } else {
    snprintf(timestamp, sizeof(timestamp), "NA");
  }

  auto printFloat = [&](double value, uint8_t digits) {
    if (isnan(value)) {
      logFile.print(F("NA"));
    } else {
      logFile.print(value, digits);
    }
  };

  logFile.print(timestamp);
  logFile.print(',');
  printFloat(temperature, 1);
  logFile.print(',');
  printFloat(humidity, 1);
  logFile.print(',');
  printFloat(lux, 1);
  logFile.print(',');
  printFloat(pressure, 1);
  logFile.print(',');
  logFile.print(gpsFix ? F("YES") : F("NO"));
  logFile.print(',');
  printFloat(latitude, 6);
  logFile.print(',');
  printFloat(longitude, 6);
  logFile.print(',');
  if (satellites >= 0) {
    logFile.print(satellites);
  } else {
    logFile.print(F("NA"));
  }
  logFile.print(',');
  printFloat(hdop, 2);
  logFile.print(',');
  printFloat(speed, 1);
  logFile.print(',');
  printFloat(altitude, 1);
  logFile.println();
  logFile.close();

  Serial.print(F("SD: logged at "));
  Serial.println(timestamp);
}
