#include <Arduino.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>

#include "audio_recorder.h"
#include "button.h"
#include "config.h"
#include "led.h"
#include "queue.h"
#include "storage.h"
#include "time_utils.h"
#include "uploader.h"
#include "wifi_client.h"

enum class AppState {
  kBoot,
  kIdle,
  kRecording,
  kSaveRecording,
  kTryUpload,
  kQueueForLater,
  kPowerOffPending,
  kError,
};

enum class RetryReason {
  kNone,
  kWifiUnavailable,
  kUploadFailed,
};

Button gRecordButton(kButtonPinPrimary, kButtonActiveLow);
Button gPowerButton(kButtonPinSecondary, kButtonActiveLow);
Led gLed(kLedPin, kLedCount);
AudioRecorder gRecorder;
Storage gStorage;
WifiClient gWifi;
Uploader gUploader;
UploadQueue gQueue;

AppState gState = AppState::kBoot;
String gCurrentRecordingPath;
String gCurrentRecordingCreatedAt;
String gPendingUploadPath;
String gPendingUploadCreatedAt;
unsigned long gNextUploadRetryAtMs = 0;
unsigned long gPowerButtonPressedAtMs = 0;
bool gPowerHoldBlockedLogged = false;
RetryReason gRetryReason = RetryReason::kNone;

const char* stateName(AppState state) {
  switch (state) {
    case AppState::kBoot:
      return "boot";
    case AppState::kIdle:
      return "idle";
    case AppState::kRecording:
      return "recording";
    case AppState::kSaveRecording:
      return "save_recording";
    case AppState::kTryUpload:
      return "try_upload";
    case AppState::kQueueForLater:
      return "queue_for_later";
    case AppState::kPowerOffPending:
      return "power_off_pending";
    case AppState::kError:
      return "error";
  }

  return "unknown";
}

void transitionTo(AppState nextState) {
  if (gState != nextState) {
    Serial.print("[state] ");
    Serial.print(stateName(gState));
    Serial.print(" -> ");
    Serial.println(stateName(nextState));
  }
  gState = nextState;
}

bool canEnterPowerOffPending() {
  return gState == AppState::kIdle || gState == AppState::kQueueForLater ||
         gState == AppState::kError;
}

void logWakeReason() {
  const esp_sleep_wakeup_cause_t wakeCause = esp_sleep_get_wakeup_cause();
  switch (wakeCause) {
    case ESP_SLEEP_WAKEUP_UNDEFINED:
      Serial.println("[power] cold boot");
      break;
    case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("[power] woke from deep sleep by power button");
      break;
    default:
      Serial.print("[power] woke from deep sleep, cause=");
      Serial.println(static_cast<int>(wakeCause));
      break;
  }
}

void enterDeepSleep() {
  Serial.println("[power] entering deep sleep");
  persistCurrentTimeEstimate();
  gLed.setMode(LedMode::kOff);
  gLed.update();

  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(kButtonPinSecondary), 0);
  rtc_gpio_pullup_en(static_cast<gpio_num_t>(kButtonPinSecondary));
  rtc_gpio_pulldown_dis(static_cast<gpio_num_t>(kButtonPinSecondary));

  delay(50);
  esp_deep_sleep_start();
}

void handlePowerButton() {
  if (gPowerButton.wasPressed()) {
    gPowerButtonPressedAtMs = millis();
    gPowerHoldBlockedLogged = false;
    Serial.println("[power] button pressed");
  }

  if (gPowerButton.wasReleased()) {
    if (gState == AppState::kPowerOffPending) {
      Serial.println("[power] button released, going to sleep");
      enterDeepSleep();
      return;
    }

    gPowerButtonPressedAtMs = 0;
    gPowerHoldBlockedLogged = false;
    Serial.println("[power] button released");
  }

  if (gState == AppState::kPowerOffPending || !gPowerButton.isPressed() ||
      gPowerButtonPressedAtMs == 0) {
    return;
  }

  if (!canEnterPowerOffPending()) {
    if (!gPowerHoldBlockedLogged && millis() - gPowerButtonPressedAtMs >= kPowerButtonHoldMs) {
      Serial.print("[power] hold ignored in state: ");
      Serial.println(stateName(gState));
      gPowerHoldBlockedLogged = true;
    }
    return;
  }

  if (millis() - gPowerButtonPressedAtMs >= kPowerButtonHoldMs) {
    Serial.println("[power] hold detected, release to power off");
    transitionTo(AppState::kPowerOffPending);
  }
}

void handleBoot() {
  Serial.println("[boot] initializing modules");
  beginTimekeeping();
  gRecordButton.begin();
  gPowerButton.begin();
  gLed.begin();
  gRecorder.begin();
  gWifi.begin();
  gUploader.begin();
  gQueue.begin();

  const bool storageReady = gStorage.begin() && gStorage.ensureLayout();

  if (!storageReady) {
    Serial.println("[boot] storage init failed");
    gLed.setMode(LedMode::kUploadError);
    transitionTo(AppState::kError);
    return;
  }

  Serial.println("[boot] storage ready");
  gLed.setMode(LedMode::kReady);
  transitionTo(AppState::kIdle);
}

void handleIdle() {
  if (gQueue.hasPending() && millis() < gNextUploadRetryAtMs &&
      gRetryReason == RetryReason::kWifiUnavailable) {
    gLed.setMode(LedMode::kQueuedOffline);
  } else {
    gLed.setMode(LedMode::kReady);
  }

  if (gQueue.hasPending()) {
    const unsigned long now = millis();
    if (now < gNextUploadRetryAtMs) {
      return;
    }

    gPendingUploadPath = gQueue.peek();
    Serial.print("[queue] retry pending file: ");
    Serial.println(gPendingUploadPath);
    transitionTo(AppState::kTryUpload);
    return;
  }

  if (gRecordButton.wasPressed()) {
    Serial.println("[button] record pressed");
    if (gRecorder.start()) {
      Serial.print("[recording] started: ");
      Serial.println(gRecorder.currentFilename());
      gLed.setMode(LedMode::kRecording);
      transitionTo(AppState::kRecording);
    } else {
      Serial.println("[recording] start failed");
      transitionTo(AppState::kError);
    }
  }
}

void handleRecording() {
  gLed.setMode(LedMode::kRecording);

  if (!gRecorder.captureStep()) {
    Serial.println("[recording] capture step failed");
    transitionTo(AppState::kError);
    return;
  }

  if (gRecordButton.wasReleased()) {
    Serial.println("[button] record released");
    transitionTo(AppState::kSaveRecording);
  }
}

void handleSaveRecording() {
  if (!gRecorder.stop()) {
    Serial.println("[recording] stop failed");
    transitionTo(AppState::kError);
    return;
  }

  gCurrentRecordingPath = gRecorder.currentFilename();
  gCurrentRecordingCreatedAt = gRecorder.currentCreatedAt();
  Serial.print("[recording] stopped: ");
  Serial.println(gCurrentRecordingPath);
  Serial.println("[storage] recording saved to SD");
  gPendingUploadPath = gCurrentRecordingPath;
  gPendingUploadCreatedAt = gCurrentRecordingCreatedAt;
  gNextUploadRetryAtMs = 0;
  gRetryReason = RetryReason::kNone;
  transitionTo(AppState::kTryUpload);
}

void handleTryUpload() {
  gLed.setMode(LedMode::kUploading);
  Serial.print("[upload] trying file: ");
  Serial.println(gPendingUploadPath);

  const bool wifiReady = gWifi.isConnected() || gWifi.connect();
  if (!wifiReady) {
    Serial.println("[wifi] connect failed, queueing for later");
    gRetryReason = RetryReason::kWifiUnavailable;
    gLed.setMode(LedMode::kQueuedOffline);
    transitionTo(AppState::kQueueForLater);
    return;
  }

  if (isFallbackIsoTimestamp(gPendingUploadCreatedAt) && isWallClockValid()) {
    gPendingUploadCreatedAt = makeIsoTimestamp();
    Serial.print("[time] updated created_at after NTP sync: ");
    Serial.println(gPendingUploadCreatedAt);

    const String renamedPath = makeRecordingFilename();
    if (renamedPath != gPendingUploadPath) {
      if (gStorage.renameFile(gPendingUploadPath, renamedPath)) {
        Serial.print("[storage] renamed recording after NTP sync: ");
        Serial.println(renamedPath);
        gPendingUploadPath = renamedPath;
      } else {
        Serial.println("[storage] rename after NTP sync failed, keeping original path");
      }
    }
  }

  if (gUploader.upload(gPendingUploadPath, gPendingUploadCreatedAt)) {
    Serial.println("[upload] success");
    gStorage.moveToSent(gPendingUploadPath);
    if (gQueue.hasPending() && gQueue.peek() == gPendingUploadPath) {
      gQueue.pop();
    }
    gLed.setMode(LedMode::kSuccessFlash);
    gPendingUploadPath = String();
    gPendingUploadCreatedAt = String();
    transitionTo(AppState::kIdle);
    return;
  }

  Serial.println("[upload] failed");
  gRetryReason = RetryReason::kUploadFailed;
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
      Serial.println("[queue] enqueue failed, marking file as failed");
      gStorage.markFailed(gPendingUploadPath);
      transitionTo(AppState::kError);
      return;
    }
    Serial.print("[queue] enqueued: ");
    Serial.println(gPendingUploadPath);
  } else {
    Serial.println("[queue] file already pending");
  }

  unsigned long retryIntervalMs = kUploadRetryIntervalMs;
  if (gRetryReason == RetryReason::kWifiUnavailable) {
    retryIntervalMs = kWifiRetryIntervalMs;
  }

  gNextUploadRetryAtMs = millis() + retryIntervalMs;
  Serial.print("[queue] next retry in ms: ");
  Serial.println(retryIntervalMs);
  gPendingUploadPath = String();
  transitionTo(AppState::kIdle);
}

void handleError() {
  gLed.setMode(LedMode::kUploadError);
}

void handlePowerOffPending() {
  gLed.setMode(LedMode::kOff);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("[echolet] boot");
  logWakeReason();
}

void loop() {
  gRecordButton.update();
  gPowerButton.update();
  handlePowerButton();
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
    case AppState::kPowerOffPending:
      handlePowerOffPending();
      break;
    case AppState::kError:
      handleError();
      break;
  }
}
