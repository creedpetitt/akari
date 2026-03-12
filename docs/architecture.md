# Akari Architecture & Philosophy

Akari is built on the principle that high-performance networking should be **deterministic**. This guide explains the core engine that makes Akari unique.

## 1. Zero-Allocation Request Lifecycle

In a standard web server, memory is allocated dynamically as requests arrive:
1.  A new socket is opened (`malloc` for the connection).
2.  The request is parsed into a tree of objects (`malloc` for headers/body).
3.  The response is built as a string (`malloc` for formatting).

**Akari eliminates all three steps.**
*   **Pre-allocated Pool:** Akari creates a static array of `akari_connection` objects at startup. When a client connects, Akari simply grabs an available slot.
*   **In-Place Parsing:** Akari uses `picohttpparser` to find the "start" and "end" of every header directly within the raw receive buffer. It never copies these strings.
*   **Static Response Buffering:** Akari uses a fixed-size stack buffer for `akari_printf`.

## 2. Zero-Copy Networking

Akari treats strings as **Pointer-Length Pairs** rather than Null-Terminated strings.

```c
typedef struct {
    const char* val;
    size_t val_len;
} akari_path_param;
```

This design allows Akari to point directly into the data provided by the network card. When you access a path parameter or a query string, you are looking at the actual bytes received from the wire—no copying, no duplication, no overhead.

## 3. The Event Loop Engine

Akari uses a layered event system to maintain cross-platform compatibility without sacrificing performance.

### epoll (Linux)
On Linux systems, Akari uses `epoll` in its default level-triggered mode. This allows the server to handle multiple concurrent connections efficiently. It is a robust and widely-supported mechanism for high-performance I/O on Linux.

### poll (Embedded/POSIX)
On microcontrollers (like the ESP32) or older Unix systems, Akari falls back to the `poll()` system call. While less scalable than `epoll`, it is highly portable and perfectly suited for the 1-10 concurrent connections typical of an IoT device.

## 4. Deterministic Performance

Because Akari never calls `malloc()`, its performance is **linearly predictable**. 
*   **Latency:** The time to route a request is constant, regardless of how long the server has been running.
*   **Memory:** RAM usage is a "flat line." It will never grow over time, making "Memory Leaks" architecturally impossible.
