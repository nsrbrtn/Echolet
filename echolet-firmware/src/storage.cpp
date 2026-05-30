#include "storage.h"

#include <FS.h>
#include <SD.h>
#include <SPI.h>

#include "config.h"

namespace {

SPIClass gSdSpi(FSPI);

constexpr const char* kRootDir = "/echolet";
constexpr const char* kAudioDir = "/echolet/audio";
constexpr const char* kAudioNewDir = "/echolet/audio/new";
constexpr const char* kAudioSentDir = "/echolet/audio/sent";
constexpr const char* kAudioFailedDir = "/echolet/audio/failed";
constexpr const char* kMetaDir = "/echolet/meta";
constexpr const char* kLogsDir = "/echolet/logs";
constexpr const char* kQueueFile = "/echolet/queue.json";
constexpr const char* kDeviceLogFile = "/echolet/logs/device.log";

bool ensureDir(const char* path) {
  if (SD.exists(path)) {
    return true;
  }
  return SD.mkdir(path);
}

bool ensureFile(const char* path, const char* initialContent = "") {
  if (SD.exists(path)) {
    return true;
  }

  File file = SD.open(path, FILE_WRITE);
  if (!file) {
    return false;
  }

  const bool ok = file.print(initialContent) > 0 || strlen(initialContent) == 0;
  file.close();
  return ok;
}

bool writeSilenceWav(File& file) {
  constexpr uint32_t kSampleRate = 16000;
  constexpr uint16_t kBitsPerSample = 16;
  constexpr uint16_t kChannels = 1;
  constexpr uint32_t kDurationMs = 500;
  constexpr uint32_t kSampleCount = (kSampleRate * kDurationMs) / 1000;
  constexpr uint32_t kDataSize = kSampleCount * kChannels * (kBitsPerSample / 8);
  constexpr uint32_t kChunkSize = 36 + kDataSize;
  constexpr uint32_t kByteRate = kSampleRate * kChannels * (kBitsPerSample / 8);
  constexpr uint16_t kBlockAlign = kChannels * (kBitsPerSample / 8);

  const uint8_t header[44] = {
      'R', 'I', 'F', 'F',
      static_cast<uint8_t>(kChunkSize & 0xff),
      static_cast<uint8_t>((kChunkSize >> 8) & 0xff),
      static_cast<uint8_t>((kChunkSize >> 16) & 0xff),
      static_cast<uint8_t>((kChunkSize >> 24) & 0xff),
      'W', 'A', 'V', 'E',
      'f', 'm', 't', ' ',
      16, 0, 0, 0,
      1, 0,
      static_cast<uint8_t>(kChannels), 0,
      static_cast<uint8_t>(kSampleRate & 0xff),
      static_cast<uint8_t>((kSampleRate >> 8) & 0xff),
      static_cast<uint8_t>((kSampleRate >> 16) & 0xff),
      static_cast<uint8_t>((kSampleRate >> 24) & 0xff),
      static_cast<uint8_t>(kByteRate & 0xff),
      static_cast<uint8_t>((kByteRate >> 8) & 0xff),
      static_cast<uint8_t>((kByteRate >> 16) & 0xff),
      static_cast<uint8_t>((kByteRate >> 24) & 0xff),
      static_cast<uint8_t>(kBlockAlign), 0,
      static_cast<uint8_t>(kBitsPerSample), 0,
      'd', 'a', 't', 'a',
      static_cast<uint8_t>(kDataSize & 0xff),
      static_cast<uint8_t>((kDataSize >> 8) & 0xff),
      static_cast<uint8_t>((kDataSize >> 16) & 0xff),
      static_cast<uint8_t>((kDataSize >> 24) & 0xff),
  };

  if (file.write(header, sizeof(header)) != sizeof(header)) {
    return false;
  }

  uint8_t silence[256] = {};
  uint32_t remaining = kDataSize;
  while (remaining > 0) {
    const size_t chunkSize = remaining > sizeof(silence) ? sizeof(silence) : remaining;
    if (file.write(silence, chunkSize) != chunkSize) {
      return false;
    }
    remaining -= chunkSize;
  }

  return true;
}

}  // namespace

bool Storage::begin() {
  gSdSpi.begin(kSdCardSckPin, kSdCardMisoPin, kSdCardMosiPin, kSdCardCsPin);
  const bool mounted = SD.begin(kSdCardCsPin, gSdSpi);
  if (!mounted) {
    Serial.println("[storage] SD mount failed");
    return false;
  }

  const uint64_t cardSizeMb = SD.cardSize() / (1024ULL * 1024ULL);
  Serial.print("[storage] SD mounted, size MB: ");
  Serial.println(static_cast<unsigned long>(cardSizeMb));
  return true;
}

bool Storage::ensureLayout() {
  if (!ensureDir(kRootDir) || !ensureDir(kAudioDir) || !ensureDir(kAudioNewDir) ||
      !ensureDir(kAudioSentDir) || !ensureDir(kAudioFailedDir) || !ensureDir(kMetaDir) ||
      !ensureDir(kLogsDir)) {
    Serial.println("[storage] failed to create directory layout");
    return false;
  }

  if (!ensureFile(kQueueFile, "[]\n") || !ensureFile(kDeviceLogFile, "")) {
    Serial.println("[storage] failed to create support files");
    return false;
  }

  return true;
}

bool Storage::saveRecordingPlaceholder(const String& path) {
  File file = SD.open(path, FILE_WRITE);
  if (!file) {
    Serial.print("[storage] failed to open for write: ");
    Serial.println(path);
    return false;
  }

  const bool ok = writeSilenceWav(file);
  file.close();
  return ok;
}

bool Storage::moveToSent(const String& path) {
  String toPath = path;
  toPath.replace("/audio/new/", "/audio/sent/");
  return SD.rename(path, toPath);
}

bool Storage::markFailed(const String& path) {
  String toPath = path;
  toPath.replace("/audio/new/", "/audio/failed/");
  return SD.rename(path, toPath);
}

int Storage::usagePercent() const {
  const uint64_t totalBytes = SD.totalBytes();
  if (totalBytes == 0) {
    return 0;
  }

  const uint64_t usedBytes = SD.usedBytes();
  return static_cast<int>((usedBytes * 100ULL) / totalBytes);
}
