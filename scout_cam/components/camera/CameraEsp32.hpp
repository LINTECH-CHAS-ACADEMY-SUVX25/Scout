#pragma once
#include "ICamera.hpp"

// Concrete ICamera implementation for the AI-Thinker ESP32-CAM + OV2640.
// Delegates to camera_init(), camera_capture() and camera_release() from camera.h.
class CameraEsp32 : public ICamera {
public:
    bool init() override;
    bool capture(Frame &frame) override;
    void releaseFrame() override;
};
