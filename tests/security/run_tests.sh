#!/bin/bash
set -e

cd "$(dirname "$0")/../.."

CC="${CC:-gcc}"
CFLAGS="-Wall -Wextra -O2 -DAKARI_DEBUG -fsanitize=address,undefined -g"
INCLUDES="-Iinclude -Ivendor/picohttpparser -Ivendor/jsmn"
TEST_PORT=9777

echo "Building test server..."
$CC $CFLAGS $INCLUDES \
    tests/security/test_server.c \
    src/akari_core.c \
    src/akari_event.c \
    src/akari_http.c \
    vendor/picohttpparser/picohttpparser.c \
    src/akari_event_epoll.c \
    -o tests/security/test_server

echo "Building test client..."
$CC -Wall -Wextra -O2 -g \
    tests/security/test_security.c \
    -o tests/security/test_runner

cleanup() {
    if [ -n "$SERVER_PID" ] && kill -0 "$SERVER_PID" 2>/dev/null; then
        kill -TERM "$SERVER_PID" 2>/dev/null
        wait "$SERVER_PID" 2>/dev/null || true
    fi
    rm -f tests/security/test_server tests/security/test_runner
}
trap cleanup EXIT

echo "Starting test server on port $TEST_PORT..."
./tests/security/test_server &
SERVER_PID=$!
sleep 1

if ! kill -0 "$SERVER_PID" 2>/dev/null; then
    echo "ERROR: Server failed to start"
    exit 1
fi

echo "Running security tests..."
echo ""

./tests/security/test_runner
EXIT_CODE=$?

echo "Server memory usage:"
ps -o rss,vsz,comm -p "$SERVER_PID" 2>/dev/null || true

exit $EXIT_CODE
