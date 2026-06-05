#include "monitor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <inttypes.h>

static const char *TAG = "monitor";

static void monitor_task(void *arg)
{
    (void)arg;
    while (1) {
        uint32_t free_heap  = esp_get_free_heap_size();
        uint32_t min_heap   = esp_get_minimum_free_heap_size();
        UBaseType_t n_tasks = uxTaskGetNumberOfTasks();
        ESP_LOGI(TAG, "heap free=%"PRIu32" B  min=%"PRIu32" B  tasks=%u",
                 free_heap, min_heap, (unsigned)n_tasks);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void monitor_start(void)
{
    xTaskCreate(monitor_task, "monitor", 2048, NULL, 2, NULL);
}
