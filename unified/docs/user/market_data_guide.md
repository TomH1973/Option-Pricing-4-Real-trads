# Market Data Integration Guide

This guide explains how to use the market data integration features of the Unified Option Pricing System.

## Overview

The market data integration allows you to:

- Automatically fetch current prices for stocks, ETFs, and indexes
- Retrieve dividend yields from financial data providers
- Calculate historical volatility based on past price movements
- Access Treasury yield curves for risk-free rates
- Cache data locally to improve performance
- Support multiple data sources for redundancy

## Supported Data Sources

The system currently supports the following data sources:

1. **Alpha Vantage**: Comprehensive financial data API with stocks, ETFs, and indexes
2. **Finnhub**: Real-time market data with global coverage
3. **Polygon.io**: Financial market data with real-time and historical data

## Configuration

### Setting Up API Keys

To use the market data features, you need to obtain API keys from the data providers:

1. Create a configuration file at `~/.config/option_tools/market_data.conf`
2. Add your API keys:

```
ALPHAVANTAGE_API_KEY=your_alphavantage_key
FINNHUB_API_KEY=your_finnhub_key
POLYGON_API_KEY=your_polygon_key
```

You can get free API keys from:
- [Alpha Vantage](https://www.alphavantage.co/)
- [Finnhub](https://finnhub.io/)
- [Polygon](https://polygon.io/)

### Configuration Options

Additional configurations you can set:

```
# Set preferred data source (1=Alpha Vantage, 2=Finnhub, 3=Polygon)
PREFERRED_DATA_SOURCE=1

# Cache timeout in seconds (default: 3600 = 1 hour)
CACHE_TIMEOUT=3600

# Path for cache storage
CACHE_DIR=~/.cache/option_tools
```

## Command-Line Usage

### Basic Market Data Retrieval

The `get_market_data.sh` script provides a command-line interface for market data:

```bash
./scripts/get_market_data.sh TICKER
```

This fetches the current price for the specified ticker.

### Specifying Data Types

You can request specific data types:

```bash
./scripts/get_market_data.sh --type [price|dividend|volatility|rate] TICKER
```

Examples:

```bash
# Get dividend yield for Microsoft
./scripts/get_market_data.sh --type dividend MSFT

# Get 30-day historical volatility for SPY
./scripts/get_market_data.sh --type volatility --days 30 SPY

# Get 10-year Treasury rate
./scripts/get_market_data.sh --type rate --rate-term 10year
```

### Specifying Data Sources

You can specify which data source to use:

```bash
./scripts/get_market_data.sh --source [0-3] TICKER
```

Where:
- 0: Default (uses the preferred source from configuration)
- 1: Alpha Vantage
- 2: Finnhub
- 3: Polygon

Example:

```bash
# Get AAPL price from Finnhub
./scripts/get_market_data.sh --source 2 AAPL
```

### Verbose Output

For more detailed information:

```bash
./scripts/get_market_data.sh --verbose TICKER
```

## Historical Price Data

### Retrieving Historical Prices

The `get_historical_prices.sh` script retrieves historical prices:

```bash
./scripts/get_historical_prices.sh TICKER DAYS
```

Example:

```bash
# Get 30 days of SPX history
./scripts/get_historical_prices.sh SPX 30
```

### Output Formats

You can specify the output format:

```bash
./scripts/get_historical_prices.sh --output [csv|json] TICKER DAYS
```

Example:

```bash
# Get AAPL history in JSON format
./scripts/get_historical_prices.sh --output json AAPL 60
```

### Volatility Calculation

To include historical volatility calculations:

```bash
./scripts/get_historical_prices.sh --with-volatility TICKER DAYS
```

## Integration with Option Pricing

### Ticker-Based Option Pricing

The main `option_pricer.sh` script can use market data:

```bash
./scripts/option_pricer.sh --ticker TICKER [SPOT_PRICE] STRIKE_PRICE DAYS_TO_EXPIRY [MARKET_PRICE] [DIVIDEND_YIELD]
```

When you use the `--ticker` option:
- The spot price can be 0 to automatically fetch the current price
- If omitted, dividend yield is retrieved from the data source
- The risk-free rate is fetched from Treasury yields based on the option expiry

Example:

```bash
# Price an AAPL option automatically fetching market data
./scripts/option_pricer.sh --ticker AAPL 0 180 45
```

### Using Historical Volatility

To use historical volatility instead of a fixed value:

```bash
./scripts/option_pricer.sh --ticker TICKER --auto-vol [SPOT_PRICE] STRIKE_PRICE DAYS_TO_EXPIRY
```

The system automatically calculates an appropriate historical volatility based on the option expiry length.

Example:

```bash
# Price an SPX option using historical volatility
./scripts/option_pricer.sh --ticker SPX --auto-vol 0 4750 60
```

### Risk-Free Rate Terms

You can specify which Treasury rate term to use:

```bash
./scripts/option_pricer.sh --ticker TICKER --rate-term [1month|3month|6month|1year|2year|5year|10year|30year] ...
```

Example:

```bash
# Use the 5-year rate for a long-dated option
./scripts/option_pricer.sh --ticker AAPL --rate-term 5year 0 190 365
```

## Supported Tickers

### Stocks and ETFs

The system supports most US-listed stocks and ETFs by their ticker symbols:

- AAPL (Apple)
- MSFT (Microsoft)
- AMZN (Amazon)
- GOOGL (Google/Alphabet)
- SPY (S&P 500 ETF)
- QQQ (Nasdaq-100 ETF)
- And thousands more...

### Market Indexes

Major market indexes are supported:

- SPX (S&P 500 Index)
- DJI (Dow Jones Industrial Average)
- IXIC (NASDAQ Composite Index)
- RUT (Russell 2000 Index)

## Caching System

The market data module includes a caching system to improve performance and reduce API calls:

- Data is cached locally for the timeout period specified in the configuration
- Cached data is used automatically when available and within the timeout
- You can force a refresh of cached data using the `--force-refresh` flag

Example:

```bash
# Force refresh of cached data
./scripts/get_market_data.sh --force-refresh AAPL
```

## Troubleshooting

### Common Errors

#### "Error: API key not set"

Make sure you've set up your API keys in the configuration file.

#### "Error: API request failed"

Network issue or API rate limit reached. Try using a different data source.

#### "Error: Invalid ticker symbol"

Check that the ticker symbol is valid and recognized by the data source.

#### "Error: Data not available"

The requested data type might not be available for the ticker (e.g., dividend data for an index).

### Debugging Tips

1. Use the `--verbose` flag for detailed information:
   ```bash
   ./scripts/get_market_data.sh --verbose AAPL
   ```

2. Check your API usage limits on the provider's website

3. Try a different data source if one is unavailable:
   ```bash
   ./scripts/get_market_data.sh --source 2 AAPL
   ```

4. Examine the cache files in the cache directory to see if data is being properly cached

## Programming with Market Data

### Initializing the Market Data Module

```c
#include "unified/include/market_data.h"

// Initialize with default config path
int ret = market_data_init(NULL);

// Or specify a custom config path
int ret = market_data_init("/path/to/config.conf");
```

### Getting Current Price

```c
int error_code;
double price = get_current_price("AAPL", DATA_SOURCE_DEFAULT, &error_code);
if (error_code == 0) {
    printf("Price: %.2f\n", price);
}
```

### Getting Dividend Yield

```c
int error_code;
double yield = get_dividend_yield("MSFT", DATA_SOURCE_DEFAULT, &error_code);
if (error_code == 0) {
    printf("Dividend Yield: %.2f%%\n", yield * 100.0);
}
```

### Getting Historical Volatility

```c
int error_code;
double vol = get_historical_volatility("SPY", 30, DATA_SOURCE_DEFAULT, &error_code);
if (error_code == 0) {
    printf("30-day Historical Vol: %.2f%%\n", vol * 100.0);
}
```

### Getting Risk-Free Rate

```c
int error_code;
double rate = get_risk_free_rate(RATE_TERM_10Y, &error_code);
if (error_code == 0) {
    printf("10-year Rate: %.2f%%\n", rate * 100.0);
}
```

### Getting Historical Prices

```c
int error_code;
double* prices = NULL;
char** dates = NULL;
int count = get_historical_prices("AAPL", 30, DATA_SOURCE_DEFAULT, 
                                 &prices, &dates, &error_code);

if (error_code == 0) {
    for (int i = 0; i < count; i++) {
        printf("%s: %.2f\n", dates[i], prices[i]);
        free(dates[i]);
    }
    free(dates);
    free(prices);
}
```

### Setting Cache Timeout

```c
// Set cache timeout to 30 minutes (1800 seconds)
set_cache_timeout(1800);
```

### Refreshing Cached Data

```c
// Refresh data for a specific ticker
refresh_cached_data("AAPL");

// Refresh all cached data
refresh_cached_data(NULL);
```

### Cleanup

Always clean up when done:

```c
market_data_cleanup();
``` 