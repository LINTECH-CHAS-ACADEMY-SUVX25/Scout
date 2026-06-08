#include "render.hpp"
#include "IDisplay.hpp"

extern "C" {
#include "stream.h"
#include "ui.h"
#include "rc_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
}

#include <inttypes.h>
#include <string.h>

// Drives the render loop on core 1. Each tick: updates LVGL, renders the LVGL
// frame, sends the current RC command, and blits any newly decoded camera frame.
// render never includes display.h — it draws through IDisplay.

static const char *TAG = "render";

static constexpr int SCREEN_W = 1024;
static constexpr int SCREEN_H = 600;
static constexpr int CAM_W    = 640;
static constexpr int CAM_H    = 480;
static constexpr int CAM_X    = (SCREEN_W - CAM_W) / 2;
static constexpr int CAM_Y    = (SCREEN_H - CAM_H) / 2;

static IDisplay  *s_display;
static uint16_t  *s_canvas_buf;
static bool       s_has_frame = false;

// Helpers

static const char *cmd_str(uint8_t c)
{
    if(c == CMD_STOP) return "STOP";
    static char buf[32];
    int pos = 0;
    if(c & CMD_FORWARD)  pos += sprintf(buf + pos, "FWD ");
    if(c & CMD_BACKWARD) pos += sprintf(buf + pos, "BWD ");
    if(c & CMD_LEFT)     pos += sprintf(buf + pos, "LEFT ");
    if(c & CMD_RIGHT)    pos += sprintf(buf + pos, "RIGHT ");
    if(pos > 0) buf[pos - 1] = '\0';
    return buf;
}

static void send_rc_command(uint8_t *last_cmd)
{
    uint8_t c = ui_get_cmd();
    if(c != *last_cmd) {
        ESP_LOGI(TAG, "RC cmd: %s (0x%02x)", cmd_str(c), c);
        *last_cmd = c;
    }
    stream_send_cmd(c);
}

static void blit_camera_frame(void)
{
    s_display->drawBitmap(CAM_X, CAM_Y, CAM_X + CAM_W, CAM_Y + CAM_H, s_canvas_buf);
}

static void clear_camera_area(void)
{
    memset(s_canvas_buf, 0, CAM_W * CAM_H * sizeof(uint16_t));
    s_display->drawBitmap(CAM_X, CAM_Y, CAM_X + CAM_W, CAM_Y + CAM_H, s_canvas_buf);
    s_has_frame = false;
}

static void try_decode_frame(void)
{
    if(stream_try_decode((uint8_t *)s_canvas_buf, CAM_W * CAM_H * sizeof(uint16_t)))
        s_has_frame = true;
    if(s_has_frame)
        blit_camera_frame();
}

// Render task

static void render_task(void *arg)
{
    (void)arg;
    uint8_t last_cmd      = CMD_STOP;
    bool    was_connected = false;

    while(1) {
        bool connected = stream_is_connected();
        if(connected != was_connected) {
            ui_set_connected(connected);
            if(!connected)
                clear_camera_area();
        }
        was_connected = connected;

        ui_tick();

        int64_t render_t = esp_timer_get_time();
        ui_render_frame();
        int64_t render_ms = (esp_timer_get_time() - render_t) / 1000;
        if(render_ms > 5)
            ESP_LOGI(TAG, "lvgl frame rendered in %" PRId64 "ms", render_ms);

        send_rc_command(&last_cmd);
        try_decode_frame();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Init

void render_init(IDisplay *display)
{
    s_display    = display;
    s_canvas_buf = static_cast<uint16_t *>(
        heap_caps_aligned_alloc(16, CAM_W * CAM_H * sizeof(uint16_t), MALLOC_CAP_SPIRAM));
    assert(s_canvas_buf);
    xTaskCreatePinnedToCore(render_task, "render", 8192, NULL, 4, NULL, 1);
}
