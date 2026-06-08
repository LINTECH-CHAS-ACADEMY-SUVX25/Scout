#pragma once
#include <stdint.h>

typedef enum {
    FRAG_DISCARD,   // packet is malformed or out of range — ignore
    FRAG_PARTIAL,   // fragment accepted; frame not yet complete
    FRAG_COMPLETE,  // all fragments received; frame is ready in frame_buf_asm()
} frag_result_t;

// Parses a raw UDP video packet and copies its payload into frame_buf_asm().
// On FRAG_COMPLETE, out_frame_len and out_transfer_ms are set.
frag_result_t frag_rx(const uint8_t *pkt, int pkt_len,
                       uint32_t *out_frame_len, int32_t *out_transfer_ms);
