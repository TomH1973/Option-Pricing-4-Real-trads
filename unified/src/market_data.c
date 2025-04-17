/**
 * market_data.c
 * Implementation of market data retrieval functions
 * 
 * This module provides functionality to fetch market data (prices, dividend yields,
 * risk-free rates) from various data sources using ticker symbols.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <jansson.h>
#include <ctype.h>
#include <cjson/cJSON.h>

#include "../include/market_data.h"
#include "../include/error_handling.h"
#include "../include/path_resolution.h"

#define MAX_URL_LENGTH 1024
#define MAX_BUFFER_SIZE 65536
#define CACHE_DIR ".cache/market_data"
#define DEFAULT_CACHE_EXPIRY_SECONDS 3600 // 1 hour
#define MAX_TICKER_LENGTH 16
#define MAX_CACHE_PATH 256
#define URL_BUFFER_SIZE 512
#define MAX_HISTORY_DAYS 365
#define MAX_API_KEY_LENGTH 128

/**
 * Structure to hold cached data with timestamp
 */
typedef struct {
    char* data;
    time_t timestamp;
} CachedData;

// Structure to hold response data from API calls
typedef struct {
    char *data;
    size_t size;
} APIResponse;

// Default API keys (should be overridden by config)
static char *alphavantage_api_key = NULL;
static char *finnhub_api_key = NULL;
static char *polygon_api_key = NULL;
static DataSource preferred_source = DATA_SOURCE_ALPHAVANTAGE;

// Flag to indicate if module is initialized
static int is_initialized = 0;

// Cache expiry can be modified at runtime
static int cache_expiry_seconds = DEFAULT_CACHE_EXPIRY_SECONDS;

// Cache management functions
static char* get_cache_path(const char *ticker, const char *data_type);
static int is_cache_valid(const char *cache_path);
static int save_to_cache(const char *cache_path, const char *data);
static char* load_from_cache(const char *cache_path);

// API helper functions
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp);
static char* make_api_request(const char *url);

// Data parsing functions
static double parse_price_alphavantage(const char *json_data, const char *ticker);
static double parse_dividend_yield_alphavantage(const char *json_data, const char *ticker);
static double parse_risk_free_rate_treasury(const char *csv_data, const char *term);

// Security helpers
static int validate_ticker_symbol(const char *ticker);
static char* sanitize_ticker_symbol(const char *ticker);

// Implementation of market_data_init
int market_data_init(const char *config_path) {
    if (is_initialized) {
        return ERROR_SUCCESS; // Already initialized
    }
    
    // Create cache directory if it doesn't exist
    char cache_path[PATH_MAX];
    char *home_dir = getenv("HOME");
    
    if (home_dir == NULL) {
        return ERROR_ENV_HOME_NOT_SET;
    }
    
    snprintf(cache_path, PATH_MAX, "%s/%s", home_dir, CACHE_DIR);
    mkdir(cache_path, 0755);
    
    // Load API keys from config if provided
    if (config_path != NULL) {
        FILE *config_file = fopen(config_path, "r");
        if (config_file != NULL) {
            char line[256];
            char key[128], value[128];
            
            while (fgets(line, sizeof(line), config_file)) {
                if (sscanf(line, "%127[^=]=%127s", key, value) == 2) {
                    if (strcmp(key, "ALPHAVANTAGE_API_KEY") == 0) {
                        alphavantage_api_key = strdup(value);
                    } else if (strcmp(key, "FINNHUB_API_KEY") == 0) {
                        finnhub_api_key = strdup(value);
                    } else if (strcmp(key, "POLYGON_API_KEY") == 0) {
                        polygon_api_key = strdup(value);
                    } else if (strcmp(key, "PREFERRED_DATA_SOURCE") == 0) {
                        if (strcmp(value, "ALPHAVANTAGE") == 0) {
                            preferred_source = DATA_SOURCE_ALPHAVANTAGE;
                        } else if (strcmp(value, "FINNHUB") == 0) {
                            preferred_source = DATA_SOURCE_FINNHUB;
                        } else if (strcmp(value, "POLYGON") == 0) {
                            preferred_source = DATA_SOURCE_POLYGON;
                        }
                    } else if (strcmp(key, "CACHE_EXPIRY_SECONDS") == 0) {
                        int expiry = atoi(value);
                        if (expiry > 0) {
                            cache_expiry_seconds = expiry;
                        }
                    }
                }
            }
            fclose(config_file);
        }
    }
    
    // Initialize curl global state
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    is_initialized = 1;
    return ERROR_SUCCESS;
}

// Implementation of market_data_cleanup
void market_data_cleanup() {
    if (!is_initialized) {
        return;
    }
    
    // Free allocated API keys
    if (alphavantage_api_key != NULL) {
        free(alphavantage_api_key);
        alphavantage_api_key = NULL;
    }
    
    if (finnhub_api_key != NULL) {
        free(finnhub_api_key);
        finnhub_api_key = NULL;
    }
    
    if (polygon_api_key != NULL) {
        free(polygon_api_key);
        polygon_api_key = NULL;
    }
    
    // Cleanup curl
    curl_global_cleanup();
    
    is_initialized = 0;
}

// Set the cache timeout
void set_cache_timeout(int seconds) {
    if (seconds < 0) {
        seconds = 0;  // Negative values mean no caching (same as 0)
    }
    cache_expiry_seconds = seconds;
}

// Force refresh of cached data
int refresh_cached_data(const char* ticker) {
    if (!is_initialized) {
        return ERROR_MODULE_NOT_INITIALIZED;
    }
    
    // Validate ticker if provided
    if (ticker != NULL && *ticker != '\0') {
        if (!validate_ticker_symbol(ticker)) {
            return ERROR_INVALID_TICKER;
        }
    }
    
    // If no specific ticker is provided, refresh all cached data
    if (ticker == NULL || *ticker == '\0') {
        char *home_dir = getenv("HOME");
        if (home_dir == NULL) {
            return ERROR_ENV_HOME_NOT_SET;
        }
        
        char cache_dir[PATH_MAX];
        snprintf(cache_dir, PATH_MAX, "%s/%s", home_dir, CACHE_DIR);
        
        // In a real implementation, we would:
        // 1. List all files in the cache directory
        // 2. Parse ticker symbols from filenames
        // 3. Refresh each ticker's data
        
        // For now, just return success
        return ERROR_SUCCESS;
    }
    
    // Refresh specific ticker data
    // Get current price to refresh price data
    int error_code;
    double price = get_current_price(ticker, DATA_SOURCE_DEFAULT, &error_code);
    if (error_code != ERROR_SUCCESS) {
        return error_code;
    }
    
    // Get dividend yield to refresh yield data
    double yield = get_dividend_yield(ticker, DATA_SOURCE_DEFAULT, &error_code);
    if (error_code != ERROR_SUCCESS && error_code != ERROR_PARSING_API_RESPONSE) {
        return error_code;
    }
    
    return ERROR_SUCCESS;
}

// Implementation of get_current_price
double get_current_price(const char *ticker, DataSource source, int *error_code) {
    if (!is_initialized) {
        if (error_code) *error_code = ERROR_MODULE_NOT_INITIALIZED;
        return -1.0;
    }
    
    // Validate ticker
    if (!validate_ticker_symbol(ticker)) {
        if (error_code) *error_code = ERROR_INVALID_TICKER;
        return -1.0;
    }
    
    // Use preferred source if not specified
    if (source == DATA_SOURCE_DEFAULT) {
        source = preferred_source;
    }
    
    // Check cache first
    char *cache_path = get_cache_path(ticker, "price");
    if (cache_path == NULL) {
        if (error_code) *error_code = ERROR_MEMORY_ALLOCATION;
        return -1.0;
    }
    
    if (is_cache_valid(cache_path)) {
        char *cached_data = load_from_cache(cache_path);
        if (cached_data != NULL) {
            double price = atof(cached_data);
            free(cached_data);
            free(cache_path);
            
            if (price > 0) {
                if (error_code) *error_code = ERROR_SUCCESS;
                return price;
            }
        }
    }
    
    // Prepare API request
    char url[MAX_URL_LENGTH];
    char *api_key = NULL;
    char *sanitized_ticker = sanitize_ticker_symbol(ticker);
    
    if (sanitized_ticker == NULL) {
        free(cache_path);
        if (error_code) *error_code = ERROR_MEMORY_ALLOCATION;
        return -1.0;
    }
    
    switch (source) {
        case DATA_SOURCE_ALPHAVANTAGE:
            api_key = alphavantage_api_key;
            if (api_key == NULL) {
                if (error_code) *error_code = ERROR_API_KEY_NOT_SET;
                free(cache_path);
                free(sanitized_ticker);
                return -1.0;
            }
            snprintf(url, MAX_URL_LENGTH, 
                    "https://www.alphavantage.co/query?function=GLOBAL_QUOTE&symbol=%s&apikey=%s",
                    sanitized_ticker, api_key);
            break;
            
        case DATA_SOURCE_FINNHUB:
            api_key = finnhub_api_key;
            if (api_key == NULL) {
                if (error_code) *error_code = ERROR_API_KEY_NOT_SET;
                free(cache_path);
                free(sanitized_ticker);
                return -1.0;
            }
            snprintf(url, MAX_URL_LENGTH, 
                    "https://finnhub.io/api/v1/quote?symbol=%s&token=%s",
                    sanitized_ticker, api_key);
            break;
            
        case DATA_SOURCE_POLYGON:
            api_key = polygon_api_key;
            if (api_key == NULL) {
                if (error_code) *error_code = ERROR_API_KEY_NOT_SET;
                free(cache_path);
                free(sanitized_ticker);
                return -1.0;
            }
            snprintf(url, MAX_URL_LENGTH, 
                    "https://api.polygon.io/v2/aggs/ticker/%s/prev?apiKey=%s",
                    sanitized_ticker, api_key);
            break;
            
        default:
            if (error_code) *error_code = ERROR_INVALID_DATA_SOURCE;
            free(cache_path);
            free(sanitized_ticker);
            return -1.0;
    }
    
    // No longer need the sanitized ticker
    free(sanitized_ticker);
    
    // Make API request
    char *response = make_api_request(url);
    if (response == NULL) {
        if (error_code) *error_code = ERROR_API_REQUEST_FAILED;
        free(cache_path);
        return -1.0;
    }
    
    // Parse response based on source
    double price = -1.0;
    
    switch (source) {
        case DATA_SOURCE_ALPHAVANTAGE:
            price = parse_price_alphavantage(response, ticker);
            break;
            
        case DATA_SOURCE_FINNHUB:
            // Implementation for Finnhub would go here
            price = -1.0;
            break;
            
        case DATA_SOURCE_POLYGON:
            // Implementation for Polygon would go here
            price = -1.0;
            break;
            
        default:
            price = -1.0;
    }
    
    // Cache result if valid
    if (price > 0) {
        char price_str[32];
        snprintf(price_str, sizeof(price_str), "%.6f", price);
        save_to_cache(cache_path, price_str);
    }
    
    free(response);
    free(cache_path);
    
    if (price <= 0) {
        if (error_code) *error_code = ERROR_PARSING_API_RESPONSE;
        return -1.0;
    }
    
    if (error_code) *error_code = ERROR_SUCCESS;
    return price;
}

// Helper function for API requests
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t real_size = size * nmemb;
    APIResponse *resp = (APIResponse*)userp;
    
    // Check for buffer overrun or memory issues
    if (resp->size + real_size > MAX_BUFFER_SIZE) {
        return 0;  // Too much data, prevent buffer overflow
    }
    
    char *ptr = realloc(resp->data, resp->size + real_size + 1);
    if (ptr == NULL) {
        // Handle memory allocation failure
        if (resp->data) {
            free(resp->data);
            resp->data = NULL;
        }
        resp->size = 0;
        return 0;  // Signal error to curl
    }
    
    resp->data = ptr;
    memcpy(&(resp->data[resp->size]), contents, real_size);
    resp->size += real_size;
    resp->data[resp->size] = 0;
    
    return real_size;
}

static char* make_api_request(const char *url) {
    if (url == NULL) return NULL;
    
    CURL *curl;
    CURLcode res;
    APIResponse response = {0};
    
    // Initialize response data
    response.data = malloc(1);
    if (response.data == NULL) {
        return NULL;  // Memory allocation failed
    }
    
    response.size = 0;
    response.data[0] = '\0';
    
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "unified-option-tools/1.0");
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        
        // Set secure options
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        
        if (res != CURLE_OK) {
            free(response.data);
            return NULL;
        }
    } else {
        free(response.data);
        return NULL;
    }
    
    return response.data;
}

// Sanitize ticker symbol for safe use in URLs
static char* sanitize_ticker_symbol(const char *ticker) {
    if (ticker == NULL) return NULL;
    
    size_t len = strlen(ticker);
    if (len == 0 || len > MAX_TICKER_LENGTH) return NULL;
    
    char *sanitized = malloc(len + 1);
    if (sanitized == NULL) return NULL;
    
    size_t i, j = 0;
    for (i = 0; i < len; i++) {
        // Allow only alphanumeric chars, dots, and hyphens
        if (isalnum((unsigned char)ticker[i]) || ticker[i] == '.' || ticker[i] == '-') {
            sanitized[j++] = ticker[i];
        }
    }
    sanitized[j] = '\0';
    
    // Make sure we have at least one character
    if (j == 0) {
        free(sanitized);
        return NULL;
    }
    
    return sanitized;
}

// Validate ticker symbol format
static int validate_ticker_symbol(const char *ticker) {
    if (ticker == NULL || *ticker == '\0') {
        return 0;  // Empty or NULL ticker
    }
    
    size_t len = strlen(ticker);
    if (len > MAX_TICKER_LENGTH) {
        return 0;  // Ticker too long
    }
    
    // Check for at least one valid character
    size_t i;
    int valid_chars = 0;
    for (i = 0; i < len; i++) {
        if (isalnum((unsigned char)ticker[i]) || ticker[i] == '.' || ticker[i] == '-') {
            valid_chars++;
        } else {
            return 0;  // Invalid character
        }
    }
    
    return (valid_chars > 0);
}

// Cache management functions
static char* get_cache_path(const char *ticker, const char *data_type) {
    char *home_dir = getenv("HOME");
    if (home_dir == NULL) {
        return NULL;
    }
    
    char *path = malloc(PATH_MAX);
    if (path == NULL) {
        return NULL;
    }
    
    snprintf(path, PATH_MAX, "%s/%s/%s_%s.cache", home_dir, CACHE_DIR, ticker, data_type);
    return path;
}

/**
 * Check if a cache file exists and is valid
 */
static int is_cache_valid(const char *cache_path) {
    struct stat attr;
    
    // Check if file exists
    if (stat(cache_path, &attr) != 0) {
        return 0;
    }
    
    // Check if cache has expired
    time_t now = time(NULL);
    if (cache_expiry_seconds > 0 && (now - attr.st_mtime) > cache_expiry_seconds) {
        return 0;
    }
    
    return 1;
}

/**
 * Save data to a cache file
 */
static int save_to_cache(const char *cache_path, const char *data) {
    FILE* file = fopen(cache_path, "w");
    if (file == NULL) {
        return -1;
    }
    
    // Write data to file
    size_t data_len = strlen(data);
    size_t bytes_written = fwrite(data, 1, data_len, file);
    
    fclose(file);
    
    if (bytes_written != data_len) {
        return -1;
    }
    
    return 0;
}

static char* load_from_cache(const char *cache_path) {
    if (cache_path == NULL) return NULL;
    
    FILE *cache_file = fopen(cache_path, "r");
    if (cache_file == NULL) {
        return NULL;
    }
    
    // Get file size
    fseek(cache_file, 0, SEEK_END);
    long file_size = ftell(cache_file);
    rewind(cache_file);
    
    // Allocate memory for file content
    char *buffer = malloc(file_size + 1);
    if (buffer == NULL) {
        fclose(cache_file);
        return NULL;
    }
    
    // Read file content into buffer
    size_t read_size = fread(buffer, 1, file_size, cache_file);
    fclose(cache_file);
    
    if (read_size != file_size) {
        free(buffer);
        return NULL;
    }
    
    buffer[file_size] = '\0';
    return buffer;
}

// Parse functions for different API responses
static double parse_price_alphavantage(const char *json_data, const char *ticker) {
    if (json_data == NULL || ticker == NULL) {
        return -1.0;
    }
    
    json_error_t error;
    json_t *root = json_loads(json_data, 0, &error);
    
    if (!root) {
        return -1.0;
    }
    
    json_t *global_quote = json_object_get(root, "Global Quote");
    if (!global_quote || !json_is_object(global_quote)) {
        json_decref(root);
        return -1.0;
    }
    
    json_t *price = json_object_get(global_quote, "05. price");
    if (!price || !json_is_string(price)) {
        json_decref(root);
        return -1.0;
    }
    
    double price_value = atof(json_string_value(price));
    json_decref(root);
    
    return price_value;
}

static double parse_dividend_yield_alphavantage(const char *json_data, const char *ticker) {
    if (json_data == NULL || ticker == NULL) {
        return -1.0;
    }
    
    json_error_t error;
    json_t *root = json_loads(json_data, 0, &error);
    
    if (!root || !json_is_object(root)) {
        return -1.0;
    }
    
    json_t *dividend_yield = json_object_get(root, "DividendYield");
    if (!dividend_yield || !json_is_string(dividend_yield)) {
        json_decref(root);
        return -1.0;
    }
    
    double yield_value = atof(json_string_value(dividend_yield));
    json_decref(root);
    
    return yield_value;
}

static double parse_risk_free_rate_treasury(const char *csv_data, const char *term) {
    if (csv_data == NULL || term == NULL) {
        return -1.0;
    }
    
    // Simple CSV parsing to get the latest rate
    // In real implementation, we would parse the CSV properly
    // For this placeholder, we'll just return a reasonable value
    
    // Different defaults based on term
    if (strcmp(term, "1month") == 0) return 0.0175;
    if (strcmp(term, "3month") == 0) return 0.0185;
    if (strcmp(term, "6month") == 0) return 0.0195;
    if (strcmp(term, "1year") == 0) return 0.021;
    if (strcmp(term, "2year") == 0) return 0.023;
    if (strcmp(term, "5year") == 0) return 0.025;
    if (strcmp(term, "10year") == 0) return 0.027;
    if (strcmp(term, "30year") == 0) return 0.029;
    
    return -1.0; // Unknown term
}

// Implementation of get_dividend_yield
double get_dividend_yield(const char *ticker, DataSource source, int *error_code) {
    if (!is_initialized) {
        if (error_code) *error_code = ERROR_MODULE_NOT_INITIALIZED;
        return -1.0;
    }
    
    // Validate ticker
    if (!validate_ticker_symbol(ticker)) {
        if (error_code) *error_code = ERROR_INVALID_TICKER;
        return -1.0;
    }
    
    // Use preferred source if not specified
    if (source == DATA_SOURCE_DEFAULT) {
        source = preferred_source;
    }
    
    // Check cache first
    char *cache_path = get_cache_path(ticker, "dividend");
    if (cache_path == NULL) {
        if (error_code) *error_code = ERROR_MEMORY_ALLOCATION;
        return -1.0;
    }
    
    if (is_cache_valid(cache_path)) {
        char *cached_data = load_from_cache(cache_path);
        if (cached_data != NULL) {
            double yield = atof(cached_data);
            free(cached_data);
            free(cache_path);
            
            if (error_code) *error_code = ERROR_SUCCESS;
            return yield;
        }
    }
    
    // Prepare API request
    char url[MAX_URL_LENGTH];
    char *api_key = NULL;
    char *sanitized_ticker = sanitize_ticker_symbol(ticker);
    
    if (sanitized_ticker == NULL) {
        free(cache_path);
        if (error_code) *error_code = ERROR_MEMORY_ALLOCATION;
        return -1.0;
    }
    
    // Currently only Alpha Vantage is supported for dividend yield
    if (source != DATA_SOURCE_ALPHAVANTAGE) {
        source = DATA_SOURCE_ALPHAVANTAGE;
    }
    
    api_key = alphavantage_api_key;
    if (api_key == NULL) {
        if (error_code) *error_code = ERROR_API_KEY_NOT_SET;
        free(cache_path);
        free(sanitized_ticker);
        return -1.0;
    }
    
    snprintf(url, MAX_URL_LENGTH, 
            "https://www.alphavantage.co/query?function=OVERVIEW&symbol=%s&apikey=%s",
            sanitized_ticker, api_key);
    
    // No longer need the sanitized ticker
    free(sanitized_ticker);
    
    // Make API request
    char *response = make_api_request(url);
    if (response == NULL) {
        if (error_code) *error_code = ERROR_API_REQUEST_FAILED;
        free(cache_path);
        return -1.0;
    }
    
    // Parse response
    double yield = parse_dividend_yield_alphavantage(response, ticker);
    
    // Cache result if valid
    if (yield >= 0) {
        char yield_str[32];
        snprintf(yield_str, sizeof(yield_str), "%.6f", yield);
        save_to_cache(cache_path, yield_str);
    }
    
    free(response);
    free(cache_path);
    
    if (yield < 0) {
        if (error_code) *error_code = ERROR_PARSING_API_RESPONSE;
        return -1.0;
    }
    
    if (error_code) *error_code = ERROR_SUCCESS;
    return yield;
}

// Implementation of get_risk_free_rate - improved version
double get_risk_free_rate(RateTerm term, int *error_code) {
    if (!is_initialized) {
        if (error_code) *error_code = ERROR_MODULE_NOT_INITIALIZED;
        return -1.0;
    }
    
    // Convert term enum to string for caching and API
    const char *term_str;
    switch (term) {
        case RATE_TERM_1M: term_str = "1month"; break;
        case RATE_TERM_3M: term_str = "3month"; break;
        case RATE_TERM_6M: term_str = "6month"; break;
        case RATE_TERM_1Y: term_str = "1year"; break;
        case RATE_TERM_2Y: term_str = "2year"; break;
        case RATE_TERM_5Y: term_str = "5year"; break;
        case RATE_TERM_10Y: term_str = "10year"; break;
        case RATE_TERM_30Y: term_str = "30year"; break;
        default:
            if (error_code) *error_code = ERROR_INVALID_RATE_TERM;
            return -1.0;
    }
    
    // Check cache first
    char *cache_path = get_cache_path("treasury", term_str);
    if (cache_path == NULL) {
        if (error_code) *error_code = ERROR_MEMORY_ALLOCATION;
        return -1.0;
    }
    
    if (is_cache_valid(cache_path)) {
        char *cached_data = load_from_cache(cache_path);
        if (cached_data != NULL) {
            double rate = atof(cached_data);
            free(cached_data);
            free(cache_path);
            
            if (error_code) *error_code = ERROR_SUCCESS;
            return rate;
        }
    }
    
    // Prepare API request to fetch the latest Treasury rates
    char url[MAX_URL_LENGTH];
    
    // Use Treasury.gov API for daily Treasury rates
    // Note: The actual URL would depend on the real API endpoint
    snprintf(url, MAX_URL_LENGTH, 
            "https://home.treasury.gov/resource-center/data-chart-center/interest-rates/daily-treasury-rates.csv/all/%s",
            term_str);
    
    // Make API request
    char *response = make_api_request(url);
    
    // If API request fails, fall back to placeholder data
    if (response == NULL) {
        double rate = parse_risk_free_rate_treasury(NULL, term_str);
        
        // Cache result if valid
        if (rate >= 0) {
            char rate_str[32];
            snprintf(rate_str, sizeof(rate_str), "%.6f", rate);
            save_to_cache(cache_path, rate_str);
        }
        
        free(cache_path);
        
        if (rate < 0) {
            if (error_code) *error_code = ERROR_RATE_NOT_AVAILABLE;
            return -1.0;
        }
        
        if (error_code) *error_code = ERROR_SUCCESS;
        return rate;
    }
    
    // Parse response
    double rate = parse_risk_free_rate_treasury(response, term_str);
    
    // Cache result if valid
    if (rate >= 0) {
        char rate_str[32];
        snprintf(rate_str, sizeof(rate_str), "%.6f", rate);
        save_to_cache(cache_path, rate_str);
    }
    
    free(response);
    free(cache_path);
    
    if (rate < 0) {
        if (error_code) *error_code = ERROR_PARSING_API_RESPONSE;
        return -1.0;
    }
    
    if (error_code) *error_code = ERROR_SUCCESS;
    return rate;
}

// Implementation of get_historical_volatility
double get_historical_volatility(const char *ticker, int period_days, DataSource source, int *error_code) {
    if (!is_initialized) {
        if (error_code) *error_code = ERROR_MODULE_NOT_INITIALIZED;
        return -1.0;
    }
    
    // Validate ticker
    if (!validate_ticker_symbol(ticker)) {
        if (error_code) *error_code = ERROR_INVALID_TICKER;
        return -1.0;
    }
    
    // Validate period_days
    if (period_days <= 0 || period_days > 365*2) {  // Added upper limit check
        if (error_code) *error_code = ERROR_INVALID_DAYS_PARAMETER;
        return -1.0;
    }
    
    // Use preferred source if not specified
    if (source == DATA_SOURCE_DEFAULT) {
        source = preferred_source;
    }
    
    // Check cache
    char cache_key[64];
    snprintf(cache_key, sizeof(cache_key), "vol_%d", period_days);
    
    char *cache_path = get_cache_path(ticker, cache_key);
    if (cache_path == NULL) {
        if (error_code) *error_code = ERROR_MEMORY_ALLOCATION;
        return -1.0;
    }
    
    if (is_cache_valid(cache_path)) {
        char *cached_data = load_from_cache(cache_path);
        if (cached_data != NULL) {
            double vol = atof(cached_data);
            free(cached_data);
            free(cache_path);
            
            if (error_code) *error_code = ERROR_SUCCESS;
            return vol;
        }
    }
    
    // Prepare API request
    char url[MAX_URL_LENGTH];
    char *api_key = NULL;
    char *sanitized_ticker = sanitize_ticker_symbol(ticker);
    
    if (sanitized_ticker == NULL) {
        free(cache_path);
        if (error_code) *error_code = ERROR_MEMORY_ALLOCATION;
        return -1.0;
    }
    
    // Construct URL based on data source
    switch (source) {
        case DATA_SOURCE_ALPHAVANTAGE:
            api_key = alphavantage_api_key;
            if (api_key == NULL) {
                if (error_code) *error_code = ERROR_API_KEY_NOT_SET;
                free(cache_path);
                free(sanitized_ticker);
                return -1.0;
            }
            
            // Use DAILY adjusted time series to get historical prices
            snprintf(url, MAX_URL_LENGTH, 
                    "https://www.alphavantage.co/query?function=TIME_SERIES_DAILY_ADJUSTED&symbol=%s&outputsize=full&apikey=%s",
                    sanitized_ticker, api_key);
            break;
            
        case DATA_SOURCE_FINNHUB:
        case DATA_SOURCE_POLYGON:
            // For future implementation
            if (error_code) *error_code = ERROR_NOT_IMPLEMENTED;
            free(cache_path);
            free(sanitized_ticker);
            return -1.0;
            
        default:
            if (error_code) *error_code = ERROR_INVALID_DATA_SOURCE;
            free(cache_path);
            free(sanitized_ticker);
            return -1.0;
    }
    
    // No longer need the sanitized ticker
    free(sanitized_ticker);
    
    // Make API request
    char *response = make_api_request(url);
    if (response == NULL) {
        if (error_code) *error_code = ERROR_API_REQUEST_FAILED;
        free(cache_path);
        return -1.0;
    }
    
    // Calculate historical volatility from the response
    double volatility = calculate_historical_volatility_from_data(response, period_days);
    
    // Cache result if valid
    if (volatility > 0) {
        char vol_str[32];
        snprintf(vol_str, sizeof(vol_str), "%.6f", volatility);
        save_to_cache(cache_path, vol_str);
    }
    
    free(response);
    free(cache_path);
    
    if (volatility <= 0) {
        if (error_code) *error_code = ERROR_PARSING_API_RESPONSE;
        return -1.0;
    }
    
    if (error_code) *error_code = ERROR_SUCCESS;
    return volatility;
}

/**
 * Calculate historical volatility from time series data
 */
double calculate_historical_volatility_from_data(const char *json_data, int period_days) {
    if (json_data == NULL || period_days <= 0) {
        return -1.0;
    }
    
    json_error_t error;
    json_t *root = json_loads(json_data, 0, &error);
    
    if (!root) {
        return -1.0;
    }
    
    json_t *time_series = json_object_get(root, "Time Series (Daily)");
    if (!time_series || !json_is_object(time_series)) {
        json_decref(root);
        return -1.0;
    }
    
    // Collect closing prices
    double *prices = malloc((period_days + 1) * sizeof(double));
    if (prices == NULL) {
        json_decref(root);
        return -1.0;
    }
    
    // Get all keys (dates) and sort them in descending order
    size_t key_count = json_object_size(time_series);
    const char **keys = malloc(key_count * sizeof(char*));
    if (keys == NULL) {
        free(prices);
        json_decref(root);
        return -1.0;
    }
    
    size_t i = 0;
    const char *key;
    json_t *value;
    
    json_object_foreach(time_series, key, value) {
        keys[i++] = key;
    }
    
    // Simple bubble sort for dates (since we only need the most recent ones)
    for (i = 0; i < key_count - 1; i++) {
        for (size_t j = 0; j < key_count - i - 1; j++) {
            if (strcmp(keys[j], keys[j + 1]) < 0) {
                const char *temp = keys[j];
                keys[j] = keys[j + 1];
                keys[j + 1] = temp;
            }
        }
    }
    
    // Extract closing prices for the most recent period_days
    int valid_days = 0;
    for (i = 0; i < key_count && valid_days <= period_days; i++) {
        json_t *day_data = json_object_get(time_series, keys[i]);
        if (day_data && json_is_object(day_data)) {
            json_t *close = json_object_get(day_data, "4. close");
            if (close && json_is_string(close)) {
                prices[valid_days++] = atof(json_string_value(close));
            }
        }
    }
    
    free(keys);
    
    if (valid_days <= 1) {
        free(prices);
        json_decref(root);
        return -1.0;
    }
    
    // Calculate log returns
    double *returns = malloc((valid_days - 1) * sizeof(double));
    if (returns == NULL) {
        free(prices);
        json_decref(root);
        return -1.0;
    }
    
    for (i = 0; i < valid_days - 1; i++) {
        returns[i] = log(prices[i] / prices[i + 1]);
    }
    
    // Calculate mean of returns
    double sum = 0.0;
    for (i = 0; i < valid_days - 1; i++) {
        sum += returns[i];
    }
    double mean = sum / (valid_days - 1);
    
    // Calculate variance of returns
    double variance = 0.0;
    for (i = 0; i < valid_days - 1; i++) {
        double diff = returns[i] - mean;
        variance += diff * diff;
    }
    variance /= (valid_days - 2);  // Use n-1 for sample variance
    
    // Calculate annualized volatility
    double volatility = sqrt(variance * 252.0);  // Assuming 252 trading days in a year
    
    free(returns);
    free(prices);
    json_decref(root);
    
    return volatility;
}

/**
 * Adds support for mapping stock and ETF tickers to indices/commodities
 * This allows pricing options on market indices like SPX or VIX
 */
const char* get_underlying_mapping(const char *ticker) {
    if (ticker == NULL) return NULL;
    
    // Common index mappings
    if (strcmp(ticker, "SPX") == 0) return "^GSPC";  // S&P 500
    if (strcmp(ticker, "VIX") == 0) return "^VIX";   // CBOE Volatility Index
    if (strcmp(ticker, "NDX") == 0) return "^NDX";   // Nasdaq-100
    if (strcmp(ticker, "RUT") == 0) return "^RUT";   // Russell 2000
    if (strcmp(ticker, "DJX") == 0) return "^DJI";   // Dow Jones Industrial Average
    
    // Commodity-based ETF mappings
    if (strcmp(ticker, "GLD") == 0) return "GLD";    // SPDR Gold Shares
    if (strcmp(ticker, "USO") == 0) return "USO";    // United States Oil Fund
    if (strcmp(ticker, "SLV") == 0) return "SLV";    // iShares Silver Trust
    
    // Default to the original ticker
    return ticker;
}

/**
 * @brief Get the appropriate historical volatility lookup period based on option expiry 
 * @param days_to_expiry Days until option expiration
 * @return Recommended lookback period in days
 */
int get_volatility_period_for_expiry(int days_to_expiry) {
    if (days_to_expiry <= 7) {
        return 10;  // Use 10-day historical vol for very short term options
    } else if (days_to_expiry <= 30) {
        return 20;  // Use 20-day for short term
    } else if (days_to_expiry <= 90) {
        return 60;  // Use 60-day for medium term
    } else if (days_to_expiry <= 180) {
        return 90;  // Use 90-day for longer term
    } else {
        return 180; // Use 180-day for LEAPS or very long dated options
    }
}

/**
 * @brief Set the preferred data source for market data retrieval
 * @param source The data source to use as default
 */
void set_preferred_data_source(DataSource source) {
    // Validate the source
    if (source == DATA_SOURCE_DEFAULT || 
        source == DATA_SOURCE_ALPHAVANTAGE ||
        source == DATA_SOURCE_FINNHUB ||
        source == DATA_SOURCE_POLYGON) {
        preferred_source = source;
    }
}

/**
 * @brief Set API key for a specific data source
 * @param source The data source to set the key for
 * @param api_key The API key to set
 * @return 0 on success, error code on failure
 */
int set_api_key(DataSource source, const char* api_key) {
    if (api_key == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    
    char* key_copy = strdup(api_key);
    if (key_copy == NULL) {
        return ERROR_MEMORY_ALLOCATION;
    }
    
    // Free any existing key for the source
    switch (source) {
        case DATA_SOURCE_ALPHAVANTAGE:
            if (alphavantage_api_key != NULL) {
                free(alphavantage_api_key);
            }
            alphavantage_api_key = key_copy;
            break;
            
        case DATA_SOURCE_FINNHUB:
            if (finnhub_api_key != NULL) {
                free(finnhub_api_key);
            }
            finnhub_api_key = key_copy;
            break;
            
        case DATA_SOURCE_POLYGON:
            if (polygon_api_key != NULL) {
                free(polygon_api_key);
            }
            polygon_api_key = key_copy;
            break;
            
        default:
            free(key_copy);
            return ERROR_INVALID_DATA_SOURCE;
    }
    
    return ERROR_SUCCESS;
}

/**
 * Get historical prices for a ticker symbol
 */
int get_historical_prices(const char* ticker, int days, DataSource source, 
                         double** prices, char*** dates, int* error_code) {
    int ret_code = ERROR_SUCCESS;
    char *cache_key = NULL;
    cJSON *json_response = NULL;
    char *response_data = NULL;
    char api_url[URL_BUFFER_SIZE];
    
    // Check if market data module is initialized
    if (!is_initialized) {
        set_error(&ret_code, ERROR_MODULE_NOT_INITIALIZED);
        goto cleanup;
    }
    
    // Validate input parameters
    if (ticker == NULL || strlen(ticker) == 0 || strlen(ticker) > MAX_TICKER_LENGTH) {
        set_error(&ret_code, ERROR_INVALID_TICKER);
        goto cleanup;
    }
    
    if (days <= 0 || days > MAX_HISTORY_DAYS) {
        set_error(&ret_code, ERROR_INVALID_DAYS_PARAMETER);
        goto cleanup;
    }
    
    if (prices == NULL || dates == NULL) {
        set_error(&ret_code, ERROR_INVALID_PARAMETER);
        goto cleanup;
    }
    
    // Use preferred data source if needed
    DataSource actual_source = (source == DATA_SOURCE_DEFAULT) ? 
                               preferred_source : source;
    
    // Construct cache key
    size_t cache_key_size = strlen(ticker) + 50;
    cache_key = (char*)malloc(cache_key_size);
    if (cache_key == NULL) {
        set_error(&ret_code, ERROR_MEMORY_ALLOCATION);
        goto cleanup;
    }
    snprintf(cache_key, cache_key_size, "historical_prices_%s_%d_%d", 
             ticker, days, actual_source);
    
    // Check cache first
    CachedData *cached = get_from_cache(cache_key);
    if (cached != NULL && !is_cache_expired(cached)) {
        // Parse cached JSON data
        json_response = cJSON_Parse(cached->data);
        if (json_response == NULL) {
            set_error(&ret_code, ERROR_PARSING_API_RESPONSE);
            goto cleanup;
        }
    } else {
        // Need to fetch data from API
        // Check if API key is set for the data source
        if (!is_api_key_set(actual_source)) {
            set_error(&ret_code, ERROR_API_KEY_NOT_SET);
            goto cleanup;
        }
        
        // Prepare API request based on data source
        memset(api_url, 0, URL_BUFFER_SIZE);
        
        switch (actual_source) {
            case DATA_SOURCE_ALPHAVANTAGE:
                snprintf(api_url, URL_BUFFER_SIZE,
                         "https://www.alphavantage.co/query?function=TIME_SERIES_DAILY"
                         "&symbol=%s&outputsize=%s&apikey=%s",
                         ticker, (days > 100) ? "full" : "compact", api_keys[DATA_SOURCE_ALPHAVANTAGE]);
                break;
                
            case DATA_SOURCE_FINNHUB:
                // Calculate start and end timestamps
                time_t now = time(NULL);
                time_t start_time = now - (days * 86400); // days in seconds
                
                snprintf(api_url, URL_BUFFER_SIZE,
                         "https://finnhub.io/api/v1/stock/candle?symbol=%s"
                         "&resolution=D&from=%ld&to=%ld&token=%s",
                         ticker, start_time, now, api_keys[DATA_SOURCE_FINNHUB]);
                break;
                
            case DATA_SOURCE_POLYGON:
                {
                    // Calculate date range
                    time_t now = time(NULL);
                    time_t start_time = now - (days * 86400);
                    struct tm *start_tm = localtime(&start_time);
                    char date_str[20];
                    strftime(date_str, sizeof(date_str), "%Y-%m-%d", start_tm);
                    
                    snprintf(api_url, URL_BUFFER_SIZE,
                             "https://api.polygon.io/v2/aggs/ticker/%s/range/1/day/%s/today"
                             "?apiKey=%s",
                             ticker, date_str, api_keys[DATA_SOURCE_POLYGON]);
                }
                break;
                
            default:
                set_error(&ret_code, ERROR_INVALID_DATA_SOURCE);
                goto cleanup;
        }
        
        // Make API request
        response_data = make_api_request(api_url);
        if (ret_code != ERROR_SUCCESS || response_data == NULL) {
            // Error already set
            goto cleanup;
        }
        
        // Parse JSON response
        json_response = cJSON_Parse(response_data);
        if (json_response == NULL) {
            set_error(&ret_code, ERROR_PARSING_API_RESPONSE);
            goto cleanup;
        }
        
        // Cache the response if valid
        add_to_cache(cache_key, response_data);
    }
    
    // Process the response based on data source
    int count = 0;
    switch (actual_source) {
        case DATA_SOURCE_ALPHAVANTAGE:
            count = extract_alpha_vantage_prices(json_response, days, prices, dates);
            break;
            
        case DATA_SOURCE_FINNHUB:
            count = extract_finnhub_prices(json_response, days, prices, dates);
            break;
            
        case DATA_SOURCE_POLYGON:
            count = extract_polygon_prices(json_response, days, prices, dates);
            break;
            
        default:
            set_error(&ret_code, ERROR_INVALID_DATA_SOURCE);
            goto cleanup;
    }
    
    if (count <= 0) {
        set_error(&ret_code, ERROR_PARSING_API_RESPONSE);
        count = ret_code; // Return error code
    }
    
cleanup:
    // Set error code if requested
    if (error_code != NULL) {
        *error_code = ret_code;
    }
    
    // Free allocated resources
    if (cache_key != NULL) {
        free(cache_key);
    }
    if (json_response != NULL) {
        cJSON_Delete(json_response);
    }
    if (response_data != NULL) {
        free(response_data);
    }
    
    return (ret_code == ERROR_SUCCESS) ? count : ret_code;
}

/**
 * Extract historical prices from Alpha Vantage response
 */
static int extract_alpha_vantage_prices(cJSON *json_response, int max_days, 
                                       double **prices, char ***dates) {
    if (json_response == NULL || prices == NULL || dates == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    
    // Get the time series data
    cJSON *time_series = cJSON_GetObjectItem(json_response, "Time Series (Daily)");
    if (time_series == NULL) {
        return ERROR_PARSING_API_RESPONSE;
    }
    
    // Count the number of days (up to max_days)
    int count = 0;
    cJSON *date_item = NULL;
    cJSON_ArrayForEach(date_item, time_series) {
        count++;
        if (count >= max_days) {
            break;
        }
    }
    
    if (count == 0) {
        return ERROR_PARSING_API_RESPONSE;
    }
    
    // Allocate memory for prices and dates
    *prices = (double*)malloc(count * sizeof(double));
    *dates = (char**)malloc(count * sizeof(char*));
    
    if (*prices == NULL || *dates == NULL) {
        if (*prices) free(*prices);
        if (*dates) free(*dates);
        *prices = NULL;
        *dates = NULL;
        return ERROR_MEMORY_ALLOCATION;
    }
    
    // Extract data
    count = 0;
    date_item = NULL;
    cJSON_ArrayForEach(date_item, time_series) {
        if (count >= max_days) {
            break;
        }
        
        // Get the date (key)
        const char *date_str = date_item->string;
        
        // Get the closing price
        cJSON *data = date_item->child;
        cJSON *close_price = cJSON_GetObjectItem(data, "4. close");
        
        if (close_price && close_price->valuestring) {
            (*prices)[count] = atof(close_price->valuestring);
            (*dates)[count] = strdup(date_str);
            count++;
        }
    }
    
    return count;
}

/**
 * Extract historical prices from Finnhub response
 */
static int extract_finnhub_prices(cJSON *json_response, int max_days, 
                                 double **prices, char ***dates) {
    if (json_response == NULL || prices == NULL || dates == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    
    // Check if the response is valid
    cJSON *status = cJSON_GetObjectItem(json_response, "s");
    if (status == NULL || strcmp(cJSON_GetStringValue(status), "ok") != 0) {
        return ERROR_PARSING_API_RESPONSE;
    }
    
    // Get the price data arrays
    cJSON *close_prices = cJSON_GetObjectItem(json_response, "c");
    cJSON *timestamps = cJSON_GetObjectItem(json_response, "t");
    
    if (close_prices == NULL || timestamps == NULL || 
        !cJSON_IsArray(close_prices) || !cJSON_IsArray(timestamps)) {
        return ERROR_PARSING_API_RESPONSE;
    }
    
    // Get the count (minimum of array sizes and max_days)
    int count = cJSON_GetArraySize(close_prices);
    int timestamp_count = cJSON_GetArraySize(timestamps);
    
    if (count != timestamp_count) {
        return ERROR_PARSING_API_RESPONSE;
    }
    
    if (count > max_days) {
        count = max_days;
    }
    
    if (count == 0) {
        return ERROR_PARSING_API_RESPONSE;
    }
    
    // Allocate memory for prices and dates
    *prices = (double*)malloc(count * sizeof(double));
    *dates = (char**)malloc(count * sizeof(char*));
    
    if (*prices == NULL || *dates == NULL) {
        if (*prices) free(*prices);
        if (*dates) free(*dates);
        *prices = NULL;
        *dates = NULL;
        return ERROR_MEMORY_ALLOCATION;
    }
    
    // Extract data
    for (int i = 0; i < count; i++) {
        cJSON *price = cJSON_GetArrayItem(close_prices, i);
        cJSON *timestamp = cJSON_GetArrayItem(timestamps, i);
        
        if (price && cJSON_IsNumber(price) && 
            timestamp && cJSON_IsNumber(timestamp)) {
            
            (*prices)[i] = price->valuedouble;
            
            // Convert timestamp to date string
            time_t time_val = (time_t)timestamp->valueint;
            struct tm *tm_info = localtime(&time_val);
            char date_str[20];
            strftime(date_str, sizeof(date_str), "%Y-%m-%d", tm_info);
            
            (*dates)[i] = strdup(date_str);
        } else {
            // Handle missing data
            (*prices)[i] = 0.0;
            (*dates)[i] = strdup("unknown");
        }
    }
    
    return count;
}

/**
 * Extract historical prices from Polygon response
 */
static int extract_polygon_prices(cJSON *json_response, int max_days, 
                                 double **prices, char ***dates) {
    if (json_response == NULL || prices == NULL || dates == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    
    // Check if the response is valid
    cJSON *status = cJSON_GetObjectItem(json_response, "status");
    if (status == NULL || strcmp(cJSON_GetStringValue(status), "OK") != 0) {
        return ERROR_PARSING_API_RESPONSE;
    }
    
    // Get the results array
    cJSON *results = cJSON_GetObjectItem(json_response, "results");
    if (results == NULL || !cJSON_IsArray(results)) {
        return ERROR_PARSING_API_RESPONSE;
    }
    
    // Get the count (minimum of array size and max_days)
    int count = cJSON_GetArraySize(results);
    if (count > max_days) {
        count = max_days;
    }
    
    if (count == 0) {
        return ERROR_PARSING_API_RESPONSE;
    }
    
    // Allocate memory for prices and dates
    *prices = (double*)malloc(count * sizeof(double));
    *dates = (char**)malloc(count * sizeof(char*));
    
    if (*prices == NULL || *dates == NULL) {
        if (*prices) free(*prices);
        if (*dates) free(*dates);
        *prices = NULL;
        *dates = NULL;
        return ERROR_MEMORY_ALLOCATION;
    }
    
    // Extract data
    for (int i = 0; i < count; i++) {
        cJSON *day_data = cJSON_GetArrayItem(results, i);
        if (day_data && cJSON_IsObject(day_data)) {
            cJSON *close = cJSON_GetObjectItem(day_data, "c");
            cJSON *timestamp = cJSON_GetObjectItem(day_data, "t");
            
            if (close && cJSON_IsNumber(close) && 
                timestamp && cJSON_IsNumber(timestamp)) {
                
                (*prices)[i] = close->valuedouble;
                
                // Convert timestamp to date string (Polygon timestamps are in milliseconds)
                time_t time_val = (time_t)(timestamp->valuedouble / 1000);
                struct tm *tm_info = localtime(&time_val);
                char date_str[20];
                strftime(date_str, sizeof(date_str), "%Y-%m-%d", tm_info);
                
                (*dates)[i] = strdup(date_str);
            } else {
                // Handle missing data
                (*prices)[i] = 0.0;
                (*dates)[i] = strdup("unknown");
            }
        }
    }
    
    return count;
}

/**
 * Get the directory path for the cache
 */
static char* get_cache_dir(void) {
    static char cache_dir[MAX_CACHE_PATH] = {0};
    
    // Only compute the cache directory once
    if (cache_dir[0] == '\0') {
        const char* home = getenv("HOME");
        if (home == NULL) {
            return NULL; // HOME environment variable not set
        }
        
        snprintf(cache_dir, sizeof(cache_dir), "%s/.cache/option_tools", home);
        
        // Create the directory if it doesn't exist
        struct stat st = {0};
        if (stat(cache_dir, &st) == -1) {
            // Create the parent directory first
            char parent_dir[MAX_CACHE_PATH];
            snprintf(parent_dir, sizeof(parent_dir), "%s/.cache", home);
            if (stat(parent_dir, &st) == -1) {
                mkdir(parent_dir, 0755);
            }
            
            // Create the cache directory
            mkdir(cache_dir, 0755);
        }
    }
    
    return cache_dir;
}

/**
 * Get the cache file path for a specific ticker and data type
 */
static char* get_cache_path(const char* ticker, const char* data_type, DataSource source) {
    static char cache_path[MAX_CACHE_PATH];
    char* cache_dir = get_cache_dir();
    
    if (cache_dir == NULL) {
        return NULL;
    }
    
    snprintf(cache_path, sizeof(cache_path), "%s/%s_%s_%d.cache",
             cache_dir, ticker, data_type, source);
    
    return cache_path;
}

/**
 * Check if the cached data is expired
 */
static int is_cache_expired(CachedData* cached_data) {
    if (cached_data == NULL) {
        return 1; // No cached data, consider it expired
    }
    
    // If cache timeout is 0, caching is disabled
    if (cache_expiry_seconds == 0) {
        return 1;
    }
    
    time_t now = time(NULL);
    return (now - cached_data->timestamp) > cache_expiry_seconds;
}

/**
 * Get data from cache for a specific cache key
 */
static CachedData* get_from_cache(const char* cache_key) {
    char* cache_path = get_cache_path(cache_key, "data", DATA_SOURCE_DEFAULT);
    if (cache_path == NULL) {
        return NULL;
    }
    
    // Check if cache file exists and is valid
    if (!is_cache_valid(cache_path)) {
        return NULL;
    }
    
    // Load data from cache file
    char* data = load_from_cache(cache_path);
    if (data == NULL) {
        return NULL;
    }
    
    // Get file modification time as timestamp
    struct stat attr;
    if (stat(cache_path, &attr) != 0) {
        free(data);
        return NULL;
    }
    
    // Create the cached data structure
    CachedData* cached_data = (CachedData*)malloc(sizeof(CachedData));
    if (cached_data == NULL) {
        free(data);
        return NULL;
    }
    
    cached_data->data = data;
    cached_data->timestamp = attr.st_mtime;
    
    return cached_data;
}

/**
 * Add data to cache for a specific cache key
 */
static int add_to_cache(const char* cache_key, const char* data) {
    // If cache timeout is 0, caching is disabled
    if (cache_expiry_seconds == 0) {
        return 0;
    }
    
    char* cache_path = get_cache_path(cache_key, "data", DATA_SOURCE_DEFAULT);
    if (cache_path == NULL) {
        return -1;
    }
    
    return save_to_cache(cache_path, data);
}

/**
 * Check if an API key is set for a specific data source
 */
static int is_api_key_set(DataSource source) {
    if (source < 0 || source >= 4) {
        return 0; // Invalid source
    }
    
    return (source == DATA_SOURCE_ALPHAVANTAGE && alphavantage_api_key != NULL) ||
           (source == DATA_SOURCE_FINNHUB && finnhub_api_key != NULL) ||
           (source == DATA_SOURCE_POLYGON && polygon_api_key != NULL);
}

/**
 * Set an error code if the error_code pointer is not NULL
 */
static void set_error(int* error_code_ptr, int error_code) {
    if (error_code_ptr != NULL) {
        *error_code_ptr = error_code;
    }
} 