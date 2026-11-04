#include "oled.h"

#ifdef ENABLE_OLED
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include "config.h"
#include "rover_status.h"

namespace {
Adafruit_SSD1306 display(128, 64, &Wire, -1);
bool ready = false;
BleStatusOverlay bleOverlay;
}

void startOled() {
  if (!Wire.begin(config::kOledSdaPin, config::kOledSclPin)) return;
  Wire.setTimeOut(5);
  Wire.beginTransmission(config::kOledAddress);
  if (Wire.endTransmission() != 0) return;
  ready = display.begin(SSD1306_SWITCHCAPVCC, config::kOledAddress, false, false);
  if (!ready) return;
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);
  updateOled("NO GGA", false, false, 0, 0);
}

void updateOled(const char* fixLabel, bool connected, bool haveAccuracy,
                float horizontalAccuracy, float verticalAccuracy) {
  if (!ready) return;
  // Stop updates after a disconnect instead of repeating failed frame transfers.
  Wire.beginTransmission(config::kOledAddress);
  if (Wire.endTransmission() != 0) {
    ready = false;
    return;
  }
  display.clearDisplay();
  display.setCursor(0, 8);
  display.print(fixLabel);
  const char* bleLabel = bleOverlay.update(connected, millis());
  if (bleLabel != nullptr) {
    display.setCursor(0, 40);
    display.print(bleLabel);
  } else {
    const float values[] = {horizontalAccuracy, verticalAccuracy};
    const char* labels[] = {"H", "V"};
    for (int i = 0; i < 2; ++i) {
      const int y = 32 + i * 16;
      // The default font lacks Greek delta, so draw its outline.
      display.drawTriangle(0, y + 13, 5, y + 1, 10, y + 13, SSD1306_WHITE);
      display.setCursor(14, y);
      display.print(labels[i]);
      display.print(' ');
      if (haveAccuracy) {
        char accuracy[24];
        formatAccuracy(accuracy, sizeof(accuracy), values[i]);
        display.print(accuracy);
      } else {
        display.print("N/A");
      }
    }
  }
  display.display();
}
#endif
