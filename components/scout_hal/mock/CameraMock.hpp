#pragma once
#include "ICamera.hpp"

class CameraMock : public ICamera {
public:
    bool init() override;
    bool capture(Frame &frame) override;
    void releaseFrame() override;

    bool initialized  = false;
    int  capture_count = 0;

    // Inject custom JPEG data for tests.
    const uint8_t *fake_data = nullptr;
    size_t         fake_len  = 0;
};
