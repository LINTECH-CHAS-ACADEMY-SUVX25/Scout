#include "monitor.h"
#include "uart_console.h"
#include "wifi_sta.h"
#include "motor_task.hpp"
#include "udp_stream.hpp"

extern "C" {
#include "rc_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
}

#include <string.h>

// Interactive UART diagnostics for the cam node. Reads commands from UART0
// and prints system state on request. All SDK I/O goes through uart_console
// and wifi_sta so this task never touches hardware headers directly.

static const char *TAG = "monitor";

static void decode_motors(uint8_t cmd, const char **left, const char **right)
{
    bool fwd = (cmd & CMD_FORWARD) != 0;
    bool bwd = (cmd & CMD_BACKWARD) != 0;
    bool lft = (cmd & CMD_LEFT) != 0;
    bool rgt = (cmd & CMD_RIGHT) != 0;

    if(fwd && bwd) { fwd = false; bwd = false; }
    if(lft && rgt) { lft = false; rgt = false; }

    if(fwd && lft)      { *left = "fwd";  *right = "stop"; }
    else if(fwd && rgt) { *left = "stop"; *right = "fwd";  }
    else if(bwd && lft) { *left = "bwd";  *right = "stop"; }
    else if(bwd && rgt) { *left = "stop"; *right = "bwd";  }
    else if(fwd)        { *left = "fwd";  *right = "fwd";  }
    else if(bwd)        { *left = "bwd";  *right = "bwd";  }
    else if(lft)        { *left = "fwd";  *right = "bwd";  }
    else if(rgt)        { *left = "bwd";  *right = "fwd";  }
    else                { *left = "stop"; *right = "stop"; }
}

static void cmd_status(void)
{
    uint32_t uptime_s = (uint32_t)(esp_timer_get_time() / 1000000);
    uint8_t  last_cmd = motor_task_get_last_cmd();

    uart_console_println("--- STATUS ---");
    uart_console_printfln("uptime       %lus", (unsigned long)uptime_s);
    uart_console_printfln("free heap    %luB", (unsigned long)esp_get_free_heap_size());
    uart_console_printfln("free PSRAM   %luB", (unsigned long)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    uart_console_printfln("WiFi         %s",   wifi_sta_is_connected() ? "connected" : "disconnected");
    uart_console_printfln("motors       %s",   last_cmd == CMD_STOP ? "stopped" : "moving");
}

static void cmd_motor(void)
{
    uint8_t    cmd = motor_task_get_last_cmd();
    const char *left;
    const char *right;
    decode_motors(cmd, &left, &right);

    uart_console_println("--- MOTOR ---");
    uart_console_printfln("last cmd     0x%02X", cmd);
    uart_console_printfln("left motor   %s",     left);
    uart_console_printfln("right motor  %s",     right);
}

static void cmd_sensor(void)
{
    udp_stream_stats_t stats;
    udp_stream_get_stats(&stats);

    uart_console_println("--- SENSOR ---");
    if(wifi_sta_is_connected()) {
        uart_console_printfln("RSSI         %ddBm", wifi_sta_rssi());
        uart_console_printfln("WiFi ch      %d",    wifi_sta_channel());
    } else {
        uart_console_println("RSSI         n/a (not connected)");
    }
    uart_console_printfln("frames sent  %lu",  (unsigned long)stats.frames_sent);
    uart_console_printfln("last frame   %luB", (unsigned long)stats.last_frame_bytes);
}

static void cmd_config(const char *args)
{
    uart_console_println("--- CONFIG ---");
    if(strlen(args) == 0) {
        uart_console_println("usage: CONFIG <param> <value>");
        uart_console_println("no configurable params yet");
    } else {
        uart_console_printfln("unknown param: %s", args);
    }
}

static void cmd_diag(void)
{
    uart_console_println("--- DIAG ---");
    uart_console_printfln("tasks        %lu",  (unsigned long)uxTaskGetNumberOfTasks());
    uart_console_printfln("min heap     %luB", (unsigned long)esp_get_minimum_free_heap_size());
    uart_console_printfln("free int     %luB", (unsigned long)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    uart_console_printfln("free PSRAM   %luB", (unsigned long)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

static void cmd_help(void)
{
    uart_console_println("commands:");
    uart_console_println("  STATUS          uptime, heap, WiFi, motor state");
    uart_console_println("  MOTOR           direction per motor");
    uart_console_println("  SENSOR          WiFi RSSI, frames sent");
    uart_console_println("  CONFIG <p> <v>  change config param");
    uart_console_println("  DIAG            heap watermarks, task count");
    uart_console_println("  HELP            this list");
}

static void handle_command(char *line)
{
    int len = (int)strlen(line);
    while(len > 0 && (line[len - 1] == ' ' || line[len - 1] == '\t'))
        line[--len] = '\0';
    if(len == 0) return;

    if(strcmp(line, "STATUS") == 0)           cmd_status();
    else if(strcmp(line, "MOTOR") == 0)       cmd_motor();
    else if(strcmp(line, "SENSOR") == 0)      cmd_sensor();
    else if(strcmp(line, "DIAG") == 0)        cmd_diag();
    else if(strcmp(line, "HELP") == 0)        cmd_help();
    else if(strncmp(line, "CONFIG", 6) == 0) {
        const char *args = line + 6;
        while(*args == ' ') args++;
        cmd_config(args);
    } else {
        uart_console_printfln("unknown command '%s' — try HELP", line);
    }
}

static void monitor_task(void *arg)
{
    static char line[64];
    int pos = 0;

    uart_console_println("\r\nScout CAM monitor — type HELP");

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
            if(pos > 0) { pos--; uart_console_write("\b \b"); }
        } else if(pos < (int)sizeof(line) - 1) {
            line[pos++] = (char)ch;
            char echo[2] = {(char)ch, '\0'};
            uart_console_write(echo);
        }
    }
}

void monitor_init(void)
{
    uart_console_init();
    xTaskCreate(monitor_task, "monitor", 3072, NULL, 2, NULL);
    ESP_LOGI(TAG, "monitor ready on UART0");
}
