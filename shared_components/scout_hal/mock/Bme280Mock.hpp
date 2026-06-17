#pragma once
#include "IBme280.hpp"

class Bme280Mock : public IBme280 {
public:
    bool init() override;
    bool read(float &temp_c, float &humidity_pct, float &pressure_pa) override;

    bool  initialized = false;
    bool  fail_read   = false;
    float inject_temp = 22.5f;
    float inject_hum  = 55.0f;
    float inject_pres = 101325.0f;
    int   read_count  = 0;
};
