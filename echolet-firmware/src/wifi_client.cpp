#include "wifi_client.h"

#include "config.h"

void WifiClient::begin() {
  connected_ = false;
}

bool WifiClient::connect() {
  connected_ = kEnableWifiStub;
  return connected_;
}

bool WifiClient::isConnected() const {
  return connected_;
}
