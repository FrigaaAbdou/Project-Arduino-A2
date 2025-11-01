#include "rgbled.h"

namespace {
constexpr uint8_t RED_PIN = 8;
constexpr uint8_t GREEN_PIN = 5;
constexpr uint8_t BLUE_PIN = 6;

constexpr bool COMMON_ANODE = true;

struct PatternStep {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  unsigned int durationMs;
};

struct Pattern {
  const PatternStep *steps;
  uint8_t stepCount;
  bool isStatic;
};

constexpr uint8_t OFF = 0;
constexpr uint8_t FULL = 255;
constexpr uint8_t HALF = 128;

const PatternStep SOLID_OFF_STEPS[] = {{OFF, OFF, OFF, 0}};
const PatternStep SOLID_GREEN_STEPS[] = {{OFF, FULL, OFF, 0}};
const PatternStep SOLID_YELLOW_STEPS[] = {{FULL, FULL, OFF, 0}};
const PatternStep SOLID_BLUE_STEPS[] = {{OFF, OFF, FULL, 0}};
const PatternStep SOLID_ORANGE_STEPS[] = {{FULL, HALF, OFF, 0}};

const PatternStep BLINK_MAGENTA_STEPS[] = {
    {FULL, OFF, FULL, 500},
    {OFF, OFF, OFF, 500},
};

const PatternStep BLINK_RED_YELLOW_STEPS[] = {
    {FULL, OFF, OFF, 500},
    {FULL, FULL, OFF, 500},
};

const PatternStep BLINK_YELLOW_STEPS[] = {
    {FULL, FULL, OFF, 500},
    {OFF, OFF, OFF, 500},
};

const PatternStep PULSE_RED_GREEN_STEPS[] = {
    {FULL, OFF, OFF, 200},
    {OFF, OFF, OFF, 100},
    {OFF, FULL, OFF, 800},
    {OFF, OFF, OFF, 100},
};

const PatternStep BLINK_WHITE_RED_STEPS[] = {
    {FULL, FULL, FULL, 500},
    {FULL, OFF, OFF, 500},
};

const PatternStep PULSE_RED_WHITE_STEPS[] = {
    {FULL, OFF, OFF, 200},
    {OFF, OFF, OFF, 100},
    {FULL, FULL, FULL, 800},
    {OFF, OFF, OFF, 100},
};

const Pattern PATTERN_SOLID_OFF{SOLID_OFF_STEPS, 1, true};
const Pattern PATTERN_SOLID_GREEN{SOLID_GREEN_STEPS, 1, true};
const Pattern PATTERN_SOLID_YELLOW{SOLID_YELLOW_STEPS, 1, true};
const Pattern PATTERN_SOLID_BLUE{SOLID_BLUE_STEPS, 1, true};
const Pattern PATTERN_SOLID_ORANGE{SOLID_ORANGE_STEPS, 1, true};
const Pattern PATTERN_ERROR_MAGENTA{BLINK_MAGENTA_STEPS, 2, false};
const Pattern PATTERN_ERROR_RED_YELLOW{BLINK_RED_YELLOW_STEPS, 2, false};
const Pattern PATTERN_ERROR_YELLOW{BLINK_YELLOW_STEPS, 2, false};
const Pattern PATTERN_ERROR_RED_GREEN_PULSE{PULSE_RED_GREEN_STEPS, 4, false};
const Pattern PATTERN_ERROR_WHITE_RED{BLINK_WHITE_RED_STEPS, 2, false};
const Pattern PATTERN_ERROR_RED_WHITE_PULSE{PULSE_RED_WHITE_STEPS, 4, false};

const Pattern *currentPattern = &PATTERN_SOLID_OFF;
uint8_t currentStep = 0;
unsigned long stepStartedAt = 0;
RgbLedState currentState = RgbLedState::Off;

void applyColor(uint8_t r, uint8_t g, uint8_t b) {
  if (COMMON_ANODE) {
    r = 255 - r;
    g = 255 - g;
    b = 255 - b;
  }
  analogWrite(RED_PIN, r);
  analogWrite(GREEN_PIN, g);
  analogWrite(BLUE_PIN, b);
}

const Pattern &patternFor(RgbLedState state) {
  switch (state) {
    case RgbLedState::Off:
      return PATTERN_SOLID_OFF;
    case RgbLedState::SolidGreen:
      return PATTERN_SOLID_GREEN;
    case RgbLedState::SolidYellow:
      return PATTERN_SOLID_YELLOW;
    case RgbLedState::SolidBlue:
      return PATTERN_SOLID_BLUE;
    case RgbLedState::SolidOrange:
      return PATTERN_SOLID_ORANGE;
    case RgbLedState::ErrorRtc:
      return PATTERN_ERROR_MAGENTA;
    case RgbLedState::ErrorGps:
      return PATTERN_ERROR_RED_YELLOW;
    case RgbLedState::ErrorSensorAccess:
      return PATTERN_ERROR_YELLOW;
    case RgbLedState::ErrorSensorIncoherent:
      return PATTERN_ERROR_RED_GREEN_PULSE;
    case RgbLedState::ErrorSdFull:
      return PATTERN_ERROR_WHITE_RED;
    case RgbLedState::ErrorSdAccess:
      return PATTERN_ERROR_RED_WHITE_PULSE;
  }
  return PATTERN_SOLID_OFF;
}

void applyStep(uint8_t stepIndex) {
  const PatternStep &step = currentPattern->steps[stepIndex];
  applyColor(step.r, step.g, step.b);
}
}  // namespace

void rgbInit() {
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  rgbSetState(RgbLedState::Off);
}

void rgbSetState(RgbLedState state) {
  currentState = state;
  currentPattern = &patternFor(state);
  currentStep = 0;
  stepStartedAt = millis();
  applyStep(currentStep);
}

RgbLedState rgbCurrentState() {
  return currentState;
}

void rgbUpdate(unsigned long now) {
  if (currentPattern->isStatic) {
    return;
  }

  const PatternStep &step = currentPattern->steps[currentStep];
  if (step.durationMs == 0) {
    return;
  }

  if (now - stepStartedAt < step.durationMs) {
    return;
  }

  stepStartedAt += step.durationMs;
  currentStep = (currentStep + 1) % currentPattern->stepCount;
  applyStep(currentStep);
}
