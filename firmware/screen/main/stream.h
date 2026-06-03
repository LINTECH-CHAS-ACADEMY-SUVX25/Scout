#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void stream_init(void);
int  stream_get_client_sock(void);
bool stream_try_decode(uint8_t *out_buf, size_t out_size);
