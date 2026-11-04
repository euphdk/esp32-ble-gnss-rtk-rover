#pragma once

#ifdef ENABLE_OLED
void startOled();
void updateOled(const char* fixLabel, bool connected, bool haveAccuracy,
                float horizontalAccuracy, float verticalAccuracy);
#endif
