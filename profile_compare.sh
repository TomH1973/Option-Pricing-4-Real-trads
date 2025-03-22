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

# Function to run a test with profiling
run_profile_test() {
    local executable=$1
    local name=$2
    local profile_data="/tmp/profile_data/${name}.out"
    local profile_file="/tmp/profile_data/${name}.prof"
    
    echo "Testing $name version..."
    echo "Running $ITERATIONS iterations..."
    
    # Clear any previous profiling data
    rm -f gmon.out
    
    # Create a unique output file for this run
    gmon_file="/tmp/profile_data/gmon.${name}.out"
    
    # Run the test multiple times for better profiling data
    time (
        for i in $(seq 1 $ITERATIONS); do
            # Run with profiling output to the specified file
            # Note: gprof will use the file named 'gmon.out' by default
            $executable $OPTION_PRICE $STOCK_PRICE $STRIKE $TIME $RISK_FREE $DIVIDEND > /dev/null
            
            # If gmon.out was created, append to our collection
            if [ -f gmon.out ]; then
                cat gmon.out >> "$gmon_file"
                rm gmon.out
            fi
        done
    )
    
    # Generate profiling report if we have data
    if [ -f "$gmon_file" ]; then
        gprof $executable "$gmon_file" > "$profile_file"
        
        # Extract and display the top 10 functions by self time
        echo ""
        echo "Top functions by self time for $name:"
        gprof -b $executable "$gmon_file" | head -n 20
        echo ""
        
        echo "Detailed profile saved to $profile_file"
    else
        echo ""
        echo "Warning: No profiling data was generated for $name"
        echo "This may happen if the code executed too quickly"
    fi
    echo ""
}

# Run and profile the original version
run_profile_test "./calculate_sv_v3_profile" "original"

# Run and profile the optimized version
run_profile_test "./calculate_sv_v4" "optimized"

echo "========== Comparison Summary =========="
echo "Full profiling data is available in /tmp/profile_data/"
echo ""
echo "To view detailed profiling results for the original version:"
echo "gprof -b ./calculate_sv_v3_profile /tmp/profile_data/gmon.original.out | less"
echo ""
echo "To view detailed profiling results for the optimized version:"
echo "gprof -b ./calculate_sv_v4 /tmp/profile_data/gmon.optimized.out | less"
echo ""