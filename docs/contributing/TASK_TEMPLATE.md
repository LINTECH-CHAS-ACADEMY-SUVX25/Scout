# Task file template

Every task file in Scout follows this pattern. A task owns a FreeRTOS loop and nothing else. All state and logic live in the adapters the task calls.

## Rules

- One public function: `<name>_init`
- `<name>_init` allocates resources, configures adapters, and spawns the task
- `<name>_run` is `static` — it is an implementation detail, not part of the public API
- No SDK headers (`driver/`, `lwip/`, `esp_wifi.h`) — only adapter headers

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

static void task_name_run(void *arg);

void task_name_init(void)
{
    adapter_a_init();
    adapter_b_init();
    xTaskCreate(task_name_run, "task_name", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "task_name ready");
}

static void task_name_run(void *arg)
{
    while(1) {
        adapter_a_do_thing();
        adapter_b_do_other_thing();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

## task_name.h

```c
#pragma once

// One-line description of what this task does. Spawns the task.
void task_name_init(void);
```

## main.c pattern

```c
stream_init();
render_init();
monitor_init();

vTaskDelete(NULL);
```

## What belongs in an adapter, not a task

| Belongs in adapter | Reason |
|---|---|
| Static state (`s_buf`, `s_mutex`, `s_connected`) | State outlives any single loop iteration |
| Helper functions (`reassemble`, `publish`, `dispatch`) | Logic that can be named and reused |
| Struct definitions used by the task | Type definitions belong with their owner |
| SDK calls | Tasks never include SDK headers directly |
