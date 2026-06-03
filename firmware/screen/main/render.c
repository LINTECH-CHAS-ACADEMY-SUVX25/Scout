#include "render.h"
#include "stream.h"
#include "lvgl_port.h"
#include "ui.h"
#include "rc_protocol.h"
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "render";

#define CAM_W 240
#define CAM_H 176

static uint16_t s_canvas_buf[CAM_W * CAM_H];
static void    *s_canvas;

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

static void run_render_frame(void)
{
    int64_t render_t = esp_timer_get_time();
    lvgl_port_render_frame();
    int64_t render_ms = (esp_timer_get_time() - render_t) / 1000;
    if (render_ms > 5)
        ESP_LOGI(TAG, "[lvgl] frame in %"PRId64"ms", render_ms);
}

static void send_rc_command(uint8_t *last_cmd)
{
    int sock = stream_get_client_sock();
    if (sock < 0) return;
    uint8_t c = ui_get_cmd();
    if (c != *last_cmd) {
        ESP_LOGI(TAG, "[render] RC cmd: %s (0x%02x)", cmd_str(c), c);
        *last_cmd = c;
    }
    send(sock, &c, 1, MSG_DONTWAIT);
}

static void try_decode_frame(void)
{
    if (stream_try_decode((uint8_t *)s_canvas_buf, sizeof(s_canvas_buf)))
        lvgl_port_canvas_invalidate(s_canvas);
}

// ── Render task ───────────────────────────────────────────────────────────────

static void render_task(void *arg)
{
    (void)arg;
    uint8_t last_cmd = 0xFF;

    while (1) {
        ui_tick();
        run_render_frame();
        send_rc_command(&last_cmd);
        try_decode_frame();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ── Init ──────────────────────────────────────────────────────────────────────

void render_init(void)
{
    s_canvas = lvgl_port_create_video_canvas((uint8_t *)s_canvas_buf, CAM_W, CAM_H);
    xTaskCreate(render_task, "render", 8192, NULL, 4, NULL);
}
