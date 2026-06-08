#include "DisplayEsp32.hpp"

extern "C" {
#include "display.h"
}

bool DisplayEsp32::init()
{
    display_init();
    return true;
}

void DisplayEsp32::drawBitmap(int x1, int y1, int x2, int y2, const void *buf)
{
    display_draw_bitmap(x1, y1, x2, y2, buf);
}

void DisplayEsp32::backlightOn()
{
    // Backlight is enabled inside display_init() via wavesahre_rgb_lcd_bl_on().
    // Expose explicit control here if runtime toggling is needed later.
}

void DisplayEsp32::backlightOff()
{
    // TODO: call waveshare_rgb_lcd_bl_off() once it is exposed from rgb_lcd_port.
}
