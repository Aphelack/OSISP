#!/bin/bash
# Script to run the self-restoring process with Wine

# Change to the script's directory
cd "$(dirname "$0")"

echo "Starting Self-Restoring Process Demo..."
echo "========================================"
echo ""
echo "Instructions:"
echo "1. Enter text in the 'Message' field"
echo "2. Click 'Increment Counter' to increase the counter"
echo "3. Click 'Close (Self-Restore)' to restart the process"
echo "4. The new window should open with the same counter and message values"
echo ""
echo "Starting application..."
echo ""

wine self_restoring.exe
