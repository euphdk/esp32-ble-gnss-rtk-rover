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
  if (strncmp(line + 3, "RMC,", 4) == 0) {
    const char* fields[10] = {line};
    for (int i = 1; i < 10; ++i) {
      fields[i] = strchr(fields[i - 1], ',');
      if (fields[i] == nullptr || fields[i] >= star) return false;
      ++fields[i];
    }
    const char* utc = fields[1];
    const char* utcEnd = fields[2] - 1;
    const char* date = fields[9];
    const char* dateEnd = strchr(date, ',');
    if (dateEnd == nullptr) dateEnd = star;
    if (utcEnd - utc < 6 || dateEnd - date != 6) return false;
    for (int i = 0; i < 6; ++i) {
      if (utc[i] < '0' || utc[i] > '9' || date[i] < '0' || date[i] > '9') return false;
    }
    if (utcEnd - utc > 6) {
      if (utc[6] != '.' || utcEnd - utc == 7) return false;
      for (const char* p = utc + 7; p < utcEnd; ++p) {
        if (*p < '0' || *p > '9') return false;
      }
    }
    const unsigned int hour = (utc[0] - '0') * 10 + utc[1] - '0';
    const unsigned int minute = (utc[2] - '0') * 10 + utc[3] - '0';
    const unsigned int second = (utc[4] - '0') * 10 + utc[5] - '0';
    const unsigned int day = (date[0] - '0') * 10 + date[1] - '0';
    const unsigned int month = (date[2] - '0') * 10 + date[3] - '0';
    const unsigned int shortYear = (date[4] - '0') * 10 + date[5] - '0';
    const unsigned int year = shortYear >= 80 ? 1900 + shortYear : 2000 + shortYear;
    if (hour > 23 || minute > 59 || second > 59 || month < 1 || month > 12 || day < 1) return false;
    constexpr unsigned int monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    unsigned int maxDay = monthDays[month - 1];
    if (month == 2 && year % 4 == 0) ++maxDay;
    if (day > maxDay) return false;
    snprintf(stats.utcDateTime, sizeof(stats.utcDateTime), "%s%.2s/%.2s/%.2s %.2s:%.2s:%.2sZ",
             shortYear >= 80 ? "19" : "20", date + 4, date + 2, date, utc, utc + 2, utc + 4);
    ++stats.rmcSentences;
    return false;
  }
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
