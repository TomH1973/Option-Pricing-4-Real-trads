#!/bin/bash

# Test script to evaluate SV model across a wide range of parameters
# This should help us determine if the SV model produces varied IVs

echo "===== Testing SV Model with Wide Parameter Range ====="

# Ensure the program is compiled
cd /home/usr0/projects/option_tools
gcc -o calculate_sv_v2 calculate_sv_v2.c -lfftw3 -lm

# Function to run test
run_test() {
    local S=$1
    local K=$2
    local T=$3
    local price=$4
    local r=$5
    local q=$6
    
    printf "Test: S=%.2f K=%.2f T=%.4f price=%.2f r=%.4f q=%.4f " $S $K $T $price $r $q
    
    # Run the SV calculator
    result=$(/home/usr0/projects/option_tools/calculate_sv_v2 $price $S $K $T $r $q 2>/dev/null)
    if [ $? -eq 0 ]; then
        # Convert to percentage
        pct=$(echo "$result * 100" | bc -l)
        printf "IV=%.2f%%\n" $pct
    else
        echo "Error calculating IV"
    fi
}

# Test a wide variety of option scenarios

# ATM options with different times to expiry
echo "--- ATM Options with Different Expirations ---"
run_test 100 100 0.1 3.0 0.05 0.02   # 36 days
run_test 100 100 0.25 5.0 0.05 0.02  # 3 months
run_test 100 100 0.5 7.0 0.05 0.02   # 6 months
run_test 100 100 1.0 10.0 0.05 0.02  # 1 year
run_test 100 100 2.0 14.0 0.05 0.02  # 2 years

# ITM options (different moneyness)
echo "--- ITM Options with Different Moneyness ---"
run_test 100 95 0.25 7.0 0.05 0.02   # Slightly ITM
run_test 100 90 0.25 11.0 0.05 0.02  # ITM
run_test 100 80 0.25 20.5 0.05 0.02  # Deep ITM

# OTM options (different moneyness)
echo "--- OTM Options with Different Moneyness ---"
run_test 100 105 0.25 2.5 0.05 0.02  # Slightly OTM
run_test 100 110 0.25 1.2 0.05 0.02  # OTM
run_test 100 120 0.25 0.4 0.05 0.02  # Deep OTM

# Different rates and yields
echo "--- Options with Different Rates and Yields ---"
run_test 100 100 0.25 5.0 0.01 0.02  # Low interest rate
run_test 100 100 0.25 5.0 0.08 0.02  # High interest rate
run_test 100 100 0.25 5.0 0.05 0.00  # No dividend
run_test 100 100 0.25 5.0 0.05 0.05  # High dividend

# Price variation for same strike/expiry
echo "--- Same Option with Different Market Prices ---"
run_test 100 100 0.25 3.0 0.05 0.02  # Low price - low IV
run_test 100 100 0.25 5.0 0.05 0.02  # Mid price - mid IV
run_test 100 100 0.25 8.0 0.05 0.02  # High price - high IV

echo "===== Test Complete ====="