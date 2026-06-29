#include "NetworkMock.hpp"
#include <cstring>

bool NetworkMock::connect(const char *host, uint16_t port)
{
    (void)host; (void)port;
    connected = true;
    return true;
}

void NetworkMock::disconnect()        { connected = false; }
bool NetworkMock::isConnected() const { return connected; }

bool NetworkMock::send(const uint8_t *buf, size_t len)
{
    (void)buf; (void)len;
    if (fail_next_send) {
        fail_next_send = false;
        return false;
    }
    send_count++;
    return true;
}

int NetworkMock::recv(uint8_t *buf, size_t len)
{
    int n = inject_len < (int)len ? inject_len : (int)len;
    std::memcpy(buf, inject_buf, n);
    inject_len = 0;
    return n;
}
