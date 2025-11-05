#!/bin/bash
# Simple run script - just build

echo "=== Building Log Server Demo ==="
make clean
make

if [ $? -eq 0 ]; then
    echo ""
    echo "Build successful!"
    echo ""
    echo "To run the demo:"
    echo "  ./run_demo.sh         # Automated demo with multiple clients"
    echo ""
    echo "Or manually:"
    echo "  wine log_server.exe   # In one terminal"
    echo "  wine log_client.exe 1 5 1000  # In another terminal (client ID, msg count, delay)"
else
    echo "Build failed!"
    exit 1
fi
