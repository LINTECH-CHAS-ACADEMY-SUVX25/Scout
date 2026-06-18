#include "Bme280Mock.hpp"
#include <cstdio>

bool Bme280Mock::init()
{
    initialized = true;
    printf("[Bme280Mock] init\n");
    return true;
}

bool Bme280Mock::read(float &temp_c, float &humidity_pct, float &pressure_pa)
{
    if (fail_read) {
        printf("[Bme280Mock] read FAILED (injected)\n");
        return false;
    }
    temp_c       = inject_temp;
    humidity_pct = inject_hum;
    pressure_pa  = inject_pres;
    read_count++;
    printf("[Bme280Mock] read temp=%.1f hum=%.1f pres=%.1f\n",
           inject_temp, inject_hum, inject_pres);
    return true;
}
