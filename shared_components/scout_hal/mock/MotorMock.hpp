#pragma once
#include "IMotor.hpp"

class MotorMock : public IMotor {
public:
    bool    init() override;
    void    apply(uint8_t direction, uint8_t speed_pct) override;

    bool    initialized    = false;
    uint8_t last_direction = 0;
    uint8_t last_speed     = 0;
};
