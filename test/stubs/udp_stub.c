#include "udp.h"
#include "rc_protocol.h"
#include <string.h>

uint8_t  udp_stub_pkts[64][PKT_MAX];
size_t   udp_stub_lens[64];
int      udp_stub_call_count;

void udp_stub_reset(void)
{
    udp_stub_call_count = 0;
}

struct sockaddr_in udp_addr(const char *ip, uint16_t port)
{
    (void)ip; (void)port;
    struct sockaddr_in addr = {0};
    return addr;
}

void udp_tx(int sock, const struct sockaddr_in *dst, const void *buf, size_t len)
{
    (void)sock; (void)dst;
    int i = udp_stub_call_count;
    if (i < 64) {
        udp_stub_lens[i] = len;
        memcpy(udp_stub_pkts[i], buf, len);
    }
    udp_stub_call_count++;
}
