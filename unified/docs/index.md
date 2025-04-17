# Unified Option Pricing System Documentation

Welcome to the documentation for the Unified Option Pricing System. This comprehensive documentation will help you understand, use, and extend the system.

## Documentation Structure

### User Documentation

- [**User Guide**](user/user_guide.md): Comprehensive guide for using the system
- [**Market Data Guide**](user/market_data_guide.md): Guide for using market data features
- [**Examples**](examples/basic_examples.md): Practical usage examples

### Technical Documentation

- [**API Reference**](api/api_reference.md): Detailed API documentation
- [**Developer Guide**](dev/developer_guide.md): Guide for extending the system

## Quick Start

To get started with the Unified Option Pricing System:

1. **Build the system**:
   ```bash
   cd unified
   make -f Makefile.unified
   ```

2. **Price an option**:
   ```bash
   ./scripts/option_pricer.sh 100 105 30 0.05 0.01
   ```

3. **Use market data**:
   ```bash
   ./scripts/option_pricer.sh --ticker AAPL 0 185 45
   ```

For more detailed examples, see the [Basic Examples](examples/basic_examples.md) page.

## Features Overview

The Unified Option Pricing System provides:

### Pricing Models
- Black-Scholes model for European options
- Heston stochastic volatility model
- Support for multiple numerical methods including FFT

### Market Data Integration
- Automatic retrieval of current prices
- Dividend yield data from financial data providers
- Historical volatility calculation
- Treasury yield curves for risk-free rates

### Option Analysis
- Greek calculations (delta, gamma, theta, vega, rho)
- Implied volatility calculation
- Volatility smile generation

### Command-Line Interface
- User-friendly shell scripts for all functionality
- Support for ticker-based option pricing
- Various customization options

## System Requirements

- Linux-based operating system
- GCC compiler (version 7.0+)
- Make build system
- FFTW3 library for FFT calculations
- libcurl for API requests
- Jansson JSON library for API response parsing

## Additional Resources

- [GitHub Repository](https://github.com/user/option_tools)
- [API Keys for Market Data](user/market_data_guide.md#setting-up-api-keys)
- [Contributing Guidelines](dev/developer_guide.md)

## License

This project is licensed under the MIT License - see the LICENSE file for details. 