#include "gpssensor.h"

#include <SoftwareSerial.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

namespace {
constexpr uint8_t GPS_RX_PIN = 4;  // Arduino reads from GPS TX
constexpr uint8_t GPS_TX_PIN = 3;  // Arduino writes to GPS RX (optional)
constexpr uint32_t GPS_BAUD = 9600;

SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);
unsigned long lastPrint = 0;
constexpr unsigned long PRINT_INTERVAL_MS = 1000;
constexpr unsigned long SLOW_MODE_INTERVAL_MS = 2000;
char sentenceBuffer[96];
uint8_t sentenceLength = 0;

struct GpsState {
  bool fix;
  float latitude;
  float longitude;
  int satellites;
  float hdop;
  float speedKmph;
  float altitude;
  bool timeValid;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  unsigned long lastUpdateMillis;
  unsigned long lastFixMillis;
} state;

void resetSentence() {
  sentenceLength = 0;
}

float parseCoordinate(const char *token, char hemi) {
  if (!token || !*token) {
    return NAN;
  }
  double value = atof(token);
  if (value == 0.0) {
    return NAN;
  }
  int degrees = static_cast<int>(value / 100.0);
  double minutes = value - degrees * 100.0;
  double decimal = degrees + minutes / 60.0;
  if (hemi == 'S' || hemi == 'W') {
    decimal = -decimal;
  }
  return static_cast<float>(decimal);
}

float parseFloatField(const char *token) {
  if (!token || !*token) {
    return NAN;
  }
  return static_cast<float>(atof(token));
}

uint8_t parseUint8Field(const char *token) {
  if (!token || !*token) {
    return 0;
  }
  return static_cast<uint8_t>(atoi(token));
}

void updateTimeFromToken(const char *token) {
  if (!token || strlen(token) < 6) {
    return;
  }
  state.hour = (token[0] - '0') * 10 + (token[1] - '0');
  state.minute = (token[2] - '0') * 10 + (token[3] - '0');
  state.second = (token[4] - '0') * 10 + (token[5] - '0');
  state.timeValid = true;
}

void splitCSV(char *data, char *tokens[], uint8_t maxTokens) {
  uint8_t count = 0;
  char *cursor = data;
  while (count < maxTokens && cursor) {
    tokens[count++] = cursor;
    char *comma = strchr(cursor, ',');
    if (!comma) {
      break;
    }
    *comma = '\0';
    cursor = comma + 1;
  }
  while (count < maxTokens) {
    tokens[count++] = nullptr;
  }
}

void handleRmc(char *sentence, unsigned long now) {
  char *data = strchr(sentence, ',');
  if (!data) {
    return;
  }
  ++data;
  char *checksum = strchr(data, '*');
  if (checksum) {
    *checksum = '\0';
  }

  char *fields[12];
  splitCSV(data, fields, 12);

  const char *status = fields[1];
  const char *lat = fields[2];
  const char *latHemi = fields[3];
  const char *lon = fields[4];
  const char *lonHemi = fields[5];
  const char *speed = fields[6];

  if (status && *status == 'A') {
    state.fix = true;
    state.latitude = parseCoordinate(lat, latHemi ? latHemi[0] : 'N');
    state.longitude = parseCoordinate(lon, lonHemi ? lonHemi[0] : 'E');
    state.speedKmph = parseFloatField(speed) * 1.852f;
    state.lastFixMillis = now;
  } else if (status && *status == 'V') {
    state.fix = false;
  }

  updateTimeFromToken(fields[0]);
  state.lastUpdateMillis = now;
}

void handleGga(char *sentence, unsigned long now) {
  char *data = strchr(sentence, ',');
  if (!data) {
    return;
  }
  ++data;
  char *checksum = strchr(data, '*');
  if (checksum) {
    *checksum = '\0';
  }

  char *fields[15];
  splitCSV(data, fields, 15);

  const char *lat = fields[1];
  const char *latHemi = fields[2];
  const char *lon = fields[3];
  const char *lonHemi = fields[4];
  const char *quality = fields[5];
  const char *satellites = fields[6];
  const char *hdop = fields[7];
  const char *altitude = fields[8];

  uint8_t fixQuality = quality ? static_cast<uint8_t>(atoi(quality)) : 0;
  if (fixQuality > 0) {
    state.fix = true;
    state.latitude = parseCoordinate(lat, latHemi ? latHemi[0] : 'N');
    state.longitude = parseCoordinate(lon, lonHemi ? lonHemi[0] : 'E');
    state.lastFixMillis = now;
  }

  state.satellites = parseUint8Field(satellites);
  state.hdop = parseFloatField(hdop);
  state.altitude = parseFloatField(altitude);
  updateTimeFromToken(fields[0]);
  state.lastUpdateMillis = now;
}

void processSentence(char *sentence, unsigned long now) {
  if (strncmp(sentence, "$GPRMC", 6) == 0 ||
      strncmp(sentence, "$GNRMC", 6) == 0) {
    handleRmc(sentence, now);
  } else if (strncmp(sentence, "$GPGGA", 6) == 0 ||
             strncmp(sentence, "$GNGGA", 6) == 0) {
    handleGga(sentence, now);
  }
}
}  // namespace

void gpsInit() {
  gpsSerial.begin(GPS_BAUD);
  Serial.println(F("Waiting for GPS... (go outside for first fix)"));
  state.fix = false;
  state.latitude = NAN;
  state.longitude = NAN;
  state.satellites = 0;
  state.hdop = NAN;
  state.speedKmph = NAN;
  state.altitude = NAN;
  state.timeValid = false;
  state.hour = state.minute = state.second = 0;
  state.lastUpdateMillis = 0;
  state.lastFixMillis = 0;
  resetSentence();
}

void gpsUpdate(unsigned long now, bool slowMode) {
  while (gpsSerial.available()) {
    char c = gpsSerial.read();
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      sentenceBuffer[sentenceLength] = '\0';
      if (sentenceLength > 6) {
        processSentence(sentenceBuffer, now);
      }
      resetSentence();
      continue;
    }
    if (sentenceLength < sizeof(sentenceBuffer) - 1) {
      sentenceBuffer[sentenceLength++] = c;
    }
  }

  const unsigned long interval = slowMode ? SLOW_MODE_INTERVAL_MS : PRINT_INTERVAL_MS;
  if (now - lastPrint < interval) {
    return;
  }
  lastPrint = now;

  Serial.print(F("Sat: "));
  if (state.satellites > 0) {
    Serial.print(state.satellites);
  } else {
    Serial.print(F("---"));
  }

  Serial.print(F("  HDOP: "));
  if (!isnan(state.hdop)) {
    Serial.print(state.hdop, 2);
  } else {
    Serial.print(F("---"));
  }

  Serial.print(F("  Time(UTC): "));
  if (state.timeValid) {
    if (state.hour < 10) Serial.print('0');
    Serial.print(state.hour);
    Serial.print(':');
    if (state.minute < 10) Serial.print('0');
    Serial.print(state.minute);
    Serial.print(':');
    if (state.second < 10) Serial.print('0');
    Serial.print(state.second);
  } else {
    Serial.print(F("---"));
  }

  Serial.print(F("  Fix: "));
  Serial.println(state.fix ? F("YES") : F("NO"));
}

bool gpsHasFix() {
  return state.fix;
}

double gpsGetLatitude() {
  return static_cast<double>(state.latitude);
}

double gpsGetLongitude() {
  return static_cast<double>(state.longitude);
}

int gpsGetSatelliteCount() {
  return state.satellites > 0 ? state.satellites : -1;
}

double gpsGetHdop() {
  return static_cast<double>(state.hdop);
}

double gpsGetSpeedKmph() {
  return static_cast<double>(state.speedKmph);
}

double gpsGetAltitudeMeters() {
  return static_cast<double>(state.altitude);
}

bool gpsTimeIsValid() {
  return state.timeValid;
}

uint8_t gpsGetHour() {
  return state.hour;
}

uint8_t gpsGetMinute() {
  return state.minute;
}

uint8_t gpsGetSecond() {
  return state.second;
}

unsigned long gpsGetLastUpdateMillis() {
  return state.lastUpdateMillis;
}

unsigned long gpsGetLastFixMillis() {
  return state.lastFixMillis;
}
