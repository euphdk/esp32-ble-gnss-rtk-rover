#include "nmea.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

namespace {
constexpr size_t kLineCapacity = 128;
char line[kLineCapacity];
size_t lineLength = 0;

void parseLine(NmeaStats& stats) {
  if (lineLength < 7 || line[0] != '$') return;
  char* star = strchr(line, '*');
  if (star == nullptr || star + 2 >= line + lineLength) return;
  unsigned int expected = 0;
  if (sscanf(star + 1, "%2x", &expected) != 1) return;
  unsigned int actual = 0;
  for (char* p = line + 1; p < star; ++p) actual ^= static_cast<unsigned char>(*p);
  if (actual != expected) {
    ++stats.checksumErrors;
    return;
  }

  ++stats.sentences;
  if (strncmp(line + 3, "GGA,", 4) != 0) return;
  ++stats.ggaSentences;
  char* field = strchr(line, ',');
  for (int i = 0; i < 5 && field != nullptr; ++i) field = strchr(field + 1, ',');
  if (field != nullptr) stats.fixQuality = strtoul(field + 1, nullptr, 10);
}
}

void consumeNmeaByte(char byte, NmeaStats& stats) {
  if (byte == '\n') {
    parseLine(stats);
    lineLength = 0;
  } else if (byte == '\r') {
    return;
  } else if (lineLength < kLineCapacity - 1) {
    line[lineLength++] = byte;
    line[lineLength] = '\0';
  } else {
    lineLength = 0;
  }
}
