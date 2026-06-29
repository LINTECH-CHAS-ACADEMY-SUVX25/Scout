#include "MotorMock.hpp"

bool MotorMock::init()
{
    initialized = true;
    return true;
}

void MotorMock::apply(uint8_t direction, uint8_t speed_pct)
{
    last_direction = direction;
    last_speed     = speed_pct;
}
