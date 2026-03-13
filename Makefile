CC = gcc
CFLAGS = -Wall -Wextra -O2 -DAKARI_DEBUG

INCLUDES = -Iinclude -Ivendor/picohttpparser -Ivendor/jsmn

# Platform detection (default to linux)
PLATFORM ?= linux

# Core source files
SRCS = examples/basic_server/main.c \
       src/akari_core.c \
       src/akari_event.c \
       src/akari_http.c \
       vendor/picohttpparser/picohttpparser.c

# Select the event engine based on platform
ifeq ($(PLATFORM), linux)
    SRCS += src/akari_event_epoll.c
else
    SRCS += src/akari_event_poll.c
    CFLAGS += -DAKARI_USE_POLL
endif

TARGET = akari_server

all: $(TARGET)

$(TARGET):
	$(CC) $(CFLAGS) $(INCLUDES) $(SRCS) -o $(TARGET)

# Bundle the single-header and recompile the self-contained CLI
release:
	python3 scripts/gen_single_header.py
	$(CC) -O2 tools/akari_cli.c -o akari

clean:
	rm -f $(TARGET)

.PHONY: all clean
