# Sequence Diagrams

Three core flows in the Scout system.

---

## 1. WiFi Startup

The screen starts a WiFi AP. The car connects as a STA client and blocks inside
`wifi_connect()` until an IP address has been assigned.

```mermaid
sequenceDiagram
    participant S as Scout Screen (ESP32-S3)
    participant C as Scout Cam (ESP32-CAM)

    Note over S: app_main
    S->>S: wifi_ap_start()
    Note right of S: Starts AP "Scout_AP"\nWPA2, max 1 client

    S->>S: stream_init()
    Note right of S: udp_open(VID_PORT 3334)

    S->>S: render_init()
    S->>S: monitor_init()

    Note over C: app_main (concurrently)
    C->>C: motor_init()
    C->>S: wifi_connect() → associates with Scout_AP
    S-->>C: DHCP: IP address assigned
    Note right of C: xEventGroupSetBits(CONNECTED)\nwifi_connect() returns

    C->>C: camera_init()
    C->>C: stream_init()
    Note right of C: udp_open(CMD_PORT 3335)\nfrag_tx_init(S3_IP, VID_PORT)

    Note over S,C: System ready — streaming begins
```

---

## 2. JPEG Frame Flow

The camera's `stream_run` captures JPEG frames and splits them into UDP packets.
The screen's `stream_run` reassembles the fragments and publishes complete frames
to the render task via ping-pong buffers.

```mermaid
sequenceDiagram
    participant CAM as stream_run (Cam)
    participant NET as UDP / WiFi
    participant SR  as stream_run (Screen, core 0)
    participant FB  as frame_buf
    participant RR  as render_run (Screen, core 1)
    participant DSP as display

    CAM->>CAM: camera_capture(&buf, &len)
    Note right of CAM: OV2640 → JPEG

    loop For each fragment (fi = 0..N-1)
        CAM->>NET: frag_tx: [seq][fi][frags] | data
        Note right of CAM: Fragment 0 carries\n5-byte frame header:\n[FRAME_MAGIC][frame_len_be]
    end

    SR->>NET: udp_rx(sock, pkt, PKT_MAX, &src)
    NET-->>SR: UDP packet

    SR->>SR: frag_rx(pkt, n, &frame_len, &ms)
    Note right of SR: Copies data into assembly buffer\nMarks fragment in bitmask

    alt All fragments received (rx_mask complete)
        SR->>SR: cam_cmd_learn(&src)
        Note right of SR: Records camera IP\n(used for RC commands)
        SR->>FB: frame_buf_publish(frame_len)
        Note right of FB: Flips to next\nping-pong buffer
    end

    RR->>FB: frame_buf_try_acquire(&src, &src_len)
    FB-->>RR: Pointer to complete JPEG frame

    RR->>RR: jpeg_decode_rgb565(src, src_len, canvas, ...)
    Note right of RR: Hardware JPEG decode\nto RGB565 canvas in PSRAM

    RR->>FB: frame_buf_release()

    RR->>DSP: display_blit_region(x, y, w, h, canvas)
    Note right of DSP: DMA transfer to\nRGB LCD panel
```

---

## 3. Joystick → Motor

The render task reads the joystick position from LVGL and sends it as a UDP
packet to the camera's CMD port. The camera's `stream_run` receives the packet
and forwards the x/y coordinates to the motor task via a FreeRTOS queue.

```mermaid
sequenceDiagram
    participant UI  as LVGL / scout_ui (Screen)
    participant RR  as render_run (Screen, core 1)
    participant CC  as cam_cmd
    participant NET as UDP / WiFi
    participant SR  as stream_run (Cam)
    participant MQ  as motor_cmd queue
    participant MR  as motor_run (Cam)
    participant HW  as L298N / GPIO

    UI->>RR: scout_ui_get_joy(&jx, &jy)
    Note right of UI: Virtual joystick\nReturns x,y ∈ [-512, 512]

    RR->>CC: cam_cmd_send_throttled(jx, jy)
    Note right of CC: Sends if value changed > 5\nor ≥ 200 ms since last send

    CC->>NET: udp_tx(sock, cam_addr, &joy_pkt, sizeof(joy_pkt))
    Note right of CC: joy_pkt_t: { int16_t x, y }\nUDP to CAM CMD_PORT 3335

    SR->>NET: udp_try_recv(sock, &pkt, sizeof(pkt))
    NET-->>SR: joy_pkt

    SR->>MQ: motor_cmd_send(pkt.x, pkt.y)
    Note right of MQ: xQueueSend to motor queue

    MR->>MQ: motor_cmd_recv(&x, &y, timeout_ms=500)
    MQ-->>MR: x, y

    MR->>MR: joy_to_motor(x, y, &cmd, &speed)
    Note right of MR: Deadzone ±112\nBitmask: CMD_FORWARD/BACKWARD/LEFT/RIGHT\nSpeed = sqrt(x²+y²)

    MR->>HW: l298n_apply(cmd, speed)
    Note right of HW: Sets IN1–IN4 GPIO\nvia PWM on speed pin

    alt No joystick data for 500 ms
        MR->>HW: l298n_apply(CMD_STOP, 0)
        Note right of MR: Safety stop
    end
```
