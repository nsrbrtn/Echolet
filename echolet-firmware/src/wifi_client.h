#pragma once

#include <Arduino.h>

class WifiClient {
 public:
  void begin();
  bool connect();
  bool isConnected() const;

 private:
  bool connected_ = false;
};
