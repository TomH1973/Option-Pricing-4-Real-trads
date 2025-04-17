#ifndef MARKET_DATA_H
#define MARKET_DATA_H

/**
 * @file market_data.h
 * @brief Market data retrieval interfaces for the unified option pricing system
 */

#include <time.h>

/**
 * @brief Enumeration for data sources
 */
typedef enum {
    DATA_SOURCE_DEFAULT = 0,       /**< Use the default/preferred data source */
    DATA_SOURCE_ALPHAVANTAGE = 1,  /**< Alpha Vantage data source */
    DATA_SOURCE_FINNHUB = 2,       /**< Finnhub data source */
    DATA_SOURCE_POLYGON = 3        /**< Polygon data source */
} DataSource;

/**
 * @brief Enumeration for rate terms
 */
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

/* Market data error codes (extends error_handling.h) */
#define ERROR_SUCCESS                  0    /**< No error */
#define ERROR_MODULE_NOT_INITIALIZED   -101 /**< Market data module not initialized */
#define ERROR_INVALID_TICKER           -102 /**< Invalid ticker symbol */
#define ERROR_INVALID_DATA_SOURCE      -103 /**< Invalid data source */
#define ERROR_API_KEY_NOT_SET          -104 /**< Required API key not set */
#define ERROR_API_REQUEST_FAILED       -105 /**< API request failed */
#define ERROR_PARSING_API_RESPONSE     -106 /**< Error parsing API response */
#define ERROR_INVALID_RATE_TERM        -107 /**< Invalid rate term */
#define ERROR_NOT_IMPLEMENTED          -108 /**< Feature not implemented */
#define ERROR_ENV_HOME_NOT_SET         -109 /**< HOME environment variable not set */
#define ERROR_INVALID_DAYS_PARAMETER   -110 /**< Invalid days parameter */
#define ERROR_RATE_NOT_AVAILABLE       -111 /**< Risk-free rate not available */

/**
 * @brief Initialize the market data module
 * @param config_path Path to configuration file containing API keys (NULL for default)
 * @return 0 on success, error code on failure
 */
int market_data_init(const char* config_path);

/**
 * @brief Clean up market data module and free resources
 */
void market_data_cleanup(void);

/**
 * @brief Get current price for a ticker symbol
 * @param ticker The ticker symbol (e.g., "AAPL", "MSFT")
 * @param source Preferred data source
 * @param error_code Pointer to store error code (NULL to ignore)
 * @return Current price on success, negative value on failure
 */
double get_current_price(const char* ticker, DataSource source, int* error_code);

/**
 * @brief Get historical prices for a ticker symbol
 * @param ticker The ticker symbol
 * @param days Number of days of historical data to retrieve
 * @param source Preferred data source
 * @param prices Pointer to array where prices will be stored (caller must free)
 * @param dates Pointer to array where dates will be stored (caller must free)
 * @param error_code Pointer to store error code (NULL to ignore)
 * @return Number of data points retrieved, negative value on failure
 */
int get_historical_prices(const char* ticker, int days, DataSource source, 
                         double** prices, char*** dates, int* error_code);

/**
 * @brief Get dividend yield for a ticker symbol
 * @param ticker The ticker symbol
 * @param source Preferred data source
 * @param error_code Pointer to store error code (NULL to ignore)
 * @return Dividend yield on success, negative value on failure
 */
double get_dividend_yield(const char* ticker, DataSource source, int* error_code);

/**
 * @brief Get risk-free rate for a specific term
 * @param term Term length
 * @param error_code Pointer to store error code (NULL to ignore)
 * @return Risk-free rate on success, negative value on failure
 */
double get_risk_free_rate(RateTerm term, int* error_code);

/**
 * @brief Calculate historical volatility for a ticker symbol
 * @param ticker The ticker symbol
 * @param days Number of trading days to consider
 * @param source Preferred data source
 * @param error_code Pointer to store error code (NULL to ignore)
 * @return Historical volatility on success, negative value on failure
 */
double get_historical_volatility(const char* ticker, int days, DataSource source, int* error_code);

/**
 * @brief Calculate historical volatility from time series data
 * @param json_data JSON data containing time series price information
 * @param period_days Number of days to use for calculation
 * @return Historical volatility on success, negative value on failure
 */
double calculate_historical_volatility_from_data(const char* json_data, int period_days);

/**
 * @brief Set the cache timeout for market data
 * @param seconds Timeout in seconds (0 to disable caching)
 */
void set_cache_timeout(int seconds);

/**
 * @brief Force refresh of cached market data
 * @param ticker The ticker symbol to refresh (NULL to refresh all)
 * @return 0 on success, error code on failure
 */
int refresh_cached_data(const char* ticker);

/**
 * @brief Get the appropriate historical volatility period based on option expiry
 * @param days_to_expiry Days until option expiration
 * @return Recommended lookback period in days
 */
int get_volatility_period_for_expiry(int days_to_expiry);

/**
 * @brief Set the preferred data source for market data retrieval
 * @param source The data source to use as default
 */
void set_preferred_data_source(DataSource source);

/**
 * @brief Set API key for a specific data source
 * @param source The data source to set the key for
 * @param api_key The API key to set
 * @return 0 on success, error code on failure
 */
int set_api_key(DataSource source, const char* api_key);

/**
 * @brief Get mapping for ticker symbols to standard market identifiers
 * @param ticker The ticker symbol to map
 * @return The mapped ticker or the original if no mapping exists
 */
const char* get_underlying_mapping(const char* ticker);

#endif /* MARKET_DATA_H */ 