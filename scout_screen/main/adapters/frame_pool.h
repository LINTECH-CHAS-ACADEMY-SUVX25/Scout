#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void     frame_pool_init(void);

/**
 * @brief  Registers the render task to be notified via xTaskNotify when a frame is published.
 * @param  task  Handle of the render task.
 */
void     frame_pool_set_render_task(TaskHandle_t task);

/** @brief Returns a pointer to the assembly buffer for stream_run to write fragments into. */
uint8_t *frame_pool_asm(void);

uint8_t *frame_pool_pkt(void);

/**
 * @brief  Swaps the assembled frame in for decoding; drops it if the decoder is busy.
 * @param  len  Length of the assembled frame in bytes.
 */
void     frame_pool_publish(uint32_t len);

/**
 * @brief  Acquires the latest published frame for decoding.
 *         Marks the buffer as in-use so publish cannot overwrite it.
 * @param[out] src  Set to a pointer into the ping-pong buffer.
 * @param[out] len  Set to the frame length in bytes.
 * @return true if a new frame was acquired; false if none is ready.
 * @post   Must be followed by exactly one call to frame_pool_release().
 */
bool     frame_pool_try_acquire(const uint8_t **src, uint32_t *len);

void     frame_pool_release(void);
