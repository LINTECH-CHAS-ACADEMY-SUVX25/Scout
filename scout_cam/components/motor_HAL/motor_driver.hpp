#pragma once
#include "esp_err.h"
#include <stdint.h>

// Low-level L298N H-bridge driver for the AI-Thinker ESP32-CAM board.
// ENA and ENB are wired together to GPIO 1 (UART TX — logging stops after init).
// Speed is set via the LEDC duty cycle; direction via IN1-IN4.

class MotorDriver
{
public:
	esp_err_t init();

	// Applies a CMD_* direction bitmask (rc_protocol.h) at speed 0-255.
	// Conflicting directions (e.g. forward+backward) cancel each other out.
	esp_err_t drive(uint8_t cmd, uint8_t speed);

	esp_err_t stop();

private:
	esp_err_t set_speed(uint8_t speed);
	void set_direction(uint8_t cmd);
};
