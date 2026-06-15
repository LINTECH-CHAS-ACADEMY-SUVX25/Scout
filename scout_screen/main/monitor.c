#include "monitor.h"
#include "console.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Task — UART diagnostic CLI for the dashboard side.
// Runs a line-input loop on UART0 and dispatches single-word commands.

static const char *TAG = "monitor";

static void monitor_run(void *arg);

void monitor_init(void)
{
    term_init();
    xTaskCreate(monitor_run, "monitor", 3072, NULL, 2, NULL);
    ESP_LOGI(TAG, "monitor ready on UART0");
}

static void monitor_run(void *arg)
{
    term_println("\r\nScout monitor — type HELP");
    term_run_handler(term_dispatch);
}
