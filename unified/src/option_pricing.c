#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../include/option_pricing.h"
#include "../include/error_handling.h"
#include "../include/path_resolution.h"
#include "../include/black_scholes_adapter.h"
#include "../include/heston_adapter.h"
#include "../include/market_data.h"

/**
 * Validate common input parameters
 */
static int validate_inputs(
    double spot_price,
    double strike_price,
    double time_to_expiry,
    double risk_free_rate,
    OptionType option_type,
    ModelType model_type,
    NumericalMethod method
) {
    /* Check for non-negative values where required */
    if (spot_price <= 0) {
        set_error(ERROR_INVALID_PARAMETER);
        return 0;
    }
    
    if (strike_price <= 0) {
        set_error(ERROR_INVALID_PARAMETER);
        return 0;
    }
    
    if (time_to_expiry <= 0) {
        set_error(ERROR_INVALID_PARAMETER);
        return 0;
    }
    
    /* Check for valid enums */
    if (option_type != OPTION_CALL && option_type != OPTION_PUT) {
        set_error(ERROR_INVALID_OPTION_TYPE);
        return 0;
    }
    
    if (model_type != MODEL_BLACK_SCHOLES && model_type != MODEL_HESTON) {
        set_error(ERROR_INVALID_MODEL_TYPE);
        return 0;
    }
    
    if (method != METHOD_ANALYTIC && method != METHOD_QUADRATURE && method != METHOD_FFT) {
        set_error(ERROR_INVALID_NUMERICAL_METHOD);
        return 0;
    }
    
    /* Check for method compatibility with model */
    if (model_type == MODEL_BLACK_SCHOLES && method != METHOD_ANALYTIC) {
        /* Black-Scholes only supports analytic method */
        set_error(ERROR_INVALID_NUMERICAL_METHOD);
        return 0;
    }
    
    if (model_type == MODEL_HESTON && method == METHOD_ANALYTIC) {
        /* Heston does not support analytic method */
        set_error(ERROR_INVALID_NUMERICAL_METHOD);
        return 0;
    }
    
    return 1; /* Validation passed */
}

/**
 * @brief Price an option using the specified model and method
 */
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
) {
    int ret;
    
    /* Initialize result structure */
    if (result == NULL) {
        set_error(ERROR_INVALID_PARAMETER);
        return ERROR_INVALID_PARAMETER;
    }
    
    result->price = 0;
    result->implied_volatility = 0;
    result->delta = 0;
    result->gamma = 0;
    result->theta = 0;
    result->vega = 0;
    result->rho = 0;
    result->error_code = 0;
    
    /* Validate input parameters */
    if (!validate_inputs(spot_price, strike_price, time_to_expiry, 
                         risk_free_rate, option_type, model_type, method)) {
        result->error_code = get_error();
        return result->error_code;
    }
    
    /* If ticker symbol is provided, fetch market data */
    if (ticker_symbol != NULL && *ticker_symbol) {
        double fetched_spot = 0.0;
        double fetched_div = 0.0;
        
        /* Try to get market data */
        if (get_market_data(ticker_symbol, &fetched_spot, &fetched_div) == ERROR_NONE) {
            /* Only use fetched data if it's valid */
            if (fetched_spot > 0) {
                spot_price = fetched_spot;
            }
            
            /* Only override if dividend_yield was not explicitly provided */
            if (dividend_yield == 0) {
                dividend_yield = fetched_div;
            }
        }
    }
    
    /* Dispatch to the appropriate model-specific function */
    switch (model_type) {
        case MODEL_BLACK_SCHOLES:
            /* For Black-Scholes, we can just use our adapter directly */
            ret = price_with_black_scholes(
                spot_price, strike_price, time_to_expiry,
                risk_free_rate, dividend_yield, volatility,
                option_type, market_price, result
            );
            
            /* Calculate Greeks if requested */
            if (ret == ERROR_NONE && 
                (greeks_flags.delta || greeks_flags.gamma || greeks_flags.theta || 
                 greeks_flags.vega || greeks_flags.rho)) {
                
                PricingResult greeks_result;
                ret = calculate_black_scholes_greeks(
                    spot_price, strike_price, time_to_expiry,
                    risk_free_rate, dividend_yield, 
                    result->implied_volatility > 0 ? result->implied_volatility : volatility,
                    option_type, &greeks_result
                );
                
                if (ret == ERROR_NONE) {
                    /* Copy the Greeks to the result */
                    result->delta = greeks_result.delta;
                    result->gamma = greeks_result.gamma;
                    result->theta = greeks_result.theta;
                    result->vega = greeks_result.vega;
                    result->rho = greeks_result.rho;
                } else {
                    /* Set error code but don't return, as pricing was successful */
                    result->error_code = ERROR_GREEKS_CALCULATION;
                }
            }
            break;
            
        case MODEL_HESTON:
            /* For Heston, use the Heston adapter */
            ret = price_with_heston(
                spot_price, strike_price, time_to_expiry,
                risk_free_rate, dividend_yield, volatility,
                option_type, method, market_price, result
            );
            
            /* Calculate Greeks if requested */
            if (ret == ERROR_NONE && 
                (greeks_flags.delta || greeks_flags.gamma || greeks_flags.theta || 
                 greeks_flags.vega || greeks_flags.rho)) {
                
                PricingResult greeks_result;
                ret = calculate_heston_greeks(
                    spot_price, strike_price, time_to_expiry,
                    risk_free_rate, dividend_yield, 
                    result->implied_volatility > 0 ? result->implied_volatility : volatility,
                    option_type, method, &greeks_result
                );
                
                if (ret == ERROR_NONE) {
                    /* Copy the Greeks to the result */
                    result->delta = greeks_result.delta;
                    result->gamma = greeks_result.gamma;
                    result->theta = greeks_result.theta;
                    result->vega = greeks_result.vega;
                    result->rho = greeks_result.rho;
                } else {
                    /* Set error code but don't return, as pricing was successful */
                    result->error_code = ERROR_GREEKS_CALCULATION;
                }
            }
            break;
            
        default:
            set_error(ERROR_INVALID_MODEL_TYPE);
            result->error_code = ERROR_INVALID_MODEL_TYPE;
            return ERROR_INVALID_MODEL_TYPE;
    }
    
    return ret;
}

/**
 * @brief Calculate implied volatility for an option
 */
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
) {
    PricingResult result;
    GreeksFlags no_greeks = {0}; /* Don't calculate Greeks */
    
    /* Validate input parameters */
    if (!validate_inputs(spot_price, strike_price, time_to_expiry, 
                         risk_free_rate, option_type, model_type, method)) {
        return -1; /* Error already set by validate_inputs */
    }
    
    if (market_price <= 0) {
        set_error(ERROR_INVALID_PARAMETER);
        return -1;
    }
    
    /* Call the main pricing function with market_price set */
    if (price_option(
            spot_price, strike_price, time_to_expiry,
            risk_free_rate, dividend_yield, 0, /* Initial volatility guess of 0 */
            option_type, model_type, method,
            market_price, no_greeks, NULL, &result) != 0) {
        return -1; /* Error already set by price_option */
    }
    
    return result.implied_volatility;
}

/**
 * @brief Calculate Greeks for an option
 */
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
) {
    /* Call the main pricing function without market_price */
    return price_option(
        spot_price, strike_price, time_to_expiry,
        risk_free_rate, dividend_yield, volatility,
        option_type, model_type, method,
        0, /* No market price provided */
        greeks_flags, NULL, result
    );
}

/**
 * @brief Get market data for a ticker symbol
 */
int get_market_data(
    const char* ticker_symbol,
    double* spot_price,
    double* dividend_yield
) {
    int error_code = ERROR_NONE;
    
    /* Initialize pointers with default values */
    if (spot_price) {
        *spot_price = 0.0;
    }
    
    if (dividend_yield) {
        *dividend_yield = 0.0;
    }
    
    /* Validate input */
    if (ticker_symbol == NULL || *ticker_symbol == '\0') {
        set_error(ERROR_INVALID_PARAMETER);
        return ERROR_INVALID_PARAMETER;
    }
    
    /* Get spot price if requested */
    if (spot_price) {
        double price = get_current_price(ticker_symbol, DATA_SOURCE_DEFAULT, &error_code);
        if (error_code == ERROR_SUCCESS && price > 0) {
            *spot_price = price;
        } else if (error_code != ERROR_SUCCESS) {
            set_error(error_code);
            return error_code;
        }
    }
    
    /* Get dividend yield if requested */
    if (dividend_yield) {
        double yield = get_dividend_yield(ticker_symbol, DATA_SOURCE_DEFAULT, &error_code);
        if (error_code == ERROR_SUCCESS && yield >= 0) {
            *dividend_yield = yield;
        } else if (error_code != ERROR_SUCCESS) {
            /* Only return an error if we don't already have a spot price */
            if (*spot_price <= 0) {
                set_error(error_code);
                return error_code;
            }
        }
    }
    
    /* If we have at least a spot price, consider it a success */
    if (spot_price && *spot_price > 0) {
        return ERROR_NONE;
    }
    
    /* No data could be retrieved */
    set_error(ERROR_DATA_SOURCE_UNAVAILABLE);
    return ERROR_DATA_SOURCE_UNAVAILABLE;
}

/* 
 * Temporary stub implementation for Heston model 
 * This will be replaced with an adapter to the legacy Heston implementation
 */
static int price_with_heston(
    double spot_price,
    double strike_price,
    double time_to_expiry,
    double risk_free_rate,
    double dividend_yield,
    double volatility,
    OptionType option_type,
    NumericalMethod method,
    double market_price,
    GreeksFlags greeks_flags,
    PricingResult* result
) {
    /* This is a placeholder that will be replaced with actual implementation */
    result->error_code = ERROR_CALCULATION_FAILED;
    set_error(ERROR_CALCULATION_FAILED);
    return ERROR_CALCULATION_FAILED;
} 