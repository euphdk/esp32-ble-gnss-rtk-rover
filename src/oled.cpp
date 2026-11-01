#include "oled.h"

#ifdef ENABLE_OLED
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include "config.h"
#include "rover_status.h"

namespace {
Adafruit_SSD1306 display(64, 48, &Wire, -1);
bool ready = false;
}

void startOled() {
  if (!Wire.begin(config::kOledSdaPin, config::kOledSclPin)) return;
  Wire.setTimeOut(5);
  Wire.beginTransmission(config::kOledAddress);
  if (Wire.endTransmission() != 0) return;
  ready = display.begin(SSD1306_SWITCHCAPVCC, config::kOledAddress, false, false);
  if (!ready) return;
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);
  updateOled("NO GGA", false);
}

void updateOled(const char* fixLabel, bool connected) {
  if (!ready) return;
  // Stop updates after a disconnect instead of repeating failed frame transfers.
  Wire.beginTransmission(config::kOledAddress);
  if (Wire.endTransmission() != 0) {
    ready = false;
    return;
  }
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("ROVER");
  display.setCursor(0, 16);
  display.print(fixLabel);
  display.setCursor(0, 32);
  display.print(bleStatusLabel(connected));
  display.display();
}
#endif
