# Unified Option Pricing System

A comprehensive system for pricing options using various models including Black-Scholes and Heston, with automatic market data integration.

## Features

- Support for multiple option pricing models:
  - Black-Scholes
  - Heston stochastic volatility model

- Multiple numerical methods:
  - Analytic solutions (for Black-Scholes)
  - Quadrature integration (for Heston)
  - Fast Fourier Transform (for Heston)

- Comprehensive market data integration:
  - Automatic price retrieval by ticker symbol
  - Dividend yield lookup
  - Historical volatility calculation
  - Historical price retrieval
  - Risk-free rate from Treasury yield curves

- Greek calculations:
  - Delta, Gamma, Theta, Vega, Rho

- Full shell script interface with multiple options
- Caching system for market data to improve performance
- Support for multiple data sources (Alpha Vantage, Finnhub, Polygon)

## Installation

### Prerequisites

- GCC compiler (version 7.0+)
- Make build system
- FFTW3 library for FFT calculations
- libcurl for API requests
- Jansson JSON library for API response parsing

### Quick Install

1. Clone the repository:
   ```
   git clone https://github.com/user/option_tools.git
   cd option_tools
   ```

2. Build the unified system:
   ```
   cd unified
   make -f Makefile.unified
   ```

3. Run tests to verify the installation:
   ```
   cd tests
   ./test_basic.sh
   ./test_market_data.sh
   ./test_ticker_option_pricing.sh
   ./test_historical_prices.sh
   ```

## Usage

### Basic Option Pricing

```bash
# Price a call option using Black-Scholes
./scripts/option_pricer.sh 100 105 30 0.05 0.01

# Price a put option using Heston with FFT
./scripts/option_pricer.sh -m heston -n fft -t put 100 95 60 0.05 0.02

# Calculate implied volatility for a call option with market price of 2.50
./scripts/option_pricer.sh 100 105 30 2.50 0.01
```

### Using Market Data Integration

```bash
# Price an AAPL option by automatically fetching the current price
./scripts/option_pricer.sh --ticker AAPL 0 185 45

# Price an SPX option with auto-volatility based on the option expiry
./scripts/option_pricer.sh --ticker SPX --auto-vol 4700 4750 90

# Calculate Greeks for an MSFT option
./scripts/option_pricer.sh --ticker MSFT --greeks 320 330 60

# Use a longer-term risk-free rate for LEAPS options
./scripts/option_pricer.sh --ticker SPX --rate-term 5year 4700 4800 365
```

### Fetching Market Data Directly

```bash
# Get current price for a ticker
./scripts/get_market_data.sh AAPL

# Get dividend yield
./scripts/get_market_data.sh --type dividend MSFT

# Get historical volatility for the past 60 days
./scripts/get_market_data.sh --type volatility --days 60 SPY

# Get the 10-year Treasury rate
./scripts/get_market_data.sh --type rate --rate-term 10year
```

### Retrieving Historical Prices

```bash
# Get 30 days of historical prices for SPX in CSV format
./scripts/get_historical_prices.sh SPX 30

# Get historical prices with volatility calculation
./scripts/get_historical_prices.sh --with-volatility AAPL 60

# Use a specific data source
./scripts/get_historical_prices.sh --source 1 MSFT 90

# Get data in JSON format
./scripts/get_historical_prices.sh --output json SPY 45

# Print detailed information
./scripts/get_historical_prices.sh --verbose AMZN 120
```

## API Keys

To use external market data sources, you need to obtain API keys:

1. Create a configuration file at `~/.config/option_tools/market_data.conf`
2. Add your API keys in the format:
   ```
   ALPHAVANTAGE_API_KEY=your_key_here
   FINNHUB_API_KEY=your_key_here
   POLYGON_API_KEY=your_key_here
   ```

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is licensed under the MIT License - see the LICENSE file for details. 