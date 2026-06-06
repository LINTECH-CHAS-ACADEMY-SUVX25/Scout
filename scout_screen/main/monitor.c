#include "monitor.h"
#include "stream.h"
#include "rc_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_wifi.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "monitor";

// ── Output helpers ────────────────────────────────────────────────────────────

static void uart_puts(const char *s)
{
    uart_write_bytes(UART_NUM_0, s, strlen(s));
}

static void uart_println(const char *s)
{
    uart_puts(s);
    uart_puts("\r\n");
}

// ── Commands ──────────────────────────────────────────────────────────────────

static void cmd_status(void)
{
    uint32_t uptime_s = (uint32_t)(esp_timer_get_time() / 1000000);

    wifi_sta_list_t sta_list = {0};
    esp_wifi_ap_get_sta_list(&sta_list);

    char buf[80];
    uart_println("--- STATUS ---");
    snprintf(buf, sizeof(buf), "Uptime:       %lus", (unsigned long)uptime_s);
    uart_println(buf);
    snprintf(buf, sizeof(buf), "Free heap:    %luB", (unsigned long)esp_get_free_heap_size());
    uart_println(buf);
    snprintf(buf, sizeof(buf), "Free PSRAM:   %luB", (unsigned long)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    uart_println(buf);
    snprintf(buf, sizeof(buf), "WiFi clients: %d", sta_list.num);
    uart_println(buf);
    snprintf(buf, sizeof(buf), "Cam stream:   %s", stream_is_connected() ? "CONNECTED" : "DISCONNECTED");
    uart_println(buf);
}

static void cmd_stream(void)
{
    stream_stats_t s;
    stream_get_stats(&s);

    char buf[80];
    uart_println("--- STREAM ---");
    snprintf(buf, sizeof(buf), "Frames decoded: %lu", (unsigned long)s.frame_count);
    uart_println(buf);
    snprintf(buf, sizeof(buf), "Last frame:     %luB", (unsigned long)s.last_frame_bytes);
    uart_println(buf);
    snprintf(buf, sizeof(buf), "Last transfer:  %ldms", (long)s.last_transfer_ms);
    uart_println(buf);
    snprintf(buf, sizeof(buf), "Last decode:    %ldms", (long)s.last_decode_ms);
    uart_println(buf);
}

static void cmd_diag(void)
{
    char buf[80];
    uart_println("--- DIAG ---");
    snprintf(buf, sizeof(buf), "Tasks running:  %lu", (unsigned long)uxTaskGetNumberOfTasks());
    uart_println(buf);
    snprintf(buf, sizeof(buf), "Min free heap:  %luB", (unsigned long)esp_get_minimum_free_heap_size());
    uart_println(buf);
    snprintf(buf, sizeof(buf), "Free internal:  %luB", (unsigned long)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    uart_println(buf);
    snprintf(buf, sizeof(buf), "Free PSRAM:     %luB", (unsigned long)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    uart_println(buf);
}

static void cmd_help(void)
{
    uart_println("Commands:");
    uart_println("  STATUS  uptime, heap, WiFi clients, stream connection");
    uart_println("  STREAM  frame count, size, transfer/decode timing");
    uart_println("  DIAG    heap watermarks, task count");
    uart_println("  HELP    this list");
}

static void handle_command(char *line)
{
    // trim trailing whitespace
    int len = (int)strlen(line);
    while (len > 0 && (line[len-1] == ' ' || line[len-1] == '\t'))
        line[--len] = '\0';

    if (len == 0) return;

    if      (strcmp(line, "STATUS") == 0) cmd_status();
    else if (strcmp(line, "STREAM") == 0) cmd_stream();
    else if (strcmp(line, "DIAG")   == 0) cmd_diag();
    else if (strcmp(line, "HELP")   == 0) cmd_help();
    else {
        char buf[64];
        snprintf(buf, sizeof(buf), "Unknown: '%s' — try HELP", line);
        uart_println(buf);
    }
}

// ── Task ──────────────────────────────────────────────────────────────────────

static void monitor_task(void *arg)
{
    static char line[64];
    int pos = 0;

    uart_println("\r\nScout Screen Monitor — type HELP");

    while (1) {
        uint8_t ch;
        if (uart_read_bytes(UART_NUM_0, &ch, 1, pdMS_TO_TICKS(20)) <= 0)
            continue;

        if (ch == '\r' || ch == '\n') {
            if (pos > 0) {
                line[pos] = '\0';
                uart_println("");
                handle_command(line);
                pos = 0;
            }
        } else if (ch == 0x7F || ch == 0x08) {
            if (pos > 0) {
                pos--;
                uart_puts("\b \b");
            }
        } else if (pos < (int)sizeof(line) - 1) {
            line[pos++] = (char)ch;
            uart_write_bytes(UART_NUM_0, &ch, 1); // echo
        }
    }
}

// ── Init ──────────────────────────────────────────────────────────────────────

void monitor_init(void)
{
    esp_err_t ret = uart_driver_install(UART_NUM_0, 256, 0, 0, NULL, 0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "uart_driver_install: %s", esp_err_to_name(ret));
        return;
    }
    xTaskCreate(monitor_task, "monitor", 3072, NULL, 2, NULL);
    ESP_LOGI(TAG, "Monitor ready on UART0 (115200)");
}
