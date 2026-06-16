#pragma once
#include "rc_protocol.h"
#include <stdint.h>
#include <stdbool.h>

void motor_queue_init(void);

/** @brief Enqueues a joystick packet. Thread-safe; drops and logs if the queue is full. */
void motor_queue_send(int16_t x, int16_t y);

/**
 * @brief Blocks up to timeout_ms for a packet.
 * @return true if a packet was received. Call from motor_run only.
 */
bool motor_queue_recv(int16_t *x, int16_t *y, uint32_t timeout_ms);
