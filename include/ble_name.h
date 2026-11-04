#pragma once

#include <stdint.h>
#include <stdio.h>

#include "config.h"

constexpr size_t kBleDeviceNameSize = sizeof(config::kDeviceName) + 5;

inline void formatBleDeviceName(char* name, size_t capacity, const uint8_t* mac) {
  snprintf(name, capacity, "%s-%02X%02X", config::kDeviceName,
           static_cast<unsigned int>(mac[4]), static_cast<unsigned int>(mac[5]));
}
