#include "button.h"

Button::Button(int pin, bool activeLow)
    : pin_(pin),
      activeLow_(activeLow),
      currentPressed_(false),
      lastPressed_(false),
      pressedEdge_(false),
      releasedEdge_(false) {}

void Button::begin() {
  pinMode(pin_, INPUT_PULLUP);
  update();
  lastPressed_ = currentPressed_;
}

void Button::update() {
  const int raw = digitalRead(pin_);
  currentPressed_ = activeLow_ ? (raw == LOW) : (raw == HIGH);

  pressedEdge_ = !lastPressed_ && currentPressed_;
  releasedEdge_ = lastPressed_ && !currentPressed_;
  lastPressed_ = currentPressed_;
}

bool Button::isPressed() const {
  return currentPressed_;
}

bool Button::wasPressed() const {
  return pressedEdge_;
}

bool Button::wasReleased() const {
  return releasedEdge_;
}
