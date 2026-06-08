#include "wifi_ap.h"
#include "display.h"
#include "lvgl_port.h"
#include "monitor.h"
#include "stream.h"
#include "render.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "screen";

void app_main(void)
{
    wifi_ap_start();
    display_init();
    lvgl_port_init();
    monitor_init();
    xTaskCreate(monitor_run, "monitor", 3072, NULL, 2, NULL);
    stream_init();
    xTaskCreatePinnedToCore(stream_run, "udp_server", 4096, NULL, 5, NULL, 0);
    render_init();
    xTaskCreatePinnedToCore(render_run, "render", 8192, NULL, 4, NULL, 1);
    vTaskDelete(NULL);
}
