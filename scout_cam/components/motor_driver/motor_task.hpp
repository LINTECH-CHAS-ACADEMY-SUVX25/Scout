#pragma once
#include "IMotor.hpp"
#include <stdint.h>

// Creates the motor queue and starts the motor task.
// The task calls motor->apply() — it never touches hardware directly.
void motor_task_start(IMotor *motor);

// Enqueues a CMD_* byte for the motor task. Safe to call from any task.
// Drops the command and logs a warning if the queue is full.
void motor_cmd_send(uint8_t cmd);

// Returns the last CMD_* byte applied to the motors.
uint8_t motor_task_get_last_cmd(void);
