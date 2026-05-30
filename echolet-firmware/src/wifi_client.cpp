#include "wifi_client.h"

#include "config.h"
#include "time_utils.h"
#include <WiFi.h>

void WifiClient::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  WiFi.disconnect(true, true);
  connected_ = false;
  timeSynced_ = false;
}

bool WifiClient::connect() {
  if (kEnableWifiStub) {
    connected_ = true;
    return connected_;
  }

  if (!hasCredentials()) {
    Serial.println("[wifi] missing SSID or password in config.h");
    connected_ = false;
    return false;
  }

  if (WiFi.status() == WL_CONNECTED && WiFi.SSID() == String(kWifiSsid)) {
    connected_ = true;
    Serial.print("[wifi] already connected, ip=");
    Serial.println(WiFi.localIP());
    if (!timeSynced_) {
      timeSynced_ = syncTimeWithNtp();
      if (timeSynced_) {
        Serial.print("[time] NTP synchronized (");
        Serial.print(timeReliabilityName(getTimeReliability()));
        Serial.print("): ");
        Serial.println(makeIsoTimestamp());
      } else {
        Serial.print("[time] NTP sync failed, current reliability=");
        Serial.println(timeReliabilityName(getTimeReliability()));
      }
    }
    return true;
  }

  Serial.print("[wifi] connecting to SSID: ");
  Serial.println(kWifiSsid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(kWifiSsid, kWifiPassword);

  const unsigned long startedAtMs = millis();
  while (millis() - startedAtMs < kWifiConnectTimeoutMs) {
    const wl_status_t status = WiFi.status();
    if (status == WL_CONNECTED) {
      connected_ = true;
      Serial.print("[wifi] connected, ip=");
      Serial.println(WiFi.localIP());
      timeSynced_ = syncTimeWithNtp();
      if (timeSynced_) {
        Serial.print("[time] NTP synchronized (");
        Serial.print(timeReliabilityName(getTimeReliability()));
        Serial.print("): ");
        Serial.println(makeIsoTimestamp());
      } else {
        Serial.print("[time] NTP sync failed, current reliability=");
        Serial.println(timeReliabilityName(getTimeReliability()));
      }
      return true;
    }

    delay(kWifiPollIntervalMs);
  }

  Serial.print("[wifi] connect timeout, status=");
  Serial.println(static_cast<int>(WiFi.status()));
  WiFi.disconnect();
  connected_ = false;
  return connected_;
}

bool WifiClient::isConnected() const {
  return connected_ && WiFi.status() == WL_CONNECTED && WiFi.SSID() == String(kWifiSsid);
}

bool WifiClient::hasCredentials() const {
  return strlen(kWifiSsid) > 0 && strlen(kWifiPassword) > 0 &&
         String(kWifiSsid) != "TODO_WIFI_SSID" &&
         String(kWifiPassword) != "TODO_WIFI_PASSWORD";
}
