#include "NetworkEsp32.hpp"

extern "C" {
#include "udp.h"
}

bool NetworkEsp32::connect(const char *host, uint16_t port)
{
    sock_ = udp_open(0);
    if(sock_ < 0) return false;
    dest_      = udp_addr(host, port);
    connected_ = true;
    return true;
}

void NetworkEsp32::disconnect()
{
    udp_close(sock_);
    sock_      = -1;
    connected_ = false;
}

bool NetworkEsp32::isConnected() const
{
    return connected_;
}

bool NetworkEsp32::send(const uint8_t *buf, size_t len)
{
    if(!connected_) return false;
    udp_tx(sock_, &dest_, buf, len);
    return true;
}

int NetworkEsp32::recv(uint8_t *buf, size_t len)
{
    return udp_try_recv(sock_, buf, len);
}
