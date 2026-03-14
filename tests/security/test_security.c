#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>

#define TEST_PORT 9777
#define TEST_HOST "127.0.0.1"

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

static void report(const char* name, int pass) {
    tests_run++;
    if (pass) {
        tests_passed++;
        printf("  PASS  %s\n", name);
    } else {
        tests_failed++;
        printf("  FAIL  %s\n", name);
    }
}

static int tcp_connect(void) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TEST_PORT);
    inet_pton(AF_INET, TEST_HOST, &addr.sin_addr);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }

    struct timeval tv = { .tv_sec = 5, .tv_usec = 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    return fd;
}

static int send_all(int fd, const void* buf, size_t len) {
    const char* p = buf;
    while (len > 0) {
        ssize_t n = send(fd, p, len, 0);
        if (n <= 0) return -1;
        p += n;
        len -= n;
    }
    return 0;
}

static int recv_response(int fd, char* buf, size_t bufsize) {
    size_t total = 0;
    while (total < bufsize - 1) {
        ssize_t n = recv(fd, buf + total, bufsize - 1 - total, 0);
        if (n <= 0) break;
        total += n;
        buf[total] = '\0';
        if (strstr(buf, "\r\n\r\n")) {
            char* cl = strstr(buf, "Content-Length: ");
            if (cl) {
                size_t content_len = atoi(cl + 16);
                char* body_start = strstr(buf, "\r\n\r\n") + 4;
                size_t header_len = body_start - buf;
                size_t body_received = total - header_len;
                if (body_received >= content_len) break;
            } else {
                break;
            }
        }
    }
    return (int)total;
}

static int extract_status(const char* response) {
    if (strncmp(response, "HTTP/1.", 7) != 0) return -1;
    return atoi(response + 9);
}

/* --- Test: valid GET returns 200 --- */
static void test_valid_get(void) {
    int fd = tcp_connect();
    if (fd < 0) { report("valid_get (connect)", 0); return; }

    const char* req = "GET /api HTTP/1.1\r\nHost: localhost\r\n\r\n";
    send_all(fd, req, strlen(req));

    char buf[4096];
    int n = recv_response(fd, buf, sizeof(buf));
    close(fd);

    report("valid_get", n > 0 && extract_status(buf) == 200);
}

/* --- Test: malformed request line returns 400 --- */
static void test_malformed_request(void) {
    int fd = tcp_connect();
    if (fd < 0) { report("malformed_request (connect)", 0); return; }

    const char* req = "GARBAGEGARBAGE\r\n\r\n";
    send_all(fd, req, strlen(req));

    char buf[4096];
    int n = recv_response(fd, buf, sizeof(buf));
    close(fd);

    report("malformed_request -> 400", n > 0 && extract_status(buf) == 400);
}

/* --- Test: incomplete header (no CRLFCRLF) gets no valid response --- */
static void test_partial_headers(void) {
    int fd = tcp_connect();
    if (fd < 0) { report("partial_headers (connect)", 0); return; }

    const char* req = "GET /api HTTP/1.1\r\nHost: localhost\r\n";
    send_all(fd, req, strlen(req));

    usleep(200000);

    char buf[4096];
    struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    ssize_t n = recv(fd, buf, sizeof(buf) - 1, 0);
    close(fd);

    report("partial_headers -> no premature response",
           n <= 0 || (n > 0 && extract_status(buf) == 408));
}

/* --- Test: oversized headers return 431 --- */
static void test_oversized_headers(void) {
    int fd = tcp_connect();
    if (fd < 0) { report("oversized_headers (connect)", 0); return; }

    char req[16384];
    int off = snprintf(req, sizeof(req), "GET /api HTTP/1.1\r\nHost: localhost\r\n");

    while (off < 9000) {
        int w = snprintf(req + off, sizeof(req) - off,
                         "X-Pad-%d: %0200d\r\n", off, 0);
        if (w <= 0) break;
        off += w;
    }
    off += snprintf(req + off, sizeof(req) - off, "\r\n");

    send_all(fd, req, off);

    char buf[4096];
    int n = recv_response(fd, buf, sizeof(buf));
    close(fd);

    report("oversized_headers -> 431", n > 0 && extract_status(buf) == 431);
}

/* --- Test: oversized body returns 413 --- */
static void test_oversized_body(void) {
    int fd = tcp_connect();
    if (fd < 0) { report("oversized_body (connect)", 0); return; }

    char req[512];
    int hlen = snprintf(req, sizeof(req),
        "POST /data HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 2000000\r\n"
        "\r\n");

    send_all(fd, req, hlen);

    char buf[4096];
    int n = recv_response(fd, buf, sizeof(buf));
    close(fd);

    report("oversized_body -> 413", n > 0 && extract_status(buf) == 413);
}

/* --- Test: URI too long returns 414 --- */
static void test_uri_too_long(void) {
    int fd = tcp_connect();
    if (fd < 0) { report("uri_too_long (connect)", 0); return; }

    char req[4096];
    int off = snprintf(req, sizeof(req), "GET /");
    while (off < 2200 && off < (int)sizeof(req) - 50)
        req[off++] = 'a';
    off += snprintf(req + off, sizeof(req) - off,
                    " HTTP/1.1\r\nHost: localhost\r\n\r\n");

    send_all(fd, req, off);

    char buf[4096];
    int n = recv_response(fd, buf, sizeof(buf));
    close(fd);

    report("uri_too_long -> 414", n > 0 && extract_status(buf) == 414);
}

/* --- Test: Transfer-Encoding rejected with 501 --- */
static void test_transfer_encoding_rejected(void) {
    int fd = tcp_connect();
    if (fd < 0) { report("transfer_encoding (connect)", 0); return; }

    const char* req =
        "POST /data HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "5\r\nhello\r\n0\r\n\r\n";
    send_all(fd, req, strlen(req));

    char buf[4096];
    int n = recv_response(fd, buf, sizeof(buf));
    close(fd);

    report("transfer_encoding -> 501", n > 0 && extract_status(buf) == 501);
}

/* --- Test: conflicting Content-Length returns 400 --- */
static void test_conflicting_content_length(void) {
    int fd = tcp_connect();
    if (fd < 0) { report("conflicting_cl (connect)", 0); return; }

    const char* req =
        "POST /data HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 5\r\n"
        "Content-Length: 10\r\n"
        "\r\n"
        "hello";
    send_all(fd, req, strlen(req));

    char buf[4096];
    int n = recv_response(fd, buf, sizeof(buf));
    close(fd);

    report("conflicting_content_length -> 400", n > 0 && extract_status(buf) == 400);
}

/* --- Test: invalid Content-Length (non-numeric) returns 400 --- */
static void test_invalid_content_length(void) {
    int fd = tcp_connect();
    if (fd < 0) { report("invalid_cl (connect)", 0); return; }

    const char* req =
        "POST /data HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: abc\r\n"
        "\r\n";
    send_all(fd, req, strlen(req));

    char buf[4096];
    int n = recv_response(fd, buf, sizeof(buf));
    close(fd);

    report("invalid_content_length -> 400", n > 0 && extract_status(buf) == 400);
}

/* --- Test: keep-alive works (two requests on one connection) --- */
static void test_keepalive(void) {
    int fd = tcp_connect();
    if (fd < 0) { report("keepalive (connect)", 0); return; }

    const char* req = "GET /api HTTP/1.1\r\nHost: localhost\r\n\r\n";
    send_all(fd, req, strlen(req));

    char buf[4096];
    int n = recv_response(fd, buf, sizeof(buf));
    if (n <= 0 || extract_status(buf) != 200) {
        report("keepalive (first request)", 0);
        close(fd);
        return;
    }

    send_all(fd, req, strlen(req));
    n = recv_response(fd, buf, sizeof(buf));
    close(fd);

    report("keepalive (second request)", n > 0 && extract_status(buf) == 200);
}

/* --- Test: Connection: close honored --- */
static void test_connection_close(void) {
    int fd = tcp_connect();
    if (fd < 0) { report("connection_close (connect)", 0); return; }

    const char* req =
        "GET /api HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Connection: close\r\n"
        "\r\n";
    send_all(fd, req, strlen(req));

    char buf[4096];
    int n = recv_response(fd, buf, sizeof(buf));
    if (n <= 0) { report("connection_close", 0); close(fd); return; }

    int status = extract_status(buf);
    int has_close = (strstr(buf, "Connection: close") != NULL);
    close(fd);

    report("connection_close -> close header",
           status == 200 && has_close);
}

/* --- Test: HTTP/1.0 defaults to close --- */
static void test_http10_close(void) {
    int fd = tcp_connect();
    if (fd < 0) { report("http10_close (connect)", 0); return; }

    const char* req = "GET /api HTTP/1.0\r\nHost: localhost\r\n\r\n";
    send_all(fd, req, strlen(req));

    char buf[4096];
    int n = recv_response(fd, buf, sizeof(buf));
    close(fd);

    report("http10_close",
           n > 0 && extract_status(buf) == 200 &&
           strstr(buf, "Connection: close") != NULL);
}

/* --- Test: valid POST with body --- */
static void test_valid_post(void) {
    int fd = tcp_connect();
    if (fd < 0) { report("valid_post (connect)", 0); return; }

    const char* body = "hello world";
    char req[512];
    int hlen = snprintf(req, sizeof(req),
        "POST /data HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        strlen(body), body);

    send_all(fd, req, hlen);

    char buf[4096];
    int n = recv_response(fd, buf, sizeof(buf));
    close(fd);

    report("valid_post -> 200", n > 0 && extract_status(buf) == 200);
}

/* --- Test: flood from single IP triggers 429 --- */
static void test_rate_limit(void) {
    int got_429 = 0;
    int total_sent = 0;
    const char* req =
        "GET /api HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Connection: close\r\n"
        "\r\n";

    for (int i = 0; i < 200; i++) {
        int fd = tcp_connect();
        if (fd < 0) continue;

        send_all(fd, req, strlen(req));
        total_sent++;

        char buf[4096];
        int n = recv_response(fd, buf, sizeof(buf));
        close(fd);

        if (n > 0 && extract_status(buf) == 429) {
            got_429 = 1;
            break;
        }
    }

    report("rate_limit -> 429 after flood", got_429 && total_sent > 1);
}

/* --- Test: slowloris (drip headers byte-by-byte) gets 408 or close --- */
static void test_slowloris(void) {
    int fd = tcp_connect();
    if (fd < 0) { report("slowloris (connect)", 0); return; }

    const char* partial = "GET /api HTTP/1.1\r\nHost: localhost\r\nX-Slow: ";
    send_all(fd, partial, strlen(partial));

    struct timeval tv = { .tv_sec = 8, .tv_usec = 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    for (int i = 0; i < 60; i++) {
        usleep(200000);
        if (send(fd, "a", 1, MSG_NOSIGNAL) <= 0) break;
    }

    char buf[4096];
    ssize_t n = recv(fd, buf, sizeof(buf) - 1, 0);
    close(fd);

    int got_408 = 0;
    int got_close = 0;

    if (n > 0) {
        buf[n] = '\0';
        got_408 = (extract_status(buf) == 408);
    }
    if (n <= 0) got_close = 1;

    report("slowloris -> 408 or connection closed", got_408 || got_close);
}

/* --- Test: unknown route returns 404 --- */
static void test_unknown_route(void) {
    int fd = tcp_connect();
    if (fd < 0) { report("unknown_route (connect)", 0); return; }

    const char* req =
        "GET /nonexistent HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Connection: close\r\n"
        "\r\n";
    send_all(fd, req, strlen(req));

    char buf[4096];
    int n = recv_response(fd, buf, sizeof(buf));
    close(fd);

    report("unknown_route -> 404", n > 0 && extract_status(buf) == 404);
}

int main(void) {
    printf("\n=== Akari Security Test Suite ===\n\n");

    test_valid_get();
    test_valid_post();
    test_keepalive();
    test_connection_close();
    test_http10_close();
    test_unknown_route();
    test_malformed_request();
    test_partial_headers();
    test_oversized_headers();
    test_oversized_body();
    test_uri_too_long();
    test_transfer_encoding_rejected();
    test_conflicting_content_length();
    test_invalid_content_length();
    test_rate_limit();
    test_slowloris();

    printf("\n=== Results: %d/%d passed, %d failed ===\n\n",
           tests_passed, tests_run, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
