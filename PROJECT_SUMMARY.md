# Option Tools Project Summary

## Project Overview

This project provides a suite of command-line tools for calculating option prices and implied volatilities using different pricing models. It implements both the Black-Scholes model and Heston stochastic volatility model with various improvements across different versions.

### Core Components

1. **Black-Scholes Implementation**
   - `calculate_iv.c` / `calculate_iv_v1.c`: Basic implementation for implied volatility calculation
   - `calculate_iv_v2.c`: Enhanced version with improved numerical stability and Newton-Raphson method

2. **Stochastic Volatility Implementation**
   - `calculate_sv.c`: Initial Heston model implementation
   - `calculate_sv_v2.c`: Improved version with better calibration and parameter estimation
   - `calculate_sv_v3.c`: FFT-based implementation using Carr-Madan approach with FFTW library

3. **Utility Scripts**
   - `price_option.sh`: Main script for option pricing using v2 implementations
   - `price_option_v3.sh`: Enhanced script supporting both v2 and FFT-based v3 implementations
   - `compile_sv_v3.sh`: Compilation script for FFT-based implementation
   - `test_sv_range.sh`: Test script for evaluating SV model across various parameters

## Current State

### Implemented Features

- Black-Scholes option pricing with stable numerical calculations
- Heston stochastic volatility model with quadrature-based integration
- FFT-based option pricing using Carr-Madan approach (faster and more accurate)
- Volatility skew/smile adjustments for better market realism
- Dynamic fetching of risk-free rates from market data
- Parameter calibration for stochastic volatility models
- Result caching in FFT implementation for performance optimization

### Known Issues

1. **Caching Efficiency**
   - Cache in FFT implementation isn't being effectively reused between iterations
   - Small variations in parameters cause unnecessary cache misses
   - Current cache structure only supports a single parameter set

2. **Model Calibration**
   - Calibration of Heston parameters still relies on simplified approaches
   - Limited support for sophisticated volatility surface modeling
   - Some deep ITM/OTM options may produce unrealistic results

3. **Error Handling**
   - Error handling for FFTW library absence could be improved
   - Limited propagation of detailed error information to shell scripts

## TODOs and Future Enhancements

### Short-Term (High Priority)

1. **Caching Improvements**
   - [ ] Implement a more tolerant caching strategy with parameter sensitivity awareness
   - [ ] Create multi-level cache for different parameter sets
   - [ ] Add adaptive tolerance mechanism based on parameter sensitivity
   - [ ] Implement "close enough" parameter matching for cache reuse

2. **Error Handling**
   - [ ] Enhance error reporting and propagation from C programs to shell scripts
   - [ ] Add better fallback mechanisms when calculations fail
   - [ ] Improve numerical stability for extreme market conditions

### Medium-Term

1. **Model Enhancements**
   - [ ] Implement full Heston parameter calibration using market data
   - [ ] Add more sophisticated volatility surface modeling
   - [ ] Implement additional stochastic volatility models (SABR, etc.)
   - [ ] Support for American options pricing

2. **Performance Optimization**
   - [ ] Multi-threading support for parameter sweeps and calibration
   - [ ] GPU acceleration for FFT calculations (CUDA/OpenCL)
   - [ ] Cache persistence between program invocations

### Long-Term Vision

1. **Architecture Improvements**
   - [ ] Refactor to a shared library architecture
   - [ ] Create unified API for all pricing models
   - [ ] Implement language bindings for Python/R/etc.

2. **Feature Expansion**
   - [ ] Support for exotic options (barriers, Asians, etc.)
   - [ ] Real-time market data integration
   - [ ] Automated model selection based on option characteristics
   - [ ] Visualization of volatility surfaces and option Greeks

## Build and Usage

### Basic Usage

```bash
# Calculate option values using Black-Scholes (v2)
./price_option.sh UNDERLYING_PRICE STRIKE DAYS_TO_EXPIRY OPTION_PRICE UNDERLYING_YIELD

# Calculate option values with stochastic volatility model (v3)
./price_option_v3.sh UNDERLYING_PRICE STRIKE DAYS_TO_EXPIRY OPTION_PRICE UNDERLYING_YIELD v3
```

### Building FFT Implementation

```bash
# Requires FFTW3 library
./compile_sv_v3.sh
```

### Testing

```bash
# Run standard tests for stochastic volatility model
./test_sv_range.sh
```