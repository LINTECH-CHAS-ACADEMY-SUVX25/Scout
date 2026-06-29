#include "unity.h"
#include "DisplayMock.hpp"

void test_display_init_sets_flag(void)
{
    DisplayMock disp;
    TEST_ASSERT_FALSE(disp.initialized);
    disp.init();
    TEST_ASSERT_TRUE(disp.initialized);
}

void test_display_draw_increments_count(void)
{
    DisplayMock disp;
    disp.init();
    disp.drawBitmap(0, 0, 239, 159, nullptr);
    disp.drawBitmap(0, 0, 239, 159, nullptr);
    TEST_ASSERT_EQUAL(2, disp.draw_count);
}
