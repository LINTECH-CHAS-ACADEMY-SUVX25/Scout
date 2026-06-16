# Software Architecture

The codebase is split into five layers. Dependencies only flow downward — tasks call into adapters, adapters call into components and shared modules, components call into ESP-IDF.

```mermaid
graph TB
    subgraph TASKS["Application Tasks"]
        T1["stream_run · Vehicle"]
        T2["motor_run · Vehicle"]
        T3["stream_run core 0 · Dashboard"]
        T4["render_run core 1 · Dashboard"]
        T5["monitor_run · Dashboard"]
    end

    subgraph ADAPTERS["Adapters"]
        A1["frag_tx · Vehicle"]
        A2["motor_queue · Vehicle"]
        A3["cam_state · Vehicle"]
        A4["rc_rx · Vehicle"]
        A5["frag_rx · Dashboard"]
        A6["frame_pool · Dashboard"]
        A7["rc_tx · Dashboard"]
        A8["cfg_tx · Dashboard"]
        A9["console · Dashboard"]
        A10["screen_state · Dashboard"]
        A11["screen_stats · Dashboard"]
        A12["scene · Dashboard"]
    end

    subgraph SHARED["Shared Components"]
        S1[rc_protocol]
        S2[udp]
        S3[watchdog]
        S4[reset_info]
    end

    subgraph COMPONENTS["Components"]
        C1["camera · Vehicle"]
        C2["l298n · Vehicle"]
        C3["bme280 · Vehicle"]
        C4["wifi_sta · Vehicle"]
        C5["display · Dashboard"]
        C6["lvgl_port · Dashboard"]
        C7["wifi_ap · Dashboard"]
        C8["scout_ui · Dashboard"]
        C9["jpeg · Dashboard"]
        C10["ring_buffer · Dashboard"]
    end

    subgraph ESPIDF["ESP-IDF / External"]
        E1[esp32-camera]
        E2["GPIO / LEDC / I2C"]
        E3[esp_new_jpeg]
        E4[LVGL]
        E5[esp_lcd]
        E6["esp_wifi / lwip"]
        E7[esp_driver_uart]
    end

    T1 --> A1 & A2 & A3 & A4 & S2 & S3 & C1 & C3
    T2 --> A2 & S3 & C2
    T3 --> A5 & A6 & A7 & A8 & A10 & A11 & S2 & C7
    T4 --> A6 & A7 & A8 & A10 & A11 & A12 & S3 & C5 & C6 & C8 & C9
    T5 --> A9

    A1 --> S1 & S2
    A2 --> S1
    A3 --> C1 & A2 & C4 & S1
    A4 --> A2 & S2 & S1
    A5 --> A6 & S1
    A6 --> S1
    A7 --> S2 & S1
    A8 --> S2 & S1
    A9 --> A10 & A11 & C7 & S1
    A11 --> A10 & C10
    A12 --> A10 & C8

    C1 --> E1
    C2 --> E2
    C3 --> E2
    C4 --> E6
    C5 --> E5
    C6 --> E4 & E5
    C7 --> E6
    C8 --> E4
    C9 --> E3
    S2 --> E6
```
