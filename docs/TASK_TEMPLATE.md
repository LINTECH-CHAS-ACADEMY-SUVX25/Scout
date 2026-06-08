# Task file template

Every task file in Scout follows this pattern. A task owns a FreeRTOS loop and nothing else. All state and logic live in the adapters the task calls.

## Rules

- Exactly two public functions: `<name>_init` and `<name>_run`
- No static variables — state belongs in the adapter that owns it
- No private functions — logic belongs in the adapter that owns it
- No SDK headers (`driver/`, `lwip/`, `esp_wifi.h`) — only adapter headers
- `<name>_run` is the raw FreeRTOS task function; `main.c` passes it to `xTaskCreate`
- `<name>_init` allocates resources and configures adapters; it does not start the task

## task_name.c

```c
#include "task_name.h"
#include "adapter_a.h"
#include "adapter_b.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Task — one sentence on what this task does and why it exists.

static const char *TAG = "task_name";

void task_name_run(void *arg)
{
    while(1) {
        adapter_a_do_thing();
        adapter_b_do_other_thing();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void task_name_init(void)
{
    adapter_a_init();
    adapter_b_init();
}
```

## task_name.h

```c
#pragma once

// One-line description of what this task does.
void task_name_init(void);
// Passed directly to xTaskCreate — do not call from application code.
void task_name_run(void *arg);
```

## main.c pattern

```c
stream_init();
render_init();
monitor_init();

xTaskCreatePinnedToCore(stream_run,  "stream",  4096, NULL, 5, NULL, 0);
xTaskCreatePinnedToCore(render_run,  "render",  8192, NULL, 4, NULL, 1);
xTaskCreate           (monitor_run, "monitor", 3072, NULL, 2, NULL);

vTaskDelete(NULL);
```

## What belongs in an adapter, not a task

| Belongs in adapter | Reason |
|---|---|
| Static state (`s_buf`, `s_mutex`, `s_connected`) | State outlives any single loop iteration |
| Helper functions (`reassemble`, `publish`, `dispatch`) | Logic that can be named and reused |
| Struct definitions used by the task | Type definitions belong with their owner |
| SDK calls | Tasks never include SDK headers directly |
