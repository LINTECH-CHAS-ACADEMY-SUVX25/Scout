#pragma once
#include "IDisplay.hpp"

// Concrete IDisplay implementation for the Waveshare ESP32-S3-Touch-LCD-7B.
// Delegates to display_init() and display_draw_bitmap() from display.h.
// Swap this class out to target a different display without touching render.cpp.
class DisplayEsp32 : public IDisplay {
public:
    bool init() override;
    void drawBitmap(int x1, int y1, int x2, int y2, const void *buf) override;
    void backlightOn()  override;
    void backlightOff() override;
};
