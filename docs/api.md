# Akari API Reference

This document outlines the user-facing functions available in the Akari HTTP Framework.

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

## JSON Integration

Akari bundles an abstraction over the **jsmn** parser for high-performance JSON extraction with zero dynamic allocations.

*   **`const char* akari_json_get_string(akari_context* ctx, const char* key, size_t* out_len)`**
    Returns a pointer to the string value of a JSON key in the request body.
*   **`int akari_json_get_int(akari_context* ctx, const char* key)`**
    Parses a JSON value from the request body as an integer.
*   **`int akari_json_get_bool(akari_context* ctx, const char* key)`**
    Parses a JSON value from the request body as a boolean (`true`/`false`).

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
