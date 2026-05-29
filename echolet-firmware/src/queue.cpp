#include "queue.h"

void UploadQueue::begin() {
  count_ = 0;
}

bool UploadQueue::enqueue(const String& path) {
  if (count_ >= kMaxItems) {
    return false;
  }

  items_[count_] = path;
  ++count_;
  return true;
}

bool UploadQueue::hasPending() const {
  return count_ > 0;
}

String UploadQueue::peek() const {
  if (count_ == 0) {
    return String();
  }

  return items_[0];
}

bool UploadQueue::pop() {
  if (count_ == 0) {
    return false;
  }

  for (size_t i = 1; i < count_; ++i) {
    items_[i - 1] = items_[i];
  }

  items_[count_ - 1] = String();
  --count_;
  return true;
}

size_t UploadQueue::size() const {
  return count_;
}
