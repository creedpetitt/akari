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

# Test/Fuzz Source Files (internal)
FUZZ_SRCS = tests/fuzz_target.c \
            src/akari_core.c \
            src/akari_event.c \
            src/akari_http.c \
            vendor/picohttpparser/picohttpparser.c

all: $(TARGET)

$(TARGET):
	$(CC) $(CFLAGS) $(INCLUDES) $(SRCS) -o $(TARGET)

# AddressSanitizer + UndefinedBehaviorSanitizer
sanitize: CFLAGS += -fsanitize=address,undefined -g -O1
sanitize: $(TARGET)

# AFL++ Fuzzing Instrumentation
fuzz: CC = afl-gcc-fast
fuzz: CFLAGS = -g -O2 -Iinclude -Ivendor/picohttpparser -Ivendor/jsmn -DAKARI_USE_POLL
fuzz:
	$(CC) $(CFLAGS) $(INCLUDES) $(FUZZ_SRCS) -o akari_fuzzer

# Bundle the single-header and recompile the self-contained CLI
release:
	python3 scripts/gen_single_header.py
	$(CC) -O2 tools/akari_cli.c -o akari

security-test:
	bash tests/security/run_tests.sh

clean:
	rm -f $(TARGET)

.PHONY: all clean security-test
