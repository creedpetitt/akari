CC = gcc
CFLAGS = -O2 -Wall -Wextra -I../../include -I../../vendor/picohttpparser

SRCS = main.c \
       ../../src/akari_core.c \
       ../../src/akari_http.c \
       ../../src/akari_event_poll.c \
       ../../vendor/picohttpparser/picohttpparser.c

TARGET = akari_linux_demo

all:
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)
