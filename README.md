# Option Pricing Tools

A suite of command-line tools for calculating option prices and implied volatilities using both Black-Scholes and Heston stochastic volatility models.

## Overview

This project provides robust implementations of various option pricing models, with a focus on both accuracy and computational efficiency. It includes both traditional Black-Scholes implementations and more advanced stochastic volatility models using both quadrature and Fast Fourier Transform (FFT) approaches.

## Features

- **Black-Scholes Implementation**
  - Numerically stable calculation of option prices
  - Newton-Raphson method for implied volatility
  - Handling of edge cases and extreme market conditions

- **Stochastic Volatility (Heston Model)**
  - Quadrature-based implementation
  - FFT-based implementation using Carr-Madan approach
  - Parameter calibration and estimation
  - Volatility surface modeling (skew/smile)

- **Performance Optimizations**
  - Caching of FFT results for repeated calculations
  - Efficient parameter validation and interpolation
  - Precomputed FFT values for improved performance (v4)
  - Adaptive FFT parameters based on option characteristics (v5)

- **Error Handling & Robustness**
  - Sophisticated error detection with signal handlers
  - Multiple fallback parameter sets when calculations fail
  - Automatic detection of numerically challenging parameter sets
  - Graceful degradation to Black-Scholes in extreme cases

- **Utilities**
  - Market data integration (risk-free rates)
  - Profiling, benchmarking, and debugging tools
  - Memory leak detection utilities

## Installation

### Prerequisites

- GCC or compatible C compiler
- FFTW3 library (for FFT-based implementations)
- Bash shell environment
- Valgrind (optional, for memory debugging)

### Compiling

Use the provided Makefile:
```bash
# Build all implementations
make all

# Build a specific version
make calculate_sv_v4

# Build debug version with enhanced error handling
make calculate_sv_v4_debug

# Build latest version with adaptive parameters
make calculate_sv_v5

# Build with profiling enabled
make BUILD_TYPE=profile profile_builds
```

## Usage

### Basic Usage

Calculate option values using Black-Scholes (v2):
```bash
./price_option.sh UNDERLYING_PRICE STRIKE DAYS_TO_EXPIRY OPTION_PRICE UNDERLYING_YIELD
```

Example:
```bash
./price_option.sh 5616 5600 9 118.5 0.013
```

### Stochastic Volatility Models

Calculate option values with the Heston model (v3):
```bash
./price_option_v3.sh UNDERLYING_PRICE STRIKE DAYS_TO_EXPIRY OPTION_PRICE UNDERLYING_YIELD v3
```

Use the optimized FFT implementation (v4) with custom parameters:
```bash
./price_option_v4.sh --fft-n=8192 --alpha=1.75 --eta=0.025 UNDERLYING_PRICE STRIKE DAYS_TO_EXPIRY OPTION_PRICE UNDERLYING_YIELD
```

Use the adaptive parameter implementation (v5):
```bash
# Parameters will automatically adapt based on option characteristics
./price_option_v5.sh UNDERLYING_PRICE STRIKE DAYS_TO_EXPIRY OPTION_PRICE UNDERLYING_YIELD
```

### Running Tests and Debugging

Test the stochastic volatility model across different parameters:
```bash
./test_sv_range.sh
```

Run FFT parameter sensitivity tests:
```bash
./test_fft_params.sh
```

Compare performance between implementations:
```bash
./profile_compare.sh
```

Debug memory issues with Valgrind:
```bash
./debug_with_valgrind.sh OPTION_PRICE STOCK_PRICE STRIKE TIME RISK_FREE [DIVIDEND]
```

Compile and test the debug version:
```bash
./compile_and_test_debug.sh
```

## Technical Details

### Black-Scholes Model

The Black-Scholes model assumes constant volatility and provides a closed-form solution for European options. The implementation handles numerical challenges such as:
- Deep in-the-money/out-of-the-money options
- Very short/long expiration times
- Extreme volatility values

### Heston Stochastic Volatility Model

The Heston model allows for:
- Mean-reverting volatility
- Correlation between asset price and volatility
- Volatility of volatility
- More accurate pricing for options with skew/smile effects

### FFT Implementation

The FFT-based approach:
- Uses the Carr-Madan method for European options
- Efficiently computes option prices for multiple strikes simultaneously
- Provides caching to avoid redundant calculations
- v4 implementation includes precomputed FFT input values for better performance
- v5 implementation features adaptive parameter selection based on:
  - Option moneyness (far ITM/OTM adjustments)
  - Time to expiry (special handling for short/long-dated options)
  - Numerical stability considerations

### Error Handling Framework

The latest versions implement a robust error handling framework:
- Signal handlers for detecting segmentation faults and floating-point exceptions
- Exception handling with setjmp/longjmp for recovery from numerical issues
- Automatic parameter adaptation for numerically challenging cases
- Multiple fallback strategies with progressively safer parameter sets
- Default to Black-Scholes when necessary with sensible volatility approximations

## Project Structure

- `calculate_iv*.c`: Black-Scholes implementations
- `calculate_sv*.c`: Stochastic volatility implementations
  - `calculate_sv_v2.c`: Heston model with FFTW
  - `calculate_sv_v3.c`: FFT-based implementation with caching
  - `calculate_sv_v4.c`: Optimized FFT implementation with precomputed values
  - `calculate_sv_v4_debug.c`: Debug version with enhanced error handling
  - `calculate_sv_v5.c`: Adaptive FFT parameters for all market conditions
- `price_option*.sh`: Shell interfaces for different implementations
- `test_*.sh`: Test and benchmark scripts
- `debug_with_valgrind.sh`: Memory debugging utility
- `compile_and_test_debug.sh`: Script for testing debug versions
- `profile_compare.sh`: Performance comparison tool

## TODOs and Future Enhancements

### Short-Term (High Priority)

1. **Caching Improvements**
   - [x] Implement a more tolerant caching strategy with parameter sensitivity awareness
   - [ ] Create multi-level cache for different parameter sets
   - [x] Add adaptive tolerance mechanism based on parameter sensitivity

2. **Error Handling**
   - [x] Enhance error reporting and propagation from C programs to shell scripts
   - [x] Add better fallback mechanisms when calculations fail

### Medium-Term

1. **Model Enhancements**
   - [ ] Implement full Heston parameter calibration using market data
   - [ ] Add more sophisticated volatility surface modeling
   - [ ] Implement additional stochastic volatility models (SABR, etc.)

2. **Performance Optimization**
   - [x] Early termination in parameter search loops for faster calibration
   - [x] Optimized parameter ordering for faster convergence
   - [x] Enhanced FFT calculation with FFTW_MEASURE for better performance
   - [ ] Multi-threading support for parameter sweeps and calibration
   - [ ] Cache persistence between program invocations

### Long-Term Vision

1. **Architecture Improvements**
   - [ ] Refactor to a shared library architecture
   - [ ] Create unified API for all pricing models
   - [ ] Implement language bindings for Python/R/etc.

2. **Feature Expansion**
   - [ ] Support for exotic options (barriers, Asians, etc.)
   - [ ] Real-time market data integration
   - [ ] Visualization of volatility surfaces and option Greeks

## Version Comparison Guide

The following table compares the different versions of stochastic volatility implementations to help you choose the right tool for your specific needs:

| Version | Key Features | Best For | Limitations | When to Use |
|---------|--------------|----------|-------------|-------------|
| **v2** | Basic Heston model implementation | Learning, simple options | Lacks robustness for extreme parameters, no caching | Educational purposes, simple cases, understanding the model |
| **v3** | FFT with caching mechanism | Batch pricing with same parameters | Fixed FFT parameters, less adaptive | When pricing multiple strikes with same parameters |
| **v4** | Optimized FFT, precomputation of values | Performance-critical applications | Requires manual parameter tuning | When speed matters and you're an experienced user |
| **v5** | Adaptive parameters, robust error handling | Production use, all option types | Slightly higher overhead for simple cases | Default choice for most users and production systems |

## Troubleshooting Common Issues

### Calculation Errors

1. **"Non-finite values in characteristic function"**
   - **Cause**: Challenging parameter set producing numerical instability in the Heston characteristic function
   - **Solution**: Use v5 with `--debug` flag to see adaptive parameter selection, or try with different FFT parameters

2. **"Implied volatility calculation failed"**
   - **Cause**: Option parameters may be outside valid market ranges or create numerical issues
   - **Solution**: Verify market price is consistent with no-arbitrage bounds; for very short-dated options, try increasing FFT resolution

3. **"FFT calculation produced invalid results"**
   - **Cause**: FFT parameters may be unsuitable for the option characteristics
   - **Solution**: For v4, manually adjust parameters (try `--fft-n=8192 --alpha=1.5`); for v5, enable verbose debugging

### Build and Dependency Issues

1. **"FFTW3 library not found"**
   - **Cause**: Missing required dependency
   - **Solution**: Install with:
     - Debian/Ubuntu: `sudo apt-get install libfftw3-dev`
     - Fedora/RHEL: `sudo dnf install fftw-devel`
     - macOS: `brew install fftw`

2. **"make: Command not found"**
   - **Cause**: Build tools not installed
   - **Solution**: Install build essentials:
     - Debian/Ubuntu: `sudo apt-get install build-essential`
     - Fedora/RHEL: `sudo dnf groupinstall "Development Tools"`

3. **"Segmentation fault" or program crashes**
   - **Cause**: Memory access issues or numerical instability
   - **Solution**: Run with Valgrind using the provided script: `./debug_with_valgrind.sh` with your parameters, or try v4_debug version

## Quick Start Examples

### Example 1: Standard ATM Option Pricing

```bash
./price_option_v5.sh 100 100 30 3.5 0.02
```

This command prices an at-the-money option with:
- Underlying price: $100
- Strike price: $100
- Days to expiry: 30
- Market price: $3.50
- Dividend yield: 2%

The output will show both Black-Scholes and stochastic volatility implied volatilities, with the latter being more market-consistent for options with volatility skew.

### Example 2: Deep OTM Option with Custom Parameters

```bash
./price_option_v4.sh --fft-n=8192 --alpha=1.75 100 120 180 1.25 0.0
```

This example prices a deep out-of-the-money option with:
- Underlying price: $100
- Strike price: $120 (20% OTM)
- Days to expiry: 180 (6 months)
- Market price: $1.25
- Dividend yield: 0%
- Custom FFT parameters for greater precision with deep OTM options

### Example 3: Generating a Volatility Smile

```bash
# Script to generate a volatility smile across multiple strikes
# Save as generate_smile.sh and make executable with chmod +x generate_smile.sh

#!/bin/bash
SPOT=100
DAYS=90
YIELD=0.01
RATE=0.05
BASE_VOL=0.2

echo "Strike,BS IV,SV IV"
for STRIKE in $(seq 80 5 120); do
    # First calculate a theoretical price using Black-Scholes with a base volatility
    PRICE=$(./calculate_iv_v2 --calc-price $SPOT $STRIKE $DAYS $RATE $YIELD $BASE_VOL)
    
    # Then calculate implied vols using both models
    RESULT=$(./price_option_v5.sh $SPOT $STRIKE $DAYS $PRICE $YIELD)
    
    # Extract the implied vols using grep and awk
    BS_IV=$(echo "$RESULT" | grep "Implied Volatility(BS)" | awk '{print $3}' | sed 's/%//')
    SV_IV=$(echo "$RESULT" | grep "Implied Volatility(SV)" | awk '{print $3}' | sed 's/%//')
    
    echo "$STRIKE,$BS_IV,$SV_IV"
done
```

This script generates a volatility smile by pricing options across different strikes, showing how the stochastic volatility model captures the smile/skew effect while Black-Scholes gives flat volatilities.

### Example 4: Handling a Low-Volatility Environment

```bash
# Use v5 with automatic adaptation for a low-volatility market scenario
./price_option_v5.sh --debug 5616 5600 9 18.5 0.013
```

This example demonstrates pricing a short-dated option in a low-volatility environment where numerical stability can be challenging:
- Underlying price: $5,616
- Strike price: $5,600
- Days to expiry: 9 (very short-dated)
- Market price: $18.50
- Dividend yield: 1.3%

The `--debug` flag helps show how parameters are automatically adapted.

### Example 5: Comparing Model Versions

```bash
# Script to compare all model versions
# Save as compare_models.sh and make executable with chmod +x compare_models.sh

#!/bin/bash
SPOT=100
STRIKE=105
DAYS=30
PRICE=2.0
YIELD=0.02

# Run all versions and extract implied volatility
echo "Comparing model versions for $SPOT/$STRIKE/$DAYS option:"

# Black-Scholes
BS_IV=$(./calculate_iv_v2 $PRICE $SPOT $STRIKE $(echo "$DAYS/365" | bc -l) 0.05 $YIELD)
echo "Black-Scholes IV: $(echo "$BS_IV * 100" | bc -l)%"

# SV v3
SV3_RESULT=$(./price_option_v3.sh $SPOT $STRIKE $DAYS $PRICE $YIELD v3 2>/dev/null)
SV3_IV=$(echo "$SV3_RESULT" | grep "Implied Volatility(SV)" | awk '{print $3}')
echo "Stochastic Vol v3: $SV3_IV"

# SV v4
SV4_RESULT=$(./price_option_v4.sh $SPOT $STRIKE $DAYS $PRICE $YIELD 2>/dev/null)
SV4_IV=$(echo "$SV4_RESULT" | grep "Implied Volatility(SV)" | awk '{print $3}')
echo "Stochastic Vol v4: $SV4_IV"

# SV v5
SV5_RESULT=$(./price_option_v5.sh $SPOT $STRIKE $DAYS $PRICE $YIELD 2>/dev/null)
SV5_IV=$(echo "$SV5_RESULT" | grep "Implied Volatility(SV)" | awk '{print $3}')
echo "Stochastic Vol v5: $SV5_IV"
```

This script compares all model versions on the same option to highlight differences in calculation results.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.