#pragma once

#include <Arduino.h>
#include <FS.h>

class AudioRecorder {
 public:
  void begin();
  bool start();
  bool captureStep();
  bool stop();

  bool isRecording() const;
  const String& currentFilename() const;

 private:
  bool writeWavHeader(uint32_t dataSize);

  bool recording_ = false;
  String currentFilename_;
  File file_;
  uint32_t dataBytesWritten_ = 0;
};
