#include "frame_pool.h"
#include "rc_protocol.h"

static uint8_t s_asm_buf[FRAME_MAX];

uint8_t *frame_pool_asm(void)
{
    return s_asm_buf;
}
