#include "telemetry.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "telemetry";

static void telemetry_task(void *arg)
{
    (void)arg;
    while (1) {
        ESP_LOGI(TAG, "telemetry: placeholder task active");
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

void telemetry_start(void)
{
    xTaskCreate(telemetry_task, "telemetry", 2048, NULL, 3, NULL);
}
