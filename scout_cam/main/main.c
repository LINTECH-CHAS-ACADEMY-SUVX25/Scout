#include "wifi_sta.h"
#include "camera.h"
#include "motor.h"
#include "stream.h"
#include "watchdog.h"
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
    watchdog_init(&wtd_cfg);
    motor_init();
    wifi_connect();
    camera_init();
    stream_init();
    vTaskDelete(NULL);
}
