#include "monitor.h"
#include "motor_task.h"
#include "udp_stream.h"
#include "rc_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_wifi.h"
#include "driver/uart.h"
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

// Periodically logs heap usage and task count so regressions are visible in the monitor.

static const char *TAG = "monitor";

static void uart_puts(const char *s)
{
    uart_write_bytes(UART_NUM_0, s, strlen(s));
}

static void uart_println(const char *s)
{
    uart_puts(s);
    uart_puts("\r\n");
}

static void uart_printfln(const char *fmt, ...)
{
    char buf[96];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    uart_println(buf);
}

static void decode_motors(uint8_t cmd, const char **left, const char **right)
{
    bool fwd = (cmd & CMD_FORWARD) != 0;
    bool bwd = (cmd & CMD_BACKWARD) != 0;
    bool lft = (cmd & CMD_LEFT) != 0;
    bool rgt = (cmd & CMD_RIGHT) != 0;

    if(fwd && bwd) { fwd = false; bwd = false; }
    if(lft && rgt) { lft = false; rgt = false; }

    if(fwd && lft)       { *left = "fwd";  *right = "stop"; }
    else if(fwd && rgt)  { *left = "stop"; *right = "fwd";  }
    else if(bwd && lft)  { *left = "bwd";  *right = "stop"; }
    else if(bwd && rgt)  { *left = "stop"; *right = "bwd";  }
    else if(fwd)         { *left = "fwd";  *right = "fwd";  }
    else if(bwd)         { *left = "bwd";  *right = "bwd";  }
    else if(lft)         { *left = "fwd";  *right = "bwd";  }
    else if(rgt)         { *left = "bwd";  *right = "fwd";  }
    else                 { *left = "stop"; *right = "stop"; }
}

static void cmd_status(void)
{
    uint32_t uptime_s = (uint32_t)(esp_timer_get_time() / 1000000);
    wifi_ap_record_t ap = {0};
    bool wifi_ok = (esp_wifi_sta_get_ap_info(&ap) == ESP_OK);
    uint8_t last_cmd = motor_task_get_last_cmd();

    uart_println("--- STATUS ---");
    uart_printfln("uptime       %lus", (unsigned long)uptime_s);
    uart_printfln("free heap    %luB", (unsigned long)esp_get_free_heap_size());
    uart_printfln("free PSRAM   %luB", (unsigned long)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    uart_printfln("WiFi         %s", wifi_ok ? "connected" : "disconnected");
    uart_printfln("motors       %s", last_cmd == CMD_STOP ? "stopped" : "moving");
}

static void cmd_motor(void)
{
    uint8_t cmd = motor_task_get_last_cmd();
    const char *left;
    const char *right;
    decode_motors(cmd, &left, &right);

    uart_println("--- MOTOR ---");
    uart_printfln("last cmd     0x%02X", cmd);
    uart_printfln("left motor   %s", left);
    uart_printfln("right motor  %s", right);
}

static void cmd_sensor(void)
{
    wifi_ap_record_t ap = {0};
    bool wifi_ok = (esp_wifi_sta_get_ap_info(&ap) == ESP_OK);

    udp_stream_stats_t stats;
    udp_stream_get_stats(&stats);

    uart_println("--- SENSOR ---");
    if (wifi_ok)
    {
        uart_printfln("RSSI         %ddBm", ap.rssi);
        uart_printfln("WiFi ch      %d", ap.primary);
    }
    else
    {
        uart_println("RSSI         n/a (not connected)");
    }
    uart_printfln("frames sent  %lu", (unsigned long)stats.frames_sent);
    uart_printfln("last frame   %luB", (unsigned long)stats.last_frame_bytes);
}

static void cmd_config(const char *args)
{
    uart_println("--- CONFIG ---");
    if (strlen(args) == 0)
    {
        uart_println("usage: CONFIG <param> <value>");
        uart_println("no configurable params yet");
    }
    else
    {
        uart_printfln("unknown param: %s", args);
    }
}

static void cmd_diag(void)
{
    uart_println("--- DIAG ---");
    uart_printfln("tasks        %lu", (unsigned long)uxTaskGetNumberOfTasks());
    uart_printfln("min heap     %luB", (unsigned long)esp_get_minimum_free_heap_size());
    uart_printfln("free int     %luB", (unsigned long)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    uart_printfln("free PSRAM   %luB", (unsigned long)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

static void cmd_help(void)
{
    uart_println("commands:");
    uart_println("  STATUS          uptime, heap, WiFi, motor state");
    uart_println("  MOTOR           direction per motor");
    uart_println("  SENSOR          WiFi RSSI, frames sent");
    uart_println("  CONFIG <p> <v>  change config param");
    uart_println("  DIAG            heap watermarks, task count");
    uart_println("  HELP            this list");
}

static void handle_command(char *line)
{
    int len = (int)strlen(line);
    while (len > 0 && (line[len - 1] == ' ' || line[len - 1] == '\t'))
        line[--len] = '\0';

    if (len == 0)
        return;

    if (strcmp(line, "STATUS") == 0)
        cmd_status();
    else if (strcmp(line, "MOTOR") == 0)
        cmd_motor();
    else if (strcmp(line, "SENSOR") == 0)
        cmd_sensor();
    else if (strcmp(line, "DIAG") == 0)
        cmd_diag();
    else if (strcmp(line, "HELP") == 0)
        cmd_help();
    else if (strncmp(line, "CONFIG", 6) == 0)
    {
        const char *args = line + 6;
        while (*args == ' ')
            args++;
        cmd_config(args);
    }
    else
    {
        uart_printfln("unknown command '%s' — try HELP", line);
    }
}

static void monitor_task(void *arg)
{
    static char line[64];
    int pos = 0;

    uart_println("\r\nScout CAM monitor — type HELP");

    while (1)
    {
        uint8_t ch;
        if (uart_read_bytes(UART_NUM_0, &ch, 1, pdMS_TO_TICKS(20)) <= 0)
            continue;

        if (ch == '\r' || ch == '\n')
        {
            if (pos > 0)
            {
                line[pos] = '\0';
                uart_println("");
                handle_command(line);
                pos = 0;
            }
        }
        else if (ch == 0x7F || ch == 0x08)
        {
            if (pos > 0)
            {
                pos--;
                uart_puts("\b \b");
            }
        }
        else if (pos < (int)sizeof(line) - 1)
        {
            line[pos++] = (char)ch;
            uart_write_bytes(UART_NUM_0, &ch, 1);
        }
    }
}

void monitor_init(void)
{
    esp_err_t ret = uart_driver_install(UART_NUM_0, 256, 0, 0, NULL, 0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(TAG, "uart_driver_install: %s", esp_err_to_name(ret));
        return;
    }
    xTaskCreate(monitor_task, "monitor", 3072, NULL, 2, NULL);
    ESP_LOGI(TAG, "monitor ready on UART0");
}
