#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "i2c.h"
#include "io_extension.h"
#include "rgb_lcd_port.h"

static const char *MAIN_TAG = "scout_main";

#define LCD_W  EXAMPLE_LCD_H_RES   /* 1024 */
#define LCD_H  EXAMPLE_LCD_V_RES   /* 600  */

/* ---------- 8×8 bitmap font for S C O U T ----------
 * Each byte = one row, bit 7 = leftmost pixel.       */
static const uint8_t GLYPH_S[8] = {0x7E, 0xC0, 0xC0, 0x7E, 0x03, 0x03, 0x7E, 0x00};
static const uint8_t GLYPH_C[8] = {0x7E, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0x7E, 0x00};
static const uint8_t GLYPH_O[8] = {0x7E, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0x7E, 0x00};
static const uint8_t GLYPH_U[8] = {0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0x7E, 0x00};
static const uint8_t GLYPH_T[8] = {0xFF, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00};

static const uint8_t *SCOUT[5] = {GLYPH_S, GLYPH_C, GLYPH_O, GLYPH_U, GLYPH_T};

/* Render one 8×8 glyph scaled up by SCALE, at pixel (x0, y0) in RGB565 framebuffer. */
#define GLYPH_SCALE  10   /* each font pixel → 10×10 screen pixels */
#define GLYPH_GAP    12   /* horizontal gap between characters      */

static void draw_glyph(uint16_t *fb, int x0, int y0,
                       const uint8_t bitmap[8], uint16_t color)
{
    for (int row = 0; row < 8; row++) {
        uint8_t bits = bitmap[row];
        for (int col = 0; col < 8; col++) {
            if (!(bits & (0x80u >> col))) continue;
            for (int dy = 0; dy < GLYPH_SCALE; dy++) {
                int py = y0 + row * GLYPH_SCALE + dy;
                if (py < 0 || py >= LCD_H) continue;
                for (int dx = 0; dx < GLYPH_SCALE; dx++) {
                    int px = x0 + col * GLYPH_SCALE + dx;
                    if (px < 0 || px >= LCD_W) continue;
                    fb[py * LCD_W + px] = color;
                }
            }
        }
    }
}

static void render_splash(uint16_t *fb)
{
    /* Background: black */
    memset(fb, 0x00, LCD_W * LCD_H * sizeof(uint16_t));

    /* Centre "SCOUT" horizontally and vertically */
    const int char_w  = 8 * GLYPH_SCALE;                          /* 80 px */
    const int char_h  = 8 * GLYPH_SCALE;                          /* 80 px */
    const int total_w = 5 * char_w + 4 * GLYPH_GAP;               /* 448 px */
    const int start_x = (LCD_W - total_w) / 2;
    const int start_y = (LCD_H - char_h)  / 2;

    /* White text — RGB565: 0xFFFF */
    for (int i = 0; i < 5; i++) {
        int x = start_x + i * (char_w + GLYPH_GAP);
        draw_glyph(fb, x, start_y, SCOUT[i], 0xFFFF);
    }
}

void app_main(void)
{
    ESP_LOGI(MAIN_TAG, "Scout dashboard starting");

    /* Bring up I2C bus (SDA=8, SCL=9) */
    DEV_I2C_Init();

    /* IO expander — controls backlight (IO2), LCD reset (IO3), touch reset (IO1) */
    IO_EXTENSION_Init();

    /* RGB LCD panel — sets up DMA + double framebuffer in PSRAM */
    waveshare_esp32_s3_rgb_lcd_init();

    /* Get both DMA framebuffers.  The DMA engine scans them continuously,
     * so writing here is immediately visible — no draw_bitmap call needed.
     * We fill both so the display is consistent regardless of which buffer
     * is currently being scanned. */
    void *buf1 = NULL, *buf2 = NULL;
    waveshare_get_frame_buffer(&buf1, &buf2);

    render_splash((uint16_t *)buf1);
    render_splash((uint16_t *)buf2);

    /* Turn on backlight */
    wavesahre_rgb_lcd_bl_on();

    ESP_LOGI(MAIN_TAG, "Splash ready");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
