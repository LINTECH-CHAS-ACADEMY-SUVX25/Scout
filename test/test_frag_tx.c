#include "unity.h"
#include "frag_tx.h"
#include "rc_protocol.h"
#include <arpa/inet.h>
#include <string.h>

extern uint8_t  udp_stub_pkts[64][PKT_MAX];
extern size_t   udp_stub_lens[64];
extern int      udp_stub_call_count;
extern void     udp_stub_reset(void);

static uint8_t s_frame[FRAME_MAX];

void test_frag_tx_single_fragment(void)
{
    udp_stub_reset();
    memset(s_frame, 0xAA, 10);
    frag_tx(0, s_frame, 10, 1);
    TEST_ASSERT_EQUAL(1, udp_stub_call_count);
}

void test_frag_tx_first_fragment_has_magic(void)
{
    udp_stub_reset();
    memset(s_frame, 0x00, 10);
    frag_tx(0, s_frame, 10, 1);
    /* pkt layout: [seq:2][fi:1][frags:1][magic:1]... */
    TEST_ASSERT_EQUAL(FRAME_MAGIC, udp_stub_pkts[0][4]);
}

void test_frag_tx_frame_len_encoded_big_endian(void)
{
    udp_stub_reset();
    uint32_t frame_len = 10;
    memset(s_frame, 0x00, frame_len);
    frag_tx(0, s_frame, frame_len, 1);
    uint32_t encoded;
    memcpy(&encoded, &udp_stub_pkts[0][5], 4);
    TEST_ASSERT_EQUAL(frame_len, ntohl(encoded));
}

void test_frag_tx_sequence_number(void)
{
    udp_stub_reset();
    memset(s_frame, 0x00, 10);
    frag_tx(0, s_frame, 10, 0x1234);
    uint16_t seq;
    memcpy(&seq, udp_stub_pkts[0], 2);
    TEST_ASSERT_EQUAL(0x1234, ntohs(seq));
}

void test_frag_tx_multi_fragment_count(void)
{
    udp_stub_reset();
    /* FIRST_DATA bytes fit in frag 0; anything beyond needs extra frags */
    uint32_t len = FIRST_DATA + 1;
    memset(s_frame, 0xBB, len);
    frag_tx(0, s_frame, len, 2);
    TEST_ASSERT_EQUAL(2, udp_stub_call_count);
}
