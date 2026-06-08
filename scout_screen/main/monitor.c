#include "monitor.h"
#include "monitor_cmds.h"
#include "frame_buf.h"
#include "wifi_ap.h"
#include "uart_console.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"

// Task — UART diagnostic CLI for the dashboard side.
// Runs a line-input loop on UART0 and dispatches single-word commands.
// Exists so developers can inspect runtime state without a debugger.

static const char *TAG = "monitor";

void monitor_run(void *arg)
{
    char line[64];
    uart_console_println("\r\nScout monitor — type HELP");
    while(1) {
        if(!uart_console_read_line(line, sizeof(line))) continue;

        monitor_status_t status = {
            .uptime_s         = (uint32_t)(esp_timer_get_time() / 1000000),
            .free_heap        = esp_get_free_heap_size(),
            .free_psram       = heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
            .sta_count        = wifi_ap_sta_count(),
            .stream_connected = frame_buf_is_connected(),
        };
        stream_stats_t stream_stats;
        frame_buf_get_stats(&stream_stats);
        monitor_diag_t diag = {
            .n_tasks    = uxTaskGetNumberOfTasks(),
            .min_heap   = esp_get_minimum_free_heap_size(),
            .free_int   = heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
            .free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
        };

        monitor_dispatch(line, &status, &stream_stats, &diag);
    }
}

void monitor_init(void)
{
    uart_console_init();
    ESP_LOGI(TAG, "monitor ready on UART0");
}
