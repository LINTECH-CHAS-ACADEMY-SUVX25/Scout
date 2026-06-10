#include "monitor_cmds.h"
#include "uart_console.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>

static void cmd_status(const monitor_status_t *s)
{
    uart_console_println("--- STATUS ---");
    uart_console_printfln("uptime       %lus", (unsigned long)s->uptime_s);
    uart_console_printfln("free heap    %luB", (unsigned long)s->free_heap);
    uart_console_printfln("free PSRAM   %luB", (unsigned long)s->free_psram);
    uart_console_printfln("WiFi clients %d",   s->sta_count);
    uart_console_printfln("cam stream   %s",   s->stream_connected ? "connected" : "disconnected");
}

#define LABEL_W 11   // label column width (after the 2-space row indent)
#define LAST_W   9    // "last" value column width
#define AVG_W   10    // "avg" value column width

// Prints a section header: name flush-left, "last" / "avg" over the value columns.
static void stat_header(const char *section)
{
    uart_console_printfln("%-*s%*s%*s", LABEL_W + 2, section, LAST_W, "last", AVG_W, "avg");
}

// Prints an indented "label  last  avg" data row.
static void stat_row(const char *label, const char *val, const char *avg)
{
    uart_console_printfln("  %-*s%*s%*s", LABEL_W, label, LAST_W, val, AVG_W, avg);
}

// Prints an indented single-value row (no average column).
static void stat_one(const char *label, uint32_t val)
{
    uart_console_printfln("  %-*s%*lu", LABEL_W, label, LAST_W, (unsigned long)val);
}

// Formats fps-as-tenths into "NN.Nfps".
static void fmt_fps(char *buf, size_t size, uint32_t tenths)
{
    snprintf(buf, size, "%lu.%lufps",
        (unsigned long)(tenths / 10), (unsigned long)(tenths % 10));
}

static void stat_receive(const screen_state_t *s)
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

    // max fps = 1000 / frame gap — the ceiling the display rate can never exceed
    uint32_t max_last = s->rx_interval.last ? 10000u / (uint32_t)s->rx_interval.last : 0;
    fmt_fps(val, sizeof val, max_last);
    fmt_fps(avg, sizeof avg, s->rx_fps_tenths);
    stat_row("max fps", val, avg);

    snprintf(val, sizeof val, "%ldms", (long)s->stream_loop.last);
    snprintf(avg, sizeof avg, "%ldms", (long)s->stream_loop.avg);
    stat_row("loop", val, avg);
}

static void stat_render(const screen_state_t *s)
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

void monitor_cmd_stream(const screen_state_t *s)
{
    uart_console_println("=== STREAM ===");
    stat_receive(s);
    stat_render(s);
}

static void cmd_diag(const monitor_diag_t *d)
{
    uart_console_println("--- DIAG ---");
    uart_console_printfln("tasks       %lu",   (unsigned long)d->n_tasks);
    uart_console_printfln("min heap    %luB",  (unsigned long)d->min_heap);
    uart_console_printfln("free int    %luB",  (unsigned long)d->free_int);
    uart_console_printfln("free PSRAM  %luB",  (unsigned long)d->free_psram);
}

static void cmd_stream_live(void)
{
    char up_seq[12];
    snprintf(up_seq, sizeof(up_seq), "\033[%dA", STREAM_LINE_COUNT);
    screen_state_t stats;
    screen_state_get(&stats);
    monitor_cmd_stream(&stats);
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(200));
        if(uart_console_try_getchar() == 'q') break;
        screen_state_get(&stats);
        uart_console_write(up_seq);
        uart_console_write("\033[J");
        monitor_cmd_stream(&stats);
    }
}

static void cmd_help(void)
{
    uart_console_println("commands:");
    uart_console_println("  STATUS  uptime, heap, WiFi clients, stream connection");
    uart_console_println("  STREAM  live stream stats (q to exit)");
    uart_console_println("  DIAG    heap watermarks, task count");
    uart_console_println("  HELP    this list");
}

void monitor_dispatch(const char           *line,
                      const monitor_status_t *status,
                      const monitor_diag_t   *diag)
{
    if     (strcmp(line, "STATUS") == 0) cmd_status(status);
    else if(strcmp(line, "STREAM") == 0) cmd_stream_live();
    else if(strcmp(line, "DIAG")   == 0) cmd_diag(diag);
    else if(strcmp(line, "HELP")   == 0) cmd_help();
    else uart_console_printfln("unknown command '%s' — try HELP", line);
}
