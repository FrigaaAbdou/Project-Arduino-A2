#include "config_cli.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "config/config_manager.h"
#include "sensors/rtc/rtcsensor.h"

namespace {
constexpr unsigned long INACTIVITY_TIMEOUT_MS = 30UL * 60UL * 1000UL;
constexpr size_t LINE_BUFFER_SIZE = 96;

char lineBuffer[LINE_BUFFER_SIZE];
size_t lineLength = 0;
unsigned long lastActivityMs = 0;
bool active = false;

void printPrompt() {
  Serial.print(F("> "));
}

char *trimWhitespace(char *str) {
  while (*str && isspace(static_cast<unsigned char>(*str))) {
    ++str;
  }
  char *end = str + strlen(str);
  while (end > str && isspace(static_cast<unsigned char>(*(end - 1)))) {
    --end;
  }
  *end = '\0';
  return str;
}

void toUpperInPlace(char *str) {
  for (; *str; ++str) {
    *str = toupper(static_cast<unsigned char>(*str));
  }
}

bool parseUint16(const char *value, uint16_t &out) {
  char *end = nullptr;
  unsigned long v = strtoul(value, &end, 10);
  if (end == value || *end != '\0' || v > 0xFFFF) {
    return false;
  }
  out = static_cast<uint16_t>(v);
  return true;
}

bool parseUint8(const char *value, uint8_t &out) {
  char *end = nullptr;
  unsigned long v = strtoul(value, &end, 10);
  if (end == value || *end != '\0' || v > 0xFF) {
    return false;
  }
  out = static_cast<uint8_t>(v);
  return true;
}

bool parseInt16(const char *value, int16_t &out) {
  char *end = nullptr;
  long v = strtol(value, &end, 10);
  if (end == value || *end != '\0' || v < -32768 || v > 32767) {
    return false;
  }
  out = static_cast<int16_t>(v);
  return true;
}

int parseDayOfWeek(const char *value) {
  if (strlen(value) != 3) {
    return -1;
  }
  char upper[4];
  for (size_t i = 0; i < 3; ++i) {
    upper[i] = toupper(static_cast<unsigned char>(value[i]));
  }
  upper[3] = '\0';
  if (strcmp(upper, "SUN") == 0) return 0;
  if (strcmp(upper, "MON") == 0) return 1;
  if (strcmp(upper, "TUE") == 0) return 2;
  if (strcmp(upper, "WED") == 0) return 3;
  if (strcmp(upper, "THU") == 0) return 4;
  if (strcmp(upper, "FRI") == 0) return 5;
  if (strcmp(upper, "SAT") == 0) return 6;
  return -1;
}

bool parseUnsigned(const char *start, size_t len, uint16_t &out) {
  if (len == 0) {
    return false;
  }
  uint16_t value = 0;
  for (size_t i = 0; i < len; ++i) {
    char c = start[i];
    if (!isdigit(static_cast<unsigned char>(c))) {
      return false;
    }
    value = static_cast<uint16_t>(value * 10 + (c - '0'));
  }
  out = value;
  return true;
}

bool parseTimeString(const char *value, uint8_t &hour, uint8_t &minute,
                     uint8_t &second) {
  if (!value || strlen(value) != 8 || value[2] != ':' || value[5] != ':') {
    return false;
  }
  uint16_t temp;
  if (!parseUnsigned(value, 2, temp) || temp > 23) {
    return false;
  }
  hour = static_cast<uint8_t>(temp);
  if (!parseUnsigned(value + 3, 2, temp) || temp > 59) {
    return false;
  }
  minute = static_cast<uint8_t>(temp);
  if (!parseUnsigned(value + 6, 2, temp) || temp > 59) {
    return false;
  }
  second = static_cast<uint8_t>(temp);
  return true;
}

bool parseDateString(const char *value, uint8_t &month, uint8_t &day,
                     uint16_t &year) {
  if (!value) {
    return false;
  }
  const char *firstComma = strchr(value, ',');
  if (!firstComma) {
    return false;
  }
  const char *secondComma = strchr(firstComma + 1, ',');
  if (!secondComma) {
    return false;
  }

  uint16_t temp;
  size_t lenMonth = static_cast<size_t>(firstComma - value);
  if (lenMonth == 0 || lenMonth > 2 ||
      !parseUnsigned(value, lenMonth, temp) || temp == 0 || temp > 12) {
    return false;
  }
  month = static_cast<uint8_t>(temp);

  const char *dayPtr = firstComma + 1;
  size_t lenDay = static_cast<size_t>(secondComma - dayPtr);
  if (lenDay == 0 || lenDay > 2 ||
      !parseUnsigned(dayPtr, lenDay, temp) || temp == 0 || temp > 31) {
    return false;
  }
  day = static_cast<uint8_t>(temp);

  const char *yearPtr = secondComma + 1;
  size_t lenYear = strlen(yearPtr);
  if (lenYear == 0 || lenYear > 4 ||
      !parseUnsigned(yearPtr, lenYear, temp) || temp < 2000) {
    return false;
  }
  year = temp;
  return true;
}

void handleAssignment(char *key, char *value) {
  char keyUpper[LINE_BUFFER_SIZE];
  strncpy(keyUpper, key, sizeof(keyUpper));
  keyUpper[sizeof(keyUpper) - 1] = '\0';
  toUpperInPlace(keyUpper);

  Config config = configGet();
  bool updated = false;

  if (strcmp(keyUpper, "LOG_INTERVAL") == 0) {
    uint16_t minutes;
    if (parseUint16(value, minutes) && minutes > 0) {
      config.logIntervalMinutes = minutes;
      updated = true;
      Serial.print(F("LOG_INTERVAL set to "));
      Serial.println(minutes);
    } else {
      Serial.println(F("Invalid LOG_INTERVAL"));
    }
  } else if (strcmp(keyUpper, "FILE_MAX_SIZE") == 0) {
    uint16_t bytes;
    if (parseUint16(value, bytes) && bytes >= 256) {
      config.fileMaxSizeBytes = bytes;
      updated = true;
      Serial.print(F("FILE_MAX_SIZE set to "));
      Serial.println(bytes);
    } else {
      Serial.println(F("Invalid FILE_MAX_SIZE"));
    }
  } else if (strcmp(keyUpper, "TIMEOUT") == 0) {
    uint16_t seconds;
    if (parseUint16(value, seconds) && seconds > 0) {
      config.timeoutSeconds = seconds;
      updated = true;
      Serial.print(F("TIMEOUT set to "));
      Serial.println(seconds);
    } else {
      Serial.println(F("Invalid TIMEOUT"));
    }
  } else if (strcmp(keyUpper, "LUMIN") == 0 || strcmp(keyUpper, "TEMP_AIR") == 0 ||
             strcmp(keyUpper, "HYGR") == 0 || strcmp(keyUpper, "PRESSURE") == 0) {
    uint8_t flag;
    if (parseUint8(value, flag) && flag <= 1) {
      bool enabled = flag != 0;
      if (strcmp(keyUpper, "LUMIN") == 0) {
        config.luminEnabled = enabled;
      } else if (strcmp(keyUpper, "TEMP_AIR") == 0) {
        config.tempAirEnabled = enabled;
      } else if (strcmp(keyUpper, "HYGR") == 0) {
        config.humidityEnabled = enabled;
      } else {
        config.pressureEnabled = enabled;
      }
      updated = true;
      Serial.print(keyUpper);
      Serial.print(F("="));
      Serial.println(enabled ? F("1") : F("0"));
    } else {
      Serial.println(F("Invalid sensor flag"));
    }
  } else if (strcmp(keyUpper, "LUMIN_LOW") == 0 ||
             strcmp(keyUpper, "LUMIN_HIGH") == 0) {
    uint16_t valueNum;
    if (parseUint16(value, valueNum)) {
      if (strcmp(keyUpper, "LUMIN_LOW") == 0) {
        if (valueNum <= config.luminHigh) {
          config.luminLow = valueNum;
          updated = true;
        } else {
          Serial.println(F("LUMIN_LOW must be <= LUMIN_HIGH"));
        }
      } else {
        if (valueNum >= config.luminLow) {
          config.luminHigh = valueNum;
          updated = true;
        } else {
          Serial.println(F("LUMIN_HIGH must be >= LUMIN_LOW"));
        }
      }
      if (updated) {
        Serial.print(keyUpper);
        Serial.print(F("="));
        Serial.println(valueNum);
      }
    } else {
      Serial.println(F("Invalid LUMIN threshold"));
    }
  } else if (strcmp(keyUpper, "MIN_TEMP_AIR") == 0 ||
             strcmp(keyUpper, "MAX_TEMP_AIR") == 0) {
    int16_t valueNum;
    if (parseInt16(value, valueNum)) {
      if (strcmp(keyUpper, "MIN_TEMP_AIR") == 0) {
        if (valueNum <= config.maxTempAir) {
          config.minTempAir = valueNum;
          updated = true;
        } else {
          Serial.println(F("MIN_TEMP_AIR must be <= MAX_TEMP_AIR"));
        }
      } else {
        if (valueNum >= config.minTempAir) {
          config.maxTempAir = valueNum;
          updated = true;
        } else {
          Serial.println(F("MAX_TEMP_AIR must be >= MIN_TEMP_AIR"));
        }
      }
      if (updated) {
        Serial.print(keyUpper);
        Serial.print(F("="));
        Serial.println(valueNum);
      }
    } else {
      Serial.println(F("Invalid TEMP_AIR threshold"));
    }
  } else if (strcmp(keyUpper, "MIN_HYGR") == 0 || strcmp(keyUpper, "MAX_HYGR") == 0) {
    uint16_t valueNum;
    if (parseUint16(value, valueNum) && valueNum <= 100) {
      if (strcmp(keyUpper, "MIN_HYGR") == 0) {
        if (valueNum <= config.maxHumidity) {
          config.minHumidity = valueNum;
          updated = true;
        } else {
          Serial.println(F("MIN_HYGR must be <= MAX_HYGR"));
        }
      } else {
        if (valueNum >= config.minHumidity) {
          config.maxHumidity = valueNum;
          updated = true;
        } else {
          Serial.println(F("MAX_HYGR must be >= MIN_HYGR"));
        }
      }
      if (updated) {
        Serial.print(keyUpper);
        Serial.print(F("="));
        Serial.println(valueNum);
      }
    } else {
      Serial.println(F("Invalid HYGR threshold"));
    }
  } else if (strcmp(keyUpper, "CLOCK") == 0) {
    uint8_t hour, minute, second;
    if (parseTimeString(value, hour, minute, second)) {
      if (rtcSetTime(hour, minute, second)) {
        Serial.println(F("Clock updated"));
      } else {
        Serial.println(F("RTC not ready"));
      }
    } else {
      Serial.println(F("Invalid CLOCK format"));
    }
  } else if (strcmp(keyUpper, "DATE") == 0) {
    uint8_t month, day;
    uint16_t year;
    if (parseDateString(value, month, day, year)) {
      if (rtcSetDate(month, day, year)) {
        Serial.println(F("Date updated"));
      } else {
        Serial.println(F("RTC not ready"));
      }
    } else {
      Serial.println(F("Invalid DATE format"));
    }
  } else if (strcmp(keyUpper, "DAY") == 0) {
    int dow = parseDayOfWeek(value);
    if (dow < 0) {
      Serial.println(F("Invalid DAY value"));
    } else {
      if (rtcAdjustDayOfWeek(static_cast<uint8_t>(dow))) {
        Serial.println(F("Day-of-week updated"));
      } else {
        Serial.println(F("RTC not ready"));
      }
    }
  } else {
    Serial.println(F("Unknown parameter"));
  }

  if (updated) {
    configSave(config);
  }
}

void handleCommand(char *line) {
  char *trimmed = trimWhitespace(line);
  if (*trimmed == '\0') {
    return;
  }

  char *equals = strchr(trimmed, '=');
  if (!equals) {
    char commandUpper[LINE_BUFFER_SIZE];
    strncpy(commandUpper, trimmed, sizeof(commandUpper));
    commandUpper[sizeof(commandUpper) - 1] = '\0';
    toUpperInPlace(commandUpper);

    if (strcmp(commandUpper, "RESET") == 0) {
      configReset();
      Serial.println(F("Configuration reset to defaults"));
    } else if (strcmp(commandUpper, "VERSION") == 0) {
      Serial.println(F("Firmware version 1.0.0"));
    } else {
      Serial.println(F("Unknown command"));
    }
    return;
  }

  *equals = '\0';
  char *key = trimWhitespace(trimmed);
  char *value = trimWhitespace(equals + 1);
  handleAssignment(key, value);
}
}  // namespace

void configCliInit() {
  lineLength = 0;
  active = false;
  lastActivityMs = millis();
}

void configCliEnterMode() {
  active = true;
  lineLength = 0;
  lastActivityMs = millis();
  Serial.println();
  Serial.println(F("=== CONFIGURATION MODE ==="));
  Serial.println(F("Commands: LOG_INTERVAL, FILE_MAX_SIZE, TIMEOUT, RESET, VERSION"));
  Serial.println(F("Sensor toggles: LUMIN, TEMP_AIR, HYGR, PRESSURE"));
  Serial.println(F("Thresholds: LUMIN_LOW, LUMIN_HIGH, MIN_TEMP_AIR, MAX_TEMP_AIR, MIN_HYGR, MAX_HYGR"));
  Serial.println(F("RTC: CLOCK=HH:MM:SS, DATE=MM,DD,YYYY, DAY=MON"));
  printPrompt();
}

void configCliExitMode() {
  if (!active) {
    return;
  }
  Serial.println();
  Serial.println(F("Leaving configuration mode"));
  active = false;
}

void configCliUpdate(unsigned long now) {
  if (!active) {
    return;
  }

  while (Serial.available() > 0) {
    const char c = Serial.read();
    lastActivityMs = now;

    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      lineBuffer[lineLength] = '\0';
      handleCommand(lineBuffer);
      lineLength = 0;
      printPrompt();
      continue;
    }
    if (lineLength < LINE_BUFFER_SIZE - 1) {
      lineBuffer[lineLength++] = c;
    }
  }
}

bool configCliShouldExit(unsigned long now) {
  if (!active) {
    return false;
  }
  return (now - lastActivityMs) >= INACTIVITY_TIMEOUT_MS;
}
