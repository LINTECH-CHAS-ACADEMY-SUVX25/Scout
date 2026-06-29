#include "unity.h"
#include "Bme280Mock.hpp"

void test_bme280_init_sets_flag(void)
{
    Bme280Mock sensor;
    TEST_ASSERT_FALSE(sensor.initialized);
    sensor.init();
    TEST_ASSERT_TRUE(sensor.initialized);
}

void test_bme280_read_returns_injected_values(void)
{
    Bme280Mock sensor;
    sensor.init();
    sensor.inject_temp = 25.0f;
    sensor.inject_hum  = 60.0f;
    sensor.inject_pres = 101000.0f;
    float temp, hum, pres;
    bool ok = sensor.read(temp, hum, pres);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 25.0f,     temp);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 60.0f,     hum);
    TEST_ASSERT_FLOAT_WITHIN(1.0f,  101000.0f, pres);
}

void test_bme280_read_failure_returns_false(void)
{
    Bme280Mock sensor;
    sensor.init();
    sensor.fail_read = true;
    float temp, hum, pres;
    bool ok = sensor.read(temp, hum, pres);
    TEST_ASSERT_FALSE(ok);
}
