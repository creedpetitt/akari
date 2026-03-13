#include "akari_internal.h"
#include <poll.h>
#include <unistd.h>

#define AKARI_MAX_POLL_FDS 128

void akari_run_poll(int srv_fd, akari_callback on_data) {
    struct pollfd fds[AKARI_MAX_POLL_FDS];
    int nfds = 1;

    fds[0].fd = srv_fd;
    fds[0].events = POLLIN;

    for (int i = 1; i < AKARI_MAX_POLL_FDS; i++) {
        fds[i].fd = -1;
    }

    AKARI_LOG("poll engine started");

    while (akari_running) {
        int ready = poll(fds, nfds, 100);
        if (ready == -1) {
            if (akari_running) {
                AKARI_LOG("poll failed");
            }
            break;
        }

        for (int i = 0; i < nfds; i++) {
            if (fds[i].revents == 0) continue;

            if (fds[i].fd == srv_fd) {
                int client_fd = akari_tcp_accept(srv_fd, NULL);
                if (client_fd != -1) {
                    int added = 0;
                    for (int j = 1; j < AKARI_MAX_POLL_FDS; j++) {
                        if (fds[j].fd == -1) {
                            fds[j].fd = client_fd;
                            fds[j].events = POLLIN;
                            if (j >= nfds) nfds = j + 1;
                            added = 1;
                            break;
                        }
                    }
                    if (!added) {
                        AKARI_LOG("poll fds full");
                        close(client_fd);
                    }
                }
            } else {
                int client_fd = fds[i].fd;
                int status = akari_handle_client(client_fd, on_data);
                if (status == -1 || (fds[i].revents & (POLLHUP | POLLERR))) {
                    close(client_fd);
                    fds[i].fd = -1;
                }
            }
        }
        akari_check_timers();
    }
}
