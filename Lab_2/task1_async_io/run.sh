#!/bin/bash
# Build and run script for async_io demo

echo "=== Building async_io demo ==="
make clean
make

if [ $? -eq 0 ]; then
    echo ""
    echo "=== Running async_io demo ==="
    echo ""
    make run
else
    echo "Build failed!"
    exit 1
fi
