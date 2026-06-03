#include "display.h"
#include "gt911.h"
#include "rgb_lcd_port.h"

static esp_lcd_panel_handle_t s_panel;
static void                  *s_fb[2];
static esp_lcd_touch_handle_t s_touch;

void *display_get_panel(void) { return s_panel; }
void *display_get_touch(void) { return s_touch; }
void *display_get_fb(int i)   { return s_fb[i]; }

void display_init(void)
{
    s_touch = touch_gt911_init();
    s_panel = waveshare_esp32_s3_rgb_lcd_init();
    waveshare_get_frame_buffer(&s_fb[0], &s_fb[1]);
    wavesahre_rgb_lcd_bl_on();
}
