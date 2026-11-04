#pragma once

#include <stdint.h>
#include <stdio.h>

constexpr uint32_t kGgaStaleMs = 5000;
constexpr uint32_t kGstStaleMs = 5000;
constexpr uint32_t kRmcStaleMs = 5000;
constexpr uint32_t kBleOverlayMs = 5000;

inline const char* fixStatusLabel(bool haveGga, uint32_t lastGgaMs, uint32_t now,
                                  unsigned long quality, bool correctionsStale = false) {
  if (!haveGga) return "NO GGA";
  if (static_cast<uint32_t>(now - lastGgaMs) >= kGgaStaleMs) return "GGA STALE";
  switch (quality) {
    case 0: return "NO FIX";
    case 1: return "GNSS";
    case 2: return "DGPS";
    case 4: return correctionsStale ? "RTK FIX (!)" : "RTK FIX";
    case 5: return "RTK FLOAT";
    default: return "OTHER FIX";
  }
}

inline bool correctionDataStale(uint32_t lastCorrectionMs, uint32_t now, uint32_t timeoutMs) {
  return static_cast<uint32_t>(now - lastCorrectionMs) >= timeoutMs;
}

inline const char* bleStatusLabel(bool connected) {
  return connected ? "BLE UP" : "BLE DOWN";
}

inline bool gstAccuracyFresh(bool haveGst, uint32_t lastGstMs, uint32_t now) {
  return haveGst && static_cast<uint32_t>(now - lastGstMs) < kGstStaleMs;
}

inline const char* utcTimeLabel(bool haveRmc, uint32_t lastRmcMs, uint32_t now,
                                const char* utcDateTime) {
  if (!haveRmc || static_cast<uint32_t>(now - lastRmcMs) >= kRmcStaleMs) return "TIME N/A";
  return utcDateTime;
}

inline void formatAccuracy(char* output, size_t capacity, float meters) {
  if (meters >= 1) {
    snprintf(output, capacity, "%.1fm", meters);
  } else if (meters >= 0.1f) {
    snprintf(output, capacity, "%.0fcm", meters * 100);
  } else {
    snprintf(output, capacity, "%.1fcm", meters * 100);
  }
}

class BleStatusOverlay {
 public:
  const char* update(bool connected, uint32_t now) {
    if (connected != connected_) {
      connected_ = connected;
      haveEvent_ = true;
      eventMs_ = now;
    }
    if (!haveEvent_ || static_cast<uint32_t>(now - eventMs_) >= kBleOverlayMs) return nullptr;
    return bleStatusLabel(connected_);
  }

 private:
  bool connected_ = false;
  bool haveEvent_ = false;
  uint32_t eventMs_ = 0;
};
