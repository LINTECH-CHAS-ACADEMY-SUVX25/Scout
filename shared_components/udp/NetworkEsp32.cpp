#include "NetworkEsp32.hpp"

extern "C" {
#include "udp.h"
}

NetworkEsp32::NetworkEsp32(uint16_t bind_port)
    : m_bind_port(bind_port), m_sock(-1), m_dst{}, m_connected(false) {}

bool NetworkEsp32::connect(const char *host, uint16_t port)
{
    m_sock = udp_open(m_bind_port);
    if (m_sock < 0) return false;
    m_dst = udp_addr(host, port);
    m_connected = true;
    return true;
}

void NetworkEsp32::disconnect()
{
    if (m_sock >= 0) {
        udp_close(m_sock);
        m_sock = -1;
    }
    m_connected = false;
}

bool NetworkEsp32::isConnected() const { return m_connected; }

bool NetworkEsp32::send(const uint8_t *buf, size_t len)
{
    udp_tx(m_sock, &m_dst, buf, len);
    return true;
}

int NetworkEsp32::recv(uint8_t *buf, size_t len)
{
    return udp_rx(m_sock, buf, len, nullptr);
}
