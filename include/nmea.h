#pragma once

#include <stddef.h>

struct NmeaStats {
  unsigned long sentences = 0;
  unsigned long checksumErrors = 0;
  unsigned long ggaSentences = 0;
  unsigned long fixQuality = 0;
};

// Consumes a byte stream and updates counters for complete NMEA sentences.
void consumeNmeaByte(char byte, NmeaStats& stats);
