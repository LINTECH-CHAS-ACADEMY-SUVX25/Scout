#pragma once
#include "IMotor.hpp"

class MotorEsp32 : public IMotor {
public:
    bool init() override;
    void apply(uint8_t direction, uint8_t speed_pct) override;
};
