#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi.h"
#include "display.h"
#include "ui.h"
#include "stream.h"
#include "render.h"
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
    stream_init();
    ESP_LOGI(TAG, "Stream task started");
    render_init();
    ESP_LOGI(TAG, "Render task started");

    vTaskDelete(NULL);
}
