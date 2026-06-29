#pragma once
#include <cstdint>

class IMotor {
public:
    virtual ~IMotor() = default;
    virtual bool init() = 0;
    virtual void apply(uint8_t direction, uint8_t speed_pct) = 0;
};
