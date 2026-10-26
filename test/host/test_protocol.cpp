#include <assert.h>
#include <stdint.h>

#include <algorithm>
#include <string>
#include <vector>

#include "nmea.h"
#include "rtcm.h"

struct ShortWriter {
  std::vector<uint8_t> output;
  size_t write(const uint8_t* data, size_t length) {
    const size_t count = std::min<size_t>(length, 3);
    output.insert(output.end(), data, data + count);
    return count;
  }
};

int main() {
  const uint8_t binary[] = {0xd3, 0x00, 0x13, 0x00, 0xff, 0x80};
  ShortWriter writer;
  assert(forwardRtcm(binary, sizeof(binary), writer));
  assert(writer.output == std::vector<uint8_t>(binary, binary + sizeof(binary)));

  NmeaStats stats;
  const std::string gga = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n";
  for (char byte : gga) consumeNmeaByte(byte, stats);
  assert(stats.sentences == 1);
  assert(stats.ggaSentences == 1);
  assert(stats.fixQuality == 1);
  const std::string bad = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*00\n";
  for (char byte : bad) consumeNmeaByte(byte, stats);
  assert(stats.checksumErrors == 1);
}
