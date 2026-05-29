#include "uploader.h"

#include "config.h"

void Uploader::begin() {}

bool Uploader::upload(const String& path) {
  (void)path;
  return kEnableUploadStub;
}
