#pragma once

#include <stddef.h>
#include <stdint.h>

#include "gnss_receiver.h"

// UART1 pins come from the PlatformIO environment build flags
// (esp32-s3: GPIO16/17, esp32-c3: GPIO4/5). Defaults keep the S3 wiring.
#ifndef GNSS_RX_PIN
#define GNSS_RX_PIN 16
#endif

#ifndef GNSS_TX_PIN
#define GNSS_TX_PIN 17
#endif

namespace config {
constexpr int kGnssRxPin = GNSS_RX_PIN;
constexpr int kGnssTxPin = GNSS_TX_PIN;
constexpr uint32_t kConsoleBaud = 115200;
constexpr bool kSerialDebug = true;
constexpr char kDeviceName[] = "GNSS-RTK-Rover";
constexpr char kServiceUuid[] = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
constexpr char kUartRxUuid[] = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";
constexpr char kUartTxUuid[] = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";
constexpr size_t kRtcmChunkSize = 200;
constexpr size_t kRtcmBufferSize = 2048;
}
