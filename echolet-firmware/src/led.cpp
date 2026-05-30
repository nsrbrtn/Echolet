#include "led.h"

#include "config.h"

Led::Led(int pin, int count)
    : pixel_(count, pin, NEO_GRB + NEO_KHZ800),
      mode_(LedMode::kOff),
      lastTickMs_(0),
      blinkOn_(false) {}

void Led::begin() {
  pixel_.begin();
  pixel_.setBrightness(20);
  applyColor(0, 0, 0);
  playStartupAnimation();
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
    case LedMode::kQueuedOffline:
      if (now - lastTickMs_ >= 2000) {
        blinkOn_ = !blinkOn_;
        lastTickMs_ = now;
      }
      applyColor(blinkOn_ ? 8 : 0, blinkOn_ ? 6 : 0, 0);
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
  pixel_.setPixelColor(0, pixel_.Color(red, green, blue));
  pixel_.show();
}

void Led::playStartupAnimation() {
  applyColor(48, 0, 0);
  delay(120);
  applyColor(0, 48, 0);
  delay(120);
  applyColor(0, 0, 48);
  delay(120);
  applyColor(0, 0, 0);
  delay(80);
}
