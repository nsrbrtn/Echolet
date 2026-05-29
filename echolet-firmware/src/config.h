#pragma once

// Temporary pin mapping for scaffolding only.
// TODO: replace these values after the actual board wiring is confirmed.

constexpr int kButtonPinPrimary = 2;
constexpr int kButtonPinSecondary = 3;
constexpr int kLedPinRed = 4;
constexpr int kLedPinGreen = 5;
constexpr int kLedPinBlue = 6;

constexpr bool kButtonActiveLow = true;
constexpr bool kEnableWifiStub = false;
constexpr bool kEnableUploadStub = false;

constexpr const char* kDeviceId = "echolet-dev";
constexpr const char* kWifiSsid = "TODO_WIFI_SSID";
constexpr const char* kWifiPassword = "TODO_WIFI_PASSWORD";
constexpr const char* kUploadToken = "TODO_BEARER_TOKEN";
constexpr const char* kUploadEndpoint = "http://127.0.0.1:8000/api/v1/upload-audio";

constexpr unsigned long kReadyPulseMs = 1000;
constexpr unsigned long kErrorBlinkMs = 300;
