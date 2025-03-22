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

- **Utilities**
  - Market data integration (risk-free rates)
  - Profiling and benchmarking tools

## Installation

### Prerequisites

- GCC or compatible C compiler
- FFTW3 library (for FFT-based implementations)
- Bash shell environment

### Compiling

Use the provided Makefile:
```bash
# Build all implementations
make all

# Build a specific version
make calculate_sv_v2

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

### Running Tests

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

## Project Structure

- `calculate_iv*.c`: Black-Scholes implementations
- `calculate_sv*.c`: Stochastic volatility implementations
  - `calculate_sv_v2.c`: Heston model with FFTW
  - `calculate_sv_v3.c`: FFT-based implementation with caching
  - `calculate_sv_v4.c`: Optimized FFT implementation with precomputed values
- `price_option*.sh`: Shell interfaces for different implementations
- `test_*.sh`: Test and benchmark scripts
- `profile_compare.sh`: Performance comparison tool

## TODOs and Future Enhancements

### Short-Term (High Priority)

1. **Caching Improvements**
   - [ ] Implement a more tolerant caching strategy with parameter sensitivity awareness
   - [ ] Create multi-level cache for different parameter sets
   - [ ] Add adaptive tolerance mechanism based on parameter sensitivity

2. **Error Handling**
   - [ ] Enhance error reporting and propagation from C programs to shell scripts
   - [ ] Add better fallback mechanisms when calculations fail

### Medium-Term

1. **Model Enhancements**
   - [ ] Implement full Heston parameter calibration using market data
   - [ ] Add more sophisticated volatility surface modeling
   - [ ] Implement additional stochastic volatility models (SABR, etc.)

2. **Performance Optimization**
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

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.