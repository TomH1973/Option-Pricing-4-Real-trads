#!/bin/bash

# Script to debug option pricing tools with Valgrind

# Check if valgrind is installed
if ! command -v valgrind &> /dev/null; then
    echo "Error: Valgrind is not installed. Please install it first:"
    echo "  Ubuntu/Debian: sudo apt-get install valgrind"
    echo "  Fedora/RHEL:   sudo dnf install valgrind"
    exit 1
fi

# Check if we have enough arguments
if [ $# -lt 5 ]; then
    echo "Usage: $0 OptionPrice StockPrice Strike Time RiskFreeRate [DividendYield]"
    exit 1
fi

# Parse arguments
OPTION_PRICE=$1
STOCK_PRICE=$2
STRIKE=$3
TIME=$4
RISK_FREE=$5
DIVIDEND=${6:-0.0}  # Default to 0 if not provided

# Create log directory
LOGDIR="valgrind_logs"
mkdir -p $LOGDIR

# Run with different FFT parameters to find which configuration causes issues
run_valgrind_test() {
    local fft_n=$1
    local alpha=$2
    local eta=$3
    local logfile="$LOGDIR/valgrind_fft${fft_n}_alpha${alpha}_eta${eta}.log"
    
    echo "Running Valgrind with FFT_N=$fft_n, alpha=$alpha, eta=$eta"
    echo "Log file: $logfile"
    
    # Run with Valgrind's memcheck tool with detailed leak checking
    valgrind --tool=memcheck \
             --leak-check=full \
             --show-leak-kinds=all \
             --track-origins=yes \
             --verbose \
             --log-file="$logfile" \
             ./calculate_sv_v4 --debug --fft-n=$fft_n --alpha=$alpha --eta=$eta \
             $OPTION_PRICE $STOCK_PRICE $STRIKE $TIME $RISK_FREE $DIVIDEND
    
    # Check if valgrind found errors
    if grep -q "ERROR SUMMARY: 0 errors" "$logfile"; then
        echo "No memory errors detected with these parameters"
        return 0
    else
        echo "Memory errors detected! See $logfile for details"
        # Extract the error summary
        grep -A 3 "ERROR SUMMARY" "$logfile"
        return 1
    fi
}

# Run with different parameter sets
echo "=== Running Valgrind tests with different FFT parameters ==="
run_valgrind_test 4096 1.5 0.05  # Default parameters
run_valgrind_test 8192 1.5 0.05  # Larger FFT size
run_valgrind_test 4096 1.0 0.05  # Different alpha
run_valgrind_test 4096 1.5 0.1   # Different eta
run_valgrind_test 2048 1.25 0.075 # Alternative set

echo "=== Valgrind testing complete ==="
echo "Check the $LOGDIR directory for detailed logs"

# Provide a summary of findings
echo "=== Summary of Valgrind Findings ==="
grep -l "Invalid read" $LOGDIR/*.log | xargs -r basename
grep -l "Invalid write" $LOGDIR/*.log | xargs -r basename
grep -l "Use of uninitialised value" $LOGDIR/*.log | xargs -r basename

# Check for specific issues in calculate_sv_v4.c
echo "=== Checking for common issues in the code ==="
if grep -q "init_fft_cache" $LOGDIR/*.log; then
    echo "Potential issues in init_fft_cache function"
fi
if grep -q "precompute_fft_values" $LOGDIR/*.log; then
    echo "Potential issues in precompute_fft_values function"
fi
if grep -q "fftw_" $LOGDIR/*.log; then
    echo "Potential issues with FFTW library usage"
fi 