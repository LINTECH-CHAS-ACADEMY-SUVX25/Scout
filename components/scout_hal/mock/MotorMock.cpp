#include "MotorMock.hpp"
#include <cstdio>

bool MotorMock::init()
{
    initialized = true;
    printf("[MotorMock] init\n");
    return true;
}

void MotorMock::apply(uint8_t direction, uint8_t speed_pct)
{
    last_direction = direction;
    last_speed     = speed_pct;
    printf("[MotorMock] apply direction=0x%02X speed=%d%%\n", direction, speed_pct);
}
