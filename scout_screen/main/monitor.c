#include "monitor.h"
#include "stream.h"
#include "wifi_ap.h"
#include "uart_console.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include <string.h>

static const char *TAG = "monitor";

// Commands

static void cmd_status(void)
{
    uint32_t uptime_s = (uint32_t)(esp_timer_get_time() / 1000000);

    uart_console_println("--- STATUS ---");
    uart_console_printfln("uptime      %lus",  (unsigned long)uptime_s);
    uart_console_printfln("free heap   %luB",  (unsigned long)esp_get_free_heap_size());
    uart_console_printfln("free PSRAM  %luB",  (unsigned long)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    uart_console_printfln("WiFi clients %d",   wifi_ap_sta_count());
    uart_console_printfln("cam stream  %s",    stream_is_connected() ? "connected" : "disconnected");
}

static void cmd_stream(void)
{
    stream_stats_t s;
    stream_get_stats(&s);

    uart_console_println("--- STREAM ---");
    uart_console_printfln("frames      %lu",   (unsigned long)s.frame_count);
    uart_console_printfln("last frame  %luB",  (unsigned long)s.last_frame_bytes);
    uart_console_printfln("transfer    %ldms", (long)s.last_transfer_ms);
    uart_console_printfln("decode      %ldms", (long)s.last_decode_ms);
}

static void cmd_diag(void)
{
    uart_console_println("--- DIAG ---");
    uart_console_printfln("tasks       %lu",   (unsigned long)uxTaskGetNumberOfTasks());
    uart_console_printfln("min heap    %luB",  (unsigned long)esp_get_minimum_free_heap_size());
    uart_console_printfln("free int    %luB",  (unsigned long)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    uart_console_printfln("free PSRAM  %luB",  (unsigned long)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

static void cmd_help(void)
{
    uart_console_println("commands:");
    uart_console_println("  STATUS  uptime, heap, WiFi clients, stream connection");
    uart_console_println("  STREAM  frame count, size, transfer/decode timing");
    uart_console_println("  DIAG    heap watermarks, task count");
    uart_console_println("  HELP    this list");
}

static void handle_command(char *line)
{
    // Trim trailing whitespace left by some terminals.
    int len = (int)strlen(line);
    while(len > 0 && (line[len - 1] == ' ' || line[len - 1] == '\t'))
        line[--len] = '\0';

    if(len == 0) return;

    if     (strcmp(line, "STATUS") == 0) cmd_status();
    else if(strcmp(line, "STREAM") == 0) cmd_stream();
    else if(strcmp(line, "DIAG")   == 0) cmd_diag();
    else if(strcmp(line, "HELP")   == 0) cmd_help();
    else                                 uart_console_printfln("unknown command '%s' — try HELP", line);
}

// Task

static void monitor_task(void *arg)
{
    static char line[64];
    int pos = 0;

    uart_console_println("\r\nScout monitor — type HELP");

    while(1) {
        uint8_t ch;
        if(!uart_console_read_byte(&ch, 20)) continue;

        if(ch == '\r' || ch == '\n') {
            if(pos > 0) {
                line[pos] = '\0';
                uart_console_println("");
                handle_command(line);
                pos = 0;
            }
        } else if(ch == 0x7F || ch == 0x08) {
            // Backspace — erase the last character on the terminal.
            if(pos > 0) {
                pos--;
                uart_console_write("\b \b");
            }
        } else if(pos < (int)sizeof(line) - 1) {
            char echo[2] = { (char)ch, '\0' };
            line[pos++] = (char)ch;
            uart_console_write(echo);
        }
    }
}

// Init

void monitor_init(void)
{
    uart_console_init();
    xTaskCreate(monitor_task, "monitor", 3072, NULL, 2, NULL);
    ESP_LOGI(TAG, "monitor ready on UART0");
}
