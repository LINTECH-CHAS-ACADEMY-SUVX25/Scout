#include "uart_console.h"
#include "esp_log.h"
#include "driver/uart.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "uart_console";

void uart_console_init(void)
{
    esp_err_t ret = uart_driver_install(UART_NUM_0, 256, 0, 0, NULL, 0);
    if(ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "uart_driver_install: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "UART0 console ready");
}

void uart_console_write(const char *s)
{
    uart_write_bytes(UART_NUM_0, s, strlen(s));
}

void uart_console_println(const char *s)
{
    uart_write_bytes(UART_NUM_0, s, strlen(s));
    uart_write_bytes(UART_NUM_0, "\r\n", 2);
}

void uart_console_printfln(const char *fmt, ...)
{
    char buf[96];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    uart_console_println(buf);
}

bool uart_console_read_byte(uint8_t *ch, uint32_t timeout_ms)
{
    return uart_read_bytes(UART_NUM_0, ch, 1, pdMS_TO_TICKS(timeout_ms)) == 1;
}
