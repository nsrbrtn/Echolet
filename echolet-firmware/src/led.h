#pragma once

#include <Arduino.h>

enum class LedMode {
  kOff,
  kReady,
  kRecording,
  kUploading,
  kSuccessFlash,
  kWifiError,
  kUploadError,
  kLowBattery,
};

class Led {
 public:
  Led(int redPin, int greenPin, int bluePin);

  void begin();
  void setMode(LedMode mode);
  void update();

 private:
  void applyColor(uint8_t red, uint8_t green, uint8_t blue);

  int redPin_;
  int greenPin_;
  int bluePin_;
  LedMode mode_;
  unsigned long lastTickMs_;
  bool blinkOn_;
};
