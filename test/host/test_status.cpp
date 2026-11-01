#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <string>

#include "nmea.h"
#include "rover_status.h"

namespace {
std::string sentence(const std::string& body) {
  unsigned int checksum = 0;
  for (unsigned char byte : body) checksum ^= byte;
  char suffix[8];
  snprintf(suffix, sizeof(suffix), "*%02X\r\n", checksum);
  return "$" + body + suffix;
}

bool consume(const std::string& input, NmeaStats& stats) {
  bool updated = false;
  for (char byte : input) {
    if (consumeNmeaByte(byte, stats)) updated = true;
  }
  return updated;
}
}

int main() {
  const char* labels[] = {"NO FIX", "GNSS", "DGPS", "OTHER FIX", "RTK FIX",
                          "RTK FLOAT", "OTHER FIX", "OTHER FIX", "OTHER FIX", "OTHER FIX"};
  NmeaStats stats;
  for (unsigned int quality = 0; quality < 10; ++quality) {
    const std::string body = "GNGGA,123519,4807.038,N,01131.000,E," +
                             std::to_string(quality) + ",08,0.9,545.4,M,46.9,M,,";
    assert(consume(sentence(body), stats));
    assert(stats.fixQuality == quality);
    assert(strcmp(fixStatusLabel(true, 100, 101, stats.fixQuality), labels[quality]) == 0);
  }
  assert(strcmp(fixStatusLabel(false, 0, 0, 4), "NO GGA") == 0);
  assert(strcmp(fixStatusLabel(false, 0, 9000, 4), "NO GGA") == 0);
  assert(strcmp(fixStatusLabel(true, 0, 4999, 4), "RTK FIX") == 0);
  assert(strcmp(fixStatusLabel(true, 0, 5000, 4), "GGA STALE") == 0);
  assert(strcmp(fixStatusLabel(true, UINT32_MAX - 999, 3999, 5), "RTK FLOAT") == 0);
  assert(strcmp(fixStatusLabel(true, UINT32_MAX - 999, 4000, 5), "GGA STALE") == 0);
  assert(strcmp(fixStatusLabel(true, 5000, 5001, 4), "RTK FIX") == 0);
  assert(strcmp(fixStatusLabel(true, 0, 1, 999), "OTHER FIX") == 0);
  assert(strcmp(bleStatusLabel(true), "BLE UP") == 0);
  assert(strcmp(bleStatusLabel(false), "BLE DOWN") == 0);

  for (const char* quality : {"", "x", "-1", "12", "4x"}) {
    assert(!consume(sentence(std::string("GPGGA,123519,4807.038,N,01131.000,E,") +
                             quality + ",08,0.9"), stats));
  }
  assert(!consume(sentence("GPGGA,123519"), stats));
  assert(!consume(sentence("GPRMC,123519,A"), stats));
  std::string corrupt = sentence("GPGGA,123519,4807.038,N,01131.000,E,4,08,0.9");
  corrupt[1] = 'X';
  assert(!consume(corrupt, stats));
  assert(!consume(std::string(128, 'X') + sentence("GPGGA,1,2,3,4,5,4,8"), stats));
  assert(stats.fixQuality == 9);
  const std::string recovered = sentence("GPGGA,1,2,3,4,5,4,8");
  for (size_t i = 0; i + 1 < recovered.size(); ++i) {
    assert(!consumeNmeaByte(recovered[i], stats));
  }
  assert(consumeNmeaByte('\n', stats));
  assert(stats.fixQuality == 4);
}
