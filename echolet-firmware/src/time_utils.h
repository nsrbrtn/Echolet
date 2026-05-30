#pragma once

#include <Arduino.h>

enum class TimeReliability {
  kInvalid,
  kRestoredEstimate,
  kFreshNtp,
};

void beginTimekeeping();
void persistCurrentTimeEstimate();
bool syncTimeWithNtp();
bool isWallClockValid();
bool isFallbackIsoTimestamp(const String& value);
TimeReliability getTimeReliability();
const char* timeReliabilityName(TimeReliability reliability);
String makeRecordingFilename();
String makeIsoTimestamp();
