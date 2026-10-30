#pragma once

#include <stdint.h>
#include <stdio.h>

#define GNSS_RECEIVER_ZED_F9P 0
#define GNSS_RECEIVER_LC29H_DA 1

#ifndef GNSS_RECEIVER
#define GNSS_RECEIVER GNSS_RECEIVER_ZED_F9P
#endif

#if GNSS_RECEIVER != GNSS_RECEIVER_ZED_F9P && GNSS_RECEIVER != GNSS_RECEIVER_LC29H_DA
#error "Unsupported GNSS_RECEIVER"
#endif

namespace config {
constexpr bool kLc29hDa = GNSS_RECEIVER == GNSS_RECEIVER_LC29H_DA;
constexpr uint32_t kGnssBaud = kLc29hDa ? 115200 : 38400;
constexpr const char* kReceiverName = kLc29hDa ? "LC29H(DA)" : "ZED-F9P";
}

// Runs before BLE starts. No acknowledgment waits or retries.
template <typename Writer, typename Wait>
bool configureGnss(Writer& uart, Wait wait) {
  if constexpr (!config::kLc29hDa) return true;

  constexpr const char* commands[] = {
      "PQTMCFGRCVRMODE,W,1",
      "PQTMCFGRTK,W,1,1",
      "PQTMCFGPROT,W,1,1,00000005,00000005",
      "PQTMCFGMSGRATE,W,GGA,1",
      "PQTMCFGMSGRATE,W,RMC,1",
      "PQTMCFGMSGRATE,W,GSA,1",
      "PQTMCFGMSGRATE,W,GSV,1",
      "PQTMCFGMSGRATE,W,VTG,0",
      "PQTMCFGMSGRATE,W,GLL,0",
      "PQTMCFGMSGRATE,W,ZDA,0",
  };
  wait(1200);
  for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); ++i) {
    if (i > 0) wait(250);
    const char* body = commands[i];
    uint8_t checksum = 0;
    for (const char* p = body; *p; ++p) checksum ^= static_cast<uint8_t>(*p);
    char frame[64];
    const size_t length = snprintf(frame, sizeof(frame), "$%s*%02X\r\n", body, checksum);
    if (uart.write(reinterpret_cast<const uint8_t*>(frame), length) != length) return false;
  }
  return true;
}
