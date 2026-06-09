#include "frag_tx.h"
#include "udp.h"
#include "rc_protocol.h"
#include <string.h>

// Packet assembly and fragmentation for the UDP video stream.
// Fragment 0's payload is prefixed with [FRAME_MAGIC:1][frame_len_be:4] so the
// receiver knows the full JPEG size before reassembling.
// All other fragments carry raw JPEG data at their respective byte offset.

static struct sockaddr_in s_dest;
static uint8_t s_pkt[PKT_MAX];

void frag_tx_init(const char *ip, uint16_t port)
{
    s_dest = udp_addr(ip, port);
}

void frag_tx(int sock, const uint8_t *buf, uint32_t len, uint16_t seq)
{
    uint8_t  n_frags = (len <= FIRST_DATA) ? 1 : 1 + (len - FIRST_DATA + FRAG_SIZE - 1) / FRAG_SIZE;
    uint32_t len_be  = htonl(len);
    uint32_t sent    = 0;

    for(uint8_t fi = 0; fi < n_frags; fi++) {
        uint8_t *p      = s_pkt;
        uint16_t seq_be = htons(seq);
        memcpy(p, &seq_be, 2); p += 2;
        *p++ = fi;
        *p++ = n_frags;

        uint32_t chunk;
        if(fi == 0) {
            *p++ = FRAME_MAGIC;
            memcpy(p, &len_be, 4); p += 4;
            chunk = (len < FIRST_DATA) ? len : FIRST_DATA;
        } else {
            uint32_t remaining = len - sent;
            chunk = (remaining < FRAG_SIZE) ? remaining : FRAG_SIZE;
        }
        memcpy(p, buf + sent, chunk);
        p    += chunk;
        sent += chunk;

        udp_tx(sock, &s_dest, s_pkt, p - s_pkt);
    }
}
