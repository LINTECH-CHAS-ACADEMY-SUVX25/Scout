#include "stream.h"
#include "ui.h"
#include "rc_protocol.h"
#include <inttypes.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "lwip/sockets.h"
#include "jpeg_decoder.h"

static const char *TAG = "stream";

static uint8_t           s_frame[32 * 1024];
static uint32_t          s_frame_len   = 0;
static bool              s_new_frame   = false;
static SemaphoreHandle_t s_frame_mutex;
static SemaphoreHandle_t s_sock_mutex;
static int               s_client_sock = -1;

// ── Socket helpers ────────────────────────────────────────────────────────────

int stream_get_client_sock(void)
{
    xSemaphoreTake(s_sock_mutex, portMAX_DELAY);
    int sock = s_client_sock;
    xSemaphoreGive(s_sock_mutex);
    return sock;
}

static void set_client_sock(int sock)
{
    xSemaphoreTake(s_sock_mutex, portMAX_DELAY);
    s_client_sock = sock;
    xSemaphoreGive(s_sock_mutex);
}

// ── Receive helpers ───────────────────────────────────────────────────────────

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

static int create_tcp_server(uint16_t port)
{
    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons(port),
        .sin_addr.s_addr = INADDR_ANY,
    };
    int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server < 0) { ESP_LOGE(TAG, "[tcp] socket() failed"); return -1; }
    if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        ESP_LOGE(TAG, "[tcp] bind() failed"); close(server); return -1;
    }
    if (listen(server, 1) != 0) {
        ESP_LOGE(TAG, "[tcp] listen() failed"); close(server); return -1;
    }
    return server;
}

static bool recv_frame_header(int client, uint32_t *out_len)
{
    uint8_t magic;
    if (recv_all(client, &magic, 1) != ESP_OK) return false;
    if (magic != FRAME_MAGIC) {
        ESP_LOGE(TAG, "[tcp] Bad frame magic: 0x%02x", magic);
        return false;
    }
    uint32_t len_net;
    if (recv_all(client, &len_net, 4) != ESP_OK) return false;
    uint32_t len = ntohl(len_net);
    if (len > sizeof(s_frame)) return false;
    *out_len = len;
    return true;
}

// Receive frame bytes into s_frame. Caller must hold s_frame_mutex.
static bool try_store_frame(int client, uint32_t len)
{
    if (recv_all(client, s_frame, len) != ESP_OK) return false;
    s_frame_len = len;
    s_new_frame = true;
    return true;
}

// Drain and discard frame bytes when the decoder is busy.
static bool drain_frame(int client, uint32_t len)
{
    uint8_t sink[512];
    while (len > 0) {
        uint32_t n = len < sizeof(sink) ? len : sizeof(sink);
        if (recv_all(client, sink, n) != ESP_OK) return false;
        len -= n;
    }
    return true;
}

// ── JPEG decode (called from render task) ─────────────────────────────────────

bool stream_try_decode(uint8_t *out_buf, size_t out_size)
{
    if (xSemaphoreTake(s_frame_mutex, 0) != pdTRUE) return false;
    if (!s_new_frame) { xSemaphoreGive(s_frame_mutex); return false; }

    s_new_frame      = false;
    uint32_t len     = s_frame_len;
    int64_t  t       = esp_timer_get_time();

    esp_jpeg_image_cfg_t cfg = {
        .indata      = s_frame,
        .indata_size = len,
        .outbuf      = out_buf,
        .outbuf_size = out_size,
        .out_format  = JPEG_IMAGE_FORMAT_RGB565,
        .out_scale   = JPEG_IMAGE_SCALE_0,
    };
    esp_jpeg_image_output_t out;
    esp_err_t err = esp_jpeg_decode(&cfg, &out);
    int64_t decode_ms = (esp_timer_get_time() - t) / 1000;
    xSemaphoreGive(s_frame_mutex);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "[stream] JPEG decode failed: %s", esp_err_to_name(err));
        return false;
    }
    ESP_LOGI(TAG, "[stream] Decoded %"PRIu32"x%"PRIu32" in %"PRId64"ms (%"PRIu32" bytes)",
             (uint32_t)out.width, (uint32_t)out.height, decode_ms, len);
    return true;
}

// ── TCP server task ───────────────────────────────────────────────────────────

static void tcp_server_task(void *arg)
{
    int server = create_tcp_server(VID_PORT);
    if (server < 0) { vTaskDelete(NULL); return; }
    ESP_LOGI(TAG, "[tcp] Server ready on port %d", VID_PORT);

    while (1) {
        ESP_LOGI(TAG, "[tcp] Waiting for camera to connect...");
        int client = accept(server, NULL, NULL);
        if (client < 0) continue;

        set_client_sock(client);
        ui_set_connected(true);
        ESP_LOGI(TAG, "[tcp] Camera connected");

        uint32_t frames_rx = 0, frames_dropped = 0;
        uint32_t fps_count = 0;
        int64_t  fps_window = esp_timer_get_time();

        while (1) {
            uint32_t len;
            if (!recv_frame_header(client, &len)) break;

            if (xSemaphoreTake(s_frame_mutex, 0) == pdTRUE) {
                if (!try_store_frame(client, len)) { xSemaphoreGive(s_frame_mutex); break; }
                xSemaphoreGive(s_frame_mutex);
                frames_rx++;
                fps_count++;
            } else {
                if (!drain_frame(client, len)) break;
                frames_dropped++;
                ESP_LOGW(TAG, "[tcp] Frame dropped (decoder busy) — total: %"PRIu32"/%"PRIu32,
                         frames_dropped, frames_rx + frames_dropped);
            }

            if (esp_timer_get_time() - fps_window >= 1000000) {
                ui_set_fps(fps_count);
                fps_count  = 0;
                fps_window = esp_timer_get_time();
            }
        }

        set_client_sock(-1);
        close(client);
        ui_set_connected(false);
        ui_set_fps(0);
        ESP_LOGW(TAG, "[tcp] Camera disconnected — %"PRIu32" received, %"PRIu32" dropped",
                 frames_rx, frames_dropped);
    }
}

// ── Init ──────────────────────────────────────────────────────────────────────

void stream_init(void)
{
    s_frame_mutex = xSemaphoreCreateMutex();
    s_sock_mutex  = xSemaphoreCreateMutex();
    xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 3, NULL);
}
