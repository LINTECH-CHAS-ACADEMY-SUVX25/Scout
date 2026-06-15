#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

void term_init(void);
void term_write(const char *s);
void term_println(const char *s);
void term_printfln(const char *fmt, ...);
bool term_read_byte(uint8_t *ch, uint32_t timeout_ms);
int  term_try_getchar(void);
bool term_read_line(char *buf, size_t size);

typedef void (*term_handler_t)(const char *line);
void term_run_handler(term_handler_t handler);
void term_dispatch(const char *line);
