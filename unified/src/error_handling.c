/**
 * @file error_handling.c
 * @brief Implementation of error handling functions for the unified option pricing system
 */

#include "../include/error_handling.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/* Thread-local storage for error tracking */
static __thread int last_error_code = ERROR_SUCCESS;
static __thread char last_error_message[512] = {0};
static pthread_mutex_t error_log_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Global error log file - can be set via configuration */
static FILE* error_log_file = NULL;

/**
 * @brief Get a description string for an error code
 * 
 * @param error_code The error code to get a description for
 * @return const char* A human-readable description of the error code
 */
const char* get_error_description(int error_code) {
    switch (error_code) {
        /* General Success/Error codes */
        case ERROR_SUCCESS: 
            return "Success";
        case ERROR_UNKNOWN: 
            return "Unknown error";
            
        /* System errors */
        case ERROR_MEMORY_ALLOCATION: 
            return "Memory allocation failed";
        case ERROR_FILE_NOT_FOUND: 
            return "File not found";
        case ERROR_PERMISSION_DENIED: 
            return "Permission denied";
        case ERROR_SYSTEM_CALL_FAILED: 
            return "System call failed";
        case ERROR_MODULE_NOT_INITIALIZED: 
            return "Module not initialized";
        case ERROR_TIMEOUT: 
            return "Operation timed out";
        case ERROR_NOT_IMPLEMENTED: 
            return "Feature not implemented";
            
        /* Input validation errors */
        case ERROR_INVALID_PARAMETER: 
            return "Invalid parameter";
        case ERROR_NULL_PARAMETER: 
            return "Parameter cannot be NULL";
        case ERROR_OUT_OF_RANGE: 
            return "Parameter out of valid range";
        case ERROR_INVALID_OPTION_TYPE: 
            return "Invalid option type";
        case ERROR_INVALID_MODEL_TYPE: 
            return "Invalid model type";
        case ERROR_INVALID_RATE_TERM: 
            return "Invalid rate term";
        case ERROR_INVALID_TICKER: 
            return "Invalid ticker symbol";
        case ERROR_INVALID_DAYS_PARAMETER: 
            return "Invalid days parameter";
            
        /* Market data errors */
        case ERROR_MARKET_DATA: 
            return "Market data error";
        case ERROR_API_KEY_NOT_SET: 
            return "API key not set";
        case ERROR_API_REQUEST_FAILED: 
            return "API request failed";
        case ERROR_PARSING_API_RESPONSE: 
            return "Error parsing API response";
        case ERROR_DATA_NOT_AVAILABLE: 
            return "Data not available";
        case ERROR_INVALID_DATA_SOURCE: 
            return "Invalid data source";
        case ERROR_RATE_NOT_AVAILABLE: 
            return "Risk-free rate not available";
        case ERROR_DIVIDEND_NOT_AVAILABLE: 
            return "Dividend yield not available";
        case ERROR_VOLATILITY_NOT_AVAILABLE: 
            return "Volatility data not available";
            
        /* Pricing model errors */
        case ERROR_MODEL_CALIBRATION: 
            return "Model calibration error";
        case ERROR_CONVERGENCE_FAILED: 
            return "Convergence failed";
        case ERROR_SINGULAR_MATRIX: 
            return "Singular matrix encountered";
        case ERROR_NEGATIVE_OPTION_VALUE: 
            return "Negative option value calculated";
        case ERROR_INVALID_GREEKS_FLAGS: 
            return "Invalid Greeks flags";
            
        /* External binary errors */
        case ERROR_BINARY_NOT_FOUND: 
            return "External binary not found";
        case ERROR_BINARY_EXECUTION_FAILED: 
            return "External binary execution failed";
        case ERROR_BINARY_OUTPUT_PARSING: 
            return "Error parsing external binary output";
            
        /* Configuration errors */
        case ERROR_CONFIG_FILE_NOT_FOUND: 
            return "Configuration file not found";
        case ERROR_CONFIG_PARSE_ERROR: 
            return "Error parsing configuration file";
        case ERROR_CONFIG_KEY_NOT_FOUND: 
            return "Configuration key not found";
            
        /* Cache errors */
        case ERROR_CACHE_WRITE_FAILED: 
            return "Cache write failed";
        case ERROR_CACHE_READ_FAILED: 
            return "Cache read failed";
        case ERROR_CACHE_EXPIRED: 
            return "Cache data expired";
            
        default:
            return "Undefined error code";
    }
}

/**
 * @brief Set the last error code and message
 * 
 * @param error_code The error code to set
 * @param message Additional error message or context (can be NULL)
 */
void set_last_error(int error_code, const char* message) {
    last_error_code = error_code;
    
    if (message != NULL) {
        strncpy(last_error_message, message, sizeof(last_error_message) - 1);
        last_error_message[sizeof(last_error_message) - 1] = '\0';
    } else {
        last_error_message[0] = '\0';
    }
}

/**
 * @brief Get the last error code
 * 
 * @return int The last error code that was set
 */
int get_last_error_code(void) {
    return last_error_code;
}

/**
 * @brief Get the last error message
 * 
 * @return const char* The last error message that was set
 */
const char* get_last_error_message(void) {
    return last_error_message;
}

/**
 * @brief Clear the last error code and message
 */
void clear_last_error(void) {
    last_error_code = ERROR_SUCCESS;
    last_error_message[0] = '\0';
}

/**
 * @brief Set the error log file
 * 
 * @param log_file Pointer to an open file for error logging (NULL to disable)
 * @return int ERROR_SUCCESS on success, otherwise an error code
 */
int set_error_log_file(FILE* log_file) {
    if (log_file == NULL) {
        pthread_mutex_lock(&error_log_mutex);
        error_log_file = NULL;
        pthread_mutex_unlock(&error_log_mutex);
        return ERROR_SUCCESS;
    }
    
    /* Test that we can write to the file */
    if (fprintf(log_file, "# Unified Option Pricing System - Error Log Initialized\n") < 0) {
        return ERROR_PERMISSION_DENIED;
    }
    
    pthread_mutex_lock(&error_log_mutex);
    error_log_file = log_file;
    pthread_mutex_unlock(&error_log_mutex);
    
    return ERROR_SUCCESS;
}

/**
 * @brief Log an error message with context
 * 
 * @param error_code The error code associated with the error
 * @param function The function name where the error occurred
 * @param message Additional error message or context (can be NULL)
 */
void log_error(int error_code, const char* function, const char* message) {
    char timestamp[32];
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    pthread_mutex_lock(&error_log_mutex);
    
    if (error_log_file != NULL) {
        if (message != NULL) {
            fprintf(error_log_file, "[%s] ERROR %d: %s in %s - %s\n", 
                    timestamp, error_code, get_error_description(error_code), 
                    function, message);
        } else {
            fprintf(error_log_file, "[%s] ERROR %d: %s in %s\n", 
                    timestamp, error_code, get_error_description(error_code), 
                    function);
        }
        fflush(error_log_file);
    }
    
    /* Always record errors in stderr for critical errors */
    if (error_code >= 100 && error_code < 200) {
        if (message != NULL) {
            fprintf(stderr, "[%s] ERROR %d: %s in %s - %s\n", 
                    timestamp, error_code, get_error_description(error_code), 
                    function, message);
        } else {
            fprintf(stderr, "[%s] ERROR %d: %s in %s\n", 
                    timestamp, error_code, get_error_description(error_code), 
                    function);
        }
    }
    
    pthread_mutex_unlock(&error_log_mutex);
    
    /* Also set as the last error */
    set_last_error(error_code, message);
} 