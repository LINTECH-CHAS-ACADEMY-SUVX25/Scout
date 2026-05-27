#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi.h"
#include "display.h"
#include "ui.h"
#include "udp_video.h"
#include "esp_log.h"

static const char *TAG = "screen";

void app_main(void)
{
    // Starta WiFi-AP först — kameran kan inte ansluta förrän nätverket finns
    wifi_ap_start();
    ESP_LOGI(TAG, "WiFi AP started");

    // Display och UI initieras innan video-tasken startar så att
    // canvasen finns redo när första bilden anländer
    display_init();
    ESP_LOGI(TAG, "Display initialized");
    ui_init();
    ESP_LOGI(TAG, "UI initialized");

    // Starta UDP-mottagning och LVGL-handler som bakgrundstaskar
    udp_video_init();
    ESP_LOGI(TAG, "Video tasks started");

    // app_main-tasken behövs inte längre — allt körs nu i egna taskar
    vTaskDelete(NULL);
}
