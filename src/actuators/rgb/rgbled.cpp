#include "rgbled.h"

namespace {
const uint8_t RED_PIN = 9;
const uint8_t GREEN_PIN = 10;
const uint8_t BLUE_PIN = 11;

const bool COMMON_ANODE = false;

struct RgbColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

const RgbColor COLORS[] = {
    {255, 0, 0},    // Red
    {0, 255, 0},    // Green
    {0, 0, 255},    // Blue
    {255, 255, 0},  // Yellow
    {0, 255, 255},  // Cyan
    {255, 0, 255},  // Magenta
    {255, 255, 255} // White
};

const unsigned long COLOR_ON_TIME_MS = 800;
const unsigned long COLOR_OFF_TIME_MS = 200;

size_t currentColor = 0;
bool colorOn = false;
unsigned long lastChange = 0;

void setColor(uint8_t r, uint8_t g, uint8_t b) {
  if (COMMON_ANODE) {
    r = 255 - r;
    g = 255 - g;
    b = 255 - b;
  }

  analogWrite(RED_PIN, r);
  analogWrite(GREEN_PIN, g);
  analogWrite(BLUE_PIN, b);
}
}  // namespace

void rgbInit() {
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  setColor(0, 0, 0);
}

void rgbUpdate(unsigned long now) {
  const unsigned long interval = colorOn ? COLOR_ON_TIME_MS : COLOR_OFF_TIME_MS;
  if (now - lastChange < interval) {
    return;
  }

  colorOn = !colorOn;
  lastChange = now;

  if (colorOn) {
    const RgbColor &color = COLORS[currentColor];
    setColor(color.r, color.g, color.b);
  } else {
    setColor(0, 0, 0);
    currentColor = (currentColor + 1) % (sizeof(COLORS) / sizeof(COLORS[0]));
  }
}
