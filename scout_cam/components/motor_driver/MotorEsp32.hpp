#pragma once
#include "IMotor.hpp"

// Concrete IMotor implementation for the AI-Thinker ESP32-CAM + L298N.
// Delegates to motor_init() and motor_apply() from motor.h.
// speed_pct is accepted but ignored until LEDC PWM is wired up.
class MotorEsp32 : public IMotor {
public:
    bool init() override;
    void apply(uint8_t direction, uint8_t speed_pct) override;
};
