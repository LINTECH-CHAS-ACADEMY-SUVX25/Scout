#include "CameraEsp32.hpp"

extern "C" {
#include "camera.h"
}

bool CameraEsp32::init()
{
    camera_init();
    return true;
}

bool CameraEsp32::capture(Frame &frame)
{
    return camera_capture(&frame.data, &frame.len);
}

void CameraEsp32::releaseFrame()
{
    camera_release();
}
