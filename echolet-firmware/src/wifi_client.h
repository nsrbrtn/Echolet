#pragma once

#include <Arduino.h>

enum class WifiReadyStatus {
  kInProgress,
  kReady,
  kFailed,
};

class WifiClient {
 public:
  void begin();
  WifiReadyStatus ensureReady();
  bool isConnected() const;

 private:
  enum class WorkflowState {
    kIdle,
    kConnecting,
    kSyncingTime,
    kReady,
    kFailed,
  };

  void startConnectionAttempt();
  bool hasCredentials() const;

  WorkflowState workflowState_ = WorkflowState::kIdle;
  unsigned long workflowStartedAtMs_ = 0;
  bool timeSynced_ = false;
  bool connected_ = false;
};
