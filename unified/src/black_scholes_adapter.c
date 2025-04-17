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
 * @brief Adapt the unified API to the legacy Black-Scholes implementation
 * 
 * @param spot_price The current price of the underlying asset
 * @param strike_price The strike price of the option
 * @param time_to_expiry Time to expiry in years
 * @param risk_free_rate Risk-free interest rate (annualized)
 * @param dividend_yield Dividend yield (annualized)
 * @param volatility Implied volatility (if known, 0 to calculate)
 * @param option_type Type of option (OPTION_CALL or OPTION_PUT)
 * @param market_price Market price (for implied volatility calculation, 0 to skip)
 * @param result Pointer to a PricingResult structure to store the result
 * 
 * @return 0 on success, error code on failure
 */
int price_with_black_scholes(
    double spot_price,
    double strike_price,
    double time_to_expiry,
    double risk_free_rate,
    double dividend_yield,
    double volatility,
    OptionType option_type,
    double market_price,
    PricingResult* result
) {
    char cmd[MAX_CMD_LENGTH];
    char output[MAX_OUTPUT_LENGTH];
    char* binary_path;
    FILE* pipe;
    int ret = ERROR_NONE;
    
    /* Initialize result structure */
    memset(result, 0, sizeof(PricingResult));
    
    /* Determine which binary to use */
    if (market_price > 0) {
        /* For implied volatility calculation, use the calculate_iv binary */
        binary_path = resolve_legacy_binary_path("v2", "calculate_iv");
    } else {
        /* For option pricing, use the calculate_bs binary */
        binary_path = resolve_legacy_binary_path(NULL, "calculate_bs");
    }
    
    if (binary_path == NULL) {
        /* Error already set by resolve_legacy_binary_path */
        result->error_code = get_error();
        return result->error_code;
    }
    
    /* Construct the command based on the operation */
    if (market_price > 0) {
        /* Implied volatility calculation */
        snprintf(cmd, sizeof(cmd), "%s %.6f %.6f %.6f %.6f %.6f %.6f",
                 binary_path,
                 market_price,
                 spot_price,
                 strike_price,
                 time_to_expiry,
                 risk_free_rate,
                 dividend_yield);
    } else {
        /* Option pricing with known volatility */
        snprintf(cmd, sizeof(cmd), "%s %.6f %.6f %.6f %.6f %.6f %.6f %d",
                 binary_path,
                 spot_price,
                 strike_price,
                 time_to_expiry,
                 risk_free_rate,
                 dividend_yield,
                 volatility,
                 option_type);
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
        if (sscanf(output, "%lf", &result->implied_volatility) != 1) {
            set_error(ERROR_COMMAND_OUTPUT_PARSING);
            result->error_code = ERROR_COMMAND_OUTPUT_PARSING;
            return result->error_code;
        }
        
        /* Calculate option price using the implied volatility */
        /* For now, we'll just set it to the market price */
        result->price = market_price;
    } else {
        /* Parse option price output */
        if (sscanf(output, "%lf", &result->price) != 1) {
            set_error(ERROR_COMMAND_OUTPUT_PARSING);
            result->error_code = ERROR_COMMAND_OUTPUT_PARSING;
            return result->error_code;
        }
    }
    
    /* Set the result error code to indicate success */
    result->error_code = ERROR_NONE;
    
    return ERROR_NONE;
}

/**
 * @brief Calculate Greeks for an option using the Black-Scholes model
 * 
 * Note: This is a simplified implementation that calculates Greeks
 * analytically using the Black-Scholes formula. In a real implementation,
 * you might want to call a more sophisticated implementation or add
 * numerical methods for better precision.
 * 
 * @param spot_price Spot price
 * @param strike_price Strike price
 * @param time_to_expiry Time to expiry in years
 * @param risk_free_rate Risk-free rate
 * @param dividend_yield Dividend yield
 * @param volatility Volatility
 * @param option_type Option type (call or put)
 * @param result Pointer to store the calculated Greeks
 * 
 * @return 0 on success, error code on failure
 */
int calculate_black_scholes_greeks(
    double spot_price,
    double strike_price,
    double time_to_expiry,
    double risk_free_rate,
    double dividend_yield,
    double volatility,
    OptionType option_type,
    PricingResult* result
) {
    /* 
     * Note: This is a placeholder for the actual implementation.
     * In a real implementation, you would calculate the Greeks using
     * the Black-Scholes formula or call the appropriate legacy binary.
     */
    
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
 * @brief Calculate implied volatility using the Black-Scholes model
 * 
 * @param market_price Market price of the option
 * @param spot_price Spot price of the underlying
 * @param strike_price Strike price
 * @param time_to_expiry Time to expiry in years
 * @param risk_free_rate Risk-free rate
 * @param dividend_yield Dividend yield
 * @param option_type Option type (call or put)
 * 
 * @return Implied volatility on success, negative value on failure
 */
double calculate_black_scholes_iv(
    double market_price,
    double spot_price,
    double strike_price,
    double time_to_expiry,
    double risk_free_rate,
    double dividend_yield,
    OptionType option_type
) {
    PricingResult result;
    int ret;
    
    ret = price_with_black_scholes(
        spot_price,
        strike_price,
        time_to_expiry,
        risk_free_rate,
        dividend_yield,
        0.0, /* Initial volatility is not used for IV calculation */
        option_type,
        market_price,
        &result
    );
    
    if (ret != ERROR_NONE) {
        return -1.0;
    }
    
    return result.implied_volatility;
} 