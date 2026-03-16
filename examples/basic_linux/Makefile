CC = gcc
CFLAGS = -O2 -Wall -Wextra -DAKARI_MAX_CONNECTIONS=1024 -DAKARI_RATE_BURST=10000000 -DAKARI_RATE_REFILL_PER_SEC=10000000 -I../../include -I../../vendor/picohttpparser -I../../vendor/jsmn

SRCS = main.c \
       ../../src/akari_core.c \
       ../../src/akari_http.c \
       ../../src/akari_event.c \
       ../../src/akari_event_epoll.c \
       ../../vendor/picohttpparser/picohttpparser.c

TARGET = akari_linux_demo

all:
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)
