# Akari: Web Framework for C

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C: C11](https://img.shields.io/badge/Language-C11-blue.svg)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))
[![Platform: Linux/Embedded](https://img.shields.io/badge/Platform-Linux%2FEmbedded-lightgrey.svg)](https://en.wikipedia.org/wiki/Embedded_system)

**High-Performance • Non-Blocking • Deterministic Memory • Embedded Optimized**

---

## Overview

Akari is an industry-grade, non-blocking HTTP micro-framework written in **Pure C11**. Designed specifically for resource-constrained environments (ESP32, STM32) and high-performance Linux edge computing, Akari provides a modern, ergonomic developer experience without a single call to `malloc()` during the request lifecycle.

By strictly adhering to a **Zero-Allocation** and **Zero-Copy** philosophy, Akari guarantees zero heap fragmentation and provides a predictable, fixed memory ceiling—making it the ideal choice for IoT devices.

---

## Core Mandates

*   **Zero Dynamic Allocation:** 0% heap fragmentation. All memory is pre-allocated or stack-resident.
*   **Zero-Copy Architecture:** Pointers and lengths point directly into DMA/Kernel buffers.
*   **Non-Blocking I/O:** High-concurrency event loops powered by `epoll` (Linux) or `poll` (POSIX/Embedded).
*   **Ultra-Lightweight:** ~20KB compiled binary footprint with a fixed ~30KB RAM ceiling.
*   **Single-Header Option:** Drop `akari.h` into any project and start building instantly.

---

## Installation

Install the **Akari CLI** to scaffold, bundle, and manage projects with zero configuration.

```bash
curl -sSL https://raw.githubusercontent.com/creedpetitt/akari/main/scripts/install.sh | bash
```

*Note: This script bundles the single-header library, compiles the CLI, and installs it to `/usr/local/bin`.*

---

## Quick Start

Initialize your first Akari project:

```bash
~ akari init my-server
  🏮 Initializing project 'my-server'...
  Project ready!

~ cd my-server && make && ./app
```

### Project Structure
When you run `akari init`, the following files are generated:
*   **`main.c`**: Your application logic and route registrations.
*   **`akari.h`**: The self-contained, single-header library.
*   **`index.html`**: A starter dashboard for your IoT device.
*   **`Makefile`**: A simple build file to compile your project.

### `main.c` Implementation

```c
#define AKARI_IMPLEMENTATION
#include "akari.h"
#include <stdio.h>

void handle_home(akari_context* ctx) {
    akari_res_file(ctx, "index.html"); // Zero-copy streaming from disk
}

void handle_user(akari_context* ctx) {
    int id = akari_param_to_int(ctx, "id");
    const char* action = akari_query_str(ctx, "action", "view");

    akari_printf(ctx, "User %d is performing: %s\n", id, action);
    akari_send(ctx, 200, "text/plain");
}

int main() {
    AKARI_GET("/", handle_home);
    AKARI_GET("/user/:id", handle_user);
    
    printf("🏮 Akari starting on port 8080...\n");
    akari_http_start(8080);
    return 0;
}
```

---

## Developer Documentation

Explore the sub-guides for a deeper dive into the Akari ecosystem:

*   **[Architecture & Philosophy](docs/architecture.md)**: Deep dive into the Zero-Allocation engine and Event Loop.
*   **[Embedded Deployment](docs/embedded.md)**: Tuning memory constants for ESP32 and ARM Cortex-M.

---

## API Reference

### Routing & Request Handling

Akari features a static routing engine that supports dynamic path parameters with zero allocations.

*   **Registration:** `AKARI_GET("/api/v1/:resource", handler)`
*   **Path Params:** `akari_param_to_int(ctx, "id")` or `akari_get_path_param(ctx, "key", &len)`.
*   **Query Strings:** Lazily parsed using `akari_query_str(ctx, "key", "default")`.

### JSON Integration

Akari bundles an abstraction over the **jsmn** parser for high-performance JSON extraction.

```c
void handle_post(akari_context* ctx) {
    int age = akari_json_get_int(ctx, "age");
    const char* name = akari_json_get_string(ctx, "name", &len);
    
    akari_res_json(ctx, 200, "{\"status\": \"success\"}");
}
```

### Response Buffering

Use the ergonomic `akari_printf` to build responses without managing your own buffers.

```c
akari_printf(ctx, "Status: %s\n", "Operational");
akari_send(ctx, 200, "text/plain");
```

---

## Deterministic Memory

Configure Akari for your hardware constraints at compile-time using the following macros:

| Constant | Default | Description |
| :--- | :--- | :--- |
| `AKARI_MAX_CONNECTIONS` | 8 | Concurrent connections in the pool. |
| `AKARI_MAX_ROUTES` | 16 | Total registered API endpoints. |
| `AKARI_RES_BUF_SIZE` | 512 | Stack buffer for response formatting. |

---

## Hardware Compatibility

Akari is architected to be hardware-agnostic by separating the **Event Engine** from the **HTTP Logic**.

*   **Linux/Unix:** Native `epoll` support for extreme requests-per-second.
*   **ESP32:** Compatible with ESP-IDF via the `poll` engine and lwIP.
*   **ARM Cortex (Bare-Metal/RTOS):** Optimized for 64KB RAM environments (STM32, ESP32).

---

## Technical Philosophy: Why Zero-Copy?

Traditional C web servers copy strings into new buffers to null-terminate them for `printf`. Akari refuses to do this. By using pointer-length pairs, Akari points directly to the data already sitting in your network card's buffer. 

When you call `akari_printf(ctx, "%.*s", len, ptr)`, you are viewing the raw network data with **zero latency** and **zero extra memory usage**.

