#pragma once

#include <Arduino.h>

class UploadQueue {
 public:
  static constexpr size_t kMaxItems = 16;

  void begin();
  bool enqueue(const String& path);
  bool hasPending() const;
  String peek() const;
  bool pop();
  size_t size() const;

 private:
  String items_[kMaxItems];
  size_t count_ = 0;
};
