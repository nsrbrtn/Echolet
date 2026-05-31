#include "uploader.h"

#include <FS.h>
#include <HTTPClient.h>
#include <SD.h>
#include <limits.h>
#include <WiFiClient.h>

#include "config.h"

namespace {

String filenameFromPath(const String& path) {
  const int slashIndex = path.lastIndexOf('/');
  if (slashIndex < 0) {
    return path;
  }
  return path.substring(slashIndex + 1);
}

class MultipartUploadStream : public Stream {
 public:
  MultipartUploadStream(File& file, const String& prefix, const String& suffix)
      : file_(file), prefix_(prefix), suffix_(suffix) {}

  int available() override {
    const size_t remaining = remainingBytes();
    if (remaining > static_cast<size_t>(INT_MAX)) {
      return INT_MAX;
    }
    return static_cast<int>(remaining);
  }

  int read() override {
    uint8_t value = 0;
    return readBytes(&value, 1) == 1 ? value : -1;
  }

  int peek() override {
    if (prefixOffset_ < prefix_.length()) {
      return static_cast<uint8_t>(prefix_.charAt(prefixOffset_));
    }

    if (file_.available()) {
      return file_.peek();
    }

    if (suffixOffset_ < suffix_.length()) {
      return static_cast<uint8_t>(suffix_.charAt(suffixOffset_));
    }

    return -1;
  }

  void flush() override {}

  size_t write(uint8_t) override {
    return 0;
  }

  size_t readBytes(uint8_t* buffer, size_t length) override {
    if (buffer == nullptr || length == 0) {
      return 0;
    }

    size_t totalRead = 0;

    while (totalRead < length) {
      if (prefixOffset_ < prefix_.length()) {
        const size_t chunkSize = min(length - totalRead, prefix_.length() - prefixOffset_);
        memcpy(buffer + totalRead, prefix_.c_str() + prefixOffset_, chunkSize);
        prefixOffset_ += chunkSize;
        totalRead += chunkSize;
        continue;
      }

      if (file_.available()) {
        const size_t bytesRead = file_.read(buffer + totalRead, length - totalRead);
        if (bytesRead == 0) {
          break;
        }
        totalRead += bytesRead;
        continue;
      }

      if (suffixOffset_ < suffix_.length()) {
        const size_t chunkSize = min(length - totalRead, suffix_.length() - suffixOffset_);
        memcpy(buffer + totalRead, suffix_.c_str() + suffixOffset_, chunkSize);
        suffixOffset_ += chunkSize;
        totalRead += chunkSize;
        continue;
      }

      break;
    }

    return totalRead;
  }

 private:
  size_t remainingBytes() const {
    return (prefix_.length() - prefixOffset_) + file_.available() + (suffix_.length() - suffixOffset_);
  }

  File& file_;
  const String& prefix_;
  const String& suffix_;
  size_t prefixOffset_ = 0;
  size_t suffixOffset_ = 0;
};

}  // namespace

void Uploader::begin() {}

bool Uploader::upload(const String& path, const String& createdAt) {
  if (kEnableUploadStub) {
    return true;
  }

  File file = SD.open(path, FILE_READ);
  if (!file) {
    Serial.print("[upload] missing SD file: ");
    Serial.println(path);
    return false;
  }

  file.seek(0);
  Serial.print("[upload] local file size: ");
  Serial.println(file.size());

  const String boundary = "----echoletBoundary7MA4YWxkTrZu0gW";
  const String filename = filenameFromPath(path);
  const String battery = "unknown";

  String prefix;
  prefix.reserve(192);
  prefix += "--" + boundary + "\r\n";
  prefix += "Content-Disposition: form-data; name=\"file\"; filename=\"" + filename + "\"\r\n";
  prefix += "Content-Type: audio/wav\r\n\r\n";

  String suffix;
  suffix.reserve(384);
  suffix += "\r\n--" + boundary + "\r\n";
  suffix += "Content-Disposition: form-data; name=\"device_id\"\r\n\r\n";
  suffix += String(kDeviceId) + "\r\n";
  suffix += "--" + boundary + "\r\n";
  suffix += "Content-Disposition: form-data; name=\"created_at\"\r\n\r\n";
  suffix += createdAt + "\r\n";
  suffix += "--" + boundary + "\r\n";
  suffix += "Content-Disposition: form-data; name=\"battery\"\r\n\r\n";
  suffix += battery + "\r\n";
  suffix += "--" + boundary + "\r\n";
  suffix += "Content-Disposition: form-data; name=\"firmware_version\"\r\n\r\n";
  suffix += String(kFirmwareVersion) + "\r\n";
  suffix += "--" + boundary + "--\r\n";

  const size_t contentLength = prefix.length() + file.size() + suffix.length();
  MultipartUploadStream bodyStream(file, prefix, suffix);

  WiFiClient client;
  HTTPClient http;
  http.setConnectTimeout(static_cast<int32_t>(kWifiConnectTimeoutMs));
  http.setTimeout(static_cast<uint16_t>(kUploadResponseTimeoutMs));
  http.setReuse(false);
  http.useHTTP10(true);

  if (!http.begin(client, String(kUploadEndpoint))) {
    Serial.println("[upload] failed to initialize HTTP client");
    file.close();
    return false;
  }

  http.addHeader("Authorization", String("Bearer ") + kUploadToken);
  http.addHeader("Content-Type", String("multipart/form-data; boundary=") + boundary);
  http.addHeader("Connection", "close");

  Serial.println("[upload] sending multipart request via HTTPClient");
  const int statusCode = http.sendRequest("POST", &bodyStream, contentLength);

  if (statusCode <= 0) {
    Serial.print("[upload] HTTPClient error: ");
    Serial.println(HTTPClient::errorToString(statusCode));
    http.end();
    file.close();
    return false;
  }

  String responseBody = http.getString();
  Serial.print("[upload] response status: ");
  Serial.println(statusCode);
  if (responseBody.length() > 0) {
    Serial.print("[upload] response body: ");
    Serial.println(responseBody);
  }

  http.end();
  file.close();
  return statusCode >= 200 && statusCode < 300;
}
