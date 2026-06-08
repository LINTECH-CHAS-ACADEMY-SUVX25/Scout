#include "transport.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include <errno.h>

// UDP datagram implementation of the transport interface.

static const char *TAG = "transport";

static struct sockaddr_in to_sa(const transport_addr_t *a)
{
    return (struct sockaddr_in){
        .sin_family      = AF_INET,
        .sin_addr.s_addr = a->ip,
        .sin_port        = a->port,
    };
}

static transport_addr_t from_sa(const struct sockaddr_in *sa)
{
    return (transport_addr_t){
        .ip   = sa->sin_addr.s_addr,
        .port = sa->sin_port,
    };
}

int transport_open(uint16_t bind_port)
{
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sock < 0) {
        ESP_LOGE(TAG, "socket() failed");
        return -1;
    }
    if(bind_port != 0) {
        struct sockaddr_in addr = {
            .sin_family      = AF_INET,
            .sin_port        = htons(bind_port),
            .sin_addr.s_addr = INADDR_ANY,
        };
        if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
            ESP_LOGE(TAG, "bind() failed on port %u", bind_port);
            close(sock);
            return -1;
        }
    }
    return sock;
}

void transport_close(int handle)
{
    if(handle >= 0) close(handle);
}

void transport_set_rcvbuf(int handle, int bytes)
{
    setsockopt(handle, SOL_SOCKET, SO_RCVBUF, &bytes, sizeof(bytes));
}

void transport_set_send_timeout(int handle, int seconds)
{
    struct timeval tv = { .tv_sec = seconds, .tv_usec = 0 };
    setsockopt(handle, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
}

int transport_recv(int handle, void *buf, size_t len, transport_addr_t *src)
{
    if(src) {
        struct sockaddr_in sa;
        socklen_t slen = sizeof(sa);
        int n = recvfrom(handle, buf, len, 0, (struct sockaddr *)&sa, &slen);
        if(n >= 0) *src = from_sa(&sa);
        return n;
    }
    return recvfrom(handle, buf, len, 0, NULL, NULL);
}

int transport_try_recv(int handle, void *buf, size_t len)
{
    int n = recv(handle, buf, len, MSG_DONTWAIT);
    if(n < 0)
        return (errno == EWOULDBLOCK || errno == EAGAIN) ? 0 : -1;
    return n;
}

void transport_send(int handle, const transport_addr_t *dst,
                    const void *buf, size_t len)
{
    struct sockaddr_in sa = to_sa(dst);
    sendto(handle, buf, len, 0, (const struct sockaddr *)&sa, sizeof(sa));
}

transport_addr_t transport_make_addr(const char *ip, uint16_t port)
{
    transport_addr_t addr = { .port = htons(port) };
    inet_pton(AF_INET, ip, &addr.ip);
    return addr;
}

transport_addr_t transport_addr_with_port(const transport_addr_t *src,
                                          uint16_t port)
{
    return (transport_addr_t){ .ip = src->ip, .port = htons(port) };
}

const char *transport_addr_ip(const transport_addr_t *addr)
{
    static char buf[INET_ADDRSTRLEN];
    struct in_addr in = { .s_addr = addr->ip };
    inet_ntop(AF_INET, &in, buf, sizeof(buf));
    return buf;
}
