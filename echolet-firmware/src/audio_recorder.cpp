#include "audio_recorder.h"

#include <I2S.h>
#include <SD.h>

#include "config.h"

namespace {

constexpr size_t kWavHeaderSize = 44;

}  // namespace

void AudioRecorder::begin() {}

bool AudioRecorder::start() {
  if (recording_) {
    return false;
  }

  currentFilename_ = "/echolet/audio/new/recording-" + String(millis()) + ".wav";
  file_ = SD.open(currentFilename_, FILE_WRITE);
  if (!file_) {
    Serial.print("[recording] failed to open SD file: ");
    Serial.println(currentFilename_);
    return false;
  }

  dataBytesWritten_ = 0;
  if (!writeWavHeader(0)) {
    Serial.println("[recording] failed to write WAV header");
    file_.close();
    return false;
  }

  I2S.end();
  I2S.setAllPins(-1, kMicClockPin, kMicDataPin, -1, -1);
  if (!I2S.begin(PDM_MONO_MODE, kAudioSampleRate, kAudioBitsPerSample)) {
    Serial.println("[recording] failed to initialize I2S microphone");
    file_.close();
    return false;
  }

  recording_ = true;
  return true;
}

bool AudioRecorder::captureStep() {
  if (!recording_ || !file_) {
    return false;
  }

  uint8_t buffer[kAudioChunkBytes];
  const int bytesRead = I2S.read(buffer, sizeof(buffer));
  if (bytesRead < 0) {
    Serial.println("[recording] I2S read failed");
    return false;
  }

  if (bytesRead == 0) {
    return true;
  }

  const size_t bytesWritten = file_.write(buffer, static_cast<size_t>(bytesRead));
  if (bytesWritten != static_cast<size_t>(bytesRead)) {
    Serial.println("[recording] SD write failed");
    return false;
  }

  dataBytesWritten_ += static_cast<uint32_t>(bytesWritten);
  return true;
}

bool AudioRecorder::stop() {
  if (!recording_) {
    return false;
  }

  I2S.end();

  if (!file_) {
    recording_ = false;
    return false;
  }

  if (!writeWavHeader(dataBytesWritten_)) {
    Serial.println("[recording] failed to finalize WAV header");
    file_.close();
    recording_ = false;
    return false;
  }

  file_.flush();
  file_.close();
  recording_ = false;
  return true;
}

bool AudioRecorder::isRecording() const {
  return recording_;
}

const String& AudioRecorder::currentFilename() const {
  return currentFilename_;
}

bool AudioRecorder::writeWavHeader(uint32_t dataSize) {
  if (!file_) {
    return false;
  }

  const uint32_t chunkSize = 36 + dataSize;
  const uint32_t byteRate = kAudioSampleRate * kAudioChannels * (kAudioBitsPerSample / 8);
  const uint16_t blockAlign = kAudioChannels * (kAudioBitsPerSample / 8);

  const uint8_t header[kWavHeaderSize] = {
      'R', 'I', 'F', 'F',
      static_cast<uint8_t>(chunkSize & 0xff),
      static_cast<uint8_t>((chunkSize >> 8) & 0xff),
      static_cast<uint8_t>((chunkSize >> 16) & 0xff),
      static_cast<uint8_t>((chunkSize >> 24) & 0xff),
      'W', 'A', 'V', 'E',
      'f', 'm', 't', ' ',
      16, 0, 0, 0,
      1, 0,
      static_cast<uint8_t>(kAudioChannels), 0,
      static_cast<uint8_t>(kAudioSampleRate & 0xff),
      static_cast<uint8_t>((kAudioSampleRate >> 8) & 0xff),
      static_cast<uint8_t>((kAudioSampleRate >> 16) & 0xff),
      static_cast<uint8_t>((kAudioSampleRate >> 24) & 0xff),
      static_cast<uint8_t>(byteRate & 0xff),
      static_cast<uint8_t>((byteRate >> 8) & 0xff),
      static_cast<uint8_t>((byteRate >> 16) & 0xff),
      static_cast<uint8_t>((byteRate >> 24) & 0xff),
      static_cast<uint8_t>(blockAlign), 0,
      static_cast<uint8_t>(kAudioBitsPerSample), 0,
      'd', 'a', 't', 'a',
      static_cast<uint8_t>(dataSize & 0xff),
      static_cast<uint8_t>((dataSize >> 8) & 0xff),
      static_cast<uint8_t>((dataSize >> 16) & 0xff),
      static_cast<uint8_t>((dataSize >> 24) & 0xff),
  };

  file_.seek(0);
  return file_.write(header, sizeof(header)) == sizeof(header);
}
