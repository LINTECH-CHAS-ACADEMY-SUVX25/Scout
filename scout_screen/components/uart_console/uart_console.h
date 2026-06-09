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

// Reads one byte without blocking. Returns the byte value, or -1 if nothing is available.
int uart_console_try_getchar(void);

// Reads one line from UART0 with echo and backspace handling.
// Returns true when a complete line is ready in buf.
bool uart_console_read_line(char *buf, size_t size);
