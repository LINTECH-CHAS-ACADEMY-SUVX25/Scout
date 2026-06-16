#pragma once
#include <stdint.h>

/** @brief Configures the L298N direction GPIO pins and the LEDC PWM channel on GPIO 1. Sets motors to stopped. */
void l298n_init(void);

/**
 * @brief Sets motor speed (0–255) via LEDC duty cycle and applies CMD_* direction to IN1–IN4.
 *        Conflicting directions (e.g. forward+backward) cancel each other out.
 */
void l298n_apply(uint8_t cmd, uint8_t speed);
