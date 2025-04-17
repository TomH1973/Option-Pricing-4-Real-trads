#ifndef OPTION_TYPES_H
#define OPTION_TYPES_H

/**
 * @file option_types.h
 * @brief Core data structures and enumerations for the unified option pricing system
 */

/**
 * @brief Enumeration for option types
 */
typedef enum {
    OPTION_CALL = 0,    /**< Call option */
    OPTION_PUT = 1      /**< Put option */
} OptionType;

/**
 * @brief Enumeration for pricing models
 */
typedef enum {
    MODEL_BLACK_SCHOLES = 0,   /**< Black-Scholes model */
    MODEL_HESTON = 1,          /**< Heston stochastic volatility model */
    MODEL_DEFAULT = MODEL_BLACK_SCHOLES  /**< Default model */
} ModelType;

/**
 * @brief Enumeration for numerical methods
 */
typedef enum {
    METHOD_ANALYTIC = 0,    /**< Analytic solution (BS only) */
    METHOD_QUADRATURE = 1,  /**< Quadrature-based integration */
    METHOD_FFT = 2,         /**< Fast Fourier Transform */
    METHOD_DEFAULT = METHOD_ANALYTIC  /**< Default method */
} NumericalMethod;

/**
 * @brief Bit flags for Greeks calculation
 */
typedef struct {
    unsigned int delta: 1;   /**< Calculate delta */
    unsigned int gamma: 1;   /**< Calculate gamma */
    unsigned int theta: 1;   /**< Calculate theta */
    unsigned int vega: 1;    /**< Calculate vega */
    unsigned int rho: 1;     /**< Calculate rho */
} GreeksFlags;

/**
 * @brief Result structure for option pricing
 */
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

#endif /* OPTION_TYPES_H */ 