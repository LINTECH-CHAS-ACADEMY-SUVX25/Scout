#include "video.h"
#include "ui.h"
#include "rc_protocol.h"
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "lvgl.h"
#include "jpeg_decoder.h"

static const char *TAG = "video";

#define CAM_W 480
#define CAM_H 320

static EXT_RAM_BSS_ATTR lv_color_t s_canvas_buf[CAM_W * CAM_H];
static EXT_RAM_BSS_ATTR uint8_t    s_frame[200 * 1024];
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

static void handler_task(void *arg)
{
    int cmd_tick = 0;
    while (1) {
        lv_timer_handler();

        if (++cmd_tick >= 5) {
            cmd_tick = 0;
            if (xSemaphoreTake(s_sock_mutex, 0) == pdTRUE) {
                int sock = s_client_sock;
                xSemaphoreGive(s_sock_mutex);
                if (sock >= 0) {
                    uint8_t c = ui_get_cmd();
                    send(sock, &c, 1, MSG_DONTWAIT);
                }
            }
        }

        if (xSemaphoreTake(s_frame_mutex, 0) == pdTRUE) {
            if (s_new_frame) {
                s_new_frame = false;
                uint32_t len = s_frame_len;
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
                xSemaphoreGive(s_frame_mutex);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "JPEG decode failed: %s", esp_err_to_name(err));
                } else {
                    ESP_LOGD(TAG, "Decoded %"PRIu32"x%"PRIu32, out.width, out.height);
                    lv_obj_invalidate((lv_obj_t *)arg);
                    lv_timer_handler();
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
        ESP_LOGE(TAG, "socket() failed");
        vTaskDelete(NULL);
        return;
    }
    if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        ESP_LOGE(TAG, "bind() failed");
        close(server);
        vTaskDelete(NULL);
        return;
    }
    if (listen(server, 1) != 0) {
        ESP_LOGE(TAG, "listen() failed");
        close(server);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "TCP server ready on port %d", VID_PORT);

    while (1) {
        int client = accept(server, NULL, NULL);
        if (client < 0) continue;

        xSemaphoreTake(s_sock_mutex, portMAX_DELAY);
        s_client_sock = client;
        xSemaphoreGive(s_sock_mutex);
        ESP_LOGI(TAG, "Camera connected");

        while (1) {
            uint8_t magic;
            if (recv_all(client, &magic, 1) != ESP_OK) break;
            if (magic != FRAME_MAGIC) {
                ESP_LOGE(TAG, "Bad frame magic: 0x%02x", magic);
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
                if (!ok) break;
            }
        }

        xSemaphoreTake(s_sock_mutex, portMAX_DELAY);
        s_client_sock = -1;
        xSemaphoreGive(s_sock_mutex);
        close(client);
        ESP_LOGW(TAG, "Camera disconnected");
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
