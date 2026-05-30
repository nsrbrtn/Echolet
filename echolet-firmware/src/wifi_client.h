#pragma once

#include <Arduino.h>

class WifiClient {
 public:
  void begin();
 bool connect();
  bool isConnected() const;

 private:
  bool hasCredentials() const;
  bool timeSynced_ = false;
  bool connected_ = false;
};
