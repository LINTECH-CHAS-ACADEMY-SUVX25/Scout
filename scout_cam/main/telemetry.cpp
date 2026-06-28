#include "telemetry.h"
#include "bme280_driver.hpp"
#include "SensorController.hpp"
#include "watchdog.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Task — reads the BME280 over I2C every 2 s and caches the latest compensated
// values. The stream task pulls them via telemetry_read() and folds them into
// the periodic DIAG packet sent to the dashboard.

static const char *TAG = "telemetry";

static bme280_sensor sensor(I2C_NUM_0, 0x77);
static SensorController<bme280_sensor> controller(sensor);

static void telemetry_run(void *arg)
{
    watchdog_register();
    while(1) {
        controller.readSensor();
        watchdog_reset();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

extern "C" void telemetry_init(void)
{
    controller.initSensor();
    xTaskCreate(telemetry_run, "telemetry", 4096, NULL, 4, NULL);
    ESP_LOGI(TAG, "telemetry ready");
}

extern "C" bool telemetry_read(float *temp_c, float *hum_pct, float *pres_pa)
{
    sensorReading r;
    if(!controller.getReading(r)) return false;
    *temp_c  = r.T / 100.0f;
    *hum_pct = r.H / 1024.0f;
    *pres_pa = r.P / 256.0f;
    return true;
}
