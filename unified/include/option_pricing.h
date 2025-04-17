#ifndef OPTION_PRICING_H
#define OPTION_PRICING_H

#include "option_types.h"

/**
 * @file option_pricing.h
 * @brief Core API for the unified option pricing system
 */

/**
 * @brief Price an option using the specified model and method
 * 
 * @param spot_price The current price of the underlying asset
 * @param strike_price The strike price of the option
 * @param time_to_expiry Time to expiry in years
 * @param risk_free_rate Risk-free interest rate (annualized)
 * @param dividend_yield Dividend yield (annualized)
 * @param volatility Implied volatility (if known, 0 to calculate)
 * @param option_type Type of option (OPTION_CALL or OPTION_PUT)
 * @param model_type Pricing model to use
 * @param method Numerical method to use
 * @param market_price Market price (for implied volatility calculation, 0 to skip)
 * @param greeks_flags Flags indicating which Greeks to calculate
 * @param ticker_symbol Optional ticker symbol for data retrieval (NULL to skip)
 * @param result Pointer to a PricingResult structure to store the result
 * 
 * @return 0 on success, error code on failure
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
);

/**
 * @brief Calculate implied volatility for an option
 * 
 * @param market_price The observed market price of the option
 * @param spot_price The current price of the underlying asset
 * @param strike_price The strike price of the option
 * @param time_to_expiry Time to expiry in years
 * @param risk_free_rate Risk-free interest rate (annualized)
 * @param dividend_yield Dividend yield (annualized)
 * @param option_type Type of option (OPTION_CALL or OPTION_PUT)
 * @param model_type Pricing model to use
 * @param method Numerical method to use
 * 
 * @return Implied volatility on success, negative value on failure
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
);

/**
 * @brief Calculate Greeks for an option
 * 
 * @param spot_price The current price of the underlying asset
 * @param strike_price The strike price of the option
 * @param time_to_expiry Time to expiry in years
 * @param risk_free_rate Risk-free interest rate (annualized)
 * @param dividend_yield Dividend yield (annualized)
 * @param volatility Implied volatility
 * @param option_type Type of option (OPTION_CALL or OPTION_PUT)
 * @param model_type Pricing model to use
 * @param method Numerical method to use
 * @param greeks_flags Flags indicating which Greeks to calculate
 * @param result Pointer to a PricingResult structure to store the Greeks
 * 
 * @return 0 on success, error code on failure
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
);

/**
 * @brief Get market data for a ticker symbol
 * 
 * @param ticker_symbol The ticker symbol to lookup
 * @param spot_price Pointer to store the current price
 * @param dividend_yield Pointer to store the dividend yield
 * 
 * @return 0 on success, error code on failure
 */
int get_market_data(
    const char* ticker_symbol,
    double* spot_price,
    double* dividend_yield
);

#endif /* OPTION_PRICING_H */ 