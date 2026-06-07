#include "MotorEsp32.hpp"

extern "C" {
#include "motor.h"
}

bool MotorEsp32::init()
{
    motor_init();
    return true;
}

void MotorEsp32::apply(uint8_t direction, uint8_t speed_pct)
{
    (void)speed_pct;
    motor_apply(direction);
}
