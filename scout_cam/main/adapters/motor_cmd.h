#pragma once
#include "rc_protocol.h"
#include <stdint.h>
#include <stdbool.h>

// Creates the motor command queue. Call from motor_init.
void motor_cmd_init(void);

// Enqueues a joy_pkt_t. Safe to call from any task; drops and logs on full queue.
void motor_cmd_send(int16_t x, int16_t y);

// Blocks up to timeout_ms for a packet. Returns true if one was received.
// Call from motor_run only.
bool motor_cmd_recv(int16_t *x, int16_t *y, uint32_t timeout_ms);
