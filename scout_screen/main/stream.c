#include "stream.h"
#include "udp.h"
#include "jpeg.h"
#include "rc_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include <inttypes.h>
#include <string.h>
#include <assert.h>

// Receives JPEG frames over UDP, reassembling the fragments the camera sends
// (packet layout lives in cam/udp_stream.c). A completed frame is handed to the
// render task through two ping-pong buffers: the receiver fills one while the
// decoder reads the other, so decoding never stalls the socket.
//
// UDP has no connection, so stream_is_connected() reports liveness from how
// recently a full frame arrived.

#define LIVENESS_MS 2000

static const char *TAG = "stream";

static uint8_t          *s_asm_buf;
static uint8_t          *s_dec_buf;
static uint32_t          s_dec_len;
static bool              s_new_frame;
static bool              s_decoding;
static SemaphoreHandle_t s_frame_mutex;

static int                s_sock = -1;
static struct sockaddr_in s_cam_addr;
static bool               s_cam_known;
static SemaphoreHandle_t  s_cam_mutex;
static volatile uint32_t  s_last_rx_ms;  // 32-bit so the render task reads it atomically

static volatile uint32_t s_frame_count      = 0;
static volatile uint32_t s_last_frame_bytes = 0;
static volatile int32_t  s_last_transfer_ms = 0;
static volatile int32_t  s_last_decode_ms   = 0;

static struct {
    uint16_t seq;
    uint8_t  frags;         // 0 when no frame is in progress
    uint64_t rx_mask;       // bit N set once fragment N has arrived
    uint32_t frame_len;
} s_rx;

// Private functions

static void learn_cam_addr(const struct sockaddr_in *src)
{
    xSemaphoreTake(s_cam_mutex, portMAX_DELAY);
    s_cam_addr          = *src;
    s_cam_addr.sin_port = htons(CMD_PORT);
    s_cam_known         = true;
    xSemaphoreGive(s_cam_mutex);
    ESP_LOGI(TAG, "camera at %s", udp_ip_str(src));
}

static void begin_frame(uint16_t seq, uint8_t frags)
{
    s_rx.seq       = seq;
    s_rx.frags     = frags;
    s_rx.rx_mask   = 0;
    s_rx.frame_len = 0;
}

// Swaps the freshly assembled frame in for decoding, unless the decoder is still
// busy with the previous one — in that case we drop it and wait for the next.
static void publish_frame(void)
{
    xSemaphoreTake(s_frame_mutex, portMAX_DELAY);
    if(!s_decoding) {
        uint8_t *tmp = s_dec_buf;
        s_dec_buf   = s_asm_buf;
        s_asm_buf   = tmp;
        s_dec_len   = s_rx.frame_len;
        s_new_frame = true;
    }
    xSemaphoreGive(s_frame_mutex);
}

static void udp_server_task(void *arg)
{
    int sock = udp_open(VID_PORT);
    if(sock < 0) {
        vTaskDelete(NULL);
        return;
    }
    udp_set_rcvbuf(sock, 48 * 1024);
    s_sock = sock;
    ESP_LOGI(TAG, "UDP video server on port %d", VID_PORT);

    static uint8_t pkt[PKT_MAX];

    while(1) {
        struct sockaddr_in src;
        int n = udp_rx(sock, pkt, sizeof(pkt), &src);
        if(n < 4) continue;

        uint16_t seq;
        memcpy(&seq, pkt, 2);
        seq = ntohs(seq);
        uint8_t fi    = pkt[2];
        uint8_t frags = pkt[3];
        if(frags == 0 || fi >= frags || frags > MAX_FRAGS) continue;

        if(!s_cam_known) learn_cam_addr(&src);

        static int64_t transfer_t = 0;
        if(s_rx.frags == 0 || seq != s_rx.seq || frags != s_rx.frags) {
            begin_frame(seq, frags);
            transfer_t = esp_timer_get_time();
        }

        const uint8_t *data     = pkt + 4;
        int            data_len = n - 4;
        uint32_t       offset;

        if(fi == 0) {
            if(data_len < 5 || data[0] != FRAME_MAGIC) continue;
            uint32_t flen_be;
            memcpy(&flen_be, data + 1, 4);
            s_rx.frame_len = ntohl(flen_be);
            if(s_rx.frame_len > FRAME_MAX) {
                s_rx.frags = 0;
                continue;
            }
            data     += 5;
            data_len -= 5;
            offset = 0;
        } else {
            offset = FIRST_DATA + (uint32_t)(fi - 1) * FRAG_SIZE;
        }

        if((uint32_t)data_len > FRAME_MAX - offset) continue;
        memcpy(s_asm_buf + offset, data, data_len);
        s_rx.rx_mask |= (1ULL << fi);

        if(s_rx.rx_mask != ((1ULL << frags) - 1)) continue;

        int64_t transfer_ms = (esp_timer_get_time() - transfer_t) / 1000;
        ESP_LOGI(TAG, "%"PRIu32" bytes in %"PRId64"ms", s_rx.frame_len, transfer_ms);
        s_last_frame_bytes = s_rx.frame_len;
        s_last_transfer_ms = (int32_t)transfer_ms;
        publish_frame();
        s_rx.frags   = 0;
        s_last_rx_ms = (uint32_t)(esp_timer_get_time() / 1000);
    }
}

// Public functions

void stream_send_cmd(uint8_t cmd)
{
    xSemaphoreTake(s_cam_mutex, portMAX_DELAY);
    bool known = s_cam_known;
    struct sockaddr_in addr = s_cam_addr;
    xSemaphoreGive(s_cam_mutex);

    if(!known || s_sock < 0) return;
    udp_tx(s_sock, &addr, &cmd, 1);
}

bool stream_is_connected(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000) - s_last_rx_ms < LIVENESS_MS;
}

// Decodes the most recently published frame into out_buf. The frame mutex is
// only held while swapping pointers, never during the decode itself.
bool stream_try_decode(uint8_t *out_buf, size_t out_size)
{
    if(xSemaphoreTake(s_frame_mutex, 0) != pdTRUE) return false;
    if(!s_new_frame) { xSemaphoreGive(s_frame_mutex); return false; }

    s_new_frame = false;
    s_decoding  = true;
    uint8_t  *src = s_dec_buf;
    uint32_t  len = s_dec_len;
    int64_t   t   = esp_timer_get_time();
    xSemaphoreGive(s_frame_mutex);

    uint16_t w, h;
    bool ok = jpeg_decode_rgb565(src, (int)len, out_buf, out_size, &w, &h);
    int64_t decode_ms = (esp_timer_get_time() - t) / 1000;

    xSemaphoreTake(s_frame_mutex, portMAX_DELAY);
    s_decoding = false;
    xSemaphoreGive(s_frame_mutex);

    if(!ok) return false;
    ESP_LOGI(TAG, "decoded %"PRIu16"x%"PRIu16" in %"PRId64"ms (%"PRIu32" bytes)",
             w, h, decode_ms, len);
    s_last_decode_ms = (int32_t)decode_ms;
    s_frame_count++;
    return true;
}

void stream_get_stats(stream_stats_t *out)
{
    out->frame_count      = s_frame_count;
    out->last_frame_bytes = s_last_frame_bytes;
    out->last_transfer_ms = s_last_transfer_ms;
    out->last_decode_ms   = s_last_decode_ms;
}

void stream_init(void)
{
    s_asm_buf = heap_caps_malloc(FRAME_MAX, MALLOC_CAP_SPIRAM);
    s_dec_buf = heap_caps_malloc(FRAME_MAX, MALLOC_CAP_SPIRAM);
    assert(s_asm_buf && s_dec_buf);

    s_frame_mutex = xSemaphoreCreateMutex();
    s_cam_mutex   = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(udp_server_task, "udp_server", 4096, NULL, 5, NULL, 0);
}
