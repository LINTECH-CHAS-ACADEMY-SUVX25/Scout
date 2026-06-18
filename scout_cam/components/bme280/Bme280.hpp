#pragma once
#include "IBme280.hpp"

class Bme280 : public IBme280 {
public:
    bool init() override;
    bool read(float &temp_c, float &humidity_pct, float &pressure_pa) override;
};
