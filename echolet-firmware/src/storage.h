#pragma once

#include <Arduino.h>

class Storage {
 public:
  bool begin();
  bool ensureLayout();
  bool saveRecordingPlaceholder(const String& path);
  bool moveToSent(const String& path);
  bool markFailed(const String& path);
  int usagePercent() const;
};
