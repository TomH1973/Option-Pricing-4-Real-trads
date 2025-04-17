/**
 * @file error_handling.h
 * @brief Standardized error handling for the unified option pricing system
 */

#ifndef UNIFIED_ERROR_HANDLING_H
#define UNIFIED_ERROR_HANDLING_H

#include <stdio.h>

/**
 * @brief Error codes used throughout the unified option pricing system
 * 
 * These error codes are standardized throughout the system to provide
 * consistent error reporting and handling.
 */
typedef enum {
    /* General Success/Error codes */
    ERROR_SUCCESS = 0,
    ERROR_UNKNOWN = 1,
    
    /* System errors (100-199) */
    ERROR_MEMORY_ALLOCATION = 100,
    ERROR_FILE_NOT_FOUND = 101,
    ERROR_PERMISSION_DENIED = 102,
    ERROR_SYSTEM_CALL_FAILED = 103,
    ERROR_MODULE_NOT_INITIALIZED = 104,
    ERROR_TIMEOUT = 105,
    ERROR_NOT_IMPLEMENTED = 199,
    
    /* Input validation errors (200-299) */
    ERROR_INVALID_PARAMETER = 200,
    ERROR_NULL_PARAMETER = 201,
    ERROR_OUT_OF_RANGE = 202,
    ERROR_INVALID_OPTION_TYPE = 210,
    ERROR_INVALID_MODEL_TYPE = 211,
    ERROR_INVALID_RATE_TERM = 212,
    ERROR_INVALID_TICKER = 220,
    ERROR_INVALID_DAYS_PARAMETER = 221,
    
    /* Market data errors (300-399) */
    ERROR_MARKET_DATA = 300,
    ERROR_API_KEY_NOT_SET = 301,
    ERROR_API_REQUEST_FAILED = 302,
    ERROR_PARSING_API_RESPONSE = 303,
    ERROR_DATA_NOT_AVAILABLE = 304,
    ERROR_INVALID_DATA_SOURCE = 305,
    ERROR_RATE_NOT_AVAILABLE = 310,
    ERROR_DIVIDEND_NOT_AVAILABLE = 311,
    ERROR_VOLATILITY_NOT_AVAILABLE = 312,
    
    /* Pricing model errors (400-499) */
    ERROR_MODEL_CALIBRATION = 400,
    ERROR_CONVERGENCE_FAILED = 401,
    ERROR_SINGULAR_MATRIX = 402,
    ERROR_NEGATIVE_OPTION_VALUE = 403,
    ERROR_INVALID_GREEKS_FLAGS = 410,
    
    /* External binary errors (500-599) */
    ERROR_BINARY_NOT_FOUND = 500,
    ERROR_BINARY_EXECUTION_FAILED = 501,
    ERROR_BINARY_OUTPUT_PARSING = 502,
    
    /* Configuration errors (600-699) */
    ERROR_CONFIG_FILE_NOT_FOUND = 600,
    ERROR_CONFIG_PARSE_ERROR = 601,
    ERROR_CONFIG_KEY_NOT_FOUND = 602,
    
    /* Cache errors (700-799) */
    ERROR_CACHE_WRITE_FAILED = 700,
    ERROR_CACHE_READ_FAILED = 701,
    ERROR_CACHE_EXPIRED = 702
} ErrorCode;

/**
 * @brief Get a description string for an error code
 * 
 * @param error_code The error code to get a description for
 * @return const char* A human-readable description of the error code
 */
const char* get_error_description(int error_code);

/**
 * @brief Set the last error code and message
 * 
 * @param error_code The error code to set
 * @param message Additional error message or context (can be NULL)
 */
void set_last_error(int error_code, const char* message);

/**
 * @brief Get the last error code
 * 
 * @return int The last error code that was set
 */
/* Error code definitions */
#define ERROR_NONE                       0   /**< No error */
#define ERROR_INVALID_PARAMETER         -1   /**< Invalid parameter */
#define ERROR_INVALID_OPTION_TYPE       -2   /**< Invalid option type */
#define ERROR_INVALID_MODEL_TYPE        -3   /**< Invalid model type */
#define ERROR_INVALID_NUMERICAL_METHOD  -4   /**< Invalid numerical method */
#define ERROR_COMMAND_EXECUTION         -5   /**< Error executing external command */
#define ERROR_COMMAND_OUTPUT_PARSING    -6   /**< Error parsing command output */
#define ERROR_FILE_NOT_FOUND            -7   /**< File not found */
#define ERROR_PATH_RESOLUTION           -8   /**< Path resolution failed */
#define ERROR_CALCULATION_FAILED        -9   /**< Option pricing calculation failed */
#define ERROR_VOLATILITY_CALCULATION    -10  /**< Implied volatility calculation failed */
#define ERROR_GREEKS_CALCULATION        -11  /**< Greeks calculation failed */
#define ERROR_MEMORY_ALLOCATION         -12  /**< Memory allocation failed */
#define ERROR_NETWORK_FAILURE           -13  /**< Network/API request failed */
#define ERROR_DATA_SOURCE_UNAVAILABLE   -14  /**< Market data source unavailable */
#define ERROR_DATA_VALIDATION           -15  /**< Market data validation failed */
#define ERROR_CONFIG_PARSING            -16  /**< Configuration file parsing error */

/**
 * @brief Set the error code for the current thread
 * @param error_code The error code to set
 */
void set_error(int error_code);

/**
 * @brief Get the error message for an error code
 * @param error_code The error code
 * @return A human-readable error message
 */
const char* get_error_message(int error_code);

/**
 * @brief Check if an error code represents a fatal error
 * @param error_code The error code to check
 * @return 1 if fatal, 0 if non-fatal
 */
int is_fatal_error(int error_code);

/**
 * @brief Reset the current error state
 */
void reset_error(void);

#endif /* ERROR_HANDLING_H */ 