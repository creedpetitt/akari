CC ?= gcc
ROOT := ../..

INCLUDES := -I$(ROOT)/include -I$(ROOT)/vendor/picohttpparser -I$(ROOT)/vendor/jsmn

COMMON_DEFS := \
	-DAKARI_MAX_CONNECTIONS=1024 \
	-DAKARI_RATE_BUCKETS=4096 \
	-DAKARI_RATE_BURST=10000000 \
	-DAKARI_RATE_REFILL_PER_SEC=10000000

DEV_CFLAGS ?= -O2 -Wall -Wextra $(COMMON_DEFS)
BENCH_CFLAGS ?= -O3 -DNDEBUG -march=native -flto -Wall -Wextra $(COMMON_DEFS)

SRCS := main.c \
	$(ROOT)/src/akari_core.c \
	$(ROOT)/src/akari_http.c \
	$(ROOT)/src/akari_event.c \
	$(ROOT)/src/akari_event_epoll.c \
	$(ROOT)/vendor/picohttpparser/picohttpparser.c

TARGET := akari_linux_demo

all: build

build:
	$(CC) $(DEV_CFLAGS) $(INCLUDES) $(SRCS) -o $(TARGET)

bench:
	$(CC) $(BENCH_CFLAGS) $(INCLUDES) $(SRCS) -o $(TARGET)

run: build
	./$(TARGET)

bench-run: bench
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all build bench run bench-run clean
