# Unified Option Pricing System - User Guide

## Table of Contents

1. [Introduction](#introduction)
2. [Installation](#installation)
3. [Basic Usage](#basic-usage)
4. [Command-Line Options](#command-line-options)
5. [Working with Ticker Symbols](#working-with-ticker-symbols)
6. [Option Models](#option-models)
7. [Market Data Integration](#market-data-integration)
8. [Configuration](#configuration)
9. [Advanced Features](#advanced-features)
10. [Troubleshooting](#troubleshooting)
11. [FAQ](#faq)

## Introduction

The Unified Option Pricing System is a comprehensive tool for pricing options using various models, including Black-Scholes and the Heston stochastic volatility model. It provides:

- Multiple pricing models with different numerical methods
- Automatic market data retrieval by ticker symbol
- Greek calculations for risk management
- Support for different option types and expiry periods
- Historical volatility analysis
- Fast and accurate option pricing

This guide will help you get started with the system and show you how to use its features effectively.

## Installation

### Prerequisites

- Linux-based operating system
- GCC compiler (version 7.0+)
- Make build system
- FFTW3 library for FFT calculations
- libcurl for API requests
- Jansson JSON library for API response parsing

### Installation Steps

1. Clone the repository:
   ```bash
   git clone https://github.com/user/option_tools.git
   cd option_tools
   ```

2. Build the unified system:
   ```bash
   cd unified
   make -f Makefile.unified
   ```

3. Run tests to verify the installation:
   ```bash
   cd tests
   ./test_basic.sh
   ```

4. (Optional) Install the binaries system-wide:
   ```bash
   make -f Makefile.unified install
   ```

## Basic Usage

The simplest way to use the system is through the `option_pricer.sh` script:

```bash
./scripts/option_pricer.sh 100 105 30 0.05 0.01
```

This command prices a call option with:
- Spot price: 100
- Strike price: 105
- Days to expiry: 30
- Risk-free rate: 0.05 (5%)
- Dividend yield: 0.01 (1%)

The output will include the option price and, if applicable, implied volatility.

### Pricing a Put Option

```bash
./scripts/option_pricer.sh -t put 100 95 30 0.05 0.01
```

### Calculating Implied Volatility

If you provide a market price, the system will calculate the implied volatility:

```bash
./scripts/option_pricer.sh 100 105 30 2.45 0.01
```

This calculates the implied volatility for a call option with a market price of 2.45.

## Command-Line Options

The `option_pricer.sh` script supports many command-line options:

| Option | Description | Example |
|--------|-------------|---------|
| `-t, --type` | Option type (call or put) | `--type put` |
| `-m, --model` | Pricing model (bs or heston) | `--model heston` |
| `-n, --method` | Numerical method (analytic, quad, fft) | `--method fft` |
| `--ticker` | Ticker symbol for market data | `--ticker AAPL` |
| `--greeks` | Calculate Greeks | `--greeks` |
| `-r, --rate` | Risk-free rate (if not using ticker) | `--rate 0.05` |
| `--vol` | Initial volatility | `--vol 0.2` |
| `--auto-vol` | Use historical volatility | `--auto-vol` |
| `--rate-term` | Term for risk-free rate | `--rate-term 10year` |
| `-v, --verbose` | Show detailed output | `--verbose` |
| `-h, --help` | Show help message | `--help` |

### Full Example

```bash
./scripts/option_pricer.sh --ticker SPX --model heston --method fft --type put --greeks --rate-term 5year --auto-vol 4700 4750 90
```

This prices an SPX put option using the Heston model with FFT, calculating Greeks, using the 5-year treasury rate, and calculating historical volatility based on 90 days to expiry.

## Working with Ticker Symbols

The system can automatically fetch market data using ticker symbols:

```bash
./scripts/option_pricer.sh --ticker AAPL 0 185 45
```

When you use the `--ticker` option:
- Spot price (first positional argument) can be 0 to automatically fetch the current price
- The risk-free rate is fetched from Treasury yields
- Dividend yield is retrieved from financial data services
- With `--auto-vol`, historical volatility is calculated

### Supported Market Indexes

You can use indexes like:
- SPX (S&P 500)
- DJI (Dow Jones Industrial Average)
- IXIC (NASDAQ Composite)
- RUT (Russell 2000)

For example:
```bash
./scripts/option_pricer.sh --ticker SPX 0 4750 30
```

## Option Models

### Black-Scholes Model

The default model is Black-Scholes, which is fast and suitable for simple European options. Use it with:

```bash
./scripts/option_pricer.sh --model bs 100 105 30 0.05 0.01
```

### Heston Stochastic Volatility Model

The Heston model accounts for stochastic volatility and is better for capturing market characteristics like volatility smiles:

```bash
./scripts/option_pricer.sh --model heston 100 105 30 0.05 0.01
```

### Numerical Methods

For the Heston model, you can choose different numerical methods:

- **Quadrature** (default for Heston):
  ```bash
  ./scripts/option_pricer.sh --model heston --method quad 100 105 30 0.05 0.01
  ```

- **Fast Fourier Transform** (faster for multiple strikes):
  ```bash
  ./scripts/option_pricer.sh --model heston --method fft 100 105 30 0.05 0.01
  ```

## Market Data Integration

### Fetching Current Prices

To get the current price for a ticker:

```bash
./scripts/get_market_data.sh AAPL
```

### Getting Dividend Yields

To get the dividend yield for a ticker:

```bash
./scripts/get_market_data.sh --type dividend MSFT
```

### Historical Volatility

To calculate historical volatility:

```bash
./scripts/get_market_data.sh --type volatility --days 60 SPY
```

### Risk-Free Rates

To get Treasury rates:

```bash
./scripts/get_market_data.sh --type rate --rate-term 10year
```

### Historical Prices

To retrieve historical prices:

```bash
./scripts/get_historical_prices.sh SPX 30
```

This retrieves 30 days of historical prices for SPX in CSV format.

## Configuration

### API Keys

To use external market data sources, you need to obtain API keys and add them to your configuration:

1. Create a configuration file at `~/.config/option_tools/market_data.conf`
2. Add your API keys:
   ```
   ALPHAVANTAGE_API_KEY=your_key_here
   FINNHUB_API_KEY=your_key_here
   POLYGON_API_KEY=your_key_here
   ```

You can get free API keys from:
- [Alpha Vantage](https://www.alphavantage.co/)
- [Finnhub](https://finnhub.io/)
- [Polygon](https://polygon.io/)

### Preferred Data Source

You can set your preferred data source in the configuration file:

```
PREFERRED_DATA_SOURCE=1  # 1=Alpha Vantage, 2=Finnhub, 3=Polygon
```

### Cache Settings

Market data is cached to improve performance. You can adjust the cache timeout:

```
CACHE_TIMEOUT=3600  # Cache timeout in seconds (default: 1 hour)
```

## Advanced Features

### Calculating Greeks

To calculate Greeks (delta, gamma, theta, vega, rho):

```bash
./scripts/option_pricer.sh --greeks 100 105 30 0.05 0.01
```

### Generating Volatility Smiles

You can use the `generate_smile.sh` script to create volatility smiles across multiple strikes:

```bash
./generate_smile.sh
```

This will output a CSV file with strikes and corresponding implied volatilities.

### Comparing Models

To compare different model versions:

```bash
./compare_models.sh
```

## Troubleshooting

### Common Errors

#### "Error: Invalid parameter"
Check that your input values are valid numbers. Spot and strike prices must be positive, and time to expiry must be greater than zero.

#### "Error: API key not set"
Ensure you have set up your API keys in the configuration file.

#### "Error: Binary not found"
Make sure you have built the system with `make -f Makefile.unified`.

#### "Error: API request failed"
Check your internet connection. API services may also have rate limits.

### Debugging

Use the `--verbose` flag to get more information:

```bash
./scripts/option_pricer.sh --verbose 100 105 30 0.05 0.01
```

## FAQ

### Q: Which model should I use?
**A:** For simple European options, Black-Scholes is fast and accurate. For capturing market dynamics like volatility smiles or long-dated options, use the Heston model.

### Q: How accurate is the market data?
**A:** The system uses real-time or delayed market data from reputable providers. Data may be delayed by 15-20 minutes for free API tiers.

### Q: Can I use the system for American options?
**A:** Currently, the system is designed for European options. American options with early exercise features would require additional models.

### Q: How can I contribute to the project?
**A:** Contributions are welcome! See our GitHub repository for contribution guidelines.

### Q: Is the system suitable for professional use?
**A:** The system provides robust pricing capabilities but should be validated against market data before professional use. Always perform your own due diligence. 