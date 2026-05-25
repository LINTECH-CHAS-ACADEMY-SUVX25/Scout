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
    display_init();
    ui_init();
    video_init();

    ESP_LOGI(TAG, "Screen ready");
    vTaskDelete(NULL);
}
