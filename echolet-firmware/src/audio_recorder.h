#pragma once

#include <Arduino.h>

class AudioRecorder {
 public:
  void begin();
  bool start();
  bool stop();

  bool isRecording() const;
  const String& currentFilename() const;

 private:
  bool recording_ = false;
  String currentFilename_;
};
