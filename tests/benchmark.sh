#!/bin/bash

make clean && make sanitize

./akari_server &
SERVER_PID=$!

sleep 1

echo "🏮 Starting Stress Test (wrk)..."
wrk -t4 -c100 -d10s http://localhost:8080/api

echo "Memory usage for PID $SERVER_PID:"
ps -o rss,vsz,comm -p $SERVER_PID

kill -SIGINT $SERVER_PID
wait $SERVER_PID

echo "🏮 Memory audit complete."
