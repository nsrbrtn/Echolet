#pragma once

#include <Arduino.h>

// Current wiring confirmed by the user:
// - record button: D1 -> GPIO2, active-low to GND
// - secondary button: D2 -> GPIO3, active-low to GND
// - WS2812B data input: D3
// - WS2812B power: 3V3
// - WS2812B ground: GND
constexpr int kButtonPinPrimary = 2;
constexpr int kButtonPinSecondary = 3;
constexpr int kLedPin = D3;
constexpr int kLedCount = 1;
constexpr int kSdCardCsPin = 21;
constexpr int kSdCardSckPin = 7;
constexpr int kSdCardMisoPin = 8;
constexpr int kSdCardMosiPin = 9;
constexpr int kMicClockPin = 42;
constexpr int kMicDataPin = 41;

constexpr bool kButtonActiveLow = true;
constexpr bool kEnableWifiStub = false;
constexpr bool kEnableUploadStub = false;

constexpr const char* kDeviceId = "echolet-dev";
constexpr const char* kWifiSsid = "TP-Link_8260";
constexpr const char* kWifiPassword = "08891990";
constexpr const char* kFirmwareVersion = "0.1.0";
constexpr const char* kUploadToken = "dev-token-change-me";
constexpr const char* kUploadEndpoint = "http://192.168.0.4:8765/api/v1/upload-audio";

constexpr unsigned long kReadyPulseMs = 1000;
constexpr unsigned long kErrorBlinkMs = 300;
constexpr unsigned long kUploadRetryIntervalMs = 30000;
constexpr unsigned long kWifiRetryIntervalMs = 10000;
constexpr unsigned long kPowerButtonHoldMs = 3000;
constexpr unsigned long kWifiConnectTimeoutMs = 10000;
constexpr unsigned long kWifiPollIntervalMs = 250;
constexpr unsigned long kNtpSyncTimeoutMs = 10000;
constexpr unsigned long kNtpPollIntervalMs = 250;
constexpr unsigned long kBootTimeSyncBudgetMs = 12000;
constexpr unsigned long kUploadStreamTimeoutMs = 10000;
constexpr unsigned long kUploadResponseTimeoutMs = 10000;
constexpr const char* kNtpServerPrimary = "pool.ntp.org";
constexpr const char* kNtpServerSecondary = "time.google.com";
constexpr const char* kTimeZonePosix = "MSK-3";
constexpr uint32_t kAudioSampleRate = 16000;
constexpr uint16_t kAudioBitsPerSample = 16;
constexpr uint16_t kAudioChannels = 1;
constexpr size_t kAudioChunkBytes = 1024;
constexpr int kAudioInputGain = 3;
constexpr size_t kAudioDiscardInitialBytes = 2048;
