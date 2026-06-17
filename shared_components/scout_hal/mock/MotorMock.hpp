#pragma once
#include "IMotor.hpp"
#include <cstdint>

class MotorMock : public IMotor {
public:
    bool init() override;
    void apply(uint8_t direction, uint8_t speed_pct) override;

    uint8_t last_direction = 0;
    uint8_t last_speed     = 0;
    bool    initialized    = false;
};
