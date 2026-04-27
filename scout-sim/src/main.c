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
static joystick_input_t  joy_target = { 0.0f, 0.0f };  /* Target values for smoothing */
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
            joy_target.x =  dx / JOY_R;      /* -1..+1 left/right */
            joy_target.y = -dy / JOY_R;      /* -1..+1 back/fwd   */
        }
    } else {
        joy_dragging = false;
        joy_target.x = 0.0f;
        joy_target.y = 0.0f;
    }
}

/* ── Smooth joystick interpolation ── */
static void update_joystick_smooth(float dt_sec)
{
    /*
     * Touch-optimized smoothing:
     * - Active: Fast response for immediate control
     * - Release: Smooth deceleration for natural feel
     * - Deadzone: Prevents unintended input near center
     */
    const float smoothness_active  = 18.0f;  /* Responsive but smooth on touch */
    const float smoothness_release = 7.0f;   /* Gradual return to neutral */
    const float deadzone = 0.08f;            /* Comfortable center zone for touch */

    float smoothness = joy_dragging ? smoothness_active : smoothness_release;
    float alpha = 1.0f - expf(-smoothness * dt_sec);

    /* Apply deadzone to target values */
    float target_x = joy_target.x;
    float target_y = joy_target.y;

    if (fabsf(target_x) < deadzone) target_x = 0.0f;
    if (fabsf(target_y) < deadzone) target_y = 0.0f;

    /* Exponential smoothing for natural feel */
    joy_input.x += (target_x - joy_input.x) * alpha;
    joy_input.y += (target_y - joy_input.y) * alpha;

    /* Snap to zero when very close */
    if (fabsf(joy_input.x) < 0.01f) joy_input.x = 0.0f;
    if (fabsf(joy_input.y) < 0.01f) joy_input.y = 0.0f;
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

/* ── Main Loop ── */
int main(void)
{
    /* Initialize LVGL, HAL, and sensors */
    lv_init();
    hal_init();
    scout_sensors_init();

    /* Create dashboard UI */
    ui_dashboard_create(lv_scr_act());

    /* Main loop timing configuration */
    scout_sensors_t sensor_data;
    const uint32_t TICK_MS   = 16;    /* 60 fps frame time */
    const uint32_t UPDATE_MS = 200;   /* Sensor update interval (5 Hz) */
    uint32_t sensor_timer = 0;

    /* Main event loop */
    while (1) {
        /* Process LVGL tasks */
        lv_tick_inc(TICK_MS);
        lv_timer_handler();

        /* Update joystick with smooth interpolation */
        update_joystick_smooth(TICK_MS / 1000.0f);

        /* Tick sensor simulation */
        scout_sensors_tick(TICK_MS);
        sensor_timer += TICK_MS;

        /* Update UI with fresh sensor data */
        if (sensor_timer >= UPDATE_MS) {
            sensor_timer = 0;
            scout_sensors_read(&sensor_data);
            alert_level_t alert = scout_evaluate_alerts(&sensor_data);
            ui_dashboard_update(&sensor_data, alert, &joy_input);
        }

        /* Frame delay to maintain 60 fps */
        usleep(TICK_MS * 1000);
    }

    return 0;
}
