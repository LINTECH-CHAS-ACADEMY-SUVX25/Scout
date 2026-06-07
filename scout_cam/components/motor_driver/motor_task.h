#pragma once
#include <stdint.h>

// Creates the motor queue and starts the motor task.
void motor_task_start(void);

// Enqueues a CMD_* byte for the motor task. Safe to call from any task.
// Drops the command and logs a warning if the queue is full.
void motor_cmd_send(uint8_t cmd);
