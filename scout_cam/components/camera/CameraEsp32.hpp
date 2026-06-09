#pragma once
#include "ICamera.hpp"

class CameraEsp32 : public ICamera {
public:
    bool init() override;
    bool capture(Frame &frame) override;
    void releaseFrame() override;
};
