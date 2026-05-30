#pragma once

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

enum class LedMode {
  kOff,
  kReady,
  kRecording,
  kUploading,
  kSuccessFlash,
  kQueuedOffline,
  kWifiError,
  kUploadError,
  kLowBattery,
};

class Led {
 public:
  explicit Led(int pin, int count = 1);

  void begin();
  void setMode(LedMode mode);
 void update();

 private:
  void applyColor(uint8_t red, uint8_t green, uint8_t blue);
  void playStartupAnimation();

  Adafruit_NeoPixel pixel_;
  LedMode mode_;
  unsigned long lastTickMs_;
  bool blinkOn_;
};
