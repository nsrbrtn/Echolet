#include "uploader.h"

#include <FS.h>
#include <SD.h>
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

String isoTimestampFromMillis(unsigned long nowMs) {
  unsigned long totalSeconds = nowMs / 1000;
  const unsigned long seconds = totalSeconds % 60;
  totalSeconds /= 60;
  const unsigned long minutes = totalSeconds % 60;
  totalSeconds /= 60;
  const unsigned long hours = totalSeconds % 24;

  char buffer[32];
  snprintf(buffer, sizeof(buffer), "1970-01-01T%02lu:%02lu:%02luZ", hours, minutes, seconds);
  return String(buffer);
}

bool parseHttpUrl(const String& url, String& host, uint16_t& port, String& path) {
  constexpr const char* kHttpPrefix = "http://";
  if (!url.startsWith(kHttpPrefix)) {
    return false;
  }

  String remainder = url.substring(strlen(kHttpPrefix));
  const int slashIndex = remainder.indexOf('/');
  String hostPort = slashIndex >= 0 ? remainder.substring(0, slashIndex) : remainder;
  path = slashIndex >= 0 ? remainder.substring(slashIndex) : "/";

  const int colonIndex = hostPort.indexOf(':');
  if (colonIndex >= 0) {
    host = hostPort.substring(0, colonIndex);
    port = static_cast<uint16_t>(hostPort.substring(colonIndex + 1).toInt());
  } else {
    host = hostPort;
    port = 80;
  }

  return host.length() > 0 && path.length() > 0 && port > 0;
}

bool streamFile(WiFiClient& client, File& file) {
  uint8_t buffer[512];
  while (true) {
    const size_t bytesRead = file.read(buffer, sizeof(buffer));
    if (bytesRead == 0) {
      break;
    }
    if (client.write(buffer, bytesRead) != bytesRead) {
      return false;
    }
  }
  return true;
}

int readStatusCode(WiFiClient& client) {
  const String statusLine = client.readStringUntil('\n');
  if (!statusLine.startsWith("HTTP/1.")) {
    return -1;
  }

  const int firstSpace = statusLine.indexOf(' ');
  if (firstSpace < 0) {
    return -1;
  }
  return statusLine.substring(firstSpace + 1).toInt();
}

}  // namespace

void Uploader::begin() {}

bool Uploader::upload(const String& path) {
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

  String host;
  String requestPath;
  uint16_t port = 80;
  if (!parseHttpUrl(String(kUploadEndpoint), host, port, requestPath)) {
    Serial.println("[upload] invalid endpoint URL");
    file.close();
    return false;
  }

  const String boundary = "----echoletBoundary7MA4YWxkTrZu0gW";
  const String filename = filenameFromPath(path);
  const String createdAt = isoTimestampFromMillis(millis());
  const String battery = "unknown";

  String filePartHeader;
  filePartHeader.reserve(192);
  filePartHeader += "--" + boundary + "\r\n";
  filePartHeader += "Content-Disposition: form-data; name=\"file\"; filename=\"" + filename + "\"\r\n";
  filePartHeader += "Content-Type: audio/wav\r\n\r\n";

  String formFields;
  formFields.reserve(320);
  formFields += "\r\n--" + boundary + "\r\n";
  formFields += "Content-Disposition: form-data; name=\"device_id\"\r\n\r\n";
  formFields += String(kDeviceId) + "\r\n";
  formFields += "--" + boundary + "\r\n";
  formFields += "Content-Disposition: form-data; name=\"created_at\"\r\n\r\n";
  formFields += createdAt + "\r\n";
  formFields += "--" + boundary + "\r\n";
  formFields += "Content-Disposition: form-data; name=\"battery\"\r\n\r\n";
  formFields += battery + "\r\n";
  formFields += "--" + boundary + "\r\n";
  formFields += "Content-Disposition: form-data; name=\"firmware_version\"\r\n\r\n";
  formFields += String(kFirmwareVersion) + "\r\n";

  const String tail = "\r\n--" + boundary + "--\r\n";
  const size_t contentLength = filePartHeader.length() + file.size() + formFields.length() + tail.length();

  WiFiClient client;
  client.setTimeout(10000);
  if (!client.connect(host.c_str(), port)) {
    Serial.print("[upload] failed to connect to ");
    Serial.print(host);
    Serial.print(":");
    Serial.println(port);
    file.close();
    return false;
  }

  client.print(String("POST ") + requestPath + " HTTP/1.1\r\n");
  client.print(String("Host: ") + host + ":" + port + "\r\n");
  client.print("Connection: close\r\n");
  client.print(String("Authorization: Bearer ") + kUploadToken + "\r\n");
  client.print(String("Content-Type: multipart/form-data; boundary=") + boundary + "\r\n");
  client.print(String("Content-Length: ") + contentLength + "\r\n\r\n");

  client.print(filePartHeader);
  const bool streamed = streamFile(client, file);
  file.close();
  if (!streamed) {
    Serial.println("[upload] failed while streaming file");
    client.stop();
    return false;
  }

  client.print(formFields);
  client.print(tail);

  while (client.connected() && !client.available()) {
    delay(10);
  }

  const int statusCode = readStatusCode(client);
  String responseBody;
  while (client.available()) {
    responseBody += client.readString();
  }
  client.stop();

  Serial.print("[upload] response status: ");
  Serial.println(statusCode);
  if (responseBody.length() > 0) {
    Serial.print("[upload] response body: ");
    Serial.println(responseBody);
  }

  return statusCode >= 200 && statusCode < 300;
}
