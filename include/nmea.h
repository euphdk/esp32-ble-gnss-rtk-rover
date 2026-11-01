#pragma once

#include <stddef.h>

struct NmeaStats {
  unsigned long sentences = 0;
  unsigned long checksumErrors = 0;
  unsigned long ggaSentences = 0;
  unsigned long fixQuality = 0;
};

// Returns true only when a complete, checksum-valid GGA updates fix quality.
bool consumeNmeaByte(char byte, NmeaStats& stats);
