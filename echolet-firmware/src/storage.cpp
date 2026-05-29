#include "storage.h"

bool Storage::begin() {
  return true;
}

bool Storage::ensureLayout() {
  return true;
}

bool Storage::saveRecordingPlaceholder(const String& path) {
  (void)path;
  return true;
}

bool Storage::moveToSent(const String& path) {
  (void)path;
  return true;
}

bool Storage::markFailed(const String& path) {
  (void)path;
  return true;
}

int Storage::usagePercent() const {
  return 0;
}
