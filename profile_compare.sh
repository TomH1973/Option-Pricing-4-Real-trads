#!/bin/bash

# This script compares the performance of the original and optimized FFT implementations
# by running profiling tests with gprof

echo "========== Performance Comparison =========="
echo "Comparing original FFT (v3) vs optimized FFT input (v4)"
echo ""

# Test parameters
OPTION_PRICE=5.0
STOCK_PRICE=100.0
STRIKE=100.0
TIME=0.25
RISK_FREE=0.05
DIVIDEND=0.02

# Number of iterations for accurate profiling
ITERATIONS=100

# Create a temporary directory for profiling data
mkdir -p /tmp/profile_data

# Function to run a test with timing
run_profile_test() {
    local executable=$1
    local name=$2
    local profile_file="/tmp/profile_data/${name}.prof"
    
    echo "Testing $name version..."
    echo "Running $ITERATIONS iterations..."
    
    # Create a timing file
    mkdir -p /tmp/profile_data
    
    # Just do performance timing for now
    time (
        for i in $(seq 1 $ITERATIONS); do
            /home/usr0/projects/option_tools/$executable $OPTION_PRICE $STOCK_PRICE $STRIKE $TIME $RISK_FREE $DIVIDEND > /dev/null
        done
    ) 2> "/tmp/profile_data/${name}.time"
    
    # Display timing results
    echo ""
    echo "Timing results for $name:"
    cat "/tmp/profile_data/${name}.time"
    echo ""
}

# Run and profile the original version
run_profile_test "calculate_sv_v3_profile" "original"

# Run and profile the optimized version
run_profile_test "calculate_sv_v4" "optimized"

echo "========== Comparison Summary =========="
echo "Timing data is available in /tmp/profile_data/"
echo ""
echo "Original version timing: "
cat "/tmp/profile_data/original.time"
echo ""
echo "Optimized version timing: "
cat "/tmp/profile_data/optimized.time"
echo ""
echo "Note: For more detailed profiling, you can use a profiling tool like 'perf':"
echo "perf record /home/usr0/projects/option_tools/calculate_sv_v3 $OPTION_PRICE $STOCK_PRICE $STRIKE $TIME $RISK_FREE $DIVIDEND"
echo "perf report"
echo ""