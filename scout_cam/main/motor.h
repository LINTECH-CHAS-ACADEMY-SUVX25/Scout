#pragma once

// Initialises the L298N GPIO pins, stops the motors, and creates the command queue.
// Call before starting the motor task.
void motor_init(void);

// Passed directly to xTaskCreate — do not call from application code.
void motor_run(void *arg);
