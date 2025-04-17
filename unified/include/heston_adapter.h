#ifndef HESTON_ADAPTER_H
#define HESTON_ADAPTER_H

#include "option_types.h"

/**
 * @file heston_adapter.h
 * @brief Adapter for the legacy Heston model implementation
 */

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
);

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
);

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
);

#endif /* HESTON_ADAPTER_H */ 