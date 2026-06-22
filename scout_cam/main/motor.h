#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Inits the L298N H-bridge HAL, stops the motors, creates the command queue, and spawns the motor task. */
void motor_init(void);

#ifdef __cplusplus
}
#endif
