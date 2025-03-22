#!/bin/bash

# Script to test the impact of different FFT parameters on option pricing

echo "===== Testing FFT Parameter Sensitivity ====="

# Ensure the program is compiled
cd /home/usr0/projects/option_tools
make calculate_sv_v4

# Test parameters
OPTION_PRICE=5.0
STOCK_PRICE=100.0
STRIKE=100.0
TIME=0.25
RATE=0.05
DIVIDEND=0.02

# Test different FFT grid sizes
echo "--- Testing FFT grid sizes ---"
for FFT_N in 1024 2048 4096 8192 16384; do
    echo -n "FFT_N = $FFT_N: "
    time ./calculate_sv_v4 --fft-n=$FFT_N $OPTION_PRICE $STOCK_PRICE $STRIKE $TIME $RATE $DIVIDEND
done

echo ""

# Test different grid spacing (eta)
echo "--- Testing grid spacing (eta) ---"
for ETA in 0.1 0.05 0.025 0.01 0.005; do
    echo -n "eta = $ETA: "
    ./calculate_sv_v4 --eta=$ETA $OPTION_PRICE $STOCK_PRICE $STRIKE $TIME $RATE $DIVIDEND
done

echo ""

# Test different dampening factors (alpha)
echo "--- Testing dampening factor (alpha) ---"
for ALPHA in 0.75 1.0 1.25 1.5 1.75 2.0; do
    echo -n "alpha = $ALPHA: "
    ./calculate_sv_v4 --alpha=$ALPHA $OPTION_PRICE $STOCK_PRICE $STRIKE $TIME $RATE $DIVIDEND
done

echo ""

# Test different log strike ranges
echo "--- Testing log strike range ---"
for RANGE in 1.0 2.0 3.0 4.0 5.0; do
    echo -n "range = $RANGE: "
    ./calculate_sv_v4 --log-strike-range=$RANGE $OPTION_PRICE $STOCK_PRICE $STRIKE $TIME $RATE $DIVIDEND
done

echo ""

# Test different cache tolerances
echo "--- Testing cache tolerance ---"
for TOL in 1e-3 1e-4 1e-5 1e-6 1e-7; do
    echo -n "tolerance = $TOL: "
    ./calculate_sv_v4 --cache-tolerance=$TOL $OPTION_PRICE $STOCK_PRICE $STRIKE $TIME $RATE $DIVIDEND
done

echo ""

# Test caching effectiveness with slight parameter variations
echo "--- Testing cache reuse with parameter variations ---"
echo "Running first calculation to populate cache..."
./calculate_sv_v4 --debug $OPTION_PRICE $STOCK_PRICE $STRIKE $TIME $RATE $DIVIDEND > /dev/null

echo "Running with slightly different stock price (should miss cache)..."
./calculate_sv_v4 --debug $OPTION_PRICE $(echo "$STOCK_PRICE + 0.1" | bc) $STRIKE $TIME $RATE $DIVIDEND > /dev/null

echo "Running with original parameters again (should hit cache)..."
./calculate_sv_v4 --debug $OPTION_PRICE $STOCK_PRICE $STRIKE $TIME $RATE $DIVIDEND > /dev/null

echo "Running with very slight parameter change with increased tolerance..."
./calculate_sv_v4 --debug --cache-tolerance=0.05 $OPTION_PRICE $(echo "$STOCK_PRICE + 0.01" | bc) $STRIKE $TIME $RATE $DIVIDEND > /dev/null

echo "===== Test Complete ====="