#pragma once
#include "INetwork.hpp"
#include "lwip/sockets.h"

// INetwork implementation over UDP. bind_port is the port to receive on;
// pass 0 for a send-only socket.
class NetworkEsp32 : public INetwork {
public:
    explicit NetworkEsp32(uint16_t bind_port = 0);

    bool connect(const char *host, uint16_t port) override;
    void disconnect() override;
    bool isConnected() const override;
    bool send(const uint8_t *buf, size_t len) override;
    int  recv(uint8_t *buf, size_t len) override;

private:
    uint16_t           m_bind_port;
    int                m_sock;
    struct sockaddr_in m_dst;
    bool               m_connected;
};
