#include "telemetry.h"
#include "cam_state.h"
#include "bme280_driver.hpp"
#include "SensorController.hpp"
#include "watchdog.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Task - Calls for the sensor to read values every 2000ms and
// sends it's contents to cam_state where it is picked up by stream task
// and in stream sent to scout_screen via UDP

static const char *TAG = "telemetry";

static bme280_sensor sensor;
static SensorController<bme280_sensor> controller(sensor);

static void telemetry_run(void *arg)
{
    watchdog_register();
    // initiera struct som skapas i cam_state?
    while(1) {
        controller.readSensor();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}