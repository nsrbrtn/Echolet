#include "audio_recorder.h"

void AudioRecorder::begin() {}

bool AudioRecorder::start() {
  if (recording_) {
    return false;
  }

  recording_ = true;
  currentFilename_ = "/echolet/audio/new/recording-" + String(millis()) + ".wav";
  return true;
}

bool AudioRecorder::stop() {
  if (!recording_) {
    return false;
  }

  recording_ = false;
  return true;
}

bool AudioRecorder::isRecording() const {
  return recording_;
}

const String& AudioRecorder::currentFilename() const {
  return currentFilename_;
}
