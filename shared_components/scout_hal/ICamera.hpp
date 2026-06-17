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

    // Captures a JPEG frame. Returns false if capture fails.
    // The frame is valid until releaseFrame() is called.
    virtual bool capture(Frame &frame) = 0;

    // Releases the frame buffer back to the camera driver.
    virtual void releaseFrame() = 0;
};
