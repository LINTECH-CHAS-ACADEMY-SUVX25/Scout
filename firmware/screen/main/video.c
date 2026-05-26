#include "video.h"
#include "ui.h"
#include "rc_protocol.h"
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "lwip/sockets.h"
#include "lvgl.h"
#include "jpeg_decoder.h"

static const char *TAG = "video";

#define CAM_W 240
#define CAM_H 176

static lv_color_t s_canvas_buf[CAM_W * CAM_H];
static uint8_t    s_frame[32 * 1024];
static uint32_t          s_frame_len  = 0;
static bool              s_new_frame  = false;
static SemaphoreHandle_t s_frame_mutex;
static SemaphoreHandle_t s_sock_mutex;
static int               s_client_sock = -1;

static esp_err_t recv_all(int sock, void *buf, size_t len)
{
    uint8_t *p = buf;
    while (len > 0) {
        int n = recv(sock, p, len, 0);
        if (n <= 0) return ESP_FAIL;
        p += n;
        len -= n;
    }
    return ESP_OK;
}

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

static void handler_task(void *arg)
{
    uint8_t last_cmd = 0xFF;
    while (1) {
        ui_tick();
        lv_timer_handler();

        if (xSemaphoreTake(s_sock_mutex, 0) == pdTRUE) {
            int sock = s_client_sock;
            xSemaphoreGive(s_sock_mutex);
            if (sock >= 0) {
                uint8_t c = ui_get_cmd();
                if (c != last_cmd) {
                    ESP_LOGI(TAG, "[handler] RC cmd: %s (0x%02x)", cmd_str(c), c);
                    last_cmd = c;
                }
                send(sock, &c, 1, MSG_DONTWAIT);
            }
        }

        if (xSemaphoreTake(s_frame_mutex, 0) == pdTRUE) {
            if (s_new_frame) {
                s_new_frame = false;
                uint32_t len = s_frame_len;
                int64_t t_decode = esp_timer_get_time();
                esp_jpeg_image_cfg_t cfg = {
                    .indata      = s_frame,
                    .indata_size = len,
                    .outbuf      = (uint8_t *)s_canvas_buf,
                    .outbuf_size = sizeof(s_canvas_buf),
                    .out_format  = JPEG_IMAGE_FORMAT_RGB565,
                    .out_scale   = JPEG_IMAGE_SCALE_0,
                };
                esp_jpeg_image_output_t out;
                esp_err_t err = esp_jpeg_decode(&cfg, &out);
                int64_t decode_ms = (esp_timer_get_time() - t_decode) / 1000;
                xSemaphoreGive(s_frame_mutex);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "[handler] JPEG decode failed: %s", esp_err_to_name(err));
                } else {
                    ESP_LOGI(TAG, "[handler] Decoded %"PRIu32"x%"PRIu32" in %"PRId64"ms (%"PRIu32" bytes)", (uint32_t)out.width, (uint32_t)out.height, decode_ms, len);
                    // Just mark the canvas dirty; the lv_timer_handler() at the
                    // top of the loop redraws it next iteration. Calling it again
                    // here forced a second full-screen refresh per frame.
                    lv_obj_invalidate((lv_obj_t *)arg);
                }
            } else {
                xSemaphoreGive(s_frame_mutex);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void tcp_server_task(void *arg)
{
    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons(VID_PORT),
        .sin_addr.s_addr = INADDR_ANY,
    };

    int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server < 0) {
        ESP_LOGE(TAG, "[tcp] socket() failed");
        vTaskDelete(NULL);
        return;
    }
    if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        ESP_LOGE(TAG, "[tcp] bind() failed");
        close(server);
        vTaskDelete(NULL);
        return;
    }
    if (listen(server, 1) != 0) {
        ESP_LOGE(TAG, "[tcp] listen() failed");
        close(server);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "[tcp] Server ready on port %d", VID_PORT);

    while (1) {
        ESP_LOGI(TAG, "[tcp] Waiting for camera to connect...");
        int client = accept(server, NULL, NULL);
        if (client < 0) continue;

        xSemaphoreTake(s_sock_mutex, portMAX_DELAY);
        s_client_sock = client;
        xSemaphoreGive(s_sock_mutex);
        ESP_LOGI(TAG, "[tcp] Camera connected");
        ui_set_connected(true);

        uint32_t frames_received = 0;
        uint32_t frames_dropped  = 0;
        uint32_t fps_count       = 0;
        int64_t  fps_window      = esp_timer_get_time();

        while (1) {
            uint8_t magic;
            if (recv_all(client, &magic, 1) != ESP_OK) break;
            if (magic != FRAME_MAGIC) {
                ESP_LOGE(TAG, "[tcp] Bad frame magic: 0x%02x", magic);
                break;
            }

            uint32_t len_net;
            if (recv_all(client, &len_net, 4) != ESP_OK) break;
            uint32_t len = ntohl(len_net);
            if (len > sizeof(s_frame)) break;

            if (xSemaphoreTake(s_frame_mutex, 0) == pdTRUE) {
                if (recv_all(client, s_frame, len) != ESP_OK) {
                    xSemaphoreGive(s_frame_mutex);
                    break;
                }
                s_frame_len = len;
                s_new_frame = true;
                xSemaphoreGive(s_frame_mutex);
                frames_received++;
                fps_count++;
                if (esp_timer_get_time() - fps_window >= 1000000) {
                    ui_set_fps(fps_count);
                    fps_count  = 0;
                    fps_window = esp_timer_get_time();
                }
                ESP_LOGD(TAG, "[tcp] Frame #%"PRIu32" queued (%"PRIu32" bytes)", frames_received, len);
            } else {
                // decoder busy — drain to stay in sync
                uint8_t sink[512];
                uint32_t rem = len;
                bool ok = true;
                while (rem > 0) {
                    uint32_t n = rem < sizeof(sink) ? rem : sizeof(sink);
                    if (recv_all(client, sink, n) != ESP_OK) { ok = false; break; }
                    rem -= n;
                }
                frames_dropped++;
                ESP_LOGW(TAG, "[tcp] Frame dropped (decoder busy) — total dropped: %"PRIu32"/%"PRIu32, frames_dropped, frames_received + frames_dropped);
                if (!ok) break;
            }
        }

        xSemaphoreTake(s_sock_mutex, portMAX_DELAY);
        s_client_sock = -1;
        xSemaphoreGive(s_sock_mutex);
        close(client);
        ui_set_connected(false);
        ui_set_fps(0);
        ESP_LOGW(TAG, "[tcp] Camera disconnected — %"PRIu32" received, %"PRIu32" dropped", frames_received, frames_dropped);
    }
}

void video_init(void)
{
    s_frame_mutex = xSemaphoreCreateMutex();
    s_sock_mutex  = xSemaphoreCreateMutex();

    lv_obj_t *canvas = lv_canvas_create(lv_scr_act());
    lv_canvas_set_buffer(canvas, s_canvas_buf, CAM_W, CAM_H, LV_IMG_CF_TRUE_COLOR);
    lv_obj_align(canvas, LV_ALIGN_CENTER, 0, 0);
    lv_canvas_fill_bg(canvas, lv_color_hex(0x111111), LV_OPA_COVER);

    xTaskCreate(handler_task,    "lvgl_handler", 8192, canvas, 4, NULL);
    xTaskCreate(tcp_server_task, "tcp_server",   4096, NULL,   3, NULL);
}