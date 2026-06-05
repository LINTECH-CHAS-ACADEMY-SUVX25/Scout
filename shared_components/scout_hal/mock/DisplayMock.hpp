#pragma once
#include "IDisplay.hpp"

class DisplayMock : public IDisplay {
public:
    bool init() override;
    void drawBitmap(int x1, int y1, int x2, int y2, const void *buf) override;
    void backlightOn()  override;
    void backlightOff() override;

    bool initialized   = false;
    bool backlight_on  = false;
    int  draw_count    = 0;
};
