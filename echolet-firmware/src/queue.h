#pragma once

#include <Arduino.h>

struct UploadQueueItem {
  String path;
  String createdAt;
};

class UploadQueue {
 public:
  static constexpr size_t kMaxItems = 16;

  bool begin();
  bool enqueue(const String& path, const String& createdAt);
  bool hasPending() const;
  bool peek(UploadQueueItem& item) const;
  bool pop();
  bool containsPath(const String& path) const;
  size_t size() const;

 private:
  bool loadFromDisk();
  bool saveToDisk() const;

  UploadQueueItem items_[kMaxItems];
  size_t count_ = 0;
};
