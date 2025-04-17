# Unified Option Pricing System - API Reference

## Core API Functions

### price_option

```c
int price_option(
    double spot_price,
    double strike_price,
    double time_to_expiry,
    double risk_free_rate,
    double dividend_yield,
    double volatility,
    OptionType option_type,
    ModelType model_type,
    NumericalMethod method,
    double market_price,
    GreeksFlags greeks_flags,
    const char* ticker_symbol,
    PricingResult* result
);
```

**Description:** Prices an option using the specified model and numerical method.

**Parameters:**
- `spot_price`: Current price of the underlying asset
- `strike_price`: Strike price of the option
- `time_to_expiry`: Time to expiry in years
- `risk_free_rate`: Risk-free interest rate (annualized)
- `dividend_yield`: Dividend yield (annualized)
- `volatility`: Implied volatility (if known, 0 to calculate)
- `option_type`: Type of option (`OPTION_CALL` or `OPTION_PUT`)
- `model_type`: Pricing model to use (`MODEL_BLACK_SCHOLES` or `MODEL_HESTON`)
- `method`: Numerical method (`METHOD_ANALYTIC`, `METHOD_QUADRATURE`, or `METHOD_FFT`)
- `market_price`: Market price (for implied volatility calculation, 0 to skip)
- `greeks_flags`: Flags indicating which Greeks to calculate
- `ticker_symbol`: Optional ticker symbol for market data retrieval (NULL to skip)
- `result`: Pointer to store the pricing result

**Return value:** 0 on success, error code on failure

**Example:**
```c
PricingResult result;
GreeksFlags greeks = {1, 1, 1, 1, 1}; // Calculate all Greeks
int ret = price_option(100.0, 105.0, 0.25, 0.05, 0.01, 0.0, 
                      OPTION_CALL, MODEL_BLACK_SCHOLES, METHOD_ANALYTIC,
                      2.45, greeks, "AAPL", &result);
```

### calculate_implied_volatility

```c
double calculate_implied_volatility(
    double market_price,
    double spot_price,
    double strike_price,
    double time_to_expiry,
    double risk_free_rate,
    double dividend_yield,
    OptionType option_type,
    ModelType model_type,
    NumericalMethod method
);
```

**Description:** Calculates the implied volatility for an option given its market price.

**Parameters:**
- `market_price`: The observed market price of the option
- `spot_price`: Current price of the underlying asset
- `strike_price`: Strike price of the option
- `time_to_expiry`: Time to expiry in years
- `risk_free_rate`: Risk-free interest rate (annualized)
- `dividend_yield`: Dividend yield (annualized)
- `option_type`: Type of option (`OPTION_CALL` or `OPTION_PUT`)
- `model_type`: Pricing model to use
- `method`: Numerical method to use

**Return value:** Implied volatility on success, negative value on failure

**Example:**
```c
double iv = calculate_implied_volatility(2.45, 100.0, 105.0, 0.25, 0.05, 0.01, 
                                        OPTION_CALL, MODEL_BLACK_SCHOLES, METHOD_ANALYTIC);
```

### calculate_greeks

```c
int calculate_greeks(
    double spot_price,
    double strike_price,
    double time_to_expiry,
    double risk_free_rate,
    double dividend_yield,
    double volatility,
    OptionType option_type,
    ModelType model_type,
    NumericalMethod method,
    GreeksFlags greeks_flags,
    PricingResult* result
);
```

**Description:** Calculates the Greeks for an option.

**Parameters:**
- `spot_price`: Current price of the underlying asset
- `strike_price`: Strike price of the option
- `time_to_expiry`: Time to expiry in years
- `risk_free_rate`: Risk-free interest rate (annualized)
- `dividend_yield`: Dividend yield (annualized)
- `volatility`: Implied volatility
- `option_type`: Type of option (`OPTION_CALL` or `OPTION_PUT`)
- `model_type`: Pricing model to use
- `method`: Numerical method to use
- `greeks_flags`: Flags indicating which Greeks to calculate
- `result`: Pointer to store the calculated Greeks

**Return value:** 0 on success, error code on failure

**Example:**
```c
PricingResult result;
GreeksFlags greeks = {1, 1, 0, 1, 0}; // Calculate delta, gamma, vega
int ret = calculate_greeks(100.0, 105.0, 0.25, 0.05, 0.01, 0.2, 
                          OPTION_CALL, MODEL_BLACK_SCHOLES, METHOD_ANALYTIC,
                          greeks, &result);
```

## Market Data API

### market_data_init

```c
int market_data_init(const char* config_path);
```

**Description:** Initializes the market data module.

**Parameters:**
- `config_path`: Path to configuration file containing API keys (NULL for default)

**Return value:** 0 on success, error code on failure

**Example:**
```c
int ret = market_data_init(NULL); // Use default config path
```

### market_data_cleanup

```c
void market_data_cleanup(void);
```

**Description:** Cleans up the market data module and frees resources.

**Example:**
```c
market_data_cleanup();
```

### get_current_price

```c
double get_current_price(const char* ticker, DataSource source, int* error_code);
```

**Description:** Gets the current price for a ticker symbol.

**Parameters:**
- `ticker`: The ticker symbol (e.g., "AAPL", "MSFT")
- `source`: Preferred data source
- `error_code`: Pointer to store error code (NULL to ignore)

**Return value:** Current price on success, negative value on failure

**Example:**
```c
int error_code;
double price = get_current_price("AAPL", DATA_SOURCE_DEFAULT, &error_code);
```

### get_dividend_yield

```c
double get_dividend_yield(const char* ticker, DataSource source, int* error_code);
```

**Description:** Gets the dividend yield for a ticker symbol.

**Parameters:**
- `ticker`: The ticker symbol
- `source`: Preferred data source
- `error_code`: Pointer to store error code (NULL to ignore)

**Return value:** Dividend yield on success, negative value on failure

**Example:**
```c
int error_code;
double yield = get_dividend_yield("MSFT", DATA_SOURCE_DEFAULT, &error_code);
```

### get_risk_free_rate

```c
double get_risk_free_rate(RateTerm term, int* error_code);
```

**Description:** Gets the risk-free rate for a specific term.

**Parameters:**
- `term`: Term length (`RATE_TERM_1M`, `RATE_TERM_3M`, etc.)
- `error_code`: Pointer to store error code (NULL to ignore)

**Return value:** Risk-free rate on success, negative value on failure

**Example:**
```c
int error_code;
double rate = get_risk_free_rate(RATE_TERM_10Y, &error_code);
```

### get_historical_volatility

```c
double get_historical_volatility(const char* ticker, int days, DataSource source, int* error_code);
```

**Description:** Calculates historical volatility for a ticker symbol.

**Parameters:**
- `ticker`: The ticker symbol
- `days`: Number of trading days to consider
- `source`: Preferred data source
- `error_code`: Pointer to store error code (NULL to ignore)

**Return value:** Historical volatility on success, negative value on failure

**Example:**
```c
int error_code;
double vol = get_historical_volatility("SPY", 30, DATA_SOURCE_DEFAULT, &error_code);
```

### get_historical_prices

```c
int get_historical_prices(const char* ticker, int days, DataSource source, 
                         double** prices, char*** dates, int* error_code);
```

**Description:** Gets historical prices for a ticker symbol.

**Parameters:**
- `ticker`: The ticker symbol
- `days`: Number of days of historical data to retrieve
- `source`: Preferred data source
- `prices`: Pointer to array where prices will be stored (caller must free)
- `dates`: Pointer to array where dates will be stored (caller must free)
- `error_code`: Pointer to store error code (NULL to ignore)

**Return value:** Number of data points retrieved, negative value on failure

**Example:**
```c
int error_code;
double* prices = NULL;
char** dates = NULL;
int count = get_historical_prices("AAPL", 60, DATA_SOURCE_DEFAULT, &prices, &dates, &error_code);

// Don't forget to free the memory when done
for (int i = 0; i < count; i++) {
    free(dates[i]);
}
free(dates);
free(prices);
```

## Data Structures

### OptionType

```c
typedef enum {
    OPTION_CALL = 0,    /**< Call option */
    OPTION_PUT = 1      /**< Put option */
} OptionType;
```

### ModelType

```c
typedef enum {
    MODEL_BLACK_SCHOLES = 0,   /**< Black-Scholes model */
    MODEL_HESTON = 1,          /**< Heston stochastic volatility model */
    MODEL_DEFAULT = MODEL_BLACK_SCHOLES  /**< Default model */
} ModelType;
```

### NumericalMethod

```c
typedef enum {
    METHOD_ANALYTIC = 0,    /**< Analytic solution (BS only) */
    METHOD_QUADRATURE = 1,  /**< Quadrature-based integration */
    METHOD_FFT = 2,         /**< Fast Fourier Transform */
    METHOD_DEFAULT = METHOD_ANALYTIC  /**< Default method */
} NumericalMethod;
```

### GreeksFlags

```c
typedef struct {
    unsigned int delta: 1;   /**< Calculate delta */
    unsigned int gamma: 1;   /**< Calculate gamma */
    unsigned int theta: 1;   /**< Calculate theta */
    unsigned int vega: 1;    /**< Calculate vega */
    unsigned int rho: 1;     /**< Calculate rho */
} GreeksFlags;
```

### PricingResult

```c
typedef struct {
    double price;              /**< Option price */
    double implied_volatility; /**< Implied volatility */
    double delta;              /**< Delta (1st derivative w.r.t. spot) */
    double gamma;              /**< Gamma (2nd derivative w.r.t. spot) */
    double theta;              /**< Theta (1st derivative w.r.t. time) */
    double vega;               /**< Vega (1st derivative w.r.t. volatility) */
    double rho;                /**< Rho (1st derivative w.r.t. interest rate) */
    int error_code;            /**< Error code (0 = success) */
} PricingResult;
```

### DataSource

```c
typedef enum {
    DATA_SOURCE_DEFAULT = 0,       /**< Use the default/preferred data source */
    DATA_SOURCE_ALPHAVANTAGE = 1,  /**< Alpha Vantage data source */
    DATA_SOURCE_FINNHUB = 2,       /**< Finnhub data source */
    DATA_SOURCE_POLYGON = 3        /**< Polygon data source */
} DataSource;
```

### RateTerm

```c
typedef enum {
    RATE_TERM_1M = 0,  /**< 1-month term */
    RATE_TERM_3M = 1,  /**< 3-month term */
    RATE_TERM_6M = 2,  /**< 6-month term */
    RATE_TERM_1Y = 3,  /**< 1-year term */
    RATE_TERM_2Y = 4,  /**< 2-year term */
    RATE_TERM_5Y = 5,  /**< 5-year term */
    RATE_TERM_10Y = 6, /**< 10-year term */
    RATE_TERM_30Y = 7  /**< 30-year term */
} RateTerm;
```

## Error Handling

The system uses standardized error codes throughout. Error codes are defined in `error_handling.h`. You can get an error description using:

```c
const char* get_error_description(int error_code);
```

Common error codes include:
- `ERROR_SUCCESS` (0): No error
- `ERROR_INVALID_PARAMETER` (-1): Invalid parameter
- `ERROR_MODULE_NOT_INITIALIZED` (-101): Market data module not initialized
- `ERROR_INVALID_TICKER` (-102): Invalid ticker symbol
- `ERROR_API_KEY_NOT_SET` (-104): Required API key not set

See `error_handling.h` for the complete list of error codes. 