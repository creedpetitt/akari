#ifndef AKARI_CORE_H
#define AKARI_CORE_H

#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#ifndef AKARI_LOG
    #ifdef AKARI_DEBUG
        #include <stdio.h>
        #define AKARI_LOG(msg) puts(msg)
    #else
        #define AKARI_LOG(msg) ((void)0)
    #endif
#endif

int akari_tcp_init(void);
int akari_tcp_bind(int fd, const struct sockaddr_in* addr);
int akari_tcp_listen(int fd);
int akari_tcp_accept(int fd, const struct sockaddr_in* addr);
int akari_tcp_start(uint16_t port);
struct sockaddr_in akari_addr_init(const char* host, uint16_t port);
ssize_t akari_tcp_send(int fd, const void *buf, size_t size);
ssize_t akari_tcp_recv(int fd, void *buf, size_t size);

#endif
