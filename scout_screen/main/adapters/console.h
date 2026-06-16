#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

void term_init(void);
void term_write(const char *s);
void term_println(const char *s);
void term_printfln(const char *fmt, ...);

/**
 * @brief  Blocks up to timeout_ms waiting for one byte.
 * @param[out] ch          Byte received.
 * @param      timeout_ms  Maximum time to wait in milliseconds.
 * @return true if a byte was received; false on timeout.
 */
bool term_read_byte(uint8_t *ch, uint32_t timeout_ms);

/**
 * @brief  Non-blocking byte read.
 * @return The byte as an unsigned value, or -1 if no byte is waiting.
 */
int  term_try_getchar(void);

bool term_read_line(char *buf, size_t size);

typedef void (*term_handler_t)(const char *line);

/**
 * @brief  Runs the handler loop; blocks until the connection closes.
 * @param  handler  Callback invoked for each received line.
 */
void term_run_handler(term_handler_t handler);

/**
 * @brief  Parses and dispatches a command line to registered handlers.
 * @param  line  Null-terminated command string.
 */
void term_dispatch(const char *line);
