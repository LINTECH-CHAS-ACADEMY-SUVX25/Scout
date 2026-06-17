#include "unity.h"

void test_frag_tx_single_fragment(void);
void test_frag_tx_first_fragment_has_magic(void);
void test_frag_tx_frame_len_encoded_big_endian(void);
void test_frag_tx_sequence_number(void);
void test_frag_tx_multi_fragment_count(void);

void test_frag_rx_too_short(void);
void test_frag_rx_frags_zero(void);
void test_frag_rx_fi_out_of_range(void);
void test_frag_rx_wrong_magic(void);
void test_frag_rx_single_fragment_complete(void);
void test_frag_rx_partial_then_complete(void);
void test_frag_rx_new_seq_resets_state(void);

void test_joy_center_is_stop(void);
void test_joy_forward(void);
void test_joy_backward(void);
void test_joy_left(void);
void test_joy_right(void);
void test_joy_forward_left_combined(void);
void test_joy_speed_capped_at_255(void);

void setUp(void) {}
void tearDown(void) {}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_frag_tx_single_fragment);
    RUN_TEST(test_frag_tx_first_fragment_has_magic);
    RUN_TEST(test_frag_tx_frame_len_encoded_big_endian);
    RUN_TEST(test_frag_tx_sequence_number);
    RUN_TEST(test_frag_tx_multi_fragment_count);

    RUN_TEST(test_frag_rx_too_short);
    RUN_TEST(test_frag_rx_frags_zero);
    RUN_TEST(test_frag_rx_fi_out_of_range);
    RUN_TEST(test_frag_rx_wrong_magic);
    RUN_TEST(test_frag_rx_single_fragment_complete);
    RUN_TEST(test_frag_rx_partial_then_complete);
    RUN_TEST(test_frag_rx_new_seq_resets_state);

    RUN_TEST(test_joy_center_is_stop);
    RUN_TEST(test_joy_forward);
    RUN_TEST(test_joy_backward);
    RUN_TEST(test_joy_left);
    RUN_TEST(test_joy_right);
    RUN_TEST(test_joy_forward_left_combined);
    RUN_TEST(test_joy_speed_capped_at_255);

    return UNITY_END();
}
