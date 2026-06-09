#include "display.h"
#include "gt911.h"
#include "rgb_lcd_port.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_panel_ops.h"
#include <string.h>

static esp_lcd_panel_handle_t s_panel;
static void                  *s_fb[2];
static esp_lcd_touch_handle_t s_touch;

void *display_get_fb(int i)   { return s_fb[i]; }
void display_refresh(void)    { esp_lcd_rgb_panel_refresh(s_panel); }

void display_draw_bitmap(int x1, int y1, int x2, int y2, const void *pixels)
{
    esp_lcd_panel_draw_bitmap(s_panel, x1, y1, x2, y2, pixels);
}

bool display_read_touch(uint16_t *x, uint16_t *y)
{
    uint8_t cnt = 0;
    esp_lcd_touch_read_data(s_touch);
    return esp_lcd_touch_get_coordinates(s_touch, x, y, NULL, &cnt, 1);
}

void display_blit_region(int x, int y, int w, int h, const void *pixels)
{
    uint8_t *fb = (uint8_t *)s_fb[0];
    for(int row = 0; row < h; row++)
        memcpy(fb + ((y + row) * SCREEN_W + x) * 2,
               (const uint8_t *)pixels + row * w * 2,
               w * 2);
}

void display_clear_region(int x, int y, int w, int h)
{
    uint8_t *fb = (uint8_t *)s_fb[0];
    for(int row = 0; row < h; row++)
        memset(fb + ((y + row) * SCREEN_W + x) * 2, 0, w * 2);
}

void display_backlight_on(void)  { wavesahre_rgb_lcd_bl_on(); }
void display_backlight_off(void) { wavesahre_rgb_lcd_bl_off(); }

void display_init(void)
{
    s_touch = touch_gt911_init();
    s_panel = waveshare_esp32_s3_rgb_lcd_init();
    waveshare_get_frame_buffer(&s_fb[0], &s_fb[1]);
    wavesahre_rgb_lcd_bl_on();
}
