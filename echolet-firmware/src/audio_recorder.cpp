#include "audio_recorder.h"

#include <I2S.h>
#include <SD.h>

#include "config.h"
#include "time_utils.h"

namespace {

constexpr size_t kWavHeaderSize = 44;
constexpr float kDcBlockAlpha = 0.995f;
constexpr esp_i2s::i2s_port_t kI2sPort = esp_i2s::I2S_NUM_0;

}  // namespace

void AudioRecorder::begin() {}

bool AudioRecorder::start() {
  if (recording_) {
    return false;
  }

  currentCreatedAt_ = makeIsoTimestamp();
  currentFilename_ = makeRecordingFilename();
  file_ = SD.open(currentFilename_, FILE_WRITE);
  if (!file_) {
    Serial.print("[recording] failed to open SD file: ");
    Serial.println(currentFilename_);
    return false;
  }

  dataBytesWritten_ = 0;
  bytesDiscarded_ = 0;
  previousInputSample_ = 0;
  previousFilteredSample_ = 0.0f;
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
  size_t bytesRead = 0;
  if (esp_i2s::i2s_read(kI2sPort, buffer, sizeof(buffer), &bytesRead, portMAX_DELAY) != ESP_OK) {
    Serial.println("[recording] I2S read failed");
    return false;
  }

  if (bytesRead == 0) {
    return true;
  }

  size_t writeOffset = 0;
  size_t bytesToProcess = static_cast<size_t>(bytesRead);
  if (bytesDiscarded_ < kAudioDiscardInitialBytes) {
    const size_t bytesLeftToDiscard = kAudioDiscardInitialBytes - bytesDiscarded_;
    const size_t discardNow = bytesToProcess > bytesLeftToDiscard ? bytesLeftToDiscard : bytesToProcess;
    bytesDiscarded_ += discardNow;
    writeOffset = discardNow;
    bytesToProcess -= discardNow;
    if (bytesToProcess == 0) {
      return true;
    }
  }

  int16_t* samples = reinterpret_cast<int16_t*>(buffer + writeOffset);
  const size_t sampleCount = bytesToProcess / sizeof(int16_t);
  for (size_t i = 0; i < sampleCount; ++i) {
    const int16_t input = samples[i];
    const float filtered =
        static_cast<float>(input) - static_cast<float>(previousInputSample_) +
        (kDcBlockAlpha * previousFilteredSample_);
    previousInputSample_ = input;
    previousFilteredSample_ = filtered;

    const int32_t amplified = static_cast<int32_t>(filtered * kAudioInputGain);
    int32_t clipped = amplified;
    if (clipped > 32767) {
      clipped = 32767;
    } else if (clipped < -32768) {
      clipped = -32768;
    }
    samples[i] = static_cast<int16_t>(clipped);
  }

  const size_t bytesWritten = file_.write(buffer + writeOffset, bytesToProcess);
  if (bytesWritten != bytesToProcess) {
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

const String& AudioRecorder::currentCreatedAt() const {
  return currentCreatedAt_;
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
