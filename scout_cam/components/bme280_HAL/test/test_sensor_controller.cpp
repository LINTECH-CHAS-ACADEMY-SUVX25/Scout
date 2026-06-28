#include "unity.h"
#include "bme280_driver.hpp"
#include "SensorController.hpp"
#include "esp_err.h"

// Fake sensor injected into SensorController in place of the real bme280_sensor.
// next is the reading returned on the following read_measurement(); read_ret
// lets a test simulate an I2C failure.
struct MockSensor {
    sensorReading next     = {};
    esp_err_t     read_ret = ESP_OK;

    esp_err_t init()                              { return ESP_OK; }
    esp_err_t read_calibration()                  { return ESP_OK; }
    esp_err_t start_measurement()                 { return ESP_OK; }
    esp_err_t read_measurement(sensorReading &r)  { if(read_ret == ESP_OK) r = next; return read_ret; }
};

TEST_CASE("no reading is available before the first read", "[sensor]")
{
    MockSensor mock;
    SensorController<MockSensor> ctrl(mock);

    sensorReading r;
    TEST_ASSERT_FALSE(ctrl.getReading(r));
}

TEST_CASE("readSensor caches the latest reading", "[sensor]")
{
    MockSensor mock;
    mock.next = { .T = 2345, .P = 25700000, .H = 51200 };
    SensorController<MockSensor> ctrl(mock);

    ctrl.readSensor();

    sensorReading r;
    TEST_ASSERT_TRUE(ctrl.getReading(r));
    TEST_ASSERT_EQUAL_INT32(2345, r.T);
    TEST_ASSERT_EQUAL_UINT32(25700000, r.P);
    TEST_ASSERT_EQUAL_UINT32(51200, r.H);
}

TEST_CASE("temperature at or above 25 C flags too hot", "[sensor]")
{
    MockSensor mock;
    mock.next = { .T = 2500, .P = 0, .H = 0 };
    SensorController<MockSensor> ctrl(mock);

    ctrl.readSensor();

    TEST_ASSERT_TRUE(ctrl.getTooHot());
}

TEST_CASE("temperature below 25 C clears the too-hot flag", "[sensor]")
{
    MockSensor mock;
    SensorController<MockSensor> ctrl(mock);

    mock.next = { .T = 2600, .P = 0, .H = 0 };
    ctrl.readSensor();
    TEST_ASSERT_TRUE(ctrl.getTooHot());

    mock.next = { .T = 2000, .P = 0, .H = 0 };
    ctrl.readSensor();
    TEST_ASSERT_FALSE(ctrl.getTooHot());
}

TEST_CASE("a failed read keeps the last good reading", "[sensor]")
{
    MockSensor mock;
    mock.next = { .T = 2200, .P = 100000, .H = 40960 };
    SensorController<MockSensor> ctrl(mock);
    ctrl.readSensor();

    mock.read_ret = ESP_FAIL;
    ctrl.readSensor();

    sensorReading r;
    TEST_ASSERT_TRUE(ctrl.getReading(r));
    TEST_ASSERT_EQUAL_INT32(2200, r.T);
    TEST_ASSERT_EQUAL_UINT32(100000, r.P);
    TEST_ASSERT_EQUAL_UINT32(40960, r.H);
}
