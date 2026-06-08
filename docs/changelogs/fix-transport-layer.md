# fix/transport-layer

Added a protocol-agnostic transport adapter so task files never include `udp.h` or any lwip headers directly. Both projects build cleanly on this branch.

## transport adapter (`shared_components/transport/`)

- New `transport.h` — exposes `transport_addr_t` (opaque IPv4 endpoint) and functions: `transport_open`, `transport_close`, `transport_set_rcvbuf`, `transport_set_send_timeout`, `transport_recv`, `transport_try_recv`, `transport_send`, `transport_make_addr`, `transport_addr_with_port`, `transport_addr_ip`
- New `transport.c` — UDP implementation; `lwip/sockets.h` is confined to this file only
- `<arpa/inet.h>` included in the header so callers get `ntohs`/`htonl` without pulling in lwip types
- Placed in `shared_components/` so both firmware targets can use it

## scout_screen/main/stream.c

- Replaced `#include "udp.h"` with `#include "transport.h"`
- `static struct sockaddr_in s_cam_addr` → `static transport_addr_t s_cam_addr`
- `learn_cam_addr` parameter changed from `const struct sockaddr_in *` to `const transport_addr_t *`; body uses `transport_addr_with_port` and `transport_addr_ip`
- `udp_open`, `udp_set_rcvbuf`, `udp_rx`, `udp_tx` replaced with `transport_open`, `transport_set_rcvbuf`, `transport_recv`, `transport_send`

## scout_cam/main/udp_stream.c

- Replaced `#include "udp.h"` with `#include "transport.h"`
- `send_frame` parameter changed from `const struct sockaddr_in *dest` to `const transport_addr_t *dest`
- `udp_addr`, `udp_open`, `udp_tx`, `udp_try_recv` replaced with `transport_make_addr`, `transport_open`, `transport_send`, `transport_try_recv`
- `setsockopt(SO_SNDTIMEO)` replaced with `transport_set_send_timeout`

## CMakeLists.txt (both projects)

- `udp` removed from REQUIRES; `transport` added
