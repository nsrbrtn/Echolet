#pragma once

#include <Arduino.h>

class Uploader {
 public:
  void begin();
  bool upload(const String& path);
};
