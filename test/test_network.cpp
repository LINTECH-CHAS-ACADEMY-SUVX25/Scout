#include "test_runner.hpp"
#include "NetworkMock.hpp"

TEST(test_network_connect_sets_flag)
{
    NetworkMock net;
    ASSERT_TRUE(!net.isConnected());
    net.connect("192.168.4.1", 3334);
    ASSERT_TRUE(net.isConnected());
}

TEST(test_network_disconnect_clears_flag)
{
    NetworkMock net;
    net.connect("192.168.4.1", 3334);
    net.disconnect();
    ASSERT_TRUE(!net.isConnected());
}

TEST(test_network_send_increments_count)
{
    NetworkMock net;
    net.connect("192.168.4.1", 3334);
    uint8_t buf[] = { 0xAB, 0x01 };
    net.send(buf, sizeof(buf));
    net.send(buf, sizeof(buf));
    ASSERT_EQ(net.send_count, 2);
}

TEST(test_network_send_failure)
{
    NetworkMock net;
    net.connect("192.168.4.1", 3334);
    net.fail_next_send = true;
    uint8_t buf[] = { 0x01 };
    bool ok = net.send(buf, 1);
    ASSERT_TRUE(!ok);
}

TEST(test_network_recv_injected_bytes)
{
    NetworkMock net;
    net.connect("192.168.4.1", 3334);
    net.inject_buf[0] = 0x01; // CMD_FORWARD
    net.inject_len    = 1;

    uint8_t out = 0;
    int n = net.recv(&out, sizeof(out));
    ASSERT_EQ(n, 1);
    ASSERT_EQ(out, 0x01);
}

void run_network_tests()
{
    printf("Network:\n");
    RUN(test_network_connect_sets_flag);
    RUN(test_network_disconnect_clears_flag);
    RUN(test_network_send_increments_count);
    RUN(test_network_send_failure);
    RUN(test_network_recv_injected_bytes);
}
