#include <assert.h>
#include <math.h>
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

  assert(!consume(sentence("GNGST,123519.000,1,2,3,4,0.3,0.4,1.2"), stats));
  assert(stats.gstSentences == 1);
  assert(fabsf(stats.horizontalAccuracy - 0.5f) < 0.00001f);
  assert(fabsf(stats.verticalAccuracy - 1.2f) < 0.00001f);
  assert(stats.fixQuality == 4);
  assert(!consume(sentence("GPGST,123520.000,,,,,0,0,0"), stats));
  assert(stats.gstSentences == 2);
  assert(stats.horizontalAccuracy == 0);
  assert(stats.verticalAccuracy == 0);

  for (const char* fields : {",0.4,1.2", "0.3,,1.2", "0.3,0.4,", "0.3,0.4",
                             "-0.3,0.4,1.2", "0.3,-0.4,1.2", "0.3,0.4,-1.2",
                             "nan,0.4,1.2", "0.3,inf,1.2", "0.3,0.4,nan",
                             "0.3x,0.4,1.2", "0.3,0.4,1.2x", "1e99,0.4,1.2"}) {
    assert(!consume(sentence(std::string("GNGST,123519.000,1,2,3,4,") + fields), stats));
    assert(stats.gstSentences == 2);
    assert(stats.horizontalAccuracy == 0);
    assert(stats.verticalAccuracy == 0);
  }
  std::string badGst = sentence("GNGST,123519.000,1,2,3,4,0.3,0.4,1.2");
  badGst[1] = 'X';
  assert(!consume(badGst, stats));
  assert(stats.gstSentences == 2);
  assert(!consume(sentence("GNGST,123521.000,1,2,3,4,0.03,0.04,0.012"), stats));
  assert(stats.gstSentences == 3);
  assert(fabsf(stats.horizontalAccuracy - 0.05f) < 0.00001f);
  assert(fabsf(stats.verticalAccuracy - 0.012f) < 0.00001f);

  assert(!gstAccuracyFresh(false, 0, 0));
  assert(gstAccuracyFresh(true, 100, 5099));
  assert(!gstAccuracyFresh(true, 100, 5100));
  assert(gstAccuracyFresh(true, UINT32_MAX - 999, 3999));
  assert(!gstAccuracyFresh(true, UINT32_MAX - 999, 4000));

  BleStatusOverlay overlay;
  assert(overlay.update(false, 0) == nullptr);
  assert(strcmp(overlay.update(true, 100), "BLE UP") == 0);
  assert(strcmp(overlay.update(true, 5099), "BLE UP") == 0);
  assert(overlay.update(true, 5100) == nullptr);
  assert(overlay.update(true, 10000) == nullptr);
  assert(strcmp(overlay.update(false, 11000), "BLE DOWN") == 0);
  assert(strcmp(overlay.update(false, 15999), "BLE DOWN") == 0);
  assert(overlay.update(false, 16000) == nullptr);
  assert(strcmp(overlay.update(true, 17000), "BLE UP") == 0);
  assert(strcmp(overlay.update(false, 18000), "BLE DOWN") == 0);
  assert(strcmp(overlay.update(false, 22999), "BLE DOWN") == 0);
  assert(overlay.update(false, 23000) == nullptr);
  BleStatusOverlay wrapOverlay;
  assert(strcmp(wrapOverlay.update(true, UINT32_MAX - 999), "BLE UP") == 0);
  assert(strcmp(wrapOverlay.update(true, 3999), "BLE UP") == 0);
  assert(wrapOverlay.update(true, 4000) == nullptr);

  char accuracy[24];
  formatAccuracy(accuracy, sizeof(accuracy), 0.92f);
  assert(strcmp(accuracy, "92cm") == 0);
  formatAccuracy(accuracy, sizeof(accuracy), 0.02f);
  assert(strcmp(accuracy, "2.0cm") == 0);
  formatAccuracy(accuracy, sizeof(accuracy), 1.3f);
  assert(strcmp(accuracy, "1.3m") == 0);
  formatAccuracy(accuracy, sizeof(accuracy), 0);
  assert(strcmp(accuracy, "0.0cm") == 0);
  formatAccuracy(accuracy, sizeof(accuracy), 0.009f);
  assert(strcmp(accuracy, "0.9cm") == 0);
  formatAccuracy(accuracy, sizeof(accuracy), 0.1f);
  assert(strcmp(accuracy, "10cm") == 0);
  formatAccuracy(accuracy, sizeof(accuracy), 1);
  assert(strcmp(accuracy, "1.0m") == 0);
}
