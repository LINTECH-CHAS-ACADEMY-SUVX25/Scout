#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi.h"
#include "display.h"
#include "ui.h"
#include "video.h"
#include "esp_log.h"

static const char *TAG = "screen";

void app_main(void)
{
    wifi_ap_start();
    ESP_LOGI(TAG, "WiFi AP started");
    display_init();
    ESP_LOGI(TAG, "Display initialized");
    ui_init();
    ESP_LOGI(TAG, "UI initialized");
    video_init();
    ESP_LOGI(TAG, "Video tasks started");

    vTaskDelete(NULL);
}
