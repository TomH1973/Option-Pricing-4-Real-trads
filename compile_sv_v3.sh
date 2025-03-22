#!/bin/bash

# This script compiles the improved Heston model implementation with FFT
# and checks for necessary dependencies

echo "Checking for FFTW3 library..."

# Check if pkg-config is available
if ! command -v pkg-config &> /dev/null; then
    echo "pkg-config not found - checking libraries manually"
    
    # Check if FFTW headers and library are available without pkg-config
    if [ ! -f /usr/include/fftw3.h ] && [ ! -f /usr/local/include/fftw3.h ]; then
        echo "FFTW3 development headers not found."
        echo "Please install the FFTW3 development package."
        echo "On Ubuntu/Debian, run: sudo apt-get install libfftw3-dev"
        echo "On Fedora/RHEL, run: sudo dnf install fftw-devel"
        echo "On macOS with Homebrew, run: brew install fftw"
        exit 1
    fi
    
    # Assume standard library locations
    FFTW_CFLAGS="-I/usr/include"
    FFTW_LIBS="-lfftw3 -lm"
else
    # Use pkg-config to find FFTW3 if available
    if ! pkg-config --exists fftw3; then
        echo "FFTW3 development package not found."
        echo "Please install the FFTW3 development package."
        echo "On Ubuntu/Debian, run: sudo apt-get install libfftw3-dev"
        echo "On Fedora/RHEL, run: sudo dnf install fftw-devel"
        echo "On macOS with Homebrew, run: brew install fftw"
        exit 1
    fi
    
    FFTW_CFLAGS=$(pkg-config --cflags fftw3)
    FFTW_LIBS=$(pkg-config --libs fftw3)
fi

echo "FFTW3 library found."
echo "Compiling calculate_sv_v3.c..."

# Compile with optimization flags for better performance
gcc -O3 $FFTW_CFLAGS -o calculate_sv_v3 calculate_sv_v3.c $FFTW_LIBS

if [ $? -eq 0 ]; then
    echo "Compilation successful!"
    echo "To use the new implementation, run:"
    echo "./calculate_sv_v3 [options] OptionPrice StockPrice Strike Time RiskFreeRate DividendYield"
    
    # Create soft link to make it work with price_option.sh without modifications
    if [ ! -L calculate_sv_v3_link ]; then
        ln -sf calculate_sv_v3 calculate_sv_v3_link
        echo "Created symbolic link 'calculate_sv_v3_link' that can be used with price_option.sh"
    fi
    
    # Make a test run with sample parameters
    echo "Running a test calculation..."
    ./calculate_sv_v3 --debug 5.0 100.0 100.0 0.25 0.05 0.02
else
    echo "Compilation failed."
fi