#include "render.h"
#include "frame_buf.h"
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
#include <inttypes.h>

// Task — drives the display on core 1.
// Each tick: updates LVGL, renders the LVGL frame, sends the current RC command,
// and blits any newly decoded camera frame into the framebuffer.
// Exists to keep all display and RC output on a dedicated core away from networking.

#define CAM_X  ((SCREEN_W - CAM_W) / 2)
#define CAM_Y  ((SCREEN_H - CAM_H) / 2)

static const char *TAG = "render";

static void render_run(void *arg);

void render_init(void)
{
    jpeg_init_canvas(CAM_W, CAM_H);
    xTaskCreatePinnedToCore(render_run, "render", 8192, NULL, 4, NULL, 1);
    ESP_LOGI(TAG, "canvas %dx%d allocated", CAM_W, CAM_H);
}

static void render_run(void *arg)
{
    uint8_t last_cmd      = CMD_STOP;
    bool    was_connected = false;
    int64_t last_cmd_us   = 0;

    watchdog_register();

    while(1) {
        watchdog_reset();

        bool connected = frame_buf_is_connected();
        if(connected != was_connected) {
            lvgl_port_ui_update(connected);
            if(!connected) display_clear_region(CAM_X, CAM_Y, CAM_W, CAM_H);
        }
        was_connected = connected;

        int64_t t = esp_timer_get_time();
        lvgl_port_render_frame();
        int64_t ms = (esp_timer_get_time() - t) / 1000;
        frame_buf_record_lvgl((int32_t)ms);

        uint8_t c   = lvgl_port_get_cmd();
        int64_t now = esp_timer_get_time();
        if(c != last_cmd) {
            ESP_LOGD(TAG, "RC cmd: 0x%02x", c);
            last_cmd = c;
            cam_cmd_send(c);
            last_cmd_us = now;
        } else if(now - last_cmd_us >= 200000) {
            cam_cmd_send(c);
            last_cmd_us = now;
        }

        // Only blit when a new frame was decoded. LVGL redraws just its dirty areas
        // which don't overlap the camera region, so the last
        // frame persists in the framebuffer between decodes — no need to re-blit.
        if(frame_buf_try_decode((uint8_t *)jpeg_canvas_get(),
                                CAM_W * CAM_H * sizeof(uint16_t))) {
            int64_t tb = esp_timer_get_time();
            display_blit_region(CAM_X, CAM_Y, CAM_W, CAM_H, jpeg_canvas_get());
            frame_buf_record_blit((int32_t)((esp_timer_get_time() - tb) / 1000));
            frame_buf_record_disp_frame();
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
