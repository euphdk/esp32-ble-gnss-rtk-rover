#include <assert.h>
#include <stdint.h>

#include <algorithm>
#include <string>
#include <vector>

#include "nmea.h"
#include "rtcm.h"

struct ZeroWriter {
  size_t write(const uint8_t*, size_t) { return 0; }
};

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
  ZeroWriter zeroWriter;
  assert(!forwardRtcm(binary, sizeof(binary), zeroWriter));

  RtcmBuffer<16> buffer;
  assert(buffer.enqueue(binary, sizeof(binary)));
  const uint8_t tooLarge[17] = {};
  assert(!buffer.enqueue(tooLarge, sizeof(tooLarge)));
  uint8_t drained[4];
  assert(buffer.drain(sizeof(drained), drained) == sizeof(drained));
  assert(std::vector<uint8_t>(drained, drained + sizeof(drained)) ==
         std::vector<uint8_t>(binary, binary + sizeof(drained)));
  uint8_t remaining[16];
  const size_t remainingLength = buffer.drain(sizeof(remaining), remaining);
  assert(remainingLength == sizeof(binary) - sizeof(drained));
  assert(std::vector<uint8_t>(remaining, remaining + remainingLength) ==
         std::vector<uint8_t>(binary + sizeof(drained), binary + sizeof(binary)));
  assert(buffer.drain(sizeof(remaining), remaining) == 0);

  RtcmBuffer<8> wrapped;
  const uint8_t first[] = {0, 1, 2, 3, 4, 5};
  const uint8_t second[] = {6, 7, 8, 9, 10, 11};
  assert(wrapped.enqueue(first, sizeof(first)));
  assert(wrapped.drain(5, remaining) == 5);
  assert(wrapped.enqueue(second, sizeof(second)));
  uint8_t fifo[8];
  const uint8_t expectedFifo[] = {5, 6, 7, 8, 9, 10, 11};
  assert(wrapped.drain(sizeof(fifo), fifo) == sizeof(expectedFifo));
  assert(std::vector<uint8_t>(fifo, fifo + sizeof(expectedFifo)) ==
         std::vector<uint8_t>(expectedFifo, expectedFifo + sizeof(expectedFifo)));

  NmeaStats stats;
  const std::string gga = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n";
  for (char byte : gga) consumeNmeaByte(byte, stats);
  assert(stats.sentences == 1);
  assert(stats.ggaSentences == 1);
  assert(stats.fixQuality == 1);
  const std::string nonGga = "$GPRMC,123519,A,,,,,,,230394,,,A*49\n";
  const std::string gngga = "$GNGGA,123519,4807.038,N,01131.000,E,2,08,0.9,545.4,M,46.9,M,,*5A\n";
  for (char byte : gngga) consumeNmeaByte(byte, stats);
  assert(stats.sentences == 2);
  assert(stats.ggaSentences == 2);
  assert(stats.fixQuality == 2);
  for (char byte : nonGga) consumeNmeaByte(byte, stats);
  assert(stats.sentences == 3);
  // 7 prefix chars + 121 X overflow the 127-char limit on the 121st X, so the
  // embedded GGA's '$' arrives right after the overflow point. Discarding keeps
  // the first embedded GGA out (+1 from the second); a lineLength-only reset
  // would parse the first embedded GGA too (+2).
  const std::string oversized = "$GPGGA," + std::string(121, 'X') + gga + gga;
  for (char byte : oversized) consumeNmeaByte(byte, stats);
  assert(stats.ggaSentences == 3);
  assert(stats.sentences == 4);
  const std::string bad = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*00\n";
  for (char byte : bad) consumeNmeaByte(byte, stats);
  assert(stats.checksumErrors == 1);
}
