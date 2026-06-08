#include "render.h"
#include "frame_buf.h"
#include "cam_cmd.h"
#include "lvgl_port.h"
#include "display.h"
#include "jpeg.h"
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

void render_init(void)
{
    jpeg_init_canvas(CAM_W, CAM_H);
    ESP_LOGI(TAG, "canvas %dx%d allocated", CAM_W, CAM_H);
}

void render_run(void *arg)
{
    uint8_t last_cmd      = CMD_STOP;
    bool    was_connected = false;
    bool    has_frame     = false;

    while(1) {
        bool connected = frame_buf_is_connected();
        if(connected != was_connected) {
            lvgl_port_ui_update(connected);
            if(!connected) display_clear_region(CAM_X, CAM_Y, CAM_W, CAM_H);
        }
        was_connected = connected;

        int64_t t = esp_timer_get_time();
        lvgl_port_render_frame();
        int64_t ms = (esp_timer_get_time() - t) / 1000;
        if(ms > 5) ESP_LOGD(TAG, "lvgl frame in %"PRId64"ms", ms);

        uint8_t c = lvgl_port_get_cmd();
        if(c != last_cmd) {
            ESP_LOGD(TAG, "RC cmd: 0x%02x", c);
            last_cmd = c;
        }
        cam_cmd_send(c);

        if(frame_buf_try_decode((uint8_t *)jpeg_canvas_get(), CAM_W * CAM_H * sizeof(uint16_t)))
            has_frame = true;
        if(has_frame)
            display_blit_region(CAM_X, CAM_Y, CAM_W, CAM_H, jpeg_canvas_get());

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
