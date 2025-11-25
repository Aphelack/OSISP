#!/bin/bash
# Run script for DLL Demo

# Change to the script's directory
cd "$(dirname "$0")"

echo "=== DLL Demo ==="
echo ""

# Build
echo "Building..."
make clean
make
echo ""

# Check if build was successful
if [ ! -f client.exe ] || [ ! -f math_library.dll ]; then
    echo "Build failed!"
    exit 1
fi

# Run
echo "Running Client..."
wine client.exe
