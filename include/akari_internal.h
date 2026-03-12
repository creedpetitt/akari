#ifndef AKARI_INTERNAL_H
#define AKARI_INTERNAL_H

#include "akari_event.h"

void akari_run_epoll(int srv_fd, akari_callback on_data);
void akari_run_poll(int srv_fd, akari_callback on_data);

int akari_handle_client(int fd, akari_callback on_data);
void akari_release_conn(int fd);

extern volatile int akari_running;

#endif
