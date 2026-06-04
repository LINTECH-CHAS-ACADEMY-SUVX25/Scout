#include "udp.h"
#include <errno.h>
#include "esp_log.h"
#include "lwip/inet.h"

static const char *TAG = "udp";

int udp_open(uint16_t bind_port)
{
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0)
    {
        ESP_LOGE(TAG, "socket() failed");
        return -1;
    }
    if (bind_port != 0)
    {
        struct sockaddr_in addr =
        {
            .sin_family      = AF_INET,
            .sin_port        = htons(bind_port),
            .sin_addr.s_addr = INADDR_ANY,
        };
        if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) != 0)
        {
            ESP_LOGE(TAG, "bind() failed");
            close(sock);
            return -1;
        }
    }
    return sock;
}

void udp_close(int sock)
{
    if (sock >= 0) close(sock);
}

void udp_set_rcvbuf(int sock, int bytes)
{
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &bytes, sizeof(bytes));
}

int udp_rx(int sock, void *buf, size_t len, struct sockaddr_in *src)
{
    if (src)
    {
        socklen_t slen = sizeof(*src);
        return recvfrom(sock, buf, len, 0, (struct sockaddr *)src, &slen);
    }
    return recvfrom(sock, buf, len, 0, NULL, NULL);
}

int udp_try_recv(int sock, void *buf, size_t len)
{
    int n = recv(sock, buf, len, MSG_DONTWAIT);
    if (n < 0)
        return (errno == EWOULDBLOCK || errno == EAGAIN) ? 0 : -1;
    return n;
}

void udp_tx(int sock, const struct sockaddr_in *dst, const void *buf, size_t len)
{
    sendto(sock, buf, len, 0, (const struct sockaddr *)dst, sizeof(*dst));
}

struct sockaddr_in udp_addr(const char *ip, uint16_t port)
{
    struct sockaddr_in addr =
    {
        .sin_family = AF_INET,
        .sin_port   = htons(port),
    };
    inet_pton(AF_INET, ip, &addr.sin_addr);
    return addr;
}

const char *udp_ip_str(const struct sockaddr_in *addr)
{
    static char buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr->sin_addr, buf, sizeof(buf));
    return buf;
}
