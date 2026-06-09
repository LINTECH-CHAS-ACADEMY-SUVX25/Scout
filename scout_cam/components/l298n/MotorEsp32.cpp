#include "MotorEsp32.hpp"

extern "C" {
#include "l298n.h"
}

bool MotorEsp32::init()
{
    l298n_init();
    return true;
}

void MotorEsp32::apply(uint8_t direction, uint8_t speed_pct)
{
    (void)speed_pct;
    l298n_apply(direction);
}
