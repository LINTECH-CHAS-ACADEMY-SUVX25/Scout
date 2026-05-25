#include "display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "gt911.h"
#include "rgb_lcd_port.h"

static esp_lcd_panel_handle_t s_panel;
static void                  *s_fb[2];
static esp_lcd_touch_handle_t s_touch;

static void flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    esp_lcd_panel_draw_bitmap(s_panel, area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_p);
    lv_disp_flush_ready(drv);
}

static void touch_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    uint16_t x, y;
    uint8_t cnt = 0;
    esp_lcd_touch_read_data(s_touch);
    bool touched = esp_lcd_touch_get_coordinates(s_touch, &x, &y, NULL, &cnt, 1);
    data->point.x = x;
    data->point.y = y;
    data->state = touched ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static void tick_task(void *arg)
{
    while (1) {
        lv_tick_inc(10);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void display_init(void)
{
    s_touch = touch_gt911_init();
    s_panel = waveshare_esp32_s3_rgb_lcd_init();
    waveshare_get_frame_buffer(&s_fb[0], &s_fb[1]);
    wavesahre_rgb_lcd_bl_on();

    lv_init();

    // must be static — LVGL holds pointers to these after registration
    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, s_fb[0], s_fb[1],
                          EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res      = EXAMPLE_LCD_H_RES;
    disp_drv.ver_res      = EXAMPLE_LCD_V_RES;
    disp_drv.flush_cb     = flush_cb;
    disp_drv.draw_buf     = &draw_buf;
    disp_drv.full_refresh = 1; // RGB panels don't support partial updates
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_read_cb;
    lv_indev_drv_register(&indev_drv);

    xTaskCreate(tick_task, "lvgl_tick", 2048, NULL, 5, NULL);
}
