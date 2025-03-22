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

- **Utilities**
  - Market data integration (risk-free rates)
  - Comprehensive testing and evaluation scripts

## Installation

### Prerequisites

- GCC or compatible C compiler
- FFTW3 library (for FFT-based implementation)
- Bash shell environment

### Compiling

Standard implementations:
```bash
gcc -o calculate_iv_v2 calculate_iv_v2.c -lm
gcc -o calculate_sv_v2 calculate_sv_v2.c -lm
```

FFT-based implementation:
```bash
./compile_sv_v3.sh
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

### Stochastic Volatility Model

Calculate option values with the Heston model (v3):
```bash
./price_option_v3.sh UNDERLYING_PRICE STRIKE DAYS_TO_EXPIRY OPTION_PRICE UNDERLYING_YIELD v3
```

### Running Tests

Test the stochastic volatility model across different parameters:
```bash
./test_sv_range.sh
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

## Project Structure

- `calculate_iv*.c`: Black-Scholes implementations
- `calculate_sv*.c`: Stochastic volatility implementations
- `price_option.sh`: Main interface script
- `price_option_v3.sh`: Enhanced script for FFT-based implementation
- `test_sv_range.sh`: Test script for model evaluation
- `compile_sv_v3.sh`: Compilation script for FFT implementation

## Future Enhancements

See [PROJECT_SUMMARY.md](PROJECT_SUMMARY.md) for detailed roadmap of planned improvements.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.