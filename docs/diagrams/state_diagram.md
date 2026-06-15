# State Diagram

The dashboard drives the visible system state through four scenes. The vehicle manages its own WiFi lifecycle independently, with a composite motor state inside the active connection.

```mermaid
stateDiagram-v2
    state "Dashboard — ESP32-S3" as DASH {
        [*] --> Booting
        Booting --> Waiting : stream task ready
        Waiting --> Streaming : first frame arrives
        Streaming --> Disconnected : no frame for 2 s
        Disconnected --> Streaming : frame arrives
    }

    state "Vehicle — ESP32-CAM" as VEH {
        [*] --> Connecting
        Connecting --> Active : DHCP assigned
        Active --> Reconnecting : WiFi dropped
        Reconnecting --> Connecting : retry after 1 s

        state Active {
            [*] --> MotorIdle
            MotorIdle --> MotorRunning : cmd received
            MotorRunning --> MotorRunning : cmd received
            MotorRunning --> MotorIdle : 500 ms — no cmd
        }
    }
```
