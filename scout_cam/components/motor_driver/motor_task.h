#pragma once

// Creates the motor command queue.
void motor_task_init(void);

// Passed directly to xTaskCreate — do not call from application code.
void motor_task_run(void *arg);
