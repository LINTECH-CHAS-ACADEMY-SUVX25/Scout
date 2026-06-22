#pragma once
#include "rc_protocol.h"
#include "esp_err.h"
#include "esp_log.h"
#include <stdint.h>
#include <math.h>

// High-level motor control. Wraps a hardware driver (TMotor) and translates
// joystick positions into direction/speed commands. The driver is held by
// reference so a mock can be injected in tests.
template <typename TMotor> class MotorController
{
public:
	explicit MotorController(TMotor &motor) : motor_(motor) {}

	void initMotor()
	{
		ESP_ERROR_CHECK(motor_.init());
		ESP_LOGI(TAG, "motor driver initialised");
	}

	// Drives the car from a joystick position. The deadzone is enforced on the
	// screen side, so x=0, y=0 maps to a full stop here.
	void drive(int16_t x, int16_t y)
	{
		uint8_t cmd, speed;
		joyToMotor(x, y, cmd, speed);

		if(motor_.drive(cmd, speed) != ESP_OK) {
			ESP_LOGE(TAG, "drive failed");
			return;
		}
		moving_ = (cmd != CMD_STOP);
	}

	void stop()
	{
		if(motor_.stop() != ESP_OK) {
			ESP_LOGE(TAG, "stop failed");
			return;
		}
		moving_ = false;
	}

	bool isMoving() const { return moving_; }

private:
	// Converts a joystick position to a CMD_* direction bitmask and speed (0-255).
	static void joyToMotor(int16_t x, int16_t y, uint8_t &cmd, uint8_t &speed)
	{
		cmd = CMD_STOP;
		if(y > 0) cmd |= CMD_FORWARD;
		if(y < 0) cmd |= CMD_BACKWARD;
		if(x < 0) cmd |= CMD_LEFT;
		if(x > 0) cmd |= CMD_RIGHT;

		if(cmd == CMD_STOP) {
			speed = 0;
			return;
		}
		float mag = sqrtf((float)(x * x + y * y));
		speed = (mag > 255.0f) ? 255 : (uint8_t)mag;
	}

	TMotor &motor_;
	bool moving_ = false;
	static constexpr const char *TAG = "MotorController";
};
