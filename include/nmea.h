#pragma once

#include <stddef.h>

struct NmeaStats {
  unsigned long sentences = 0;
  unsigned long checksumErrors = 0;
  unsigned long ggaSentences = 0;
  unsigned long fixQuality = 0;
  unsigned long gstSentences = 0;
  // GST accuracy in meters: horizontal RSS of latitude/longitude, and altitude.
  float horizontalAccuracy = 0;
  float verticalAccuracy = 0;
  unsigned long rmcSentences = 0;
  char utcDateTime[21] = {};
};

// Returns true only when a complete, checksum-valid GGA updates fix quality.
bool consumeNmeaByte(char byte, NmeaStats& stats);
