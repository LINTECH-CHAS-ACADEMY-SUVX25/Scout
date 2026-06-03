#pragma once
#include "INetwork.hpp"
#include <cstdint>
#include <cstddef>

class NetworkMock : public INetwork {
public:
    bool connect(const char *host, uint16_t port) override;
    void disconnect() override;
    bool isConnected() const override;
    bool send(const uint8_t *buf, size_t len) override;
    int  recv(uint8_t *buf, size_t len) override;

    bool connected       = false;
    int  send_count      = 0;
    bool fail_next_send  = false;

    // Inject bytes to be returned by the next recv() call.
    uint8_t inject_buf[16] = {};
    int     inject_len     = 0;
};
