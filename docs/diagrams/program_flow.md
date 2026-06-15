# Program Flow

Both nodes boot independently. The vehicle waits for a DHCP address from the dashboard before starting its tasks, so the dashboard should be powered first.

```mermaid
flowchart LR
    subgraph DASH["Dashboard — ESP32-S3"]
        direction TB

        DS([Boot])
        DS --> D1["wifi_ap_start\nScout_AP"]
        D1 --> D2[display_init]
        D2 --> D3[lvgl_port_init]
        D3 --> D4["monitor_init\n→ monitor_run"]
        D4 --> D5["stream_init\n→ stream_run  core 0"]
        D5 --> D6["render_init\n→ render_run  core 1"]
        D6 --> D7([vTaskDelete])

        subgraph STREAM_LOOP["stream_run — core 0"]
            direction TB
            SL1["udp_rx  :3334"] --> SL2[frag_rx]
            SL2 --> SL3{frame\ncomplete?}
            SL3 -- no --> SL1
            SL3 -- yes --> SL4[frame_buf_publish]
            SL4 --> SL1
        end

        subgraph RENDER_LOOP["render_run — core 1"]
            direction TB
            RL1[lvgl_port_render_frame] --> RL2[cam_cmd_send_throttled]
            RL2 --> RL3[frame_buf_try_acquire]
            RL3 --> RL4{new frame?}
            RL4 -- no --> RL1
            RL4 -- yes --> RL5["jpeg_decode_rgb565\ndisplay_blit_region"]
            RL5 --> RL1
        end

        subgraph MONITOR_LOOP["monitor_run"]
            direction TB
            ML1[uart_console_read_line] --> ML2[monitor_dispatch]
            ML2 --> ML1
        end
    end

    subgraph VEH["Vehicle — ESP32-CAM"]
        direction TB

        VS([Boot])
        VS --> V1["motor_init\n→ motor_run"]
        V1 --> V2["wifi_connect\nblocks until DHCP"]
        V2 --> V3[camera_init]
        V3 --> V4["stream_init\n→ stream_run  core 0"]
        V4 --> V5([vTaskDelete])

        subgraph CAM_STREAM_LOOP["stream_run — core 0"]
            direction TB
            CL1[camera_capture] --> CL2["frag_tx  UDP :3334"]
            CL2 --> CL3["udp_try_recv  :3335"]
            CL3 --> CL4{cmd\nreceived?}
            CL4 -- no --> CL1
            CL4 -- yes --> CL5[motor_cmd_send]
            CL5 --> CL1
        end

        subgraph MOTOR_LOOP["motor_run"]
            direction TB
            MR1["motor_cmd_recv\n500 ms timeout"] --> MR2{timeout?}
            MR2 -- no --> MR3[l298n_apply CMD]
            MR2 -- yes --> MR4[l298n_apply CMD_STOP]
            MR3 --> MR1
            MR4 --> MR1
        end
    end

    V2 -. "WiFi — joins Scout_AP" .-> D1
    CL2 -. "UDP 3334  JPEG video" .-> SL1
    RL2 -. "UDP 3335  joystick cmd" .-> CL3
```
