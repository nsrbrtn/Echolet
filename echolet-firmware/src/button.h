#pragma once

#include <Arduino.h>

class Button {
 public:
  explicit Button(int pin, bool activeLow = true);

  void begin();
  void update();

  bool isPressed() const;
  bool wasPressed() const;
  bool wasReleased() const;

 private:
  int pin_;
  bool activeLow_;
  bool currentPressed_;
  bool lastPressed_;
  bool pressedEdge_;
  bool releasedEdge_;
};
