#include "akari_http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Mock handler for the fuzzer
void mock_handler(akari_context* ctx) {
    akari_res_send(ctx, 200, "text/plain", "Fuzz Success");
}

int main() {
    // Register some routes for the fuzzer to explore
    akari_http_add_route("GET", "/", mock_handler);
    akari_http_add_route("POST", "/data", mock_handler);
    akari_http_add_route("GET", "/user/:id", mock_handler);

    char buf[8192];
    ssize_t n = read(0, buf, sizeof(buf));
    if (n <= 0) return 0;

    akari_connection conn;
    memset(&conn, 0, sizeof(conn));
    conn.fd = -1;
    size_t copy_len = (size_t)n < sizeof(conn.buf) ? (size_t)n : sizeof(conn.buf);
    memcpy(conn.buf, buf, copy_len);
    conn.buf_len = copy_len;

    // Since we don't have a real event loop, we manually call the parser logic.
    // We need to declare it as it's static/internal usually.
    void akari_handle_http(akari_connection* conn);
    akari_handle_http(&conn);

    return 0;
}
