#!/bin/bash
# Run script for readers-writers demo

# Change to the script's directory
cd "$(dirname "$0")"

echo "=== Readers-Writers Demo ==="
echo ""

# Build
echo "Building..."
make clean
make
echo ""

# Check if build was successful
if [ ! -f readers_writers.exe ]; then
    echo "Build failed!"
    exit 1
fi

# Run
echo "Running simulation..."
wine readers_writers.exe
