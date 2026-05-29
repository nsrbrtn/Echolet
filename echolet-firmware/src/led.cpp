#include "led.h"

#include "config.h"

Led::Led(int redPin, int greenPin, int bluePin)
    : redPin_(redPin),
      greenPin_(greenPin),
      bluePin_(bluePin),
      mode_(LedMode::kOff),
      lastTickMs_(0),
      blinkOn_(false) {}

void Led::begin() {
  pinMode(redPin_, OUTPUT);
  pinMode(greenPin_, OUTPUT);
  pinMode(bluePin_, OUTPUT);
  applyColor(0, 0, 0);
}

void Led::setMode(LedMode mode) {
  mode_ = mode;
}

void Led::update() {
  const unsigned long now = millis();

  switch (mode_) {
    case LedMode::kOff:
      applyColor(0, 0, 0);
      break;
    case LedMode::kReady:
      applyColor(0, 16, 0);
      break;
    case LedMode::kRecording:
      applyColor(64, 0, 0);
      break;
    case LedMode::kUploading:
      applyColor(0, 0, 64);
      break;
    case LedMode::kSuccessFlash:
      applyColor(0, 64, 0);
      break;
    case LedMode::kWifiError:
      if (now - lastTickMs_ >= kErrorBlinkMs) {
        blinkOn_ = !blinkOn_;
        lastTickMs_ = now;
      }
      applyColor(blinkOn_ ? 64 : 0, blinkOn_ ? 32 : 0, 0);
      break;
    case LedMode::kUploadError:
      if (now - lastTickMs_ >= kErrorBlinkMs) {
        blinkOn_ = !blinkOn_;
        lastTickMs_ = now;
      }
      applyColor(blinkOn_ ? 64 : 0, 0, 0);
      break;
    case LedMode::kLowBattery:
      if (now - lastTickMs_ >= 2 * kReadyPulseMs) {
        blinkOn_ = !blinkOn_;
        lastTickMs_ = now;
      }
      applyColor(blinkOn_ ? 32 : 0, 0, 0);
      break;
  }
}

void Led::applyColor(uint8_t red, uint8_t green, uint8_t blue) {
  analogWrite(redPin_, red);
  analogWrite(greenPin_, green);
  analogWrite(bluePin_, blue);
}
