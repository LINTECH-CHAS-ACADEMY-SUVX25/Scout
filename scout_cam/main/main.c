#include "wifi_sta.h"
#include "motor.h"
#include "camera.h"
#include "motor_task.h"
#include "udp_stream.h"
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "scout_cam";

void app_main(void)
{
    ESP_LOGI(TAG, "Scout CAM starting");

    motor_init();
    wifi_connect();

    ESP_ERROR_CHECK(camera_init());
    ESP_LOGI(TAG, "Camera ready");

    motor_task_init();
    xTaskCreate(motor_task_run, "motor_task", 2048, NULL, 6, NULL);

    udp_stream_init();
    xTaskCreate(udp_stream_run, "udp_stream", 4096, NULL, 5, NULL);
}
