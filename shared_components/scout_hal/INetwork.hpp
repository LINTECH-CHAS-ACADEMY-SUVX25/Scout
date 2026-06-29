#pragma once
#include <cstdint>
#include <cstddef>

class INetwork {
public:
    virtual ~INetwork() = default;
    virtual bool connect(const char *host, uint16_t port) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual bool send(const uint8_t *buf, size_t len) = 0;
    virtual int  recv(uint8_t *buf, size_t len) = 0;
};
