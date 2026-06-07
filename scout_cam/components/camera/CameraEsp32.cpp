#include "CameraEsp32.hpp"

extern "C" {
#include "camera.h"
}

bool CameraEsp32::init()
{
    return camera_init() == ESP_OK;
}

bool CameraEsp32::capture(Frame &frame)
{
    return camera_capture(&frame.data, &frame.len);
}

void CameraEsp32::releaseFrame()
{
    camera_release();
}
