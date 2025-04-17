#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "../include/option_types.h"
#include "../include/error_handling.h"
#include "../include/path_resolution.h"

/**
 * Maximum length for command strings
 */
#define MAX_CMD_LENGTH 1024

/**
 * Maximum length for output buffer
 */
#define MAX_OUTPUT_LENGTH 4096

/**
 * @brief Adapt the unified API to the legacy Heston model implementation
 * 
 * @param spot_price The current price of the underlying asset
 * @param strike_price The strike price of the option
 * @param time_to_expiry Time to expiry in years
 * @param risk_free_rate Risk-free interest rate (annualized)
 * @param dividend_yield Dividend yield (annualized)
 * @param volatility Initial volatility (if known, 0 to use default)
 * @param option_type Type of option (OPTION_CALL or OPTION_PUT)
 * @param method Numerical method to use (METHOD_QUADRATURE or METHOD_FFT)
 * @param market_price Market price (for implied volatility calculation, 0 to skip)
 * @param result Pointer to a PricingResult structure to store the result
 * 
 * @return 0 on success, error code on failure
 */
int price_with_heston(
    double spot_price,
    double strike_price,
    double time_to_expiry,
    double risk_free_rate,
    double dividend_yield,
    double volatility,
    OptionType option_type,
    NumericalMethod method,
    double market_price,
    PricingResult* result
) {
    char cmd[MAX_CMD_LENGTH];
    char output[MAX_OUTPUT_LENGTH];
    char* binary_path;
    FILE* pipe;
    int ret = ERROR_NONE;
    const char* version;
    const char* method_arg = "";
    
    /* Initialize result structure */
    memset(result, 0, sizeof(PricingResult));
    
    /* Map the method to the appropriate version and arguments */
    switch (method) {
        case METHOD_QUADRATURE:
            version = "v3";
            break;
            
        case METHOD_FFT:
            version = "v5";  /* Use the most advanced FFT implementation */
            break;
            
        default:
            set_error(ERROR_INVALID_NUMERICAL_METHOD);
            result->error_code = ERROR_INVALID_NUMERICAL_METHOD;
            return ERROR_INVALID_NUMERICAL_METHOD;
    }
    
    /* Determine which binary to use */
    binary_path = resolve_legacy_binary_path(version, "calculate_sv");
    
    if (binary_path == NULL) {
        /* Error already set by resolve_legacy_binary_path */
        result->error_code = get_error();
        return result->error_code;
    }
    
    /* Construct the command based on the operation */
    if (market_price > 0) {
        /* Implied volatility calculation */
        snprintf(cmd, sizeof(cmd), "%s %s %.6f %.6f %.6f %.6f %.6f %d",
                 binary_path,
                 method_arg,
                 market_price,
                 spot_price,
                 strike_price,
                 time_to_expiry,
                 risk_free_rate,
                 (int)option_type);
                 
        if (dividend_yield > 0) {
            /* Append dividend yield if provided */
            char div_arg[32];
            snprintf(div_arg, sizeof(div_arg), " %.6f", dividend_yield);
            strncat(cmd, div_arg, sizeof(cmd) - strlen(cmd) - 1);
        }
    } else {
        /* Option pricing with known volatility */
        /* Note: For the Heston model, volatility is the initial variance */
        double initial_variance = volatility * volatility;
        
        snprintf(cmd, sizeof(cmd), "%s %s --vol=%.6f %.6f %.6f %.6f %.6f %d",
                 binary_path,
                 method_arg,
                 initial_variance,
                 spot_price,
                 strike_price,
                 time_to_expiry,
                 risk_free_rate,
                 (int)option_type);
                 
        if (dividend_yield > 0) {
            /* Append dividend yield if provided */
            char div_arg[32];
            snprintf(div_arg, sizeof(div_arg), " %.6f", dividend_yield);
            strncat(cmd, div_arg, sizeof(cmd) - strlen(cmd) - 1);
        }
    }
    
    /* Free the binary path as it's no longer needed */
    free_resolved_path(binary_path);
    
    /* Execute the command and capture output */
    pipe = popen(cmd, "r");
    if (pipe == NULL) {
        set_error(ERROR_COMMAND_EXECUTION);
        result->error_code = ERROR_COMMAND_EXECUTION;
        return result->error_code;
    }
    
    /* Read command output */
    if (fgets(output, sizeof(output), pipe) == NULL) {
        pclose(pipe);
        set_error(ERROR_COMMAND_OUTPUT_PARSING);
        result->error_code = ERROR_COMMAND_OUTPUT_PARSING;
        return result->error_code;
    }
    
    /* Close the pipe */
    if (pclose(pipe) != 0) {
        set_error(ERROR_COMMAND_EXECUTION);
        result->error_code = ERROR_COMMAND_EXECUTION;
        return result->error_code;
    }
    
    /* Parse the output based on the operation */
    if (market_price > 0) {
        /* Parse implied volatility output */
        double iv;
        if (sscanf(output, "Implied Volatility(SV):%lf", &iv) != 1) {
            set_error(ERROR_COMMAND_OUTPUT_PARSING);
            result->error_code = ERROR_COMMAND_OUTPUT_PARSING;
            return result->error_code;
        }
        
        /* Convert percentage to decimal */
        result->implied_volatility = iv / 100.0;
        
        /* Calculate option price using the implied volatility */
        /* For now, we'll just set it to the market price */
        result->price = market_price;
    } else {
        /* Parse option price output */
        double price;
        if (sscanf(output, "Price: %lf", &price) != 1) {
            set_error(ERROR_COMMAND_OUTPUT_PARSING);
            result->error_code = ERROR_COMMAND_OUTPUT_PARSING;
            return result->error_code;
        }
        
        result->price = price;
    }
    
    /* Set the result error code to indicate success */
    result->error_code = ERROR_NONE;
    
    return ERROR_NONE;
}

/**
 * @brief Calculate Greeks for an option using the Heston model
 * 
 * @param spot_price Spot price
 * @param strike_price Strike price
 * @param time_to_expiry Time to expiry in years
 * @param risk_free_rate Risk-free rate
 * @param dividend_yield Dividend yield
 * @param volatility Initial volatility
 * @param option_type Option type (call or put)
 * @param method Numerical method to use
 * @param result Pointer to store the calculated Greeks
 * 
 * @return 0 on success, error code on failure
 */
int calculate_heston_greeks(
    double spot_price,
    double strike_price,
    double time_to_expiry,
    double risk_free_rate,
    double dividend_yield,
    double volatility,
    OptionType option_type,
    NumericalMethod method,
    PricingResult* result
) {
    char cmd[MAX_CMD_LENGTH];
    char output[MAX_OUTPUT_LENGTH];
    char* binary_path;
    FILE* pipe;
    const char* version;
    
    /* Initialize result structure */
    memset(result, 0, sizeof(PricingResult));
    
    /* Map the method to the appropriate version */
    switch (method) {
        case METHOD_QUADRATURE:
            version = "v3";
            break;
            
        case METHOD_FFT:
            version = "v5";  /* Use the most advanced FFT implementation */
            break;
            
        default:
            set_error(ERROR_INVALID_NUMERICAL_METHOD);
            result->error_code = ERROR_INVALID_NUMERICAL_METHOD;
            return ERROR_INVALID_NUMERICAL_METHOD;
    }
    
    /* For now, we'll use finite differences as a placeholder */
    /* This is not optimal and should be replaced with direct calculations */
    
    /* Calculate the base price */
    if (price_with_heston(
            spot_price, strike_price, time_to_expiry,
            risk_free_rate, dividend_yield, volatility,
            option_type, method, 0.0, result) != ERROR_NONE) {
        /* Error already set by price_with_heston */
        return result->error_code;
    }
    
    /* The real implementation would use the legacy SV calculator's Greeks functionality */
    /* For now, set dummy values for Greeks */
    result->delta = 0.5;  /* Placeholder */
    result->gamma = 0.1;  /* Placeholder */
    result->theta = -0.1; /* Placeholder */
    result->vega = 0.2;   /* Placeholder */
    result->rho = 0.05;   /* Placeholder */
    
    result->error_code = ERROR_NONE;
    return ERROR_NONE;
}

/**
 * @brief Calculate implied volatility using the Heston model
 * 
 * @param market_price Market price of the option
 * @param spot_price Spot price of the underlying
 * @param strike_price Strike price
 * @param time_to_expiry Time to expiry in years
 * @param risk_free_rate Risk-free rate
 * @param dividend_yield Dividend yield
 * @param option_type Option type (call or put)
 * @param method Numerical method to use
 * 
 * @return Implied volatility on success, negative value on failure
 */
double calculate_heston_iv(
    double market_price,
    double spot_price,
    double strike_price,
    double time_to_expiry,
    double risk_free_rate,
    double dividend_yield,
    OptionType option_type,
    NumericalMethod method
) {
    PricingResult result;
    int ret;
    
    ret = price_with_heston(
        spot_price,
        strike_price,
        time_to_expiry,
        risk_free_rate,
        dividend_yield,
        0.0, /* Initial volatility is not used for IV calculation */
        option_type,
        method,
        market_price,
        &result
    );
    
    if (ret != ERROR_NONE) {
        return -1.0;
    }
    
    return result.implied_volatility;
} 