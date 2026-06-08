#include "frame_buf.h"
#include "jpeg.h"
#include "rc_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include <inttypes.h>
#include <assert.h>

// Ping-pong frame buffer and stream statistics for the stream → render pipeline.
// stream_run fills s_asm_buf one fragment at a time; frame_buf_publish swaps it with
// s_dec_buf so the render task can decode without stalling the receive loop.
// If the decoder is still busy when a new frame arrives, the new frame is dropped.
// Rolling averages (transfer, decode, frame size, FPS) use fixed-size ring buffers
// so monitor_run can read representative stats without affecting the hot path.

#define LIVENESS_MS  2000
#define STAT_WINDOW  128   // must be a power of 2

static const char *TAG = "frame_buf";

static uint8_t          *s_asm_buf;
static uint8_t          *s_dec_buf;
static uint8_t          *s_pkt;
static uint32_t          s_dec_len;
static bool              s_new_frame;
static bool              s_decoding;
static SemaphoreHandle_t s_frame_mutex;

static volatile uint32_t s_last_rx_ms;
static volatile uint32_t s_frame_count;
static volatile uint32_t s_last_frame_bytes;
static volatile int32_t  s_last_transfer_ms;
static volatile int32_t  s_last_decode_ms;

// Rolling averages — separate rings so stream and render tasks never share an index
static int32_t  s_transfer_hist[STAT_WINDOW];
static int64_t  s_transfer_sum;
static uint32_t s_transfer_idx;
static uint32_t s_transfer_count;
static int32_t  s_avg_transfer_ms;

static int32_t  s_decode_hist[STAT_WINDOW];
static int64_t  s_decode_sum;
static uint32_t s_decode_idx;
static uint32_t s_decode_count;
static int32_t  s_avg_decode_ms;

static uint32_t s_bytes_hist[STAT_WINDOW];
static uint64_t s_bytes_sum;
static uint32_t s_bytes_idx;
static uint32_t s_bytes_count;
static uint32_t s_avg_frame_bytes;

static uint32_t s_interval_hist[STAT_WINDOW];
static uint64_t s_interval_sum;
static uint32_t s_interval_idx;
static uint32_t s_interval_count;
static uint32_t s_prev_rx_ms;
static uint32_t s_fps_tenths;

void frame_buf_init(void)
{
    s_asm_buf     = heap_caps_malloc(FRAME_MAX,  MALLOC_CAP_SPIRAM);
    s_dec_buf     = heap_caps_malloc(FRAME_MAX,  MALLOC_CAP_SPIRAM);
    s_pkt         = heap_caps_malloc(PKT_MAX,    MALLOC_CAP_INTERNAL);
    s_frame_mutex = xSemaphoreCreateMutex();
    assert(s_asm_buf && s_dec_buf && s_pkt && s_frame_mutex);
}

uint8_t *frame_buf_asm(void) { return s_asm_buf; }
uint8_t *frame_buf_pkt(void) { return s_pkt;     }

void frame_buf_publish(uint32_t len, int32_t transfer_ms)
{
    s_last_frame_bytes = len;
    s_last_transfer_ms = transfer_ms;
    s_last_rx_ms       = (uint32_t)(esp_timer_get_time() / 1000);
    ESP_LOGD(TAG, "%"PRIu32" bytes in %"PRId32"ms", len, transfer_ms);

    s_transfer_sum -= s_transfer_hist[s_transfer_idx];
    s_transfer_hist[s_transfer_idx] = transfer_ms;
    s_transfer_sum += transfer_ms;
    s_transfer_idx = (s_transfer_idx + 1) & (STAT_WINDOW - 1);
    if(s_transfer_count < STAT_WINDOW) s_transfer_count++;
    s_avg_transfer_ms = (int32_t)(s_transfer_sum / s_transfer_count);

    s_bytes_sum -= s_bytes_hist[s_bytes_idx];
    s_bytes_hist[s_bytes_idx] = len;
    s_bytes_sum += len;
    s_bytes_idx = (s_bytes_idx + 1) & (STAT_WINDOW - 1);
    if(s_bytes_count < STAT_WINDOW) s_bytes_count++;
    s_avg_frame_bytes = (uint32_t)(s_bytes_sum / s_bytes_count);

    if(s_prev_rx_ms != 0) {
        uint32_t delta = s_last_rx_ms - s_prev_rx_ms;
        s_interval_sum -= s_interval_hist[s_interval_idx];
        s_interval_hist[s_interval_idx] = delta;
        s_interval_sum += delta;
        s_interval_idx = (s_interval_idx + 1) & (STAT_WINDOW - 1);
        if(s_interval_count < STAT_WINDOW) s_interval_count++;
        // fps × 10 = 10000 × count / sum_ms  (avoids float, gives one decimal place)
        if(s_interval_sum > 0)
            s_fps_tenths = (uint32_t)(10000ULL * s_interval_count / s_interval_sum);
    }
    s_prev_rx_ms = s_last_rx_ms;

    xSemaphoreTake(s_frame_mutex, portMAX_DELAY);
    if(!s_decoding) {
        uint8_t *tmp = s_dec_buf;
        s_dec_buf    = s_asm_buf;
        s_asm_buf    = tmp;
        s_dec_len    = len;
        s_new_frame  = true;
    }
    xSemaphoreGive(s_frame_mutex);
}

bool frame_buf_try_decode(uint8_t *out_buf, size_t out_size)
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
    ESP_LOGD(TAG, "decoded %"PRIu16"x%"PRIu16" in %"PRId64"ms (%"PRIu32" bytes)",
             w, h, decode_ms, len);
    s_last_decode_ms = (int32_t)decode_ms;
    s_frame_count++;

    s_decode_sum -= s_decode_hist[s_decode_idx];
    s_decode_hist[s_decode_idx] = (int32_t)decode_ms;
    s_decode_sum += (int32_t)decode_ms;
    s_decode_idx = (s_decode_idx + 1) & (STAT_WINDOW - 1);
    if(s_decode_count < STAT_WINDOW) s_decode_count++;
    s_avg_decode_ms = (int32_t)(s_decode_sum / s_decode_count);

    return true;
}

bool frame_buf_is_connected(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000) - s_last_rx_ms < LIVENESS_MS;
}

void frame_buf_get_stats(stream_stats_t *out)
{
    out->frame_count      = s_frame_count;
    out->last_frame_bytes = s_last_frame_bytes;
    out->last_transfer_ms = s_last_transfer_ms;
    out->last_decode_ms   = s_last_decode_ms;
    out->avg_frame_bytes  = s_avg_frame_bytes;
    out->avg_transfer_ms  = s_avg_transfer_ms;
    out->avg_decode_ms    = s_avg_decode_ms;
    out->fps_tenths       = s_fps_tenths;
}
