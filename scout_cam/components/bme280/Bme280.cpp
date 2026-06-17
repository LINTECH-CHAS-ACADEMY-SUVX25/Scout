#include "Bme280.hpp"
#include "bme280.h"

bool Bme280::init()
{
    return bme280_init();
}

bool Bme280::read(float &temp_c, float &humidity_pct, float &pressure_pa)
{
    return bme280_read(&temp_c, &humidity_pct, &pressure_pa);
}
