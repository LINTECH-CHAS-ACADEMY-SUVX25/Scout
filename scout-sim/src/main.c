/**
 * @file main.c
 * @brief Scout Simulator – LVGL + SDL2 @ 1024×600
 *
 * Mouse interaction:
 *   - Click the joystick ring on the right panel and drag to steer.
 *   - Click the dropdown button to toggle sensor overlay.
 *   - Click action buttons for lights / record / snapshot.
 *
 * Build:
 *   mkdir build && cd build && cmake .. && make -j$(nproc)
 *   ./scout_sim
 */

#include <unistd.h>
#include <math.h>
#include <stdbool.h>
#include "lvgl.h"
#include "sdl/sdl.h"
#include "sensor_hal.h"
#include "ui_dashboard.h"

/* ── Display ── */
#define DISP_HOR_RES  SCREEN_W    /* 1024 */
#define DISP_VER_RES  SCREEN_H    /* 600  */

static lv_color_t buf1[DISP_HOR_RES * 20];

/* ── Joystick state ── */
static joystick_input_t  joy_input = { 0.0f, 0.0f };
static bool              joy_dragging = false;

/* Joystick ring position on screen (absolute coords) */
#define JOY_CX   (CAM_W + RPANEL_W - 140/2 - 20 + 8)   /* center of ring */
#define JOY_CY   (SCREEN_H - 140/2 - 60)
#define JOY_R    60   /* effective drag radius */

/* ── SDL input callback ── */
static void mouse_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    /* Let the SDL driver fill in coordinates and press state */
    sdl_mouse_read(drv, data);

    /* Check if we're in the joystick area */
    int16_t mx = data->point.x;
    int16_t my = data->point.y;

    if (data->state == LV_INDEV_STATE_PRESSED) {
        float dx = (float)(mx - JOY_CX);
        float dy = (float)(my - JOY_CY);
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist < JOY_R + 30 || joy_dragging) {
            joy_dragging = true;
            if (dist > JOY_R) {
                dx = dx / dist * JOY_R;
                dy = dy / dist * JOY_R;
            }
            joy_input.x =  dx / JOY_R;      /* -1..+1 left/right */
            joy_input.y = -dy / JOY_R;       /* -1..+1 back/fwd   */
        }
    } else {
        joy_dragging = false;
        joy_input.x  = 0.0f;
        joy_input.y  = 0.0f;
    }
}

/* ── HAL init ── */
static void hal_init(void)
{
    sdl_init();

    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL,
                          sizeof(buf1) / sizeof(buf1[0]));

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res  = DISP_HOR_RES;
    disp_drv.ver_res  = DISP_VER_RES;
    disp_drv.flush_cb = sdl_display_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t mouse_drv;
    lv_indev_drv_init(&mouse_drv);
    mouse_drv.type    = LV_INDEV_TYPE_POINTER;
    mouse_drv.read_cb = mouse_read_cb;
    lv_indev_drv_register(&mouse_drv);
}

/* ── Main ── */
int main(void)
{
    lv_init();
    hal_init();
    scout_sensors_init();

    ui_dashboard_create(lv_scr_act());

    scout_sensors_t sensor_data;
    const uint32_t TICK_MS   = 16;    /* ~60 fps */
    const uint32_t UPDATE_MS = 200;   /* sensor refresh */
    uint32_t acc = 0;

    while (1) {
        lv_tick_inc(TICK_MS);
        lv_timer_handler();

        scout_sensors_tick(TICK_MS);
        acc += TICK_MS;

        if (acc >= UPDATE_MS) {
            acc = 0;
            scout_sensors_read(&sensor_data);
            alert_level_t alert = scout_evaluate_alerts(&sensor_data);
            ui_dashboard_update(&sensor_data, alert, &joy_input);
        }

        usleep(TICK_MS * 1000);
    }

    return 0;
}
