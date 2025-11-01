#pragma once

#include <Arduino.h>

enum class ButtonId {
  Red,
  Green
};

enum class ButtonEventType {
  ShortPress,
  LongPress
};

struct ButtonEvent {
  ButtonId button;
  ButtonEventType type;
};

void buttonManagerInit();
void buttonManagerUpdate(unsigned long now);
bool buttonManagerGetEvent(ButtonEvent &event);
bool buttonManagerIsPressed(ButtonId button);
