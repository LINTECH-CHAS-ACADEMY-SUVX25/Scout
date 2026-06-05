#include "CameraMock.hpp"
#include <cstdio>

static const uint8_t s_dummy_jpeg[] = { 0xFF, 0xD8, 0xFF, 0xD9 }; // minimal valid JPEG

bool CameraMock::init()
{
    initialized = true;
    printf("[CameraMock] init\n");
    return true;
}

bool CameraMock::capture(Frame &frame)
{
    capture_count++;
    frame.data = fake_data ? fake_data : s_dummy_jpeg;
    frame.len  = fake_data ? fake_len  : sizeof(s_dummy_jpeg);
    printf("[CameraMock] capture #%d (%zu bytes)\n", capture_count, frame.len);
    return true;
}

void CameraMock::releaseFrame()
{
    printf("[CameraMock] releaseFrame\n");
}
