#include "render.h"
#include "stream.h"
#include "ui.h"
#include "display.h"
#include "rc_protocol.h"
#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "render";

#define CAM_W    640
#define CAM_H    480
#define SCREEN_W 1024
#define SCREEN_H 600
#define CAM_X    ((SCREEN_W - CAM_W) / 2)
#define CAM_Y    ((SCREEN_H - CAM_H) / 2)

static uint16_t *s_canvas_buf;
static bool     s_has_frame = false;

// ── Helpers ───────────────────────────────────────────────────────────────────

static const char *cmd_str(uint8_t c)
{
    if (c == CMD_STOP) return "STOP";
    static char buf[32];
    int pos = 0;
    if (c & CMD_FORWARD)  pos += sprintf(buf + pos, "FWD ");
    if (c & CMD_BACKWARD) pos += sprintf(buf + pos, "BWD ");
    if (c & CMD_LEFT)     pos += sprintf(buf + pos, "LEFT ");
    if (c & CMD_RIGHT)    pos += sprintf(buf + pos, "RIGHT ");
    if (pos > 0) buf[pos - 1] = '\0';
    return buf;
}

static void send_rc_command(uint8_t *last_cmd)
{
    uint8_t c = ui_get_cmd();
    if (c != *last_cmd) {
        ESP_LOGI(TAG, "[render] RC cmd: %s (0x%02x)", cmd_str(c), c);
        *last_cmd = c;
    }
    stream_send_cmd(c);
}

static void blit_camera_frame(void)
{
    uint8_t *fb = (uint8_t *)display_get_fb(0);
    for (int row = 0; row < CAM_H; row++) {
        memcpy(fb + ((CAM_Y + row) * SCREEN_W + CAM_X) * 2,
               (uint8_t *)s_canvas_buf + row * CAM_W * 2,
               CAM_W * 2);
    }
}

static void try_decode_frame(void)
{
    if (stream_try_decode((uint8_t *)s_canvas_buf, CAM_W * CAM_H * sizeof(uint16_t)))
        s_has_frame = true;
    if (s_has_frame)
        blit_camera_frame();
}

static void clear_camera_area(void)
{
    uint8_t *fb = (uint8_t *)display_get_fb(0);
    for (int row = 0; row < CAM_H; row++) {
        memset(fb + ((CAM_Y + row) * SCREEN_W + CAM_X) * 2, 0x11, CAM_W * 2);
    }
}

// ── Render task ───────────────────────────────────────────────────────────────

static void render_task(void *arg)
{
    (void)arg;
    uint8_t last_cmd      = 0xFF; // This is probably why scout_cam drives both motors backwards when establishing a connection. Change to 0x00 maybe
    bool    was_connected = false;

    while (1) {
        bool connected = stream_is_connected();
        if (connected != was_connected) {
            ui_set_connected(connected);
            if (!connected)
                clear_camera_area();
        }
        was_connected = connected;

        ui_tick();

        int64_t render_t = esp_timer_get_time();
        ui_render_frame();
        int64_t render_ms = (esp_timer_get_time() - render_t) / 1000;
        if (render_ms > 5)
            ESP_LOGI(TAG, "[lvgl] frame rendered in %"PRId64"ms", render_ms);
        
        send_rc_command(&last_cmd);
        try_decode_frame();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ── Init ──────────────────────────────────────────────────────────────────────

void render_init(void)
{
    s_canvas_buf = heap_caps_aligned_alloc(16, CAM_W * CAM_H * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    assert(s_canvas_buf);
    xTaskCreatePinnedToCore(render_task, "render", 8192, NULL, 4, NULL, 1);
}
