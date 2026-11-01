#pragma once

#ifdef ENABLE_OLED
void startOled();
void updateOled(const char* fixLabel, bool connected);
#endif
