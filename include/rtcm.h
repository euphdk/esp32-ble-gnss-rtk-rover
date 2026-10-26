#pragma once

#include <stddef.h>
#include <stdint.h>

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
