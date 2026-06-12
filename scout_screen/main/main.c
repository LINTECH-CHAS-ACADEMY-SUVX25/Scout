#include "wifi_ap.h"
#include "display.h"
#include "lvgl_port.h"
#include "scout_ui.h"
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
    watchdog_init(NULL);
    display_init();
    lvgl_port_init();
    scout_ui_init();

    scout_ui_intro_screen(4);

    scout_ui_intro_step("WIFI");
    wifi_ap_start();

    scout_ui_intro_step("MONITOR");
    monitor_init();
    
    scout_ui_intro_step("STREAM");
    stream_init();
    
    scout_ui_intro_step("READY");
    render_init();

    vTaskDelete(NULL);
}
