#pragma once
#include <stdint.h>

typedef enum {
    FRAG_DISCARD,   // packet is malformed or out of range — ignore
    FRAG_PARTIAL,   // fragment accepted; frame not yet complete
    FRAG_COMPLETE,  // all fragments received; frame is ready in frame_pool_asm()
} frag_result_t;

/**
 * @brief  Parses a raw UDP video packet and copies its payload into frame_pool_asm().
 * @param  pkt                 Raw packet bytes.
 * @param  pkt_len             Length of pkt in bytes.
 * @param[out] out_frame_len     Total assembled frame length in bytes; valid on FRAG_COMPLETE.
 * @param[out] out_transfer_ms   Time since the first fragment of this frame in ms; valid on FRAG_COMPLETE.
 * @return FRAG_DISCARD if the packet is malformed, FRAG_PARTIAL if more fragments are pending,
 *         or FRAG_COMPLETE when the full frame is ready in frame_pool_asm().
 */
frag_result_t frag_rx(const uint8_t *pkt, int pkt_len,
                       uint32_t *out_frame_len, int32_t *out_transfer_ms);
