#pragma once

#include <Arduino.h>

namespace config {
constexpr int kGnssRxPin = 16;
constexpr int kGnssTxPin = 17;
constexpr uint32_t kGnssBaud = 115200;
constexpr uint32_t kConsoleBaud = 115200;
constexpr char kDeviceName[] = "GNSS-RTK-Rover";
constexpr char kServiceUuid[] = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
constexpr char kUartRxUuid[] = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";
constexpr char kUartTxUuid[] = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";
constexpr size_t kRtcmChunkSize = 200;
constexpr size_t kRtcmBufferSize = 2048;
}
