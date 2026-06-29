#pragma once
#include <cstdint>
#include <cstddef>

struct Frame {
    const uint8_t *data;
    size_t         len;
};

class ICamera {
public:
    virtual ~ICamera() = default;
    virtual bool init() = 0;
    virtual bool capture(Frame &frame) = 0;
    virtual void releaseFrame() = 0;
};
