#include "akari_event.h"
#include "akari_internal.h"
#include <string.h>
#include <unistd.h>
#include <time.h>

#if defined(__linux__) || defined(__APPLE__)
    #include <sys/time.h>
#elif defined(ESP_PLATFORM)
    #include "esp_timer.h"
#endif

static uint64_t get_time_ms(void) {
#if defined(__linux__) || defined(__APPLE__)
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return ((uint64_t)ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
#elif defined(ESP_PLATFORM)
    return (uint64_t)(esp_timer_get_time() / 1000ULL);
#else
    return 0;
#endif
}

#define AKARI_MAX_TIMERS 4

volatile int akari_running = 1;

typedef struct {
    akari_timer_callback cb;
    uint64_t interval_ms;
    uint64_t last_run_ms;
} akari_timer;

static akari_timer timer_pool[AKARI_MAX_TIMERS];
static int timer_count = 0;

static akari_connection conn_pool[AKARI_MAX_CONNECTIONS];
static int conn_pool_initialized = 0;

void akari_add_timer(akari_timer_callback cb, int interval_ms) {
    if (timer_count < AKARI_MAX_TIMERS) {
        timer_pool[timer_count].cb = cb;
        timer_pool[timer_count].interval_ms = interval_ms;
        timer_pool[timer_count].last_run_ms = 0;
        timer_count++;
    }
}

void akari_check_timers(void) {
    if (timer_count == 0) return;
    uint64_t now = get_time_ms();
    for (int i = 0; i < timer_count; i++) {
        if (timer_pool[i].last_run_ms == 0 ||
            (now - timer_pool[i].last_run_ms) >= timer_pool[i].interval_ms) {
            timer_pool[i].cb();
            timer_pool[i].last_run_ms = now;
        }
    }
}

void akari_stop(void) {
    akari_running = 0;
}

static void init_conn_pool(void) {
    if (conn_pool_initialized) return;
    memset(conn_pool, 0, sizeof(conn_pool));
    for (int i = 0; i < AKARI_MAX_CONNECTIONS; i++) {
        conn_pool[i].fd = -1;
        conn_pool[i].state = AKARI_CONN_IDLE;
    }
    conn_pool_initialized = 1;
}

akari_connection* akari_get_conn(int fd) {
    init_conn_pool();
    int first_empty = -1;

    for (int i = 0; i < AKARI_MAX_CONNECTIONS; i++) {
        if (conn_pool[i].fd == fd)
            return &conn_pool[i];
        if (first_empty == -1 && conn_pool[i].fd == -1)
            first_empty = i;
    }

    if (first_empty != -1) {
        akari_connection* c = &conn_pool[first_empty];
        c->fd = fd;
        c->buf_len = 0;
        c->state = AKARI_CONN_IDLE;
        c->last_activity_ms = get_time_ms();
        c->parsed_header_len = 0;
        c->expected_body_len = 0;
        memset(&c->client_ip, 0, sizeof(c->client_ip));
        return c;
    }

    return NULL;
}

void akari_release_conn(int fd) {
    for (int i = 0; i < AKARI_MAX_CONNECTIONS; i++) {
        if (conn_pool[i].fd == fd) {
            conn_pool[i].fd = -1;
            conn_pool[i].buf_len = 0;
            conn_pool[i].state = AKARI_CONN_IDLE;
            return;
        }
    }
}

void akari_sweep_timeouts(void) {
    uint64_t now = get_time_ms();
    if (now == 0) return;

    for (int i = 0; i < AKARI_MAX_CONNECTIONS; i++) {
        akari_connection* c = &conn_pool[i];
        if (c->fd < 0) continue;
        if (c->last_activity_ms == 0) continue;

        uint64_t elapsed = now - c->last_activity_ms;
        int timed_out = 0;

        switch (c->state) {
        case AKARI_CONN_READING_HEADERS:
            if (elapsed >= AKARI_HEADER_TIMEOUT_MS) timed_out = 1;
            break;
        case AKARI_CONN_READING_BODY:
            if (elapsed >= AKARI_BODY_TIMEOUT_MS) timed_out = 1;
            break;
        case AKARI_CONN_IDLE:
            if (elapsed >= AKARI_KEEPALIVE_TIMEOUT_MS) timed_out = 1;
            break;
        default:
            break;
        }

        if (timed_out) {
            AKARI_LOG("timeout on fd %d (state=%d, elapsed=%llu ms)",
                      c->fd, c->state, (unsigned long long)elapsed);
            akari_send_error(c->fd, 408, 0);
            int fd = c->fd;
            akari_release_conn(fd);
            close(fd);
        }
    }
}

int akari_handle_client(int fd, akari_callback on_data) {
    akari_connection* conn = akari_get_conn(fd);
    if (!conn) return -1;

    size_t space_left = sizeof(conn->buf) - conn->buf_len;
    if (space_left == 0) {
        if (conn->state == AKARI_CONN_READING_HEADERS)
            akari_send_error(fd, 431, 0);
        else if (conn->state == AKARI_CONN_READING_BODY)
            akari_send_error(fd, 413, 0);
        akari_release_conn(fd);
        return -1;
    }

    ssize_t n = akari_tcp_recv(fd, &conn->buf[conn->buf_len], space_left);

    if (n > 0) {
        conn->buf_len += n;
        conn->last_activity_ms = get_time_ms();
        if (conn->state == AKARI_CONN_IDLE)
            conn->state = AKARI_CONN_READING_HEADERS;
        on_data(conn);
        return 0;
    } else if (n == -2 || n == -1) {
        akari_release_conn(fd);
        return -1;
    }
    return 0;
}

void akari_run_server(uint16_t port, akari_callback on_data) {
    int srv_fd = akari_tcp_start(port);
    if (srv_fd == -1) return;

#ifdef AKARI_USE_POLL
    akari_run_poll(srv_fd, on_data);
#else
    akari_run_epoll(srv_fd, on_data);
#endif
}
