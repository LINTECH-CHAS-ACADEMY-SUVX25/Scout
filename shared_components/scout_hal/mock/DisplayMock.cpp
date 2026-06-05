#include "DisplayMock.hpp"
#include <cstdio>

bool DisplayMock::init()
{
    initialized = true;
    printf("[DisplayMock] init\n");
    return true;
}

void DisplayMock::drawBitmap(int x1, int y1, int x2, int y2, const void *buf)
{
    draw_count++;
    printf("[DisplayMock] drawBitmap (%d,%d)-(%d,%d) draw #%d\n", x1, y1, x2, y2, draw_count);
}

void DisplayMock::backlightOn()
{
    backlight_on = true;
    printf("[DisplayMock] backlight ON\n");
}

void DisplayMock::backlightOff()
{
    backlight_on = false;
    printf("[DisplayMock] backlight OFF\n");
}
