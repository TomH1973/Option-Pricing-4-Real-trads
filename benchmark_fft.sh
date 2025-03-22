#!/bin/bash

# This script benchmarks the optimized FFT implementation with different parameter settings

echo "========== FFT Parameter Benchmark =========="

# Test parameters
OPTION_PRICE=5.0
STOCK_PRICE=100.0
STRIKE=100.0
TIME=0.25
RISK_FREE=0.05
DIVIDEND=0.02

# Number of iterations for each test
ITERATIONS=10

# Function to run a benchmark test
run_benchmark() {
    local params=$1
    local desc=$2
    
    echo "Testing: $desc"
    echo "Parameters: $params"
    
    # First run to initialize cache
    ./calculate_sv_v4 $params $OPTION_PRICE $STOCK_PRICE $STRIKE $TIME $RISK_FREE $DIVIDEND > /dev/null
    
    # Benchmark run
    time (
        for i in $(seq 1 $ITERATIONS); do
            ./calculate_sv_v4 $params $OPTION_PRICE $STOCK_PRICE $STRIKE $TIME $RISK_FREE $DIVIDEND > /dev/null
        done
    )
    
    echo ""
}

# Benchmark: Default parameters
run_benchmark "" "Default parameters (N=4096, alpha=1.5, eta=0.05)"

# Benchmark: Different FFT grid sizes
run_benchmark "--fft-n=1024" "Small FFT grid (N=1024)"
run_benchmark "--fft-n=8192" "Large FFT grid (N=8192)"
run_benchmark "--fft-n=16384" "Very large FFT grid (N=16384)"

# Benchmark: Different grid spacing
run_benchmark "--eta=0.1" "Large grid spacing (eta=0.1)"
run_benchmark "--eta=0.025" "Small grid spacing (eta=0.025)"
run_benchmark "--eta=0.01" "Very small grid spacing (eta=0.01)"

# Benchmark: Different Carr-Madan alpha
run_benchmark "--alpha=1.0" "Low dampening (alpha=1.0)"
run_benchmark "--alpha=2.0" "High dampening (alpha=2.0)"

# Benchmark: Combined settings
run_benchmark "--fft-n=8192 --eta=0.025" "High resolution (N=8192, eta=0.025)"
run_benchmark "--fft-n=1024 --eta=0.1" "Low resolution (N=1024, eta=0.1)"

# Benchmark: Different cache tolerance
run_benchmark "--cache-tolerance=1e-3" "Low cache tolerance (1e-3)"
run_benchmark "--cache-tolerance=1e-7" "High cache tolerance (1e-7)"

echo "========== Benchmark Complete =========="