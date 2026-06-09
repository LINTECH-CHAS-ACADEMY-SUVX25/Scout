#include "wifi_ap.h"
#include "display.h"
#include "lvgl_port.h"
#include "monitor.h"
#include "stream.h"
#include "render.h"
#include "watchdog.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "screen";

void app_main(void)
{
    watchdog_init();
    wifi_ap_start();
    display_init();
    lvgl_port_init();
    monitor_init();
    stream_init();
    render_init();
    lvgl_port_intro_screen();
    vTaskDelete(NULL);
}
