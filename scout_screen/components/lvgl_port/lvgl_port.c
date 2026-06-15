#include "lvgl_port.h"
#include "display.h"
#include "lvgl.h"
#include "rgb_lcd_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include <stdbool.h>

// Connects LVGL to the Waveshare RGB LCD and GT911 touch controller.
// The UI layout itself lives in the ui component, shared with the simulator.

// Rectangle owned by the direct video blit. While s_video_lock is on, flush_cb
// leaves these pixels untouched so an LVGL redraw (e.g. a theme rebuild) can't
// overwrite the live frame the render task blits there.
static int  s_lock_x, s_lock_y, s_lock_w, s_lock_h;
static bool s_video_lock;

void lvgl_port_set_video_region(int x, int y, int w, int h)
{
    s_lock_x = x;
    s_lock_y = y;
    s_lock_w = w;
    s_lock_h = h;
}

void lvgl_port_video_lock(bool on)
{
    s_video_lock = on;
}

// LVGL driver callbacks

static void flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    int x1 = area->x1, y1 = area->y1, x2 = area->x2, y2 = area->y2;
    int lx1 = s_lock_x, ly1 = s_lock_y;
    int lx2 = s_lock_x + s_lock_w, ly2 = s_lock_y + s_lock_h;  // exclusive ends

    // Fast path: no protection, or the band misses the locked rectangle.
    if(!s_video_lock || x1 >= lx2 || x2 < lx1 || y1 >= ly2 || y2 < ly1) {
        display_draw_bitmap(x1, y1, x2 + 1, y2 + 1, color_p);
        lv_disp_flush_ready(drv);
        return;
    }

    int bw = x2 - x1 + 1;  // band width = source row stride

    // Rows above the locked rect — drawn whole.
    if(y1 < ly1)
        display_draw_bitmap(x1, y1, x2 + 1, ly1, color_p);

    // Rows below the locked rect — drawn whole.
    if(y2 + 1 > ly2)
        display_draw_bitmap(x1, ly2, x2 + 1, y2 + 1, color_p + (ly2 - y1) * bw);

    // Rows overlapping the locked rect — left and right segments only,
    // leaving the locked columns (the live video) untouched.
    int my1 = y1 > ly1 ? y1 : ly1;
    int my2 = (y2 + 1) < ly2 ? (y2 + 1) : ly2;  // exclusive
    for(int y = my1; y < my2; y++) {
        lv_color_t *row = color_p + (y - y1) * bw;
        if(lx1 > x1)              // left segment [x1, lx1)
            display_draw_bitmap(x1, y, lx1, y + 1, row);
        if(lx2 < x2 + 1)          // right segment [lx2, x2+1)
            display_draw_bitmap(lx2, y, x2 + 1, y + 1, row + (lx2 - x1));
    }

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
        SCREEN_W * 120 * sizeof(lv_color_t),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    assert(lvgl_buf);

    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, lvgl_buf, NULL, SCREEN_W * 120);

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
