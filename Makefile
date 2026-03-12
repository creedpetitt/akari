CC = gcc
CFLAGS = -Wall -Wextra -O2 -DAKARI_DEBUG

INCLUDES = -Iinclude -Ivendor/picohttpparser -Ivendor/jsmn

# Building for Linux because of epoll
SRCS = examples/basic_server/main.c \
       src/akari_core.c \
       src/akari_event.c \
       src/akari_event_epoll.c \
       src/akari_http.c \
       vendor/picohttpparser/picohttpparser.c

TARGET = akari_server

all: $(TARGET)

$(TARGET):
	$(CC) $(CFLAGS) $(INCLUDES) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean
