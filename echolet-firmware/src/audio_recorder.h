#pragma once

#include <Arduino.h>
#include <FS.h>

#include "time_utils.h"

class AudioRecorder {
 public:
  void begin();
  bool start();
  bool captureStep();
  bool stop();

  bool isRecording() const;
  const String& currentFilename() const;
  const String& currentCreatedAt() const;
  TimeReliability currentTimeReliability() const;

 private:
  bool writeWavHeader(uint32_t dataSize);

  bool recording_ = false;
  String currentFilename_;
  String currentCreatedAt_;
  TimeReliability currentTimeReliability_ = TimeReliability::kInvalid;
  File file_;
  uint32_t dataBytesWritten_ = 0;
  size_t bytesDiscarded_ = 0;
  int16_t previousInputSample_ = 0;
  float previousFilteredSample_ = 0.0f;
};
