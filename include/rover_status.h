#pragma once

#include <stdint.h>

constexpr uint32_t kGgaStaleMs = 5000;

inline const char* fixStatusLabel(bool haveGga, uint32_t lastGgaMs, uint32_t now,
                                  unsigned long quality) {
  if (!haveGga) return "NO GGA";
  if (static_cast<uint32_t>(now - lastGgaMs) >= kGgaStaleMs) return "GGA STALE";
  switch (quality) {
    case 0: return "NO FIX";
    case 1: return "GNSS";
    case 2: return "DGPS";
    case 4: return "RTK FIX";
    case 5: return "RTK FLOAT";
    default: return "OTHER FIX";
  }
}

inline const char* bleStatusLabel(bool connected) {
  return connected ? "BLE UP" : "BLE DOWN";
}
