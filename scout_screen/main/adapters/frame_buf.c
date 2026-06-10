#include "frame_buf.h"
#include "rc_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include <assert.h>

// Ping-pong frame buffer for the stream → render pipeline.
// stream_run fills s_asm_buf one fragment at a time; frame_buf_publish swaps it with
// s_dec_buf so the render task can decode without stalling the receive loop.
// If the decoder is still busy when a new frame arrives, the new frame is dropped.

#define LIVENESS_MS  2000

static uint8_t          *s_asm_buf;
static uint8_t          *s_dec_buf;
static uint8_t          *s_pkt;
static uint32_t          s_dec_len;
static bool              s_new_frame;
static bool              s_decoding;
static SemaphoreHandle_t s_frame_mutex;

static volatile uint32_t s_last_rx_ms;


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

void frame_buf_publish(uint32_t now_ms, uint32_t len)
{
    s_last_rx_ms = now_ms;

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

bool frame_buf_try_acquire(const uint8_t **src, uint32_t *len)
{
    if(xSemaphoreTake(s_frame_mutex, 0) != pdTRUE) return false;
    if(!s_new_frame) { xSemaphoreGive(s_frame_mutex); return false; }

    s_new_frame = false;
    s_decoding  = true;
    *src = s_dec_buf;
    *len = s_dec_len;
    xSemaphoreGive(s_frame_mutex);
    return true;
}

void frame_buf_release(void)
{
    xSemaphoreTake(s_frame_mutex, portMAX_DELAY);
    s_decoding = false;
    xSemaphoreGive(s_frame_mutex);
}

bool frame_buf_is_streaming(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000) - s_last_rx_ms < LIVENESS_MS;
}
