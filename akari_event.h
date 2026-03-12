#ifndef AKARI_EVENT_H
#define AKARI_EVENT_H

#include "akari_core.h"

typedef struct {
    int fd;
    char buf[4096];
    size_t buf_len;
} akari_connection;

typedef void (*akari_callback)(akari_connection* conn);

void akari_run_server(uint16_t port, akari_callback on_data);

#endif
