#pragma once
#include "IDisplay.hpp"

class DisplayEsp32 : public IDisplay {
public:
    bool init() override;
    void drawBitmap(int x1, int y1, int x2, int y2, const void *buf) override;
    void backlightOn() override;
    void backlightOff() override;
};
