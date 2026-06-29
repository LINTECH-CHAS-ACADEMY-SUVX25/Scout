#include "unity.h"
#include "CameraMock.hpp"

void test_camera_init_sets_flag(void)
{
    CameraMock cam;
    TEST_ASSERT_FALSE(cam.initialized);
    cam.init();
    TEST_ASSERT_TRUE(cam.initialized);
}

void test_camera_capture_returns_data(void)
{
    CameraMock cam;
    cam.init();
    Frame f = {};
    bool ok = cam.capture(f);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_NOT_NULL(f.data);
    TEST_ASSERT_GREATER_THAN(0, f.len);
}

void test_camera_capture_increments_count(void)
{
    CameraMock cam;
    cam.init();
    static const uint8_t fake_bytes[] = { 0xAB, 0xCD };
    cam.fake_data = fake_bytes;
    cam.fake_len  = sizeof(fake_bytes);
    Frame f = {};
    cam.capture(f);
    cam.capture(f);
    TEST_ASSERT_EQUAL(2, cam.capture_count);
    TEST_ASSERT_EQUAL(0xAB, f.data[0]);
}
