#pragma once
#include <stdint.h>
#include <stdbool.h>

// Installs the UART0 driver. Call once before any other uart_console function.
void uart_console_init(void);

// Writes a null-terminated string to UART0 (no newline).
void uart_console_write(const char *s);

// Writes a null-terminated string followed by \r\n to UART0.
void uart_console_println(const char *s);

// Writes a formatted line to UART0.
void uart_console_printfln(const char *fmt, ...);

// Reads one byte from UART0. Returns true if a byte was received within timeout_ms.
bool uart_console_read_byte(uint8_t *ch, uint32_t timeout_ms);
