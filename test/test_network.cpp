#include "unity.h"
#include "NetworkMock.hpp"

void test_network_connect_sets_flag(void)
{
    NetworkMock net;
    TEST_ASSERT_FALSE(net.isConnected());
    net.connect("192.168.4.1", 3334);
    TEST_ASSERT_TRUE(net.isConnected());
}

void test_network_disconnect_clears_flag(void)
{
    NetworkMock net;
    net.connect("192.168.4.1", 3334);
    net.disconnect();
    TEST_ASSERT_FALSE(net.isConnected());
}

void test_network_send_increments_count(void)
{
    NetworkMock net;
    net.connect("192.168.4.1", 3334);
    uint8_t buf[] = { 0xAB, 0x01 };
    net.send(buf, sizeof(buf));
    net.send(buf, sizeof(buf));
    TEST_ASSERT_EQUAL(2, net.send_count);
}

void test_network_send_failure(void)
{
    NetworkMock net;
    net.connect("192.168.4.1", 3334);
    net.fail_next_send = true;
    uint8_t buf[] = { 0x01 };
    bool ok = net.send(buf, 1);
    TEST_ASSERT_FALSE(ok);
}

void test_network_recv_injected_bytes(void)
{
    NetworkMock net;
    net.connect("192.168.4.1", 3334);
    net.inject_buf[0] = 0x01;
    net.inject_len    = 1;

    uint8_t out = 0;
    int n = net.recv(&out, sizeof(out));
    TEST_ASSERT_EQUAL(1, n);
    TEST_ASSERT_EQUAL(0x01, out);
}
