#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Allocates PSRAM ping-pong buffers and packet receive buffer; creates the frame mutex.
void     frame_pool_init(void);

// Registers the render task to be notified via xTaskNotify when a frame is published.
void     frame_pool_set_render_task(TaskHandle_t task);

// Returns a pointer to the assembly buffer for stream_run to write fragments into.
uint8_t *frame_pool_asm(void);

// Returns the packet receive buffer used by udp_rx inside stream_run.
uint8_t *frame_pool_pkt(void);

// Swaps the assembled frame in for decoding when the decoder is free; drops it otherwise.
void     frame_pool_publish(uint32_t len);

// Acquires the latest published frame for decoding. Sets *src and *len, marks the buffer
// as in-use so publish cannot overwrite it. Returns false if no new frame is ready.
// Must be followed by exactly one frame_pool_release() call.
bool     frame_pool_try_acquire(const uint8_t **src, uint32_t *len);

// Releases the decoding lock so the next published frame can be swapped in.
void     frame_pool_release(void);

