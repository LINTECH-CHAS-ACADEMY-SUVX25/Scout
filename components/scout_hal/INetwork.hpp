#pragma once
#include <cstdint>
#include <cstddef>

class INetwork {
public:
    virtual ~INetwork() = default;

    virtual bool connect(const char *host, uint16_t port) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;

    // Sends exactly len bytes. Returns false on error.
    virtual bool send(const uint8_t *buf, size_t len) = 0;

    // Receives up to len bytes. Returns number of bytes received, -1 on error.
    virtual int recv(uint8_t *buf, size_t len) = 0;
};
