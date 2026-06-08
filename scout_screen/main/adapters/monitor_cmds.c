#include "monitor_cmds.h"
#include "uart_console.h"
#include <string.h>

static void cmd_status(const monitor_status_t *s)
{
    uart_console_println("--- STATUS ---");
    uart_console_printfln("uptime       %lus", (unsigned long)s->uptime_s);
    uart_console_printfln("free heap    %luB", (unsigned long)s->free_heap);
    uart_console_printfln("free PSRAM   %luB", (unsigned long)s->free_psram);
    uart_console_printfln("WiFi clients %d",   s->sta_count);
    uart_console_printfln("cam stream   %s",   s->stream_connected ? "connected" : "disconnected");
}

static void cmd_stream(const stream_stats_t *s)
{
    uart_console_println("--- STREAM ---");
    uart_console_printfln("frames      %lu",   (unsigned long)s->frame_count);
    uart_console_printfln("last frame  %luB",  (unsigned long)s->last_frame_bytes);
    uart_console_printfln("transfer    %ldms", (long)s->last_transfer_ms);
    uart_console_printfln("decode      %ldms", (long)s->last_decode_ms);
}

static void cmd_diag(const monitor_diag_t *d)
{
    uart_console_println("--- DIAG ---");
    uart_console_printfln("tasks       %lu",   (unsigned long)d->n_tasks);
    uart_console_printfln("min heap    %luB",  (unsigned long)d->min_heap);
    uart_console_printfln("free int    %luB",  (unsigned long)d->free_int);
    uart_console_printfln("free PSRAM  %luB",  (unsigned long)d->free_psram);
}

static void cmd_help(void)
{
    uart_console_println("commands:");
    uart_console_println("  STATUS  uptime, heap, WiFi clients, stream connection");
    uart_console_println("  STREAM  frame count, size, transfer/decode timing");
    uart_console_println("  DIAG    heap watermarks, task count");
    uart_console_println("  HELP    this list");
}

void monitor_dispatch(const char           *line,
                      const monitor_status_t *status,
                      const stream_stats_t   *stream,
                      const monitor_diag_t   *diag)
{
    if     (strcmp(line, "STATUS") == 0) cmd_status(status);
    else if(strcmp(line, "STREAM") == 0) cmd_stream(stream);
    else if(strcmp(line, "DIAG")   == 0) cmd_diag(diag);
    else if(strcmp(line, "HELP")   == 0) cmd_help();
    else uart_console_printfln("unknown command '%s' — try HELP", line);
}
