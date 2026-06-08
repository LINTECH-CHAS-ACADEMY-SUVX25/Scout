#include "wifi_sta.h"
#include "camera.h"
#include "motor.h"
#include "stream.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "scout_cam";

void app_main(void)
{
    ESP_LOGI(TAG, "Scout CAM starting");

    motor_init();
    wifi_connect();

    ESP_ERROR_CHECK(camera_init());
    ESP_LOGI(TAG, "camera ready");

    xTaskCreate(motor_run, "motor", 2048, NULL, 6, NULL);
    stream_init();
    xTaskCreate(stream_run, "stream", 4096, NULL, 5, NULL);
}
