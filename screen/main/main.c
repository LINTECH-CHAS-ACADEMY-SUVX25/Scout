#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lvgl.h"
#include "gt911.h"
#include "rgb_lcd_port.h"

static const char *TAG = "screen";

static esp_lcd_panel_handle_t g_panel;
static void *g_fb[2];
static esp_lcd_touch_handle_t g_touch;

static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    esp_lcd_panel_draw_bitmap(g_panel, area->x1, area->y1,
                              area->x2 + 1, area->y2 + 1, color_p);
    lv_disp_flush_ready(drv);
}

static void touch_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    uint16_t x, y;
    uint8_t cnt = 0;
    esp_lcd_touch_read_data(g_touch);
    bool touched = esp_lcd_touch_get_coordinates(g_touch, &x, &y, NULL, &cnt, 1);
    if (touched) {
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static void lvgl_tick_task(void *arg)
{
    while (1) {
        lv_tick_inc(10);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void lvgl_handler_task(void *arg)
{
    while (1) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

#define JOYSTICK_OUTER_R 80
#define JOYSTICK_KNOB_R  28

static lv_obj_t *g_knob;

static void joystick_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *outer = lv_event_get_target(e);

    if (code == LV_EVENT_PRESSING) {
        lv_point_t p;
        lv_indev_get_point(lv_indev_get_act(), &p);

        lv_area_t coords;
        lv_obj_get_coords(outer, &coords);
        lv_coord_t cx = (coords.x1 + coords.x2) / 2;
        lv_coord_t cy = (coords.y1 + coords.y2) / 2;

        int32_t dx = p.x - cx;
        int32_t dy = p.y - cy;
        int32_t max_r = JOYSTICK_OUTER_R - JOYSTICK_KNOB_R;
        int32_t dist2 = dx * dx + dy * dy;
        if (dist2 > max_r * max_r) {
            float s = (float)max_r / sqrtf((float)dist2);
            dx = (int32_t)(dx * s);
            dy = (int32_t)(dy * s);
        }

        lv_obj_set_pos(g_knob, JOYSTICK_OUTER_R - JOYSTICK_KNOB_R + dx,
                               JOYSTICK_OUTER_R - JOYSTICK_KNOB_R + dy);
    } else if (code == LV_EVENT_RELEASED) {
        lv_obj_set_pos(g_knob, JOYSTICK_OUTER_R - JOYSTICK_KNOB_R,
                               JOYSTICK_OUTER_R - JOYSTICK_KNOB_R);
    }
}

static void create_joystick(void)
{
    lv_obj_t *outer = lv_obj_create(lv_scr_act());
    lv_obj_set_size(outer, JOYSTICK_OUTER_R * 2, JOYSTICK_OUTER_R * 2);
    lv_obj_align(outer, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_set_style_radius(outer, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(outer, lv_color_hex(0x2A2A2A), 0);
    lv_obj_set_style_bg_opa(outer, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(outer, lv_color_hex(0x888888), 0);
    lv_obj_set_style_border_width(outer, 2, 0);
    lv_obj_set_style_pad_all(outer, 0, 0);
    lv_obj_clear_flag(outer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(outer, joystick_event_cb, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(outer, joystick_event_cb, LV_EVENT_RELEASED, NULL);

    g_knob = lv_obj_create(outer);
    lv_obj_set_size(g_knob, JOYSTICK_KNOB_R * 2, JOYSTICK_KNOB_R * 2);
    lv_obj_set_pos(g_knob, JOYSTICK_OUTER_R - JOYSTICK_KNOB_R,
                           JOYSTICK_OUTER_R - JOYSTICK_KNOB_R);
    lv_obj_set_style_radius(g_knob, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(g_knob, lv_color_hex(0x4488FF), 0);
    lv_obj_set_style_bg_opa(g_knob, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(g_knob, 0, 0);
    lv_obj_clear_flag(g_knob, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
}

void app_main(void)
{
    g_touch = touch_gt911_init();
    g_panel = waveshare_esp32_s3_rgb_lcd_init();
    waveshare_get_frame_buffer(&g_fb[0], &g_fb[1]);
    wavesahre_rgb_lcd_bl_on();

    lv_init();

    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, g_fb[0], g_fb[1],
                          EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res  = EXAMPLE_LCD_H_RES;
    disp_drv.ver_res  = EXAMPLE_LCD_V_RES;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.full_refresh = 1;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_read_cb;
    lv_indev_drv_register(&indev_drv);

    xTaskCreate(lvgl_tick_task,    "lvgl_tick",    2048, NULL, 5, NULL);
    xTaskCreate(lvgl_handler_task, "lvgl_handler", 4096, NULL, 4, NULL);

    create_joystick();

    ESP_LOGI(TAG, "touch + joystick running");
    vTaskDelete(NULL);
}
