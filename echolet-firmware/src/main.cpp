#include <Arduino.h>

#include "audio_recorder.h"
#include "button.h"
#include "config.h"
#include "led.h"
#include "queue.h"
#include "storage.h"
#include "uploader.h"
#include "wifi_client.h"

enum class AppState {
  kBoot,
  kIdle,
  kRecording,
  kSaveRecording,
  kTryUpload,
  kQueueForLater,
  kError,
};

Button gRecordButton(kButtonPinPrimary, kButtonActiveLow);
Led gLed(kLedPinRed, kLedPinGreen, kLedPinBlue);
AudioRecorder gRecorder;
Storage gStorage;
WifiClient gWifi;
Uploader gUploader;
UploadQueue gQueue;

AppState gState = AppState::kBoot;
String gCurrentRecordingPath;
String gPendingUploadPath;

void transitionTo(AppState nextState) {
  gState = nextState;
}

void handleBoot() {
  gRecordButton.begin();
  gLed.begin();
  gRecorder.begin();
  gWifi.begin();
  gUploader.begin();
  gQueue.begin();

  const bool storageReady = gStorage.begin() && gStorage.ensureLayout();

  if (!storageReady) {
    gLed.setMode(LedMode::kUploadError);
    transitionTo(AppState::kError);
    return;
  }

  gLed.setMode(LedMode::kReady);
  transitionTo(AppState::kIdle);
}

void handleIdle() {
  gLed.setMode(LedMode::kReady);

  if (gQueue.hasPending()) {
    gPendingUploadPath = gQueue.peek();
    transitionTo(AppState::kTryUpload);
    return;
  }

  if (gRecordButton.wasPressed()) {
    if (gRecorder.start()) {
      gLed.setMode(LedMode::kRecording);
      transitionTo(AppState::kRecording);
    } else {
      transitionTo(AppState::kError);
    }
  }
}

void handleRecording() {
  gLed.setMode(LedMode::kRecording);

  if (gRecordButton.wasReleased()) {
    transitionTo(AppState::kSaveRecording);
  }
}

void handleSaveRecording() {
  if (!gRecorder.stop()) {
    transitionTo(AppState::kError);
    return;
  }

  gCurrentRecordingPath = gRecorder.currentFilename();

  if (!gStorage.saveRecordingPlaceholder(gCurrentRecordingPath)) {
    transitionTo(AppState::kError);
    return;
  }

  gPendingUploadPath = gCurrentRecordingPath;
  transitionTo(AppState::kTryUpload);
}

void handleTryUpload() {
  gLed.setMode(LedMode::kUploading);

  const bool wifiReady = gWifi.isConnected() || gWifi.connect();
  if (!wifiReady) {
    gLed.setMode(LedMode::kWifiError);
    transitionTo(AppState::kQueueForLater);
    return;
  }

  if (gUploader.upload(gPendingUploadPath)) {
    gStorage.moveToSent(gPendingUploadPath);
    if (gQueue.hasPending() && gQueue.peek() == gPendingUploadPath) {
      gQueue.pop();
    }
    gLed.setMode(LedMode::kSuccessFlash);
    gPendingUploadPath = String();
    transitionTo(AppState::kIdle);
    return;
  }

  gLed.setMode(LedMode::kUploadError);
  transitionTo(AppState::kQueueForLater);
}

void handleQueueForLater() {
  if (gPendingUploadPath.length() == 0) {
    transitionTo(AppState::kIdle);
    return;
  }

  const bool alreadyQueued = gQueue.hasPending() && gQueue.peek() == gPendingUploadPath;
  if (!alreadyQueued) {
    if (!gQueue.enqueue(gPendingUploadPath)) {
      gStorage.markFailed(gPendingUploadPath);
      transitionTo(AppState::kError);
      return;
    }
  }

  gPendingUploadPath = String();
  transitionTo(AppState::kIdle);
}

void handleError() {
  gLed.setMode(LedMode::kUploadError);
}

void setup() {
  Serial.begin(115200);
}

void loop() {
  gRecordButton.update();
  gLed.update();

  switch (gState) {
    case AppState::kBoot:
      handleBoot();
      break;
    case AppState::kIdle:
      handleIdle();
      break;
    case AppState::kRecording:
      handleRecording();
      break;
    case AppState::kSaveRecording:
      handleSaveRecording();
      break;
    case AppState::kTryUpload:
      handleTryUpload();
      break;
    case AppState::kQueueForLater:
      handleQueueForLater();
      break;
    case AppState::kError:
      handleError();
      break;
  }
}
