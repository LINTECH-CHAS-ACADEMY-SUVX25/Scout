#include "wifi.h"
#include "motor.h"
#include "camera.h"
#include "stream.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "scout_cam";

void app_main(void)
{
    ESP_LOGI(TAG, "Scout CAM starting");

    motor_init();
    wifi_connect();

    ESP_ERROR_CHECK(camera_init());
    ESP_LOGI(TAG, "Camera ready");

    stream_start();
}
