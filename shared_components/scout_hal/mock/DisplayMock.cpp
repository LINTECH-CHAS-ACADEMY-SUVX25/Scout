#include "DisplayMock.hpp"

bool DisplayMock::init()
{
    initialized = true;
    return true;
}

void DisplayMock::drawBitmap(int x1, int y1, int x2, int y2, const void *buf)
{
    (void)x1; (void)y1; (void)x2; (void)y2; (void)buf;
    draw_count++;
}
