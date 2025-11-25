#!/bin/bash
# Run script for TCP Chat

# Change to the script's directory
cd "$(dirname "$0")"

echo "=== TCP Chat Demo ==="
echo ""

# Build
echo "Building..."
make clean
make
echo ""

# Check if build was successful
if [ ! -f chat_server.exe ] || [ ! -f chat_client.exe ]; then
    echo "Build failed!"
    exit 1
fi

echo "Starting Server in background..."
wine chat_server.exe &
SERVER_PID=$!
sleep 2

echo "Starting Client..."
echo "Type a message and press Enter. Press Ctrl+C to exit."
wine chat_client.exe

# Cleanup
kill $SERVER_PID
