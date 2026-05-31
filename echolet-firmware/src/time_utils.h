#pragma once

#include <Arduino.h>

enum class TimeReliability {
  kInvalid,
  kRestoredEstimate,
  kFreshNtp,
};

enum class NtpSyncStatus {
  kIdle,
  kInProgress,
  kSucceeded,
  kTimedOut,
};

void beginTimekeeping();
void persistCurrentTimeEstimate();
bool syncTimeWithNtp();
void beginNtpSync();
NtpSyncStatus pollNtpSync();
bool isWallClockValid();
bool isFallbackIsoTimestamp(const String& value);
TimeReliability getTimeReliability();
const char* timeReliabilityName(TimeReliability reliability);
const char* ntpSyncStatusName(NtpSyncStatus status);
String makeRecordingFilename();
String makeIsoTimestamp();
