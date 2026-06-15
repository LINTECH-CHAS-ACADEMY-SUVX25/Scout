#include "console.h"
#include "screen_state.h"
#include "screen_stats.h"
#include "wifi_ap.h"
#include "rc_protocol.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// Adapter — UART0 console I/O and single-word command dispatch for the monitor task.

#define STREAM_LINE_COUNT 14

static const char *TAG = "console";

// ---- I/O ----

void term_init(void)
{
    esp_err_t ret = uart_driver_install(UART_NUM_0, 256, 0, 0, NULL, 0);
    if(ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "uart_driver_install: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "UART0 ready");
}

void term_write(const char *s)
{
    uart_write_bytes(UART_NUM_0, s, strlen(s));
}

void term_println(const char *s)
{
    uart_write_bytes(UART_NUM_0, s, strlen(s));
    uart_write_bytes(UART_NUM_0, "\r\n", 2);
}

void term_printfln(const char *fmt, ...)
{
    char buf[96];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    term_println(buf);
}

bool term_read_byte(uint8_t *ch, uint32_t timeout_ms)
{
    return uart_read_bytes(UART_NUM_0, ch, 1, pdMS_TO_TICKS(timeout_ms)) == 1;
}

int term_try_getchar(void)
{
    uint8_t ch;
    if(uart_read_bytes(UART_NUM_0, &ch, 1, 0) == 1) return (int)ch;
    return -1;
}

bool term_read_line(char *buf, size_t size)
{
    static char s_line[64];
    static int  s_pos = 0;

    uint8_t ch;
    if(!term_read_byte(&ch, 20)) return false;

    if(ch == '\r' || ch == '\n') {
        if(s_pos == 0) return false;
        s_line[s_pos] = '\0';
        term_println("");
        int len = s_pos;
        while(len > 0 && (s_line[len - 1] == ' ' || s_line[len - 1] == '\t'))
            s_line[--len] = '\0';
        s_pos = 0;
        if(len == 0) return false;
        snprintf(buf, size, "%s", s_line);
        return true;
    }
    if(ch == 0x7F || ch == 0x08) {
        if(s_pos > 0) { s_pos--; term_write("\b \b"); }
        return false;
    }
    if(s_pos < (int)sizeof(s_line) - 1) {
        char echo[2] = { (char)ch, '\0' };
        s_line[s_pos++] = (char)ch;
        term_write(echo);
    }
    return false;
}

void term_run_handler(term_handler_t handler)
{
    char line[64];
    while(1) {
        if(!term_read_line(line, sizeof(line))) continue;
        handler(line);
    }
}

// ---- Sensor formatters ----

static void fmt_temp(char *out, size_t n, const cam_diag_pkt_t *d)
{
    int t     = d->temp_cdeg;
    int whole = t / 100;
    int frac  = (t < 0 ? -t : t) % 100 / 10;
    const char *sign = (t < 0 && whole == 0) ? "-" : "";
    snprintf(out, n, "%s%d.%d C", sign, whole, frac);
}

static void fmt_humi(char *out, size_t n, const cam_diag_pkt_t *d)
{
    snprintf(out, n, "%u %%", (unsigned)d->humidity_pct);
}

static void fmt_pres(char *out, size_t n, const cam_diag_pkt_t *d)
{
    snprintf(out, n, "%lu hPa", (unsigned long)(d->pressure_pa / 100));
}

// ---- Commands ----

#define LABEL_W 11
#define LAST_W   9
#define AVG_W   10

static void stat_header(const char *section)
{
    term_printfln("%-*s%*s%*s", LABEL_W + 2, section, LAST_W, "last", AVG_W, "avg");
}

static void stat_row(const char *label, const char *val, const char *avg)
{
    term_printfln("  %-*s%*s%*s", LABEL_W, label, LAST_W, val, AVG_W, avg);
}

static void stat_one(const char *label, uint32_t val)
{
    term_printfln("  %-*s%*lu", LABEL_W, label, LAST_W, (unsigned long)val);
}

static void fmt_fps(char *buf, size_t size, uint32_t tenths)
{
    snprintf(buf, size, "%lu.%lufps",
        (unsigned long)(tenths / 10), (unsigned long)(tenths % 10));
}

static void stat_receive(const screen_stats_t *s)
{
    char val[16];
    char avg[16];

    stat_header("Receive");
    stat_one("frames", s->frame_count);

    snprintf(val, sizeof val, "%ldB", (long)s->frame_bytes.last);
    snprintf(avg, sizeof avg, "%ldB", (long)s->frame_bytes.avg);
    stat_row("frame size", val, avg);

    snprintf(val, sizeof val, "%ldms", (long)s->transfer.last);
    snprintf(avg, sizeof avg, "%ldms", (long)s->transfer.avg);
    stat_row("transfer", val, avg);

    snprintf(val, sizeof val, "%ldms", (long)s->rx_interval.last);
    snprintf(avg, sizeof avg, "%ldms", (long)s->rx_interval.avg);
    stat_row("frame gap", val, avg);

    uint32_t max_last = s->rx_interval.last ? 10000u / (uint32_t)s->rx_interval.last : 0;
    fmt_fps(val, sizeof val, max_last);
    fmt_fps(avg, sizeof avg, s->rx_fps_tenths);
    stat_row("max fps", val, avg);

    snprintf(val, sizeof val, "%ldms", (long)s->stream_loop.last);
    snprintf(avg, sizeof avg, "%ldms", (long)s->stream_loop.avg);
    stat_row("loop", val, avg);
}

static void stat_render(const screen_stats_t *s)
{
    char val[16];
    char avg[16];

    stat_header("Render");

    snprintf(val, sizeof val, "%ldms", (long)s->lvgl.last);
    snprintf(avg, sizeof avg, "%ldms", (long)s->lvgl.avg);
    stat_row("lvgl", val, avg);

    snprintf(val, sizeof val, "%ldms", (long)s->decode.last);
    snprintf(avg, sizeof avg, "%ldms", (long)s->decode.avg);
    stat_row("decode", val, avg);

    snprintf(val, sizeof val, "%ldms", (long)s->blit.last);
    snprintf(avg, sizeof avg, "%ldms", (long)s->blit.avg);
    stat_row("blit", val, avg);

    uint32_t disp_last = s->disp.last ? 10000u / (uint32_t)s->disp.last : 0;
    fmt_fps(val, sizeof val, disp_last);
    fmt_fps(avg, sizeof avg, s->disp_fps_tenths);
    stat_row("fps", val, avg);

    snprintf(val, sizeof val, "%ldms", (long)s->render_loop.last);
    snprintf(avg, sizeof avg, "%ldms", (long)s->render_loop.avg);
    stat_row("loop", val, avg);
}

static void cmd_stream_output(const screen_stats_t *s)
{
    term_println("=== STREAM ===");
    stat_receive(s);
    stat_render(s);
}

static void cmd_status(void)
{
    term_println("--- STATUS ---");
    term_printfln("uptime       %lus", (unsigned long)(esp_timer_get_time() / 1000000));
    term_printfln("free heap    %luB", (unsigned long)esp_get_free_heap_size());
    term_printfln("free PSRAM   %luB", (unsigned long)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    term_printfln("WiFi clients %d",   wifi_ap_sta_count());
    term_printfln("cam stream   %s",   screen_state_is_streaming() ? "connected" : "disconnected");
    term_printfln("scene        %s",   screen_state_scene_name(screen_state_get_scene()));
}

static void cmd_stream_live(void)
{
    char up_seq[12];
    snprintf(up_seq, sizeof(up_seq), "\033[%dA", STREAM_LINE_COUNT);
    screen_stats_t stats;
    screen_stats_get(&stats);
    cmd_stream_output(&stats);
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(200));
        if(term_try_getchar() == 'q') break;
        screen_stats_get(&stats);
        term_write(up_seq);
        term_write("\033[J");
        cmd_stream_output(&stats);
    }
}

static void cmd_diag(void)
{
    term_println("--- DIAG ---");
    term_printfln("tasks       %lu",   (unsigned long)uxTaskGetNumberOfTasks());
    term_printfln("min heap    %luB",  (unsigned long)esp_get_minimum_free_heap_size());
    term_printfln("free int    %luB",  (unsigned long)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    term_printfln("free PSRAM  %luB",  (unsigned long)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

static void cmd_camdiag(void)
{
    cam_diag_pkt_t d;
    screen_state_get_cam(&d);
    char buf[16];
    term_println("--- CAM DIAG ---");
    term_printfln("heap        %luB",  (unsigned long)d.free_heap);
    term_printfln("uptime      %lus",  (unsigned long)d.uptime_s);
    term_printfln("rssi        %ddBm", (int)d.rssi_dbm);
    fmt_temp(buf, sizeof buf, &d);
    term_printfln("temp        %s", buf);
    fmt_humi(buf, sizeof buf, &d);
    term_printfln("humidity    %s", buf);
    fmt_pres(buf, sizeof buf, &d);
    term_printfln("pressure    %s", buf);
}

static void cmd_help(void)
{
    term_println("commands:");
    term_println("  STATUS   uptime, heap, WiFi clients, stream connection, scene");
    term_println("  STREAM   live stream stats (q to exit)");
    term_println("  DIAG     heap watermarks, task count");
    term_println("  CAMDIAG  cam heap, uptime, RSSI, sensor data");
    term_println("  HELP     this list");
}

void term_dispatch(const char *line)
{
    if     (strcmp(line, "STATUS")  == 0) cmd_status();
    else if(strcmp(line, "STREAM")  == 0) cmd_stream_live();
    else if(strcmp(line, "DIAG")    == 0) cmd_diag();
    else if(strcmp(line, "CAMDIAG") == 0) cmd_camdiag();
    else if(strcmp(line, "HELP")    == 0) cmd_help();
    else term_printfln("unknown command '%s' — try HELP", line);
}
