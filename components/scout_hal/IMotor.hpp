#pragma once
#include <cstdint>

class IMotor {
public:
    virtual ~IMotor() = default;

    // Initialize hardware. Returns false if initialization fails.
    virtual bool init() = 0;

    // Apply direction and speed. direction uses CMD_* bits from rc_protocol.h.
    // speed_pct is 0–100 where 0 is stopped and 100 is full power.
    virtual void apply(uint8_t direction, uint8_t speed_pct) = 0;
};
