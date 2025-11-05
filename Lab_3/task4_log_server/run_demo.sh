#!/bin/bash
# Demo script for log server and multiple clients

echo "=== Log Server Demo ==="
echo ""

# Build
echo "Building server and client..."
make clean
make
echo ""

# Check if build was successful
if [ ! -f log_server.exe ] || [ ! -f log_client.exe ]; then
    echo "Build failed!"
    exit 1
fi

# Start server in background
echo "Starting log server..."
wine log_server.exe &
SERVER_PID=$!
echo "Server PID: $SERVER_PID"
sleep 2

# Start multiple clients with different IDs and timing
echo ""
echo "Starting client 1 (5 messages, 500ms delay)..."
wine log_client.exe 1 5 500 &

sleep 0.5

echo "Starting client 2 (3 messages, 800ms delay)..."
wine log_client.exe 2 3 800 &

sleep 0.5

echo "Starting client 3 (4 messages, 600ms delay)..."
wine log_client.exe 3 4 600 &

echo ""
echo "Clients running... Wait for completion..."
sleep 8

# Stop server
echo ""
echo "Stopping server..."
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo ""
echo "=== Demo complete ==="
echo ""
echo "Check the log file:"
cat server.log 2>/dev/null || echo "Log file not found"
