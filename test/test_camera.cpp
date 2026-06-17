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
    Frame f = {};
    cam.capture(f);
    cam.capture(f);
    TEST_ASSERT_EQUAL(2, cam.capture_count);
}

void test_camera_inject_custom_data(void)
{
    CameraMock cam;
    cam.init();

    static const uint8_t fake[] = { 0x01, 0x02, 0x03 };
    cam.fake_data = fake;
    cam.fake_len  = sizeof(fake);

    Frame f = {};
    cam.capture(f);
    TEST_ASSERT_EQUAL(sizeof(fake), f.len);
    TEST_ASSERT_EQUAL(0x01, f.data[0]);
    TEST_ASSERT_EQUAL(0x03, f.data[2]);
}
