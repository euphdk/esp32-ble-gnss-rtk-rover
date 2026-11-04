#pragma once

#include <stddef.h>
#include <stdint.h>

class BleUartBatch {
 public:
  // The default ATT MTU is 23 bytes, leaving 20 bytes for notification data.
  static constexpr size_t kPayloadSize = 20;

  template <typename Sender>
  bool append(uint8_t byte, Sender& send) {
    if (full() && !flush(send)) return false;
    bytes_[size_++] = byte;
    if (size_ == sizeof(bytes_)) flush(send);
    return true;
  }

  template <typename Sender>
  bool flush(Sender& send) {
    if (size_ == 0) return true;
    if (!send(bytes_, size_)) return false;
    clear();
    return true;
  }

  bool full() const { return size_ == sizeof(bytes_); }

  void clear() { size_ = 0; }

 private:
  uint8_t bytes_[kPayloadSize];
  size_t size_ = 0;
};
