#include "button_manager.h"

namespace {
constexpr unsigned long LONG_PRESS_MS = 5000;
constexpr size_t EVENT_QUEUE_SIZE = 4;

struct ButtonState {
  uint8_t pin;
  bool pressed;
  bool longReported;
  unsigned long pressStart;
};

ButtonState redButton{7, false, false, 0};
ButtonState greenButton{9, false, false, 0};

ButtonEvent eventQueue[EVENT_QUEUE_SIZE];
size_t queueHead = 0;
size_t queueTail = 0;

ButtonState &stateFor(ButtonId button) {
  return (button == ButtonId::Red) ? redButton : greenButton;
}

bool queueIsFull() {
  return ((queueHead + 1) % EVENT_QUEUE_SIZE) == queueTail;
}

void pushEvent(const ButtonEvent &event) {
  if (queueIsFull()) {
    return;
  }
  eventQueue[queueHead] = event;
  queueHead = (queueHead + 1) % EVENT_QUEUE_SIZE;
}

bool popEvent(ButtonEvent &event) {
  if (queueHead == queueTail) {
    return false;
  }
  event = eventQueue[queueTail];
  queueTail = (queueTail + 1) % EVENT_QUEUE_SIZE;
  return true;
}

void updateButton(ButtonId id, ButtonState &state, unsigned long now) {
  const bool rawPressed = digitalRead(state.pin) == LOW;

  if (rawPressed && !state.pressed) {
    state.pressed = true;
    state.pressStart = now;
    state.longReported = false;
  }

  if (!rawPressed && state.pressed) {
    if (!state.longReported) {
      pushEvent(ButtonEvent{id, ButtonEventType::ShortPress});
    }
    state.pressed = false;
    state.longReported = false;
  }

  if (state.pressed && !state.longReported &&
      now - state.pressStart >= LONG_PRESS_MS) {
    state.longReported = true;
    pushEvent(ButtonEvent{id, ButtonEventType::LongPress});
  }
}
}  // namespace

void buttonManagerInit() {
  pinMode(redButton.pin, INPUT_PULLUP);
  pinMode(greenButton.pin, INPUT_PULLUP);
  redButton.pressed = digitalRead(redButton.pin) == LOW;
  greenButton.pressed = digitalRead(greenButton.pin) == LOW;
  redButton.longReported = false;
  greenButton.longReported = false;
  queueHead = queueTail = 0;
}

void buttonManagerUpdate(unsigned long now) {
  updateButton(ButtonId::Red, redButton, now);
  updateButton(ButtonId::Green, greenButton, now);
}

bool buttonManagerGetEvent(ButtonEvent &event) {
  return popEvent(event);
}

bool buttonManagerIsPressed(ButtonId button) {
  return digitalRead(stateFor(button).pin) == LOW;
}
