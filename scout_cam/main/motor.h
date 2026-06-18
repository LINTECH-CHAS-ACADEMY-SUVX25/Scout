#pragma once
#include <stdint.h>

/** @brief Inits the L298N GPIO pins, stops the motors, creates the command queue, and spawns the motor task. */
void motor_init(void);

/** @brief Converts joystick position to direction bitmask and speed (0-255). */
void joy_to_motor(int16_t x, int16_t y, uint8_t *cmd, uint8_t *speed);
