#include "wifi_sta.h"
#include "cam_state.h"
#include "motor.h"
#include "stream.h"
#include "bme280.h"
#include "watchdog.h"
#include "reset_info.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "scout_cam";

static const watchdog_config_t wtd_cfg = {
    .timeout_ms     = 5000,
    .idle_core_mask = 0,
};

void app_main(void)
{
    reset_info_log();
    watchdog_init(&wtd_cfg);
    wifi_connect();
    cam_state_camera_start();
    motor_init();
    bme280_init();
    stream_init();
    vTaskDelete(NULL);
}
