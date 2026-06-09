#pragma once

// Initialises the L298N GPIO pins, stops the motors, creates the command queue,
// and spawns the motor task.
void motor_init(void);
