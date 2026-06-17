#pragma once

class IBme280 {
public:
    virtual ~IBme280() = default;
    virtual bool init() = 0;
    virtual bool read(float &temp_c, float &humidity_pct, float &pressure_pa) = 0;
};
