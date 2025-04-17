# Basic Usage Examples - Unified Option Pricing System

This document provides practical examples for using the Unified Option Pricing System.

## Shell Script Examples

### Basic Option Pricing

Calculate the price of a call option with Black-Scholes:

```bash
# Format: ./option_pricer.sh SPOT_PRICE STRIKE_PRICE DAYS_TO_EXPIRY RISK_FREE_RATE DIVIDEND_YIELD
./scripts/option_pricer.sh 100 105 30 0.05 0.01
```

Example output:
```
Option Pricing Results
======================
Spot Price:      $100.00
Strike Price:    $105.00
Days to Expiry:  30
Risk-Free Rate:  5.00%
Dividend Yield:  1.00%
Option Type:     Call
Model:           Black-Scholes

Price:           $1.75
```

### Pricing a Put Option

Calculate the price of a put option:

```bash
./scripts/option_pricer.sh --type put 100 95 30 0.05 0.01
```

Example output:
```
Option Pricing Results
======================
Spot Price:      $100.00
Strike Price:    $95.00
Days to Expiry:  30
Risk-Free Rate:  5.00%
Dividend Yield:  1.00%
Option Type:     Put
Model:           Black-Scholes

Price:           $0.42
```

### Calculating Implied Volatility

Calculate implied volatility from a market price:

```bash
# Format: ./option_pricer.sh SPOT_PRICE STRIKE_PRICE DAYS_TO_EXPIRY MARKET_PRICE DIVIDEND_YIELD
./scripts/option_pricer.sh 100 105 30 2.45 0.01
```

Example output:
```
Option Pricing Results
======================
Spot Price:      $100.00
Strike Price:    $105.00
Days to Expiry:  30
Market Price:    $2.45
Dividend Yield:  1.00%
Option Type:     Call
Model:           Black-Scholes

Implied Volatility(BS): 30.25%
Price:           $2.45
```

### Using the Heston Model

Price an option using the Heston stochastic volatility model:

```bash
./scripts/option_pricer.sh --model heston 100 105 30 0.05 0.01
```

Example output:
```
Option Pricing Results
======================
Spot Price:      $100.00
Strike Price:    $105.00
Days to Expiry:  30
Risk-Free Rate:  5.00%
Dividend Yield:  1.00%
Option Type:     Call
Model:           Heston (Quadrature)

Price:           $1.83
```

### Using Fast Fourier Transform

Price an option using the Heston model with FFT for improved performance:

```bash
./scripts/option_pricer.sh --model heston --method fft 100 105 30 0.05 0.01
```

### Calculating Option Greeks

Calculate Greek values for risk management:

```bash
./scripts/option_pricer.sh --greeks 100 105 30 0.05 0.01
```

Example output:
```
Option Pricing Results
======================
Spot Price:      $100.00
Strike Price:    $105.00
Days to Expiry:  30
Risk-Free Rate:  5.00%
Dividend Yield:  1.00%
Option Type:     Call
Model:           Black-Scholes

Price:           $1.75
Greeks:
  Delta:         0.3485
  Gamma:         0.0421
  Theta:        -0.1527
  Vega:          0.1632
  Rho:           0.0284
```

### Using Ticker Symbols

Price an option for a specific stock by automatically fetching market data:

```bash
./scripts/option_pricer.sh --ticker AAPL 0 185 45
```

Example output:
```
Option Pricing Results
======================
Ticker Symbol:   AAPL
Spot Price:      $173.25 (fetched from market)
Strike Price:    $185.00
Days to Expiry:  45
Risk-Free Rate:  4.87% (3-month Treasury rate)
Dividend Yield:  0.54% (fetched from market)
Option Type:     Call
Model:           Black-Scholes

Price:           $4.92
```

### Auto-Volatility Calculation

Use historical volatility for the pricing:

```bash
./scripts/option_pricer.sh --ticker SPY --auto-vol 0 420 30
```

Example output:
```
Option Pricing Results
======================
Ticker Symbol:   SPY
Spot Price:      $417.62 (fetched from market)
Strike Price:    $420.00
Days to Expiry:  30
Risk-Free Rate:  4.87% (1-month Treasury rate)
Dividend Yield:  1.42% (fetched from market)
Historical Vol:  15.37% (calculated from 30-day history)
Option Type:     Call
Model:           Black-Scholes

Price:           $9.38
```

## Market Data Examples

### Getting Current Price

Fetch the current price for a ticker:

```bash
./scripts/get_market_data.sh AAPL
```

Example output:
```
Current Price for AAPL: $173.25
```

### Getting Dividend Yield

Fetch the dividend yield for a ticker:

```bash
./scripts/get_market_data.sh --type dividend MSFT
```

Example output:
```
Dividend Yield for MSFT: 0.87%
```

### Calculating Historical Volatility

Calculate historical volatility for a ticker:

```bash
./scripts/get_market_data.sh --type volatility --days 60 SPY
```

Example output:
```
60-day Historical Volatility for SPY: 15.37%
```

### Fetching Risk-Free Rates

Get the current Treasury rates:

```bash
./scripts/get_market_data.sh --type rate --rate-term 10year
```

Example output:
```
10-year Treasury Rate: 4.23%
```

## Historical Prices Examples

### Getting Historical Prices in CSV Format

Retrieve historical prices for a ticker:

```bash
./scripts/get_historical_prices.sh SPX 30
```

Example output (CSV format):
```
Date,Price
2023-04-20,4134.25
2023-04-19,4154.87
2023-04-18,4175.48
...
```

### Getting Historical Prices with Volatility

Retrieve historical prices and calculate volatility:

```bash
./scripts/get_historical_prices.sh --with-volatility AAPL 60
```

## Programming Examples

### Basic C Example

```c
#include <stdio.h>
#include "unified/include/option_pricing.h"
#include "unified/include/option_types.h"
#include "unified/include/market_data.h"

int main() {
    // Initialize market data module
    market_data_init(NULL);
    
    // Configure pricing parameters
    double spot_price = 100.0;
    double strike_price = 105.0;
    double time_to_expiry = 30.0 / 365.0;  // 30 days in years
    double risk_free_rate = 0.05;
    double dividend_yield = 0.01;
    double volatility = 0.2;  // 20% volatility
    
    // Set up Greeks calculation
    GreeksFlags greeks = {1, 1, 1, 1, 1};  // Calculate all Greeks
    
    // Pricing result structure
    PricingResult result;
    
    // Price the option
    int ret = price_option(
        spot_price,
        strike_price,
        time_to_expiry,
        risk_free_rate,
        dividend_yield,
        volatility,
        OPTION_CALL,
        MODEL_BLACK_SCHOLES,
        METHOD_ANALYTIC,
        0.0,  // No market price (not calculating IV)
        greeks,
        NULL,  // No ticker symbol
        &result
    );
    
    // Check for errors
    if (ret != 0) {
        printf("Error pricing option: %s\n", get_error_description(ret));
        return 1;
    }
    
    // Print results
    printf("Option Price: $%.2f\n", result.price);
    printf("Greeks:\n");
    printf("  Delta: %.4f\n", result.delta);
    printf("  Gamma: %.4f\n", result.gamma);
    printf("  Theta: %.4f\n", result.theta);
    printf("  Vega:  %.4f\n", result.vega);
    printf("  Rho:   %.4f\n", result.rho);
    
    // Clean up
    market_data_cleanup();
    
    return 0;
}
```

### Market Data C Example

```c
#include <stdio.h>
#include "unified/include/market_data.h"

int main() {
    int error_code;
    
    // Initialize market data module
    if (market_data_init(NULL) != 0) {
        printf("Failed to initialize market data module\n");
        return 1;
    }
    
    // Get current price
    double price = get_current_price("AAPL", DATA_SOURCE_DEFAULT, &error_code);
    if (error_code == 0) {
        printf("Current price for AAPL: $%.2f\n", price);
    } else {
        printf("Error getting price: %d\n", error_code);
    }
    
    // Get dividend yield
    double yield = get_dividend_yield("AAPL", DATA_SOURCE_DEFAULT, &error_code);
    if (error_code == 0) {
        printf("Dividend yield for AAPL: %.2f%%\n", yield * 100.0);
    } else {
        printf("Error getting dividend yield: %d\n", error_code);
    }
    
    // Get historical volatility
    double volatility = get_historical_volatility("AAPL", 30, DATA_SOURCE_DEFAULT, &error_code);
    if (error_code == 0) {
        printf("30-day historical volatility for AAPL: %.2f%%\n", volatility * 100.0);
    } else {
        printf("Error getting volatility: %d\n", error_code);
    }
    
    // Get risk-free rate
    double rate = get_risk_free_rate(RATE_TERM_10Y, &error_code);
    if (error_code == 0) {
        printf("10-year Treasury rate: %.2f%%\n", rate * 100.0);
    } else {
        printf("Error getting risk-free rate: %d\n", error_code);
    }
    
    // Get historical prices
    double* prices = NULL;
    char** dates = NULL;
    int count = get_historical_prices("AAPL", 10, DATA_SOURCE_DEFAULT, &prices, &dates, &error_code);
    
    if (error_code == 0 && count > 0) {
        printf("\nHistorical prices for AAPL (last 10 days):\n");
        for (int i = 0; i < count; i++) {
            printf("%s: $%.2f\n", dates[i], prices[i]);
            free(dates[i]);  // Free each date string
        }
        
        // Free allocated memory
        free(dates);
        free(prices);
    } else {
        printf("Error getting historical prices: %d\n", error_code);
    }
    
    // Clean up
    market_data_cleanup();
    
    return 0;
}
```

### Comprehensive Example with Ticker-Based Pricing

```c
#include <stdio.h>
#include "unified/include/option_pricing.h"
#include "unified/include/option_types.h"
#include "unified/include/market_data.h"

int main() {
    int error_code;
    
    // Initialize market data module
    if (market_data_init(NULL) != 0) {
        printf("Failed to initialize market data module\n");
        return 1;
    }
    
    // Define ticker and option parameters
    const char* ticker = "AAPL";
    double strike_price = 180.0;
    int days_to_expiry = 45;
    OptionType option_type = OPTION_CALL;
    
    // Get market data automatically
    double spot_price, dividend_yield;
    if (get_market_data(ticker, &spot_price, &dividend_yield) != 0) {
        printf("Failed to get market data for %s\n", ticker);
        market_data_cleanup();
        return 1;
    }
    
    printf("Retrieved market data for %s:\n", ticker);
    printf("  Current price: $%.2f\n", spot_price);
    printf("  Dividend yield: %.2f%%\n", dividend_yield * 100.0);
    
    // Get risk-free rate based on option expiry
    RateTerm rate_term = (days_to_expiry <= 30) ? RATE_TERM_1M :
                         (days_to_expiry <= 90) ? RATE_TERM_3M :
                         (days_to_expiry <= 180) ? RATE_TERM_6M : RATE_TERM_1Y;
    
    double risk_free_rate = get_risk_free_rate(rate_term, &error_code);
    if (error_code != 0) {
        printf("Failed to get risk-free rate\n");
        market_data_cleanup();
        return 1;
    }
    
    printf("  Risk-free rate: %.2f%%\n", risk_free_rate * 100.0);
    
    // Get historical volatility
    double historical_vol = get_historical_volatility(
        ticker,
        get_volatility_period_for_expiry(days_to_expiry),
        DATA_SOURCE_DEFAULT,
        &error_code
    );
    
    if (error_code != 0) {
        printf("Failed to calculate historical volatility\n");
        market_data_cleanup();
        return 1;
    }
    
    printf("  Historical volatility: %.2f%%\n", historical_vol * 100.0);
    
    // Set up Greeks calculation
    GreeksFlags greeks = {1, 1, 1, 1, 1};  // Calculate all Greeks
    
    // Pricing result structure
    PricingResult result;
    
    // Price the option using the Black-Scholes model
    int ret = price_option(
        spot_price,
        strike_price,
        days_to_expiry / 365.0,  // Convert days to years
        risk_free_rate,
        dividend_yield,
        historical_vol,
        option_type,
        MODEL_BLACK_SCHOLES,
        METHOD_ANALYTIC,
        0.0,  // No market price (not calculating IV)
        greeks,
        ticker,  // Provide ticker for any additional data needs
        &result
    );
    
    if (ret != 0) {
        printf("Error pricing option: %s\n", get_error_description(ret));
        market_data_cleanup();
        return 1;
    }
    
    // Print option pricing results
    printf("\nOption Pricing Results:\n");
    printf("======================\n");
    printf("Ticker:          %s\n", ticker);
    printf("Spot Price:      $%.2f\n", spot_price);
    printf("Strike Price:    $%.2f\n", strike_price);
    printf("Days to Expiry:  %d\n", days_to_expiry);
    printf("Option Type:     %s\n", option_type == OPTION_CALL ? "Call" : "Put");
    printf("\n");
    printf("Price:           $%.2f\n", result.price);
    printf("Greeks:\n");
    printf("  Delta:         %.4f\n", result.delta);
    printf("  Gamma:         %.4f\n", result.gamma);
    printf("  Theta:        %.4f\n", result.theta);
    printf("  Vega:          %.4f\n", result.vega);
    printf("  Rho:           %.4f\n", result.rho);
    
    // Clean up
    market_data_cleanup();
    
    return 0;
}
```

## Compilation and Execution

To compile the C examples:

```bash
gcc -o example example.c -I. -Lunified/bin -lunified_option_pricing -lm -lpthread -lfftw3 -lcurl -ljansson
```

To execute:

```bash
./example
``` 