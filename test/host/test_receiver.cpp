#include <assert.h>

#include <string>
#include <vector>

#include "config.h"

struct ReceiverWriter {
  uint32_t now = 0;
  size_t writeLimit = SIZE_MAX;
  std::vector<std::string> frames;
  std::vector<uint32_t> times;

  size_t write(const uint8_t* bytes, size_t length) {
    frames.emplace_back(reinterpret_cast<const char*>(bytes), length);
    times.push_back(now);
    return length < writeLimit ? length : writeLimit;
  }
};

int main() {
  ReceiverWriter writer;
  std::vector<uint32_t> waits;
  auto wait = [&](uint32_t ms) {
    waits.push_back(ms);
    writer.now += ms;
  };
  assert(configureGnss(writer, wait));
#if GNSS_RECEIVER == GNSS_RECEIVER_LC29H_DA
  static_assert(config::kGnssBaud == 115200);
  const std::vector<std::string> expected = {
      "$PQTMCFGRCVRMODE,W,1*2A\r\n",
      "$PQTMCFGRTK,W,1,1*6C\r\n",
      "$PQTMCFGPROT,W,1,1,00000005,00000005*38\r\n",
      "$PQTMCFGMSGRATE,W,GGA,1*0A\r\n",
      "$PQTMCFGMSGRATE,W,RMC,1*17\r\n",
      "$PQTMCFGMSGRATE,W,GSA,1*1E\r\n",
      "$PQTMCFGMSGRATE,W,GSV,1*09\r\n",
      "$PQTMCFGMSGRATE,W,VTG,0*0F\r\n",
      "$PQTMCFGMSGRATE,W,GLL,0*0D\r\n",
      "$PQTMCFGMSGRATE,W,ZDA,0*15\r\n",
  };
  assert(writer.frames == expected);
  assert(waits.size() == 10);
  for (size_t i = 0; i < expected.size(); ++i) {
    assert(writer.times[i] == 1200 + i * 250);
    assert(waits[i] == (i == 0 ? 1200 : 250));
  }
  assert(writer.now == 3450);
  // A new boot sends the complete sequence again, without an acknowledgment source.
  ReceiverWriter reboot;
  assert(configureGnss(reboot, [&](uint32_t ms) { reboot.now += ms; }));
  assert(reboot.frames == expected);
  for (size_t limit : {0U, 3U}) {
    ReceiverWriter failed;
    failed.writeLimit = limit;
    assert(!configureGnss(failed, [&](uint32_t ms) { failed.now += ms; }));
    assert(failed.frames.size() == 1);
    assert(failed.now == 1200);
  }
#else
  static_assert(config::kGnssBaud == 38400);
  assert(writer.frames.empty());
  assert(waits.empty());
#endif
}
