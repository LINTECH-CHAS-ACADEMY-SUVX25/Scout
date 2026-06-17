#include "NetworkMock.hpp"
#include <cstdio>
#include <cstring>

bool NetworkMock::connect(const char *host, uint16_t port)
{
    connected = true;
    printf("[NetworkMock] connect %s:%d\n", host, port);
    return true;
}

void NetworkMock::disconnect()
{
    connected = false;
    printf("[NetworkMock] disconnect\n");
}

bool NetworkMock::isConnected() const
{
    return connected;
}

bool NetworkMock::send(const uint8_t *buf, size_t len)
{
    if (fail_next_send) {
        fail_next_send = false;
        printf("[NetworkMock] send FAILED (injected)\n");
        return false;
    }
    send_count++;
    printf("[NetworkMock] send %zu bytes\n", len);
    return true;
}

int NetworkMock::recv(uint8_t *buf, size_t len)
{
    if (inject_len <= 0) return 0;
    int n = inject_len < (int)len ? inject_len : (int)len;
    memcpy(buf, inject_buf, n);
    inject_len = 0;
    printf("[NetworkMock] recv %d bytes\n", n);
    return n;
}
