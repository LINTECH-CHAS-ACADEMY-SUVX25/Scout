#include "unity.h"
#include "frag_rx.h"
#include "rc_protocol.h"
#include <arpa/inet.h>
#include <string.h>

static void make_pkt(uint8_t *buf, int *len,
                     uint16_t seq, uint8_t fi, uint8_t frags,
                     uint32_t frame_len, const uint8_t *data, int data_len)
{
    uint16_t seq_be = htons(seq);
    memcpy(buf, &seq_be, 2);
    buf[2] = fi;
    buf[3] = frags;
    int pos = 4;
    if (fi == 0) {
        buf[pos++] = FRAME_MAGIC;
        uint32_t flen_be = htonl(frame_len);
        memcpy(buf + pos, &flen_be, 4);
        pos += 4;
    }
    if (data && data_len > 0) {
        memcpy(buf + pos, data, data_len);
        pos += data_len;
    }
    *len = pos;
}

void test_frag_rx_too_short(void)
{
    uint8_t pkt[] = { 0x00, 0x01, 0x00 };
    uint32_t fl; int32_t tm;
    TEST_ASSERT_EQUAL(FRAG_DISCARD, frag_rx(pkt, 3, &fl, &tm));
}

void test_frag_rx_frags_zero(void)
{
    uint8_t pkt[] = { 0x00, 0x01, 0x00, 0x00 };
    uint32_t fl; int32_t tm;
    TEST_ASSERT_EQUAL(FRAG_DISCARD, frag_rx(pkt, 4, &fl, &tm));
}

void test_frag_rx_fi_out_of_range(void)
{
    uint8_t pkt[] = { 0x00, 0x01, 0x02, 0x02 }; /* fi=2 >= frags=2 */
    uint32_t fl; int32_t tm;
    TEST_ASSERT_EQUAL(FRAG_DISCARD, frag_rx(pkt, 4, &fl, &tm));
}

void test_frag_rx_wrong_magic(void)
{
    uint8_t pkt[PKT_MAX];
    int len;
    make_pkt(pkt, &len, 1, 0, 1, 3, NULL, 0);
    pkt[4] = 0x00; /* corrupt magic */
    uint32_t fl; int32_t tm;
    TEST_ASSERT_EQUAL(FRAG_DISCARD, frag_rx(pkt, len, &fl, &tm));
}

void test_frag_rx_single_fragment_complete(void)
{
    uint8_t pkt[PKT_MAX];
    int len;
    uint8_t jpeg[] = { 0xFF, 0xD8, 0xFF };
    make_pkt(pkt, &len, 10, 0, 1, sizeof(jpeg), jpeg, sizeof(jpeg));
    uint32_t fl; int32_t tm;
    TEST_ASSERT_EQUAL(FRAG_COMPLETE, frag_rx(pkt, len, &fl, &tm));
    TEST_ASSERT_EQUAL(sizeof(jpeg), fl);
}

void test_frag_rx_partial_then_complete(void)
{
    uint8_t pkt[PKT_MAX];
    int len;
    uint8_t data[FIRST_DATA + 4];
    memset(data, 0xCC, sizeof(data));

    /* Send fragment 0 */
    make_pkt(pkt, &len, 20, 0, 2, sizeof(data), data, FIRST_DATA);
    uint32_t fl; int32_t tm;
    TEST_ASSERT_EQUAL(FRAG_PARTIAL, frag_rx(pkt, len, &fl, &tm));

    /* Send fragment 1 */
    uint8_t pkt2[PKT_MAX];
    int len2;
    uint16_t seq_be = htons(20);
    memcpy(pkt2, &seq_be, 2);
    pkt2[2] = 1; pkt2[3] = 2;
    memcpy(pkt2 + 4, data + FIRST_DATA, 4);
    len2 = 4 + 4;
    TEST_ASSERT_EQUAL(FRAG_COMPLETE, frag_rx(pkt2, len2, &fl, &tm));
    TEST_ASSERT_EQUAL(sizeof(data), fl);
}

void test_frag_rx_new_seq_resets_state(void)
{
    uint8_t pkt[PKT_MAX];
    int len;
    uint8_t jpeg[] = { 0xFF, 0xD8, 0xFF };

    /* Start seq=1 with 2 frags, only send frag 0 */
    make_pkt(pkt, &len, 1, 0, 2, sizeof(jpeg), jpeg, sizeof(jpeg));
    uint32_t fl; int32_t tm;
    frag_rx(pkt, len, &fl, &tm);

    /* New seq=2 single frag — should complete cleanly */
    make_pkt(pkt, &len, 2, 0, 1, sizeof(jpeg), jpeg, sizeof(jpeg));
    TEST_ASSERT_EQUAL(FRAG_COMPLETE, frag_rx(pkt, len, &fl, &tm));
    TEST_ASSERT_EQUAL(sizeof(jpeg), fl);
}
