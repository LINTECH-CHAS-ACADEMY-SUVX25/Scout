#include "NetworkEsp32.hpp"

extern "C" {
#include "udp.h"
}

bool NetworkEsp32::connect(const char *host, uint16_t port)
{
    s_sock = udp_open(0);
    if(s_sock < 0) return false;
    s_dest      = udp_addr(host, port);
    s_connected = true;
    return true;
}

void NetworkEsp32::disconnect()
{
    udp_close(s_sock);
    s_sock      = -1;
    s_connected = false;
}

bool NetworkEsp32::isConnected() const
{
    return s_connected;
}

bool NetworkEsp32::send(const uint8_t *buf, size_t len)
{
    if(!s_connected) return false;
    udp_tx(s_sock, &s_dest, buf, len);
    return true;
}

int NetworkEsp32::recv(uint8_t *buf, size_t len)
{
    return udp_try_recv(s_sock, buf, len);
}
