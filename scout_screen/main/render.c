#include "render.h"
#include "frame_buf.h"
#include "screen_state.h"
#include "cam_cmd.h"
#include "lvgl_port.h"
#include "display.h"
#include "jpeg.h"
#include "watchdog.h"
#include "rc_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

// Task — drives the display on core 1.
// Each tick: updates LVGL, renders the LVGL frame, sends the current RC command,
// and blits any newly decoded camera frame into the framebuffer.
// Exists to keep all display and RC output on a dedicated core away from networking.

#define CAM_X  ((SCREEN_W - CAM_W) / 2)
#define CAM_Y  ((SCREEN_H - CAM_H) / 2)

static const char    *TAG    = "render";
static screen_tick_t  s_tick = {0};

static void render_run(void *arg);

void render_init(void)
{
    jpeg_init_canvas(CAM_W, CAM_H);
    screen_state_render_tick_init(&s_tick);
    xTaskCreatePinnedToCore(render_run, "render", 8192, NULL, 4, NULL, 1);
    ESP_LOGI(TAG, "canvas %dx%d allocated", CAM_W, CAM_H);
}

static void render_run(void *arg)
{
    uint8_t last_cmd      = CMD_STOP;
    bool    was_connected = false;
    int64_t last_cmd_ms   = 0;

    watchdog_register();

    while(1) {
        watchdog_reset();
        screen_state_tick(&s_tick);

        bool cam_connected = screen_status.cam_connected;
        if(cam_connected != was_connected) {
            lvgl_port_ui_update(cam_connected);
            // TODO: trigger LVGL disconnected scene over camera region
        }
        was_connected = cam_connected;

        lvgl_port_render_frame();
        screen_state_tick_split(&s_tick, &s_tick.lvgl);

        // TODO: return x/y joystick values (-255..255) and map to CMD + PWM strength
        uint8_t c      = lvgl_port_get_cmd();
        int64_t now_ms = esp_timer_get_time() / 1000;
        if(c != last_cmd) {
            ESP_LOGD(TAG, "RC cmd: 0x%02x", c);
            last_cmd    = c;
            cam_cmd_send(c);
            last_cmd_ms = now_ms;
        } else if(now_ms - last_cmd_ms >= 200) {
            cam_cmd_send(c);
            last_cmd_ms = now_ms;
        }

        // Only blit when a new frame was decoded. LVGL redraws just its dirty areas
        // which don't overlap the camera region, so the last frame persists between decodes.
        const uint8_t *src;
        uint32_t       src_len;
        if(frame_buf_try_acquire(&src, &src_len)) {
            bool ok = jpeg_decode_rgb565(src, (int)src_len,
                                         (uint8_t *)jpeg_canvas_get(),
                                         CAM_W * CAM_H * sizeof(uint16_t), NULL, NULL);
            screen_state_tick_split(&s_tick, &s_tick.decode);
            frame_buf_release();

            if(ok) {
                display_blit_region(CAM_X, CAM_Y, CAM_W, CAM_H, jpeg_canvas_get());
                screen_state_tick_split(&s_tick, &s_tick.blit);
            }
        }

        vTaskDelay(screen_status.streaming ? 1 : pdMS_TO_TICKS(20));
    }
}
