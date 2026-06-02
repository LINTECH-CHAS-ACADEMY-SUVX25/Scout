#pragma once
#include <cstdint>

class IDisplay {
public:
    virtual ~IDisplay() = default;

    virtual bool init() = 0;

    // Draws a region of the screen. buf is RGB565, row-major.
    virtual void drawBitmap(int x1, int y1, int x2, int y2, const void *buf) = 0;

    virtual void backlightOn()  = 0;
    virtual void backlightOff() = 0;
};
