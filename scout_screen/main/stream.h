#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void stream_init(void);
void stream_send_cmd(uint8_t cmd);
bool stream_is_connected(void);
bool stream_try_decode(uint8_t *out_buf, size_t out_size);
