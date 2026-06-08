#pragma once
#include "INetwork.hpp"
#include "lwip/sockets.h"

// Concrete INetwork implementation backed by a UDP socket.
// connect() opens the socket and records the destination address.
// Swap this for a TCP variant (NetworkTcp) without touching any task code.
class NetworkEsp32 : public INetwork {
public:
    bool connect(const char *host, uint16_t port) override;
    void disconnect() override;
    bool isConnected() const override;
    bool send(const uint8_t *buf, size_t len) override;
    int  recv(uint8_t *buf, size_t len) override;

private:
    int                s_sock      = -1;
    struct sockaddr_in s_dest      = {};
    bool               s_connected = false;
};
