#include "time_utils.h"

#include <Preferences.h>
#include <time.h>
#include <sys/time.h>

#include "config.h"

namespace {

constexpr time_t kMinimumValidEpoch = 1700000000;
constexpr const char* kTimePrefsNamespace = "echolet-time";
constexpr const char* kTimePrefsEpochKey = "last_epoch";

Preferences gTimePrefs;
bool gTimePrefsReady = false;
bool gTimeRestoredFromStorage = false;
bool gHasNtpSyncThisSession = false;

void ensurePrefsOpen() {
  if (!gTimePrefsReady) {
    gTimePrefsReady = gTimePrefs.begin(kTimePrefsNamespace, false);
  }
}

bool setSystemEpoch(time_t epoch) {
  if (epoch < kMinimumValidEpoch) {
    return false;
  }

  const timeval value = {.tv_sec = epoch, .tv_usec = 0};
  return settimeofday(&value, nullptr) == 0;
}

bool saveEpochToPrefs(time_t epoch) {
  if (epoch < kMinimumValidEpoch) {
    return false;
  }

  ensurePrefsOpen();
  if (!gTimePrefsReady) {
    return false;
  }

  return gTimePrefs.putULong64(kTimePrefsEpochKey, static_cast<uint64_t>(epoch)) > 0;
}

String formatOffsetWithColon(const char* offsetRaw) {
  if (offsetRaw == nullptr || strlen(offsetRaw) != 5) {
    return String("Z");
  }

  return String(offsetRaw).substring(0, 3) + ":" + String(offsetRaw).substring(3);
}

bool buildLocalTime(struct tm& timeInfo) {
  if (!isWallClockValid()) {
    return false;
  }
  time_t now = time(nullptr);
  return localtime_r(&now, &timeInfo) != nullptr;
}

}  // namespace

TimeReliability getTimeReliability() {
  if (!isWallClockValid()) {
    return TimeReliability::kInvalid;
  }

  if (gHasNtpSyncThisSession) {
    return TimeReliability::kFreshNtp;
  }

  if (gTimeRestoredFromStorage) {
    return TimeReliability::kRestoredEstimate;
  }

  return TimeReliability::kInvalid;
}

const char* timeReliabilityName(TimeReliability reliability) {
  switch (reliability) {
    case TimeReliability::kInvalid:
      return "invalid";
    case TimeReliability::kRestoredEstimate:
      return "restored_estimate";
    case TimeReliability::kFreshNtp:
      return "fresh_ntp";
  }

  return "unknown";
}

void beginTimekeeping() {
  ensurePrefsOpen();
  if (!gTimePrefsReady) {
    Serial.println("[time] failed to open persistent time storage");
    return;
  }

  const time_t savedEpoch = static_cast<time_t>(gTimePrefs.getULong64(kTimePrefsEpochKey, 0));
  if (!setSystemEpoch(savedEpoch)) {
    Serial.println("[time] no saved wall clock available");
    return;
  }

  gTimeRestoredFromStorage = true;
  Serial.print("[time] restored last known time (");
  Serial.print(timeReliabilityName(getTimeReliability()));
  Serial.print("): ");
  Serial.println(makeIsoTimestamp());
}

void persistCurrentTimeEstimate() {
  if (!isWallClockValid()) {
    return;
  }

  if (saveEpochToPrefs(time(nullptr))) {
    Serial.print("[time] saved current time estimate (");
    Serial.print(timeReliabilityName(getTimeReliability()));
    Serial.println(")");
  }
}

bool syncTimeWithNtp() {
  if (gHasNtpSyncThisSession) {
    return true;
  }

  configTzTime(kTimeZonePosix, kNtpServerPrimary, kNtpServerSecondary);
  const unsigned long startedAtMs = millis();
  while (millis() - startedAtMs < kNtpSyncTimeoutMs) {
    if (isWallClockValid()) {
      saveEpochToPrefs(time(nullptr));
      gHasNtpSyncThisSession = true;
      gTimeRestoredFromStorage = false;
      return true;
    }
    delay(kNtpPollIntervalMs);
  }

  return false;
}

bool isWallClockValid() {
  return time(nullptr) >= kMinimumValidEpoch;
}

bool isFallbackIsoTimestamp(const String& value) {
  return value.length() == 0 || value == "1970-01-01T00:00:00Z" || value.startsWith("1970-");
}

String makeRecordingFilename() {
  if (!isWallClockValid()) {
    return "/echolet/audio/new/recording-" + String(millis()) + ".wav";
  }

  struct tm timeInfo = {};
  if (!buildLocalTime(timeInfo)) {
    return "/echolet/audio/new/recording-" + String(millis()) + ".wav";
  }

  char buffer[64];
  strftime(buffer, sizeof(buffer), "/echolet/audio/new/recording-%Y-%m-%d-%H-%M-%S.wav", &timeInfo);
  return String(buffer);
}

String makeIsoTimestamp() {
  if (!isWallClockValid()) {
    return "1970-01-01T00:00:00Z";
  }

  struct tm timeInfo = {};
  if (!buildLocalTime(timeInfo)) {
    return "1970-01-01T00:00:00Z";
  }

  char dateTimeBuffer[40];
  char offsetBuffer[8];
  strftime(dateTimeBuffer, sizeof(dateTimeBuffer), "%Y-%m-%dT%H:%M:%S", &timeInfo);
  strftime(offsetBuffer, sizeof(offsetBuffer), "%z", &timeInfo);

  String result(dateTimeBuffer);
  result += formatOffsetWithColon(offsetBuffer);
  return result;
}
