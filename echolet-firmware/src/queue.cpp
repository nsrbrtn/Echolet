#include "queue.h"

#include <FS.h>
#include <SD.h>

namespace {

constexpr const char* kQueueFilePath = "/echolet/queue.json";

String escapeJson(const String& value) {
  String escaped;
  escaped.reserve(value.length() + 8);

  for (size_t i = 0; i < value.length(); ++i) {
    const char ch = value.charAt(i);
    if (ch == '\\' || ch == '"') {
      escaped += '\\';
    }
    escaped += ch;
  }

  return escaped;
}

bool extractJsonValue(const String& source, const char* key, String& value) {
  const String pattern = String("\"") + key + "\":\"";
  const int keyStart = source.indexOf(pattern);
  if (keyStart < 0) {
    return false;
  }

  const int valueStart = keyStart + pattern.length();
  String parsed;
  bool escaping = false;
  for (int i = valueStart; i < source.length(); ++i) {
    const char ch = source.charAt(i);
    if (escaping) {
      parsed += ch;
      escaping = false;
      continue;
    }

    if (ch == '\\') {
      escaping = true;
      continue;
    }

    if (ch == '"') {
      value = parsed;
      return true;
    }

    parsed += ch;
  }

  return false;
}

}  // namespace

bool UploadQueue::begin() {
  count_ = 0;
  return loadFromDisk();
}

bool UploadQueue::enqueue(const String& path, const String& createdAt) {
  if (path.length() == 0 || createdAt.length() == 0) {
    return false;
  }

  if (containsPath(path)) {
    return true;
  }

  if (count_ >= kMaxItems) {
    return false;
  }

  const size_t insertIndex = count_;
  items_[insertIndex].path = path;
  items_[insertIndex].createdAt = createdAt;
  ++count_;
  if (saveToDisk()) {
    return true;
  }

  items_[insertIndex] = UploadQueueItem{};
  --count_;
  return false;
}

bool UploadQueue::hasPending() const {
  return count_ > 0;
}

bool UploadQueue::peek(UploadQueueItem& item) const {
  if (count_ == 0) {
    return false;
  }

  item = items_[0];
  return true;
}

bool UploadQueue::pop() {
  if (count_ == 0) {
    return false;
  }

  UploadQueueItem previousItems[kMaxItems];
  const size_t previousCount = count_;
  for (size_t i = 0; i < previousCount; ++i) {
    previousItems[i] = items_[i];
  }

  for (size_t i = 1; i < count_; ++i) {
    items_[i - 1] = items_[i];
  }

  items_[count_ - 1] = UploadQueueItem{};
  --count_;
  if (saveToDisk()) {
    return true;
  }

  for (size_t i = 0; i < previousCount; ++i) {
    items_[i] = previousItems[i];
  }
  count_ = previousCount;
  return false;
}

bool UploadQueue::containsPath(const String& path) const {
  if (path.length() == 0) {
    return false;
  }

  for (size_t i = 0; i < count_; ++i) {
    if (items_[i].path == path) {
      return true;
    }
  }

  return false;
}

size_t UploadQueue::size() const {
  return count_;
}

bool UploadQueue::loadFromDisk() {
  File file = SD.open(kQueueFilePath, FILE_READ);
  if (!file) {
    Serial.println("[queue] failed to open queue file for read");
    return false;
  }

  const String content = file.readString();
  file.close();

  count_ = 0;
  int searchFrom = 0;
  while (count_ < kMaxItems) {
    const int objectStart = content.indexOf('{', searchFrom);
    if (objectStart < 0) {
      break;
    }

    const int objectEnd = content.indexOf('}', objectStart);
    if (objectEnd < 0) {
      Serial.println("[queue] malformed queue file");
      count_ = 0;
      return saveToDisk();
    }

    const String object = content.substring(objectStart, objectEnd + 1);
    String path;
    String createdAt;
    if (extractJsonValue(object, "path", path) &&
        extractJsonValue(object, "created_at", createdAt) && path.length() > 0 &&
        createdAt.length() > 0) {
      items_[count_].path = path;
      items_[count_].createdAt = createdAt;
      ++count_;
    }

    searchFrom = objectEnd + 1;
  }

  Serial.print("[queue] restored items: ");
  Serial.println(count_);
  return true;
}

bool UploadQueue::saveToDisk() const {
  if (SD.exists(kQueueFilePath) && !SD.remove(kQueueFilePath)) {
    Serial.println("[queue] failed to replace queue file");
    return false;
  }

  File file = SD.open(kQueueFilePath, FILE_WRITE);
  if (!file) {
    Serial.println("[queue] failed to open queue file for write");
    return false;
  }

  file.print("[\n");
  for (size_t i = 0; i < count_; ++i) {
    file.print("  {\"path\":\"");
    file.print(escapeJson(items_[i].path));
    file.print("\",\"created_at\":\"");
    file.print(escapeJson(items_[i].createdAt));
    file.print("\"}");
    if (i + 1 < count_) {
      file.print(",");
    }
    file.print("\n");
  }
  file.print("]\n");
  const bool ok = !file.getWriteError();
  file.close();
  return ok;
}
