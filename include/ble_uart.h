#pragma once

#include <stddef.h>
#include <stdint.h>

class BleUartBatch {
 public:
  // The default ATT MTU is 23 bytes, leaving 20 bytes for notification data.
  static constexpr size_t kPayloadSize = 20;

  template <typename Sender>
  void append(uint8_t byte, Sender& send) {
    bytes_[size_++] = byte;
    if (size_ == sizeof(bytes_)) flush(send);
  }

  template <typename Sender>
  void flush(Sender& send) {
    if (size_ == 0) return;
    send(bytes_, size_);
    clear();
  }

  void clear() { size_ = 0; }

 private:
  uint8_t bytes_[kPayloadSize];
  size_t size_ = 0;
};
