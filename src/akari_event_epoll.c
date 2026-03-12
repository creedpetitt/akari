#include "../include/akari_internal.h"
#include <sys/epoll.h>
#include <unistd.h>

#define AKARI_MAX_EVENTS 64

void akari_run_epoll(int srv_fd, akari_callback on_data) {
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        AKARI_LOG("epoll_create1 failed");
        return;
    }

    struct epoll_event ev, events[AKARI_MAX_EVENTS];

    ev.events = EPOLLIN;
    ev.data.fd = srv_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, srv_fd, &ev) == -1) {
        AKARI_LOG("epoll_ctl server fd failed");
        close(epoll_fd);
        return;
    }

    AKARI_LOG("epoll engine started");

    while (1) {
        int nfds = epoll_wait(epoll_fd, events, AKARI_MAX_EVENTS, -1);
        if (nfds == -1) {
            AKARI_LOG("epoll_wait failed");
            break;
        }
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == srv_fd) {
                int client_fd = akari_tcp_accept(srv_fd, NULL);
                if (client_fd != -1) {
                    ev.events = EPOLLIN;
                    ev.data.fd = client_fd;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
                        AKARI_LOG("epoll_ctl client fd failed");
                        close(client_fd);
                    }
                }
            } else {
                int client_fd = events[i].data.fd;
                int status = akari_handle_client(client_fd, on_data);
                if (status == -1) {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                    close(client_fd);
                }
            }
        }
    }

    close(epoll_fd);
}
