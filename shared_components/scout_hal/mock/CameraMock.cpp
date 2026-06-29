#include "CameraMock.hpp"

bool CameraMock::init()
{
    initialized = true;
    return true;
}

bool CameraMock::capture(Frame &frame)
{
    static const uint8_t default_data[] = { 0xFF };
    frame.data = fake_data ? fake_data : default_data;
    frame.len  = fake_data ? fake_len  : sizeof(default_data);
    capture_count++;
    return true;
}

void CameraMock::releaseFrame() {}
