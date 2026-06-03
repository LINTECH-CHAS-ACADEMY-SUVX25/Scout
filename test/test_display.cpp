#include "test_runner.hpp"
#include "DisplayMock.hpp"

TEST(test_display_init_sets_flag)
{
    DisplayMock disp;
    ASSERT_TRUE(!disp.initialized);
    disp.init();
    ASSERT_TRUE(disp.initialized);
}

TEST(test_display_backlight_on)
{
    DisplayMock disp;
    disp.init();
    disp.backlightOn();
    ASSERT_TRUE(disp.backlight_on);
}

TEST(test_display_backlight_off)
{
    DisplayMock disp;
    disp.init();
    disp.backlightOn();
    disp.backlightOff();
    ASSERT_TRUE(!disp.backlight_on);
}

TEST(test_display_draw_increments_count)
{
    DisplayMock disp;
    disp.init();
    disp.drawBitmap(0, 0, 239, 159, nullptr);
    disp.drawBitmap(0, 0, 239, 159, nullptr);
    ASSERT_EQ(disp.draw_count, 2);
}

void run_display_tests()
{
    printf("Display:\n");
    RUN(test_display_init_sets_flag);
    RUN(test_display_backlight_on);
    RUN(test_display_backlight_off);
    RUN(test_display_draw_increments_count);
}
