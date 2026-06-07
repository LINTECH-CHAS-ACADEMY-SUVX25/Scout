#pragma once
#include <stdint.h>

// Configures the L298N GPIO pins and sets the motor to stopped.
void motor_init(void);

// Applies a CMD_* bitmask to the L298N H-bridge outputs.
// Conflicting directions (e.g. forward+backward) cancel each other out.
void motor_apply(uint8_t cmd);
