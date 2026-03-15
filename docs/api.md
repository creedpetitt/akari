# Akari API Reference

This document outlines the user-facing functions available in the Akari HTTP Framework.

## Lifecycle & Configuration

*   **`void akari_http_start(uint16_t port)`**
    Starts the main event loop and binds the high-performance non-blocking server to the specified port.
*   **`void akari_stop(void)`**
    Gracefully stops the event loop, safely closing all active connections and unbinding the socket. Ideal for `SIGINT` or `SIGTERM` signal handlers.
*   **`#define AKARI_DEBUG`**
    Define this macro before `#include "akari.h"` to enable verbose server diagnostic logging to `stdout`.

## Timers

*   **`void akari_add_timer(akari_timer_callback cb, int interval_ms)`**
    Registers a non-blocking recurring interval timer to the event loop. The callback `cb` will be executed safely on the same thread as the event loop approximately every `interval_ms`.

## Routing & Request Handling

*   **`void akari_http_add_route(const char* method, const char* path, akari_route_handler handler)`**
    Registers a new route within the static routing table. Commonly used via macros like `AKARI_GET("/api/endpoint", handler)`.
*   **`const char* akari_get_path_param(akari_context* ctx, const char* key, size_t* out_len)`**
    Extracts a dynamic path parameter (e.g., `:id`). Returns a direct pointer to the parsed network buffer (zero-copy), setting `out_len` to the length.
*   **`int akari_param_to_int(akari_context* ctx, const char* key)`**
    Helper function to parse a path or query parameter directly into an integer.
*   **`const char* akari_get_query_param(akari_context* ctx, const char* key, size_t* out_len)`**
    Extracts a query string parameter.
*   **`const char* akari_query_str(akari_context* ctx, const char* key, const char* def)`**
    Helper function to safely get a query string parameter, returning a default fallback string if it is missing.
*   **`AKARI_GROUP(prefix)`**
    A C-macro intended for semantic visual grouping of API routes. Usage: `AKARI_GET(AKARI_GROUP("/api/v1") "/users", my_handler);`

## JSON Integration

Akari bundles an abstraction over the **jsmn** parser for high-performance JSON extraction with zero dynamic allocations.

*   **`const char* akari_json_get_string(akari_context* ctx, const char* key, size_t* out_len)`**
    Returns a pointer to the string value of a JSON key in the request body.
*   **`int akari_json_get_int(akari_context* ctx, const char* key)`**
    Parses a JSON value from the request body as an integer.
*   **`int akari_json_get_bool(akari_context* ctx, const char* key)`**
    Parses a JSON value from the request body as a boolean (`true`/`false`).
*   **`void akari_res_json(akari_context* ctx, int status_code, const char* body)`**
    A semantic wrapper around `akari_res_send` that automatically sets the `Content-Type` to `application/json`.

## Response Buffering & Egress Transmission

Akari utilizes a high-performance **non-blocking egress queue**. Response data is securely buffered in the connection object and transmitted asynchronously over the event loop (`epoll`/`poll`).

*   **`void akari_printf(akari_context* ctx, const char* fmt, ...)`**
    Writes formatted text efficiently into the persistent connection response buffer.
*   **`void akari_send(akari_context* ctx, int status_code, const char* content_type)`**
    Finalizes and queues the response built by `akari_printf`. It generates HTTP headers and switches the socket to the `AKARI_CONN_SENDING` state.
*   **`void akari_res_data(akari_context* ctx, int status_code, const char* content_type, const void* data, size_t len)`**
    Sends raw binary or text data immediately.
*   **`void akari_res_send(akari_context* ctx, int status_code, const char* content_type, const char* body)`**
    Sends a null-terminated string response.
*   **`void akari_res_file(akari_context* ctx, const char* filepath)`**
    Initiates graceful asynchronous file streaming. By using `open()` instead of blocking `fread()` loops, Akari attaches the file descriptor to the socket. The event engine transparently chunks the disk-read and network-transmission cycle, completely preventing `EAGAIN` blockages.

## Utilities

*   **`size_t akari_url_decode(char* dest, const char* src, size_t src_len)`**
    A fast, in-place utility to decode URL-encoded components (e.g. `%20` to ` `), returning the length of the decoded string.
