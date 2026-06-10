#include "lvgl_port.h"
#include "display.h"
#include "lvgl.h"
#include "rgb_lcd_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include <stdbool.h>

// Connects LVGL to the Waveshare RGB LCD and GT911 touch controller.
// The UI layout itself lives in lvgl_port_ui.c, shared with the simulator.

// LVGL driver callbacks

static void flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    display_draw_bitmap(area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_p);
    lv_disp_flush_ready(drv);
}

static void touch_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    uint16_t x, y;
    bool touched = display_read_touch(&x, &y);
    data->point.x = x;
    data->point.y = y;
    data->state   = touched ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static void tick_task(void *arg)
{
    while(1) {
        lv_tick_inc(10);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Driver init

void lvgl_port_init(void)
{
    lv_init();

    lv_color_t *lvgl_buf = heap_caps_malloc(
        SCREEN_W * 20 * sizeof(lv_color_t),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    assert(lvgl_buf);

    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, lvgl_buf, NULL, SCREEN_W * 20);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res      = SCREEN_W;
    disp_drv.ver_res      = SCREEN_H;
    disp_drv.flush_cb     = flush_cb;
    disp_drv.draw_buf     = &draw_buf;
    disp_drv.full_refresh = 0;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_read_cb;
    lv_indev_drv_register(&indev_drv);

    xTaskCreatePinnedToCore(tick_task, "lvgl_tick", 2048, NULL, 5, NULL, 1);

    lvgl_port_ui_init();
}

// Render

void lvgl_port_render_frame(void)
{
    lv_timer_t *refr = lv_disp_get_default()->refr_timer;
    lv_timer_pause(refr);
    lv_timer_handler();              // input phase: indev fires, dirty areas accumulate
    lv_timer_resume(refr);
    lv_refr_now(lv_disp_get_default()); // render all accumulated dirty areas immediately
}
