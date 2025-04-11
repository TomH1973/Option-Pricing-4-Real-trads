#!/bin/bash

# Script to compile and test the debug version of calculate_sv_v4

# Compile the debug version
echo "Compiling calculate_sv_v4_debug.c..."
make calculate_sv_v4_debug

# Check if compilation was successful
if [ $? -ne 0 ]; then
    echo "Error: Compilation failed"
    exit 1
fi

# Make the script executable
chmod +x price_option_v4_robust.sh

# Test with the problematic parameters
echo "Testing with problematic parameters..."
./price_option_v4_robust.sh 5766.6 5645 0.1110 18.3 0.0132

# Test with standard parameters
echo "Testing with standard parameters..."
./price_option_v4_robust.sh 5.0 100.0 100.0 0.25 0.05 0.02

echo "Tests complete" 