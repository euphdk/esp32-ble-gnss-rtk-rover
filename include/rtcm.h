#pragma once

#include <stddef.h>
#include <stdint.h>

template <size_t Capacity>
class RtcmBuffer {
 public:
  bool enqueue(const uint8_t* data, size_t length) {
    if (length > Capacity - size_) return false;
    for (size_t i = 0; i < length; ++i) {
      bytes_[(head_ + size_ + i) % Capacity] = data[i];
    }
    size_ += length;
    return true;
  }

  size_t drain(size_t limit, uint8_t* output) {
    const size_t count = size_ < limit ? size_ : limit;
    for (size_t i = 0; i < count; ++i) output[i] = bytes_[(head_ + i) % Capacity];
    head_ = (head_ + count) % Capacity;
    size_ -= count;
    return count;
  }

 private:
  uint8_t bytes_[Capacity] = {};
  size_t head_ = 0;
  size_t size_ = 0;
};

// Writes every byte, tolerating short writes. Returns false on a zero-length write.
template <typename Writer>
bool forwardRtcm(const uint8_t* data, size_t length, Writer& writer) {
  size_t offset = 0;
  while (offset < length) {
    const size_t written = writer.write(data + offset, length - offset);
    if (written == 0) return false;
    offset += written;
  }
  return true;
}
