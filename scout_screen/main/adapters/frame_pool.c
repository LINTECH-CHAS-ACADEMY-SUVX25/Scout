#include "frame_pool.h"
#include "rc_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_heap_caps.h"
#include <assert.h>

// Ping-pong frame buffer for the stream → render pipeline.
// stream_run fills s_asm_buf one fragment at a time; frame_pool_publish swaps it with
// s_dec_buf so the render task can decode without stalling the receive loop.
// If the decoder is still busy when a new frame arrives, the new frame is dropped.

static uint8_t          *s_asm_buf;
static uint8_t          *s_dec_buf;
static uint8_t          *s_pkt;
static uint32_t          s_dec_len;
static bool              s_new_frame;
static bool              s_decoding;
static SemaphoreHandle_t s_frame_mutex;
static TaskHandle_t      s_render_task;


void frame_pool_init(void)
{
    s_asm_buf     = heap_caps_malloc(FRAME_MAX,  MALLOC_CAP_SPIRAM);
    s_dec_buf     = heap_caps_malloc(FRAME_MAX,  MALLOC_CAP_SPIRAM);
    s_pkt         = heap_caps_malloc(PKT_MAX,    MALLOC_CAP_INTERNAL);
    s_frame_mutex = xSemaphoreCreateMutex();
    assert(s_asm_buf && s_dec_buf && s_pkt && s_frame_mutex);
}

uint8_t *frame_pool_asm(void) { return s_asm_buf; }
uint8_t *frame_pool_pkt(void) { return s_pkt;     }

void frame_buf_pool_render_task(TaskHandle_t task) { s_render_task = task; }

void frame_pool_publish(uint32_t len)
{
    bool published = false;
    xSemaphoreTake(s_frame_mutex, portMAX_DELAY);
    if(!s_decoding) {
        uint8_t *tmp = s_dec_buf;
        s_dec_buf    = s_asm_buf;
        s_asm_buf    = tmp;
        s_dec_len    = len;
        s_new_frame  = true;
        published    = true;
    }
    xSemaphoreGive(s_frame_mutex);
    if(published && s_render_task)
        xTaskNotify(s_render_task, 0, eNoAction);
}

bool frame_pool_try_acquire(const uint8_t **src, uint32_t *len)
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

void frame_pool_release(void)
{
    xSemaphoreTake(s_frame_mutex, portMAX_DELAY);
    s_decoding = false;
    xSemaphoreGive(s_frame_mutex);
}

