# Akari: Web Framework for C

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C: C11](https://img.shields.io/badge/Language-C11-blue.svg)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))
[![Platform: Linux/Embedded](https://img.shields.io/badge/Platform-Linux%2FEmbedded-lightgrey.svg)](https://en.wikipedia.org/wiki/Embedded_system)

**High-Performance • Non-Blocking • Deterministic Memory • Embedded Optimized**

---

## Overview

Akari is an industry-grade, non-blocking HTTP micro-framework written in **Pure C11**. Designed specifically for resource-constrained environments (ESP32, STM32) and high-performance Linux edge computing, Akari provides a modern, ergonomic developer experience without a single call to `malloc()` during the request lifecycle.

By strictly adhering to a **Zero-Allocation** and **Zero-Copy** philosophy, utilizing an advanced **non-blocking egress queue**, and supporting real-time **asynchronous file streaming**, Akari guarantees zero heap fragmentation and extremely deterministic performance without stalling the event loop.

---

## Core Mandates

*   **Zero Dynamic Allocation:** 0% heap fragmentation. All memory is pre-allocated or stack-resident.
*   **Zero-Copy Architecture:** Pointers and lengths point directly into DMA/Kernel buffers.
*   **Non-Blocking I/O:** High-concurrency event loops powered by `epoll` (Linux) or `poll` (POSIX/Embedded).
*   **Ultra-Lightweight:** ~20KB compiled binary footprint with a fixed ~30KB RAM ceiling.
*   **Single-Header Option:** Drop `akari.h` into any project and start building instantly.

---

## Performance

Akari is engineered for extreme throughput on minimal hardware. Benchmarked on a standard Linux environment using `wrk` (single core):

```text
akari git:(main) ✗ wrk -t4 -c100 -d10s http://localhost:8080/api
Running 10s test @ http://localhost:8080/api
  4 threads and 100 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   428.76us  322.60us  25.57ms   99.66%
    Req/Sec    58.82k     4.78k   70.67k    65.01%
  2358010 requests in 10.10s, 247.37MB read
Requests/sec: 233469.02
Transfer/sec:     24.49MB
```

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

*   **[API Reference](docs/api.md)**: Explore the user-facing routing, JSON, and response-building functions.
*   **[Architecture & Philosophy](docs/architecture.md)**: Deep dive into the Zero-Allocation engine, Egress Queue, and Event Loop.
*   **[Embedded Deployment](docs/embedded.md)**: Tuning memory constants for ESP32 and ARM Cortex-M.

---



## Deterministic Memory

Configure Akari for your exact hardware constraints at compile-time using the following macros.

### Memory Boundaries
| Macro | Default | Description |
| :--- | :--- | :--- |
| `AKARI_MAX_CONNECTIONS` | 8 | Concurrent active TCP connections in the pool. |
| `AKARI_MAX_ROUTES` | 16 | Total registered API endpoints. |
| `AKARI_REQ_BUF_SIZE` | 4096 | Bytes allocated per connection for incoming HTTP read buffers. |
| `AKARI_RES_BUF_SIZE` | 512 | Bytes allocated per connection for outgoing persistence buffering. |

### HTTP Parsing Limits
| Macro | Default | Description |
| :--- | :--- | :--- |
| `AKARI_MAX_PATH_PARAMS` | 4 | Maximum dynamic path parameters (`/:id`). |
| `AKARI_MAX_HEADERS` | 32 | Maximum HTTP headers matched per request. |
| `AKARI_MAX_HEADER_BYTES`| 8192 | Max byte length for incoming headers. |
| `AKARI_MAX_BODY_BYTES` | 1048576 | Max byte length for an HTTP payload (1MB). |
| `AKARI_MAX_PATH_LEN` | 2048 | Max byte length for the URI path. |
| `AKARI_MAX_METHOD_LEN` | 8 | Max byte length for the HTTP Method constraint. |

### Engine & Connection Timeouts
| Macro | Default | Description |
| :--- | :--- | :--- |
| `AKARI_HEADER_TIMEOUT_MS` | 5000 | Max ms allowed to parse complete HTTP headers. |
| `AKARI_BODY_TIMEOUT_MS`  | 10000 | Max ms allowed to finish reading an HTTP body. |
| `AKARI_KEEPALIVE_TIMEOUT_MS` | 10000 | Max ms to keep an idle `keep-alive` connection alive. |

### Built-in Token Bucket Rate Limiting
| Macro | Default | Description |
| :--- | :--- | :--- |
| `AKARI_RATE_BUCKETS` | 64 | Hash buckets available to track incoming client IPs. |
| `AKARI_RATE_REFILL_PER_SEC` | 50 | Tokens (requests) replenished per second per client IP. |
| `AKARI_RATE_BURST` | 100 | Maximum burst capacity per client IP. |

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

