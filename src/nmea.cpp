#include "nmea.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {
constexpr size_t kLineCapacity = 128;
char line[kLineCapacity];
size_t lineLength = 0;
bool discardingLine = false;

bool parseLine(NmeaStats& stats) {
  if (lineLength < 7 || line[0] != '$') return false;
  char* star = strchr(line, '*');
  if (star == nullptr || star + 2 >= line + lineLength) return false;
  unsigned int expected = 0;
  if (sscanf(star + 1, "%2x", &expected) != 1) return false;
  unsigned int actual = 0;
  for (char* p = line + 1; p < star; ++p) actual ^= static_cast<unsigned char>(*p);
  if (actual != expected) {
    ++stats.checksumErrors;
    return false;
  }

  ++stats.sentences;
  if (strncmp(line + 3, "GST,", 4) == 0) {
    char* field = strchr(line, ',');
    for (int i = 0; i < 5 && field != nullptr; ++i) field = strchr(field + 1, ',');
    float deviations[3];
    for (int i = 0; i < 3; ++i) {
      if (field == nullptr || field >= star) return false;
      char* end;
      deviations[i] = strtof(field + 1, &end);
      if (end == field + 1 || *end != (i < 2 ? ',' : '*') ||
          !isfinite(deviations[i]) || deviations[i] < 0) return false;
      field = end;
    }
    const float horizontal = hypotf(deviations[0], deviations[1]);
    if (!isfinite(horizontal)) return false;
    stats.horizontalAccuracy = horizontal;
    stats.verticalAccuracy = deviations[2];
    ++stats.gstSentences;
    return false;
  }
  if (strncmp(line + 3, "GGA,", 4) != 0) return false;
  ++stats.ggaSentences;
  char* field = strchr(line, ',');
  for (int i = 0; i < 5 && field != nullptr; ++i) field = strchr(field + 1, ',');
  if (field == nullptr || field + 2 >= star || field[1] < '0' || field[1] > '9' ||
      field[2] != ',') return false;
  stats.fixQuality = static_cast<unsigned long>(field[1] - '0');
  return true;
}
}

bool consumeNmeaByte(char byte, NmeaStats& stats) {
  if (byte == '\n') {
    const bool updated = !discardingLine && parseLine(stats);
    lineLength = 0;
    discardingLine = false;
    return updated;
  } else if (byte == '\r') {
    return false;
  } else if (discardingLine) {
    return false;
  } else if (lineLength < kLineCapacity - 1) {
    line[lineLength++] = byte;
    line[lineLength] = '\0';
  } else {
    lineLength = 0;
    discardingLine = true;
  }
  return false;
}
