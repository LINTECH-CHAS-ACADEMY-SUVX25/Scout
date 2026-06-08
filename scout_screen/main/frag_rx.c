#include "frag_rx.h"
#include "frame_buf.h"
#include "rc_protocol.h"
#include "esp_timer.h"
#include "lwip/sockets.h"
#include <string.h>

// Raw packet parsing and fragment reassembly for the UDP video stream.
//
// Packet layout: [seq_hi][seq_lo][fi][frags] | data...
// Fragment 0 data starts with a 5-byte frame header:
//   [FRAME_MAGIC][frame_len_be (4 bytes)] | jpeg_data...
// All other fragments carry raw JPEG data placed at the correct byte offset.

static struct {
    uint16_t seq;
    uint8_t  frags;
    uint64_t rx_mask;
    uint32_t frame_len;
} s_rx;

static int64_t s_transfer_t;

frag_result_t frag_rx(const uint8_t *pkt, int pkt_len,
                       uint32_t *out_frame_len, int32_t *out_transfer_ms)
{
    if(pkt_len < 4) return FRAG_DISCARD;

    uint16_t seq;
    memcpy(&seq, pkt, 2);
    seq = ntohs(seq);
    uint8_t fi    = pkt[2];
    uint8_t frags = pkt[3];

    if(frags == 0 || fi >= frags || frags > MAX_FRAGS) return FRAG_DISCARD;

    if(s_rx.frags == 0 || seq != s_rx.seq || frags != s_rx.frags) {
        s_rx.seq       = seq;
        s_rx.frags     = frags;
        s_rx.rx_mask   = 0;
        s_rx.frame_len = 0;
        s_transfer_t   = esp_timer_get_time();
    }

    const uint8_t *data     = pkt + 4;
    int            data_len = pkt_len - 4;
    uint32_t       offset;

    if(fi == 0) {
        if(data_len < 5 || data[0] != FRAME_MAGIC) return FRAG_DISCARD;
        uint32_t flen_be;
        memcpy(&flen_be, data + 1, 4);
        s_rx.frame_len = ntohl(flen_be);
        if(s_rx.frame_len > FRAME_MAX) { s_rx.frags = 0; return FRAG_DISCARD; }
        data     += 5;
        data_len -= 5;
        offset    = 0;
    } else {
        offset = FIRST_DATA + (uint32_t)(fi - 1) * FRAG_SIZE;
    }

    if((uint32_t)data_len > FRAME_MAX - offset) return FRAG_DISCARD;
    memcpy(frame_buf_asm() + offset, data, data_len);
    s_rx.rx_mask |= (1ULL << fi);

    if(s_rx.rx_mask != ((1ULL << s_rx.frags) - 1)) return FRAG_PARTIAL;

    *out_frame_len   = s_rx.frame_len;
    *out_transfer_ms = (int32_t)((esp_timer_get_time() - s_transfer_t) / 1000);
    s_rx.frags = 0;
    return FRAG_COMPLETE;
}
