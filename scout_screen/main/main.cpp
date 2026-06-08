#include "DisplayEsp32.hpp"
#include "render.hpp"

extern "C" {
#include "wifi_ap.h"
#include "ui.h"
#include "monitor.h"
#include "stream.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
}

// main.cpp is the only place that knows the concrete hardware types.
// All tasks below receive interface pointers and never include hardware headers.
static DisplayEsp32 g_display;

static const char *TAG = "screen";

extern "C" void app_main(void)
{
    wifi_ap_start();
    g_display.init();
    ui_init();
    ESP_LOGI(TAG, "UI initialized");
    monitor_init();
    stream_init();
    ESP_LOGI(TAG, "Stream task started");
    render_init(&g_display);
    ESP_LOGI(TAG, "Render task started");
    vTaskDelete(NULL);
}
