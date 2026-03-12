#include "akari_event.h"
#include "akari_internal.h"
#include <string.h>
#include <unistd.h>

#define AKARI_MAX_CONNECTIONS 128

static akari_connection conn_pool[AKARI_MAX_CONNECTIONS];
static int conn_pool_initialized = 0;

static void init_conn_pool(void) {
    if (conn_pool_initialized) return;
    for (int i = 0; i < AKARI_MAX_CONNECTIONS; i++) {
        conn_pool[i].fd = -1;
    }
    conn_pool_initialized = 1;
}

akari_connection* akari_get_conn(int fd) {
    init_conn_pool();
    int first_empty = -1;

    for (int i = 0; i < AKARI_MAX_CONNECTIONS; i++) {
        if (conn_pool[i].fd == fd) {
            return &conn_pool[i];
        }
        if (first_empty == -1 && conn_pool[i].fd == -1) {
            first_empty = i;
        }
    }

    if (first_empty != -1) {
        conn_pool[first_empty].fd = fd;
        conn_pool[first_empty].buf_len = 0;
        memset(conn_pool[first_empty].buf, 0, sizeof(conn_pool[first_empty].buf));
        return &conn_pool[first_empty];
    }

    return NULL;
}

void akari_release_conn(int fd) {
    for (int i = 0; i < AKARI_MAX_CONNECTIONS; i++) {
        if (conn_pool[i].fd == fd) {
            conn_pool[i].fd = -1;
            conn_pool[i].buf_len = 0;
            return;
        }
    }
}

int akari_handle_client(int fd, akari_callback on_data) {
    akari_connection* conn = akari_get_conn(fd);

    if (!conn) {
        return -1;
    }

    size_t space_left = sizeof(conn->buf) - conn->buf_len;

    if (space_left == 0) {
        akari_release_conn(fd);
        return -1;
    }

    ssize_t n = akari_tcp_recv(fd, &conn->buf[conn->buf_len], space_left);

    if (n > 0) {
        conn->buf_len += n;
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

#ifdef __linux__
    akari_run_epoll(srv_fd, on_data);
#else
    akari_run_poll(srv_fd, on_data);
#endif
}
