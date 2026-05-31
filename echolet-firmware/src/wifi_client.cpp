#include "wifi_client.h"

#include <WiFi.h>

#include "config.h"
#include "time_utils.h"

void WifiClient::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  WiFi.disconnect(true, true);
  workflowState_ = WorkflowState::kIdle;
  workflowStartedAtMs_ = 0;
  connected_ = false;
  timeSynced_ = false;
}

WifiReadyStatus WifiClient::ensureReady() {
  if (kEnableWifiStub) {
    connected_ = true;
    timeSynced_ = true;
    workflowState_ = WorkflowState::kReady;
    return WifiReadyStatus::kReady;
  }

  if (!hasCredentials()) {
    Serial.println("[wifi] missing SSID or password in config.h");
    connected_ = false;
    workflowState_ = WorkflowState::kFailed;
    return WifiReadyStatus::kFailed;
  }

  const bool isTargetConnected =
      WiFi.status() == WL_CONNECTED && WiFi.SSID() == String(kWifiSsid);

  if (!isTargetConnected) {
    connected_ = false;
    if (workflowState_ != WorkflowState::kConnecting) {
      startConnectionAttempt();
      return WifiReadyStatus::kInProgress;
    }

    if (millis() - workflowStartedAtMs_ >= kWifiConnectTimeoutMs) {
      Serial.print("[wifi] connect timeout, status=");
      Serial.println(static_cast<int>(WiFi.status()));
      workflowState_ = WorkflowState::kFailed;
      return WifiReadyStatus::kFailed;
    }

    return WifiReadyStatus::kInProgress;
  }

  connected_ = true;

  if (workflowState_ == WorkflowState::kConnecting || workflowState_ == WorkflowState::kIdle ||
      workflowState_ == WorkflowState::kFailed) {
    Serial.print("[wifi] connected, ip=");
    Serial.println(WiFi.localIP());
    workflowState_ = WorkflowState::kSyncingTime;
    workflowStartedAtMs_ = millis();
  }

  if (timeSynced_) {
    workflowState_ = WorkflowState::kReady;
    return WifiReadyStatus::kReady;
  }

  if (workflowState_ != WorkflowState::kSyncingTime) {
    workflowState_ = WorkflowState::kSyncingTime;
    workflowStartedAtMs_ = millis();
  }

  beginNtpSync();
  const NtpSyncStatus ntpStatus = pollNtpSync();
  if (ntpStatus == NtpSyncStatus::kSucceeded) {
    timeSynced_ = true;
    workflowState_ = WorkflowState::kReady;
    Serial.print("[time] NTP synchronized (");
    Serial.print(timeReliabilityName(getTimeReliability()));
    Serial.print("): ");
    Serial.println(makeIsoTimestamp());
    return WifiReadyStatus::kReady;
  }

  if (ntpStatus == NtpSyncStatus::kTimedOut) {
    Serial.print("[time] NTP sync ");
    Serial.print(ntpSyncStatusName(ntpStatus));
    Serial.print(", current reliability=");
    Serial.println(timeReliabilityName(getTimeReliability()));
    workflowState_ = WorkflowState::kReady;
    return WifiReadyStatus::kReady;
  }

  return WifiReadyStatus::kInProgress;
}

bool WifiClient::isConnected() const {
  return connected_ && WiFi.status() == WL_CONNECTED && WiFi.SSID() == String(kWifiSsid);
}

void WifiClient::startConnectionAttempt() {
  Serial.print("[wifi] connecting to SSID: ");
  Serial.println(kWifiSsid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(kWifiSsid, kWifiPassword);
  workflowState_ = WorkflowState::kConnecting;
  workflowStartedAtMs_ = millis();
}

bool WifiClient::hasCredentials() const {
  return strlen(kWifiSsid) > 0 && strlen(kWifiPassword) > 0 &&
         String(kWifiSsid) != "TODO_WIFI_SSID" &&
         String(kWifiPassword) != "TODO_WIFI_PASSWORD";
}
