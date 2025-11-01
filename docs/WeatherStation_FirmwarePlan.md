# 🧭 Weather Station Firmware Plan (Based on “Mode de fonctionnement station météo”)

## 🎯 Objective
Develop an Arduino-based weather-station firmware managing **four operating modes** (Standard, Configuration, Maintenance, Economic), sensor data acquisition, RTC timestamping, SD-card logging, and LED + button interface control.

---

## 1. 🔧 Hardware & Pin Assignments

| Component | Function | Arduino Pin |
|------------|-----------|--------------|
| **DHT11** | Temperature + Humidity | D2 |
| **GPS module** | TX / RX (SoftwareSerial) | D3 / D4 |
| **RGB LED** | Red / Green / Blue | D8 / D5 / D6 |
| **Buttons** | Red / Green | D7 / D9 |
| **SD Card Module** | SPI (CS, MOSI, MISO, SCK) | D10, D11, D12, D13 |
| **BH1750 + TinyRTC** | I²C bus | A4 (SDA) / A5 (SCL) |
| **TinyRTC DS18B20 (optional)** | 1-Wire temperature | A0 |

---

## 2. ⚙️ Core Operating Modes

### 🟢 Mode STANDARD
- **Activation** : Normal power-up (no button pressed)
- **Behavior** :
  - Periodic sensor acquisition (`LOG_INTERVAL = 10 min` default)
  - Timeout for each sensor = 30s (`TIMEOUT`)
  - Missing sensor → “NA”
  - Data logged to SD card (1 line = 1 timestamp)
  - File naming: `YYMMDD_0.LOG`
  - When full (`FILE_MAX_SIZE = 2 KB`) → copy to `_1.LOG`, restart `_0.LOG`
- **LED** : Green steady
- **Switches** :
  - Long press red (5s) → Maintenance
  - Long press green (5s) → Economic

---

### 🟡 Mode CONFIGURATION
- **Activation** : Start with red button pressed
- **Behavior** :
  - Stop all sensor acquisitions
  - Enable UART for serial configuration
  - Modify EEPROM-stored parameters:
    ```
    LOG_INTERVAL=10
    FILE_MAX_SIZE=4096
    TIMEOUT=30
    RESET
    VERSION
    ```
  - Sensor config examples:
    - `LUMIN=1`, `TEMP_AIR=1`, `HYGR=1`, `PRESSURE=1`
    - Thresholds: `LUMIN_LOW`, `LUMIN_HIGH`, `MIN_TEMP_AIR`, etc.
  - Set RTC:
    ```
    CLOCK=HH:MM:SS
    DATE=MM,DD,YYYY
    DAY=MON/TUE/WED/THU/FRI/SAT/SUN
    ```
- **Auto-exit** : After 30 min of inactivity
- **LED** : Yellow steady

---

### 🟠 Mode MAINTENANCE
- **Access** : Hold red button 5s (from Standard or Economic)
- **Behavior** :
  - Stop SD writes
  - Print live data to Serial
  - Safe SD removal
  - Hold red 5s again → return to previous mode
- **LED** : Orange steady

---

### 🔵 Mode ÉCONOMIQUE
- **Access** : Hold green button 5s (from Standard)
- **Behavior** :
  - Disable GPS or reduce frequency
  - Double `LOG_INTERVAL`
  - Hold red 5s → back to Standard
- **LED** : Blue steady

---

## 3. 💡 LED Error Codes

| LED Pattern | Meaning |
|--------------|----------|
| Red + Blue blink (1 Hz) | RTC access error |
| Red + Yellow (= Red+Green) blink (1 Hz) | GPS data error |
| Red + Green blink (1 Hz) | Sensor access error |
| Red short + Green long | Sensor incoherent data |
| Red + White (= Red+Green+Blue) blink (1 Hz) | SD full |
| Red short + White long | SD write/access error |

---

## 4. 🧱 Software Architecture

```
setup():
 ├─ initSerial()
 ├─ initLED()
 ├─ initButtons()
 ├─ initI2C() → BH1750 + RTC
 ├─ initSensors() (DHT11, GPS, etc.)
 ├─ initSD()
 ├─ detectStartupMode()
 └─ showModeLED()

loop():
 ├─ checkButtons() → detect long presses
 ├─ handleModeSwitch()
 ├─ runCurrentMode()
 └─ blinkErrorLEDs()
```

Each mode handled by:
- `runStandardMode()`
- `runConfigMode()`
- `runMaintenanceMode()`
- `runEcoMode()`

---

## 5. ⏱️ Data Logging Logic

- Each measurement = 1 CSV line:
  ```
  yyyy-mm-dd hh:mm:ss, tempC, humidity, lux, pressure, GPS_lat, GPS_long
  ```
- Timeout → `NA`
- File rotation:
  - Write to `YYMMDD_0.LOG`
  - If > `FILE_MAX_SIZE`, copy to `_1.LOG`, restart `_0.LOG`
- SD full → Red/White blink
- SD error → Red short/White long

---

## 6. ⚙️ Configuration Management (EEPROM)

| Parameter | Type | Default |
|------------|------|----------|
| `LOG_INTERVAL` | uint16_t (min) | 10 |
| `FILE_MAX_SIZE` | uint16_t (bytes) | 2048 |
| `TIMEOUT` | uint16_t (sec) | 30 |
| Sensor enable flags | uint8_t | 1 |
| Thresholds | int16_t | Various |

Functions:
- `loadConfigFromEEPROM()`
- `saveConfigToEEPROM()`
- `resetDefaultConfig()`

---

## 7. 🧭 Buttons & LED Behavior

- Use internal pull-ups (`INPUT_PULLUP`)
- Long press (≥5s) = mode switch
- Debounced using `millis()` timers
- LED updates continuously (no delays)

---

## 8. 🕓 RTC Integration

- RTC = TinyRTC DS1307 (I²C)
- First boot → set to compile time
- Config mode → set via commands
- RTC errors trigger Red+Blue blink

---

## 9. 📡 GPS Integration

- RX/TX = D4/D3 (SoftwareSerial)
- Economic mode → read 1/2 cycles
- Timeout → “NA” + Red/Yellow blink

---

## 10. ⚙️ Error Handling Table

| Error | Action | LED Pattern |
|--------|--------|-------------|
| RTC unreachable | Skip timestamp | Red + Blue blink |
| GPS no data | Skip GPS, mark NA | Red + Yellow blink |
| Sensor timeout | mark NA | Red + Green blink |
| Sensor incoherent | ignore reading | Red short / Green long |
| SD full | stop logging | Red + White blink |
| SD write fail | retry | Red short / White long |

---

## 11. 🧩 Power & Memory Optimization

- In Economic mode:
  - Disable high-power sensors (GPS)
  - Double logging interval
  - MCU sleep between readings

---

## 12. 🧮 File Naming Logic

```
getLogFileName():
  sprintf(filename, "%02d%02d%02d_%d.LOG",
          year%100, month, day, revision);
```

---

## 13. 🧪 Test Plan

| Test | Expected Result |
|-------|-----------------|
| Power-on (no button) | Green LED, data logged |
| Red press on start | Yellow LED, serial config |
| Red long press | Orange LED, SD safe |
| Green long press | Blue LED, GPS slow |
| RTC unplugged | Red+Blue blink |
| SD full | Red+White blink |
| No battery | Time resets |

---

## 14. 📘 Deliverables

1. `main.ino` – main loop and mode logic  
2. `ConfigManager.cpp/h` – EEPROM storage  
3. `ModeManager.cpp/h` – mode switching  
4. `SensorDrivers` – DHT, BH1750, GPS, RTC, SD  
5. LED state machine implementation  
6. README.md with wiring + usage  
7. Test validation document

---
