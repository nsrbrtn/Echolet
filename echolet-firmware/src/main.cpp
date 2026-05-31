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
TimeReliability gCurrentRecordingTimeReliability = TimeReliability::kInvalid;
String gPendingUploadPath;
String gPendingUploadCreatedAt;
unsigned long gNextUploadRetryAtMs = 0;
unsigned long gPowerButtonPressedAtMs = 0;
bool gPowerHoldBlockedLogged = false;
RetryReason gRetryReason = RetryReason::kNone;
bool gUploadAttemptLogged = false;
bool gBootInitialized = false;
unsigned long gBootPreflightStartedAtMs = 0;

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
  if (nextState != AppState::kTryUpload) {
    gUploadAttemptLogged = false;
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
  if (!gBootInitialized) {
    Serial.println("[boot] initializing modules");
    beginTimekeeping();
    gRecordButton.begin();
    gPowerButton.begin();
    gLed.begin();
    gRecorder.begin();
    gWifi.begin();
    gUploader.begin();

    const bool storageReady = gStorage.begin() && gStorage.ensureLayout();
    if (!storageReady) {
      Serial.println("[boot] storage init failed");
      gLed.setMode(LedMode::kUploadError);
      transitionTo(AppState::kError);
      return;
    }

    if (!gQueue.begin()) {
      Serial.println("[boot] queue init failed");
      gLed.setMode(LedMode::kUploadError);
      transitionTo(AppState::kError);
      return;
    }

    gBootInitialized = true;
    gBootPreflightStartedAtMs = millis();
    Serial.println("[boot] storage ready");
    Serial.println("[boot] waiting briefly for Wi-Fi/NTP before ready");
    return;
  }

  const WifiReadyStatus wifiStatus = gWifi.ensureReady();
  if (wifiStatus == WifiReadyStatus::kReady) {
    Serial.print("[boot] ready with time reliability=");
    Serial.println(timeReliabilityName(getTimeReliability()));
    gLed.setMode(LedMode::kReady);
    transitionTo(AppState::kIdle);
    return;
  }

  if (wifiStatus == WifiReadyStatus::kFailed) {
    Serial.print("[boot] continuing without network time, reliability=");
    Serial.println(timeReliabilityName(getTimeReliability()));
    gLed.setMode(LedMode::kReady);
    transitionTo(AppState::kIdle);
    return;
  }

  gLed.setMode(LedMode::kUploading);
  if (millis() - gBootPreflightStartedAtMs >= kBootTimeSyncBudgetMs) {
    Serial.print("[boot] Wi-Fi/NTP preflight timed out, reliability=");
    Serial.println(timeReliabilityName(getTimeReliability()));
    gLed.setMode(LedMode::kReady);
    transitionTo(AppState::kIdle);
  }
}

void handleIdle() {
  if (gQueue.hasPending() && millis() < gNextUploadRetryAtMs &&
      gRetryReason == RetryReason::kWifiUnavailable) {
    gLed.setMode(LedMode::kQueuedOffline);
  } else {
    gLed.setMode(LedMode::kReady);
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
    return;
  }

  if (!gQueue.hasPending()) {
    return;
  }

  const unsigned long now = millis();
  if (gRetryReason == RetryReason::kWifiUnavailable) {
    if (gWifi.isConnected()) {
      gNextUploadRetryAtMs = 0;
    } else if (now >= gNextUploadRetryAtMs) {
      const WifiReadyStatus wifiStatus = gWifi.ensureReady();
      if (wifiStatus == WifiReadyStatus::kFailed) {
        gNextUploadRetryAtMs = now + kWifiRetryIntervalMs;
        Serial.print("[wifi] retry deferred for ms: ");
        Serial.println(kWifiRetryIntervalMs);
      }
      return;
    } else {
      return;
    }
  } else if (now < gNextUploadRetryAtMs) {
    return;
  }

  UploadQueueItem pendingItem;
  if (!gQueue.peek(pendingItem)) {
    Serial.println("[queue] failed to read pending item");
    transitionTo(AppState::kError);
    return;
  }

  gPendingUploadPath = pendingItem.path;
  gPendingUploadCreatedAt = pendingItem.createdAt;
  Serial.print("[queue] retry pending file: ");
  Serial.println(gPendingUploadPath);
  transitionTo(AppState::kTryUpload);
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
  gCurrentRecordingTimeReliability = gRecorder.currentTimeReliability();
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
  if (!gUploadAttemptLogged) {
    Serial.print("[upload] trying file: ");
    Serial.println(gPendingUploadPath);
    gUploadAttemptLogged = true;
  }

  const WifiReadyStatus wifiStatus = gWifi.ensureReady();
  if (wifiStatus == WifiReadyStatus::kInProgress) {
    return;
  }

  if (wifiStatus == WifiReadyStatus::kFailed) {
    Serial.println("[wifi] connect failed, queueing for later");
    gRetryReason = RetryReason::kWifiUnavailable;
    gLed.setMode(LedMode::kQueuedOffline);
    transitionTo(AppState::kQueueForLater);
    return;
  }

  if (gCurrentRecordingTimeReliability != TimeReliability::kFreshNtp &&
      getTimeReliability() == TimeReliability::kFreshNtp && isWallClockValid()) {
    gPendingUploadCreatedAt = makeIsoTimestamp();
    Serial.print("[time] refreshed created_at after fresh NTP sync: ");
    Serial.println(gPendingUploadCreatedAt);

    const String renamedPath = makeRecordingFilename();
    if (renamedPath != gPendingUploadPath) {
      if (gStorage.renameFile(gPendingUploadPath, renamedPath)) {
        Serial.print("[storage] renamed recording after fresh NTP sync: ");
        Serial.println(renamedPath);
        gPendingUploadPath = renamedPath;
        gCurrentRecordingPath = renamedPath;
      } else {
        Serial.println("[storage] rename after fresh NTP sync failed, keeping original path");
      }
    }
    gCurrentRecordingTimeReliability = TimeReliability::kFreshNtp;
  }

  if (gUploader.upload(gPendingUploadPath, gPendingUploadCreatedAt)) {
    Serial.println("[upload] success");
    const String uploadedPath = gPendingUploadPath;
    if (!gStorage.moveToSent(gPendingUploadPath)) {
      Serial.println("[storage] failed to move uploaded file to sent");
      transitionTo(AppState::kError);
      return;
    }
    Serial.print("[storage] moved uploaded file to sent: ");
    Serial.println(uploadedPath);

    if (!gStorage.cleanupSentIfNeeded()) {
      Serial.println("[storage] cleanup failed after upload");
    }

    UploadQueueItem pendingItem;
    if (gQueue.peek(pendingItem) && pendingItem.path == gPendingUploadPath) {
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

  const bool alreadyQueued = gQueue.containsPath(gPendingUploadPath);
  if (!alreadyQueued) {
    if (!gQueue.enqueue(gPendingUploadPath, gPendingUploadCreatedAt)) {
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
