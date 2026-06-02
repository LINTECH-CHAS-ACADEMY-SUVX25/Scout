#include "test_runner.hpp"
#include "CameraMock.hpp"

TEST(test_camera_init_sets_flag)
{
    CameraMock cam;
    ASSERT_TRUE(!cam.initialized);
    cam.init();
    ASSERT_TRUE(cam.initialized);
}

TEST(test_camera_capture_returns_data)
{
    CameraMock cam;
    cam.init();
    Frame f = {};
    bool ok = cam.capture(f);
    ASSERT_TRUE(ok);
    ASSERT_TRUE(f.data != nullptr);
    ASSERT_TRUE(f.len > 0);
}

TEST(test_camera_capture_increments_count)
{
    CameraMock cam;
    cam.init();
    Frame f = {};
    cam.capture(f);
    cam.capture(f);
    ASSERT_EQ(cam.capture_count, 2);
}

TEST(test_camera_inject_custom_data)
{
    CameraMock cam;
    cam.init();

    static const uint8_t fake[] = { 0x01, 0x02, 0x03 };
    cam.fake_data = fake;
    cam.fake_len  = sizeof(fake);

    Frame f = {};
    cam.capture(f);
    ASSERT_EQ(f.len, sizeof(fake));
    ASSERT_EQ(f.data[0], 0x01);
    ASSERT_EQ(f.data[2], 0x03);
}

void run_camera_tests()
{
    printf("Camera:\n");
    RUN(test_camera_init_sets_flag);
    RUN(test_camera_capture_returns_data);
    RUN(test_camera_capture_increments_count);
    RUN(test_camera_inject_custom_data);
}
