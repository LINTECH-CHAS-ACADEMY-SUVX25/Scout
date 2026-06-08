#pragma once
#include <stdint.h>
#include <stdbool.h>

// Creates the motor command queue. Call from motor_task_init.
void motor_cmd_init(void);

// Enqueues a CMD_* byte. Safe to call from any task; drops and logs on full queue.
void motor_cmd_send(uint8_t cmd);

// Blocks up to timeout_ms for a command. Returns true if one was received.
// Call from motor_task_run only.
bool motor_cmd_recv(uint8_t *cmd, uint32_t timeout_ms);
