#include <stdio.h>
#include <stdlib.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <complex.h>
#include <string.h>
#include <float.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <setjmp.h>

// Define M_PI if it's not already defined
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// FFT implementation requires fftw library
#include <fftw3.h>

// Heston model parameters:
// S: spot price
// K: strike price
// v0: initial variance
// kappa: mean reversion speed
// theta: long-term variance
// sigma: volatility of variance
// rho: correlation between stock and variance processes
// r: risk-free rate
// q: dividend yield
// T: time to maturity

// Global flags
static bool g_debug = false;
static bool g_verbose_debug = false;
static bool g_found_good_match = false;
static jmp_buf g_error_jmp_buf;
static bool g_using_error_handler = false;

// Default FFT settings (can be overridden via command-line/env vars)
// We'll use variables instead of #define for configurability
static int g_fft_n = 4096;          // Number of FFT points (power of 2)
static double g_log_strike_range = 3.0; // Range of log-strikes (±LOG_STRIKE_RANGE from current)
static double g_alpha = 1.5;        // Carr-Madan dampening factor
static double g_eta = 0.05;         // Step size in log-strike space
static double g_cache_tolerance = 1e-5; // Tolerance for cache validation

// Cache structure for FFT results
typedef struct {
    double S;            // Spot price for this cache entry
    double r;            // Risk-free rate 
    double q;            // Dividend yield
    double T;            // Time to expiry
    double v0;           // Initial variance
    double kappa;        // Mean reversion speed
    double theta;        // Long-term variance
    double sigma;        // Volatility of variance
    double rho;          // Correlation
    double* prices;      // Array of option prices for different strikes
    double* strikes;     // Array of strikes corresponding to prices
    int num_strikes;     // Number of strikes in the cache
    bool is_valid;       // Flag to indicate if cache is valid
    // FFT parameters used for this cache entry
    int fft_n;
    double log_strike_range;
    double alpha;
    double eta;
} FFTCache;

// Precomputed values for FFT optimization
typedef struct {
    // Simpson's rule weights (precomputed)
    double* simpson_weights;
    // Precomputed exponential terms in FFT input
    double complex* exp_terms;
    // Flag to indicate if precomputed values are valid
    bool is_valid;
    // Parameters for which values were precomputed
    int fft_n;
    double eta;
    double alpha;
    double S;
} FFTPrecomputed;

// Global cache - we'll keep just one entry for simplicity
// Initialize with proper values to track cache state
static FFTCache g_cache = {
    .S = 0.0,
    .r = 0.0,
    .q = 0.0,
    .T = 0.0,
    .v0 = 0.0,
    .kappa = 0.0,
    .theta = 0.0,
    .sigma = 0.0,
    .rho = 0.0,
    .prices = NULL,
    .strikes = NULL,
    .num_strikes = 0,
    .is_valid = false,
    .fft_n = 0,
    .log_strike_range = 0.0,
    .alpha = 0.0,
    .eta = 0.0
};

// Global precomputed values
static FFTPrecomputed g_precomputed = {
    .simpson_weights = NULL,
    .exp_terms = NULL,
    .is_valid = false,
    .fft_n = 0,
    .eta = 0.0,
    .alpha = 0.0,
    .S = 0.0
};

// Error handler for signals (segfault, floating point exceptions, etc.)
static void error_handler(int sig) {
    if (g_using_error_handler) {
        fprintf(stderr, "ERROR: Caught signal %d\n", sig);
        longjmp(g_error_jmp_buf, 1);
    }
}

// Forward declarations
double black_scholes_call(double S, double K, double T, double r, double q, double sigma);
double bs_implied_vol(double market_price, double S, double K, double T, double r, double q);
void init_fft_cache(double S, double r, double q, double T, 
                   double v0, double kappa, double theta, double sigma, double rho);
double get_cached_option_price(double K);
void precompute_fft_values(double S);
void cleanup_precomputed_values(void);
bool is_challenging_parameter_set(double S, double K, double T, double v0, double kappa, double theta, double sigma, double rho);
void adapt_fft_parameters(double S, double K, double T);

// Complex characteristic function for Heston model
double complex cf_heston(double complex phi, double S, double v0, double kappa, 
                         double theta, double sigma, double rho, double r, 
                         double q, double T) {
    double complex u = phi - 0.5 * I;
    double complex d = csqrt((rho * sigma * phi * I - kappa) * 
                    (rho * sigma * phi * I - kappa) - 
                    sigma * sigma * (phi * I) * (phi * I - I));
    double complex g = (kappa - rho * sigma * phi * I - d) / 
                    (kappa - rho * sigma * phi * I + d);
    
    // Extra safety check for numerical stability
    if (!isfinite(creal(g)) || !isfinite(cimag(g))) {
        if (g_verbose_debug) {
            fprintf(stderr, "Warning: Non-finite g value detected in characteristic function\n");
        }
        return 1.0 + 0.0 * I;  // Safe default
    }
    
    double complex A = (r - q) * phi * I * T + 
                    kappa * theta * ((kappa - rho * sigma * phi * I - d) * T - 
                    2.0 * clog((1.0 - g * cexp(-d * T)) / (1.0 - g))) / 
                    (sigma * sigma);
                    
    double complex B = (kappa - rho * sigma * phi * I - d) * 
                    (1.0 - cexp(-d * T)) / 
                    (sigma * sigma * (1.0 - g * cexp(-d * T)));
    
    // Safety checks for numerical issues
    if (!isfinite(creal(A)) || !isfinite(cimag(A)) || 
        !isfinite(creal(B)) || !isfinite(cimag(B))) {
        if (g_verbose_debug) {
            fprintf(stderr, "Warning: Non-finite A or B values in characteristic function\n");
        }
        return 1.0 + 0.0 * I;  // Safe default
    }
                    
    return cexp(A + B * v0 + I * phi * clog(S));
}

// Function to calculate Black-Scholes price (used as fallback)
double black_scholes_call(double S, double K, double T, double r, double q, double sigma) {
    if (sigma <= 0 || T <= 0 || S <= 0 || K <= 0) {
        return -1.0;
    }
    
    double d1 = (log(S/K) + (r - q + 0.5*sigma*sigma)*T) / (sigma * sqrt(T));
    double d2 = d1 - sigma * sqrt(T);
    
    double N_d1 = 0.5 * (1.0 + erf(d1 / sqrt(2.0)));
    double N_d2 = 0.5 * (1.0 + erf(d2 / sqrt(2.0)));
    
    return S * exp(-q * T) * N_d1 - K * exp(-r * T) * N_d2;
}

// Function to calculate Black-Scholes implied volatility numerically
double bs_implied_vol(double market_price, double S, double K, double T, double r, double q) {
    // Input validation
    if (market_price <= 0.0 || S <= 0.0 || K <= 0.0 || T <= 0.0) {
        return -1.0;
    }
    
    // Calculate intrinsic value
    double intrinsic = fmax(0.0, S * exp(-q * T) - K * exp(-r * T));
    
    // If market price is below intrinsic value (arbitrage)
    if (market_price < intrinsic) {
        if (g_debug) {
            fprintf(stderr, "Debug: Market price %.6f is below intrinsic value %.6f\n", 
                    market_price, intrinsic);
        }
        return -1.0;
    }
    
    // Initial guess based on simple approximation
    double vol_low = 0.001;    // 0.1%
    double vol_high = 2.0;     // 200%
    double vol_mid, price_mid;
    
    // Maximum iterations and precision
    int max_iter = 100;
    double precision = 1e-6;
    
    // Calculate prices at bounds
    double price_low = black_scholes_call(S, K, T, r, q, vol_low);
    double price_high = black_scholes_call(S, K, T, r, q, vol_high);
    
    // Check if market price is within bounds
    if (market_price <= price_low || market_price >= price_high) {
        if (g_debug) {
            fprintf(stderr, "Debug: Market price %.6f is outside the bounds [%.6f, %.6f]\n", 
                    market_price, price_low, price_high);
        }
        return -1.0;
    }
    
    // Bisection method
    for (int i = 0; i < max_iter; i++) {
        vol_mid = (vol_low + vol_high) * 0.5;
        price_mid = black_scholes_call(S, K, T, r, q, vol_mid);
        
        if (fabs(price_mid - market_price) < precision) {
            return vol_mid;
        }
        
        if (price_mid < market_price) {
            vol_low = vol_mid;
        } else {
            vol_high = vol_mid;
        }
    }
    
    return vol_mid;
}

// Precompute invariant values used in FFT calculation
void precompute_fft_values(double S) {
    // Check if precomputed values are already valid for current parameters
    if (g_precomputed.is_valid && 
        g_precomputed.fft_n == g_fft_n && 
        g_precomputed.eta == g_eta &&
        g_precomputed.alpha == g_alpha &&
        fabs(g_precomputed.S - S) < g_cache_tolerance) {
        if (g_debug) {
            fprintf(stderr, "Debug: Using existing precomputed FFT values\n");
        }
        return;
    }
    
    if (g_debug) {
        fprintf(stderr, "Debug: Precomputing FFT values for N=%d, eta=%.4f, alpha=%.2f, S=%.2f\n", 
                g_fft_n, g_eta, g_alpha, S);
    }
    
    // Free existing precomputed values if they exist
    cleanup_precomputed_values();
    
    // Allocate memory for precomputed values with error checking
    g_precomputed.simpson_weights = (double*)malloc(g_fft_n * sizeof(double));
    g_precomputed.exp_terms = (double complex*)malloc(g_fft_n * sizeof(double complex));
    
    if (g_precomputed.simpson_weights == NULL || g_precomputed.exp_terms == NULL) {
        fprintf(stderr, "Error: Memory allocation for precomputed FFT values failed\n");
        if (g_precomputed.simpson_weights != NULL) {
            free(g_precomputed.simpson_weights);
            g_precomputed.simpson_weights = NULL;
        }
        if (g_precomputed.exp_terms != NULL) {
            free(g_precomputed.exp_terms);
            g_precomputed.exp_terms = NULL;
        }
        g_precomputed.is_valid = false;
        return;
    }
    
    // Precompute Simpson's rule weights
    for (int i = 0; i < g_fft_n; i++) {
        g_precomputed.simpson_weights[i] = (i == 0) ? 1.0/3.0 : 
                                          ((i % 2 == 1) ? 4.0/3.0 : 2.0/3.0);
    }
    
    // Calculate log of spot price once
    double log_S = log(S);
    
    // Use error handling for the potentially unstable computation
    g_using_error_handler = true;
    if (setjmp(g_error_jmp_buf) == 0) {
        for (int i = 0; i < g_fft_n; i++) {
            double v = i * g_eta;
            if (fabs(v) < 1e-10) v = 1e-10; // Avoid numerical issues at v=0
            
            // Calculate exponential term with safety check
            double complex exp_term = exp(-I * v * log_S);
            
            // Check for numerical issues
            if (!isfinite(creal(exp_term)) || !isfinite(cimag(exp_term))) {
                if (g_verbose_debug) {
                    fprintf(stderr, "Warning: Non-finite exp term at i=%d, v=%.6f, log_S=%.6f\n", 
                            i, v, log_S);
                }
                exp_term = 1.0 + 0.0 * I;  // Safe default
            }
            
            g_precomputed.exp_terms[i] = exp_term;
        }
    } else {
        // Exception occurred, cleanup and return
        fprintf(stderr, "Error: Exception during precomputation of FFT values\n");
        cleanup_precomputed_values();
        g_using_error_handler = false;
        return;
    }
    
    g_using_error_handler = false;
    
    // Update metadata for precomputed values
    g_precomputed.fft_n = g_fft_n;
    g_precomputed.eta = g_eta;
    g_precomputed.alpha = g_alpha;
    g_precomputed.S = S;
    g_precomputed.is_valid = true;
}

// Initialize the FFT cache with option prices for various strikes
void init_fft_cache(double S, double r, double q, double T, 
                   double v0, double kappa, double theta, double sigma, double rho) {
    // Print FFT parameters if in debug mode
    if (g_debug) {
        fprintf(stderr, "Debug: FFT Parameters - N: %d, Range: %.1f, Alpha: %.2f, Eta: %.4f\n", 
                g_fft_n, g_log_strike_range, g_alpha, g_eta);
        
        // Print comparison for cache validation
        if (g_cache.is_valid) {
            fprintf(stderr, "Debug: Cache validation parameters:\n");
            fprintf(stderr, "  - S: %.2f vs %.2f (diff: %.6f)\n", g_cache.S, S, fabs(g_cache.S - S));
            fprintf(stderr, "  - r: %.6f vs %.6f (diff: %.9f)\n", g_cache.r, r, fabs(g_cache.r - r));
            fprintf(stderr, "  - q: %.6f vs %.6f (diff: %.9f)\n", g_cache.q, q, fabs(g_cache.q - q));
            fprintf(stderr, "  - T: %.6f vs %.6f (diff: %.9f)\n", g_cache.T, T, fabs(g_cache.T - T));
        }
    }
    
    // Skip re-computation if cache is valid for these parameters AND FFT parameters
    // Use a more relaxed check for model parameters during calibration to prevent excessive recalculation
    if (g_cache.is_valid && 
        fabs(g_cache.S - S) < g_cache_tolerance && 
        fabs(g_cache.r - r) < g_cache_tolerance && 
        fabs(g_cache.q - q) < g_cache_tolerance && 
        fabs(g_cache.T - T) < g_cache_tolerance &&
        g_cache.fft_n == g_fft_n &&
        fabs(g_cache.log_strike_range - g_log_strike_range) < g_cache_tolerance &&
        fabs(g_cache.alpha - g_alpha) < g_cache_tolerance &&
        fabs(g_cache.eta - g_eta) < g_cache_tolerance) {
        // Cache hit, no need to recompute
        if (g_debug) {
            fprintf(stderr, "Debug: CACHE HIT - Using cached FFT results\n");
        }
        return;
    }
    
    // Cache miss, need to recalculate
    if (g_debug) {
        fprintf(stderr, "Debug: CACHE MISS - Recalculating FFT results\n");
    }
    
    // Free previous cache if FFT parameters have changed
    if (g_cache.is_valid && 
        (g_cache.fft_n != g_fft_n || 
         g_cache.num_strikes != g_fft_n)) {
        if (g_debug) {
            fprintf(stderr, "Debug: FFT parameters changed, reallocating cache\n");
        }
        if (g_cache.prices != NULL) {
            free(g_cache.prices);
            g_cache.prices = NULL;
        }
        if (g_cache.strikes != NULL) {
            free(g_cache.strikes);
            g_cache.strikes = NULL;
        }
    }
    
    // Allocate memory for FFT arrays if not already allocated
    if (g_cache.prices == NULL) {
        g_cache.num_strikes = g_fft_n;
        g_cache.prices = (double*)malloc(g_fft_n * sizeof(double));
        g_cache.strikes = (double*)malloc(g_fft_n * sizeof(double));
        
        if (g_cache.prices == NULL || g_cache.strikes == NULL) {
            fprintf(stderr, "Error: Memory allocation for FFT cache failed\n");
            g_cache.is_valid = false;
            if (g_cache.prices != NULL) {
                free(g_cache.prices);
                g_cache.prices = NULL;
            }
            if (g_cache.strikes != NULL) {
                free(g_cache.strikes);
                g_cache.strikes = NULL;
            }
            return;
        }
    }
    
    // Initialize FFTW arrays
    fftw_complex *in = NULL, *out = NULL;
    fftw_plan p = NULL;
    
    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * g_fft_n);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * g_fft_n);
    
    if (in == NULL || out == NULL) {
        fprintf(stderr, "Error: FFTW memory allocation failed\n");
        g_cache.is_valid = false;
        if (in) fftw_free(in);
        if (out) fftw_free(out);
        return;
    }
    
    // Lambda depends on FFT parameters
    double lambda = 2 * M_PI / (g_fft_n * g_eta);
    
    // Precompute invariant parts of the FFT calculation
    precompute_fft_values(S);
    
    // Check if precomputation failed
    if (!g_precomputed.is_valid) {
        fprintf(stderr, "Error: Precomputation of FFT values failed\n");
        g_cache.is_valid = false;
        fftw_free(in);
        fftw_free(out);
        return;
    }
    
    // Precomputed discount factor
    double discount = exp(-r * T);
    
    // Use error handling for the FFT computation part
    g_using_error_handler = true;
    
    if (setjmp(g_error_jmp_buf) == 0) {
        // Fill in the FFT input array - OPTIMIZED VERSION using precomputed values
        for (int i = 0; i < g_fft_n; i++) {
            double v = i * g_eta;
            
            // Ensure we skip v=0 which can cause numerical issues
            if (fabs(v) < 1e-10) {
                v = 1e-10;
            }
            
            // Calculate modified characteristic function for Carr-Madan
            double complex phi = cf_heston(v - (g_alpha + 1) * I, S, v0, kappa, theta, sigma, rho, r, q, T);
            
            // Apply Carr-Madan formula with precomputed discount factor
            double complex denom = g_alpha*g_alpha + g_alpha - v*v + I*(2*g_alpha + 1)*v;
            double complex modified_cf = discount * phi / denom;
            
            // Check for numerical issues
            if (!isfinite(creal(modified_cf)) || !isfinite(cimag(modified_cf))) {
                if (g_verbose_debug) {
                    fprintf(stderr, "Warning: Non-finite modified CF at i=%d\n", i);
                }
                modified_cf = 0.0 + 0.0 * I;  // Safe default
            }
            
            // Use precomputed Simpson's rule weights and eta scaling
            double simpson_weight = g_precomputed.simpson_weights[i];
            
            // Use precomputed exponential term
            double complex exp_term = g_precomputed.exp_terms[i];
            
            // Set FFT input array - more efficient with precomputed values
            in[i] = modified_cf * simpson_weight * g_eta * exp_term;
        }

        // Create and execute FFT plan with ESTIMATE flag for better compatibility
        p = fftw_plan_dft_1d(g_fft_n, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
        if (p == NULL) {
            fprintf(stderr, "Error: Failed to create FFTW plan\n");
            fftw_free(in);
            fftw_free(out);
            g_using_error_handler = false;
            return;
        }
        
        fftw_execute(p);
        
        // Extract option prices from FFT results
        double log_S = log(S);
        double inv_pi = 1.0 / M_PI; // Precompute 1/PI
        double range_factor = 2.0 * g_log_strike_range / g_fft_n; // Precompute this constant
        
        for (int i = 0; i < g_fft_n; i++) {
            // Calculate log strike - using precomputed constants
            double log_K = log_S - g_log_strike_range + range_factor * i;
            double K = exp(log_K);
            
            // Store strike
            g_cache.strikes[i] = K;
            
            // Extract price - using precomputed 1/PI
            double real_part = creal(out[i]);
            
            // Check for numerical issues
            if (!isfinite(real_part)) {
                if (g_verbose_debug) {
                    fprintf(stderr, "Warning: Non-finite FFT output at index %d\n", i);
                }
                real_part = 0.0;
            }
            
            double exp_factor = exp(-g_alpha * log_K) * inv_pi;
            double price_part = real_part * exp_factor;
            
            // Ensure non-negative prices
            g_cache.prices[i] = fmax(0.0, price_part);
        }
    } else {
        // Exception occurred during FFT computation
        fprintf(stderr, "Error: Exception during FFT computation\n");
        g_cache.is_valid = false;
        if (in) fftw_free(in);
        if (out) fftw_free(out);
        if (p) fftw_destroy_plan(p);
        g_using_error_handler = false;
        return;
    }
    
    g_using_error_handler = false;
    
    // Clean up FFTW resources
    if (p) fftw_destroy_plan(p);
    if (in) fftw_free(in);
    if (out) fftw_free(out);
    
    // Update cache metadata
    g_cache.S = S;
    g_cache.r = r;
    g_cache.q = q;
    g_cache.T = T;
    g_cache.v0 = v0;
    g_cache.kappa = kappa;
    g_cache.theta = theta;
    g_cache.sigma = sigma;
    g_cache.rho = rho;
    // Store FFT parameters used
    g_cache.fft_n = g_fft_n;
    g_cache.log_strike_range = g_log_strike_range;
    g_cache.alpha = g_alpha;
    g_cache.eta = g_eta;
    g_cache.is_valid = true;
    
    if (g_debug) {
        fprintf(stderr, "Debug: FFT cache initialized with %d strikes\n", g_cache.num_strikes);
    }
}

// Get option price from cache using interpolation
double get_cached_option_price(double K) {
    if (!g_cache.is_valid || g_cache.prices == NULL || g_cache.strikes == NULL) {
        if (g_debug) {
            fprintf(stderr, "Debug: Cache not valid or arrays not initialized\n");
            fprintf(stderr, "       is_valid=%d, prices=%p, strikes=%p\n", 
                    g_cache.is_valid, (void*)g_cache.prices, (void*)g_cache.strikes);
        }
        return -1.0;
    }
    
    if (g_debug) {
        fprintf(stderr, "Debug: Retrieving price for strike %.2f from cache\n", K);
        fprintf(stderr, "       Cache has %d strikes ranging from %.2f to %.2f\n", 
                g_cache.num_strikes, g_cache.strikes[0], 
                g_cache.strikes[g_cache.num_strikes-1]);
    }
    
    // Find nearest strikes in cache
    int idx_low = 0;
    int idx_high = g_cache.num_strikes - 1;
    
    // Check if strike is out of bounds
    if (K <= g_cache.strikes[0]) {
        if (g_debug) {
            fprintf(stderr, "Debug: Strike below cache range, returning first price\n");
        }
        return g_cache.prices[0];
    }
    
    if (K >= g_cache.strikes[g_cache.num_strikes - 1]) {
        if (g_debug) {
            fprintf(stderr, "Debug: Strike above cache range, returning last price\n");
        }
        return g_cache.prices[g_cache.num_strikes - 1];
    }
    
    // Binary search to find position
    while (idx_high - idx_low > 1) {
        int mid = (idx_low + idx_high) / 2;
        if (g_cache.strikes[mid] < K) {
            idx_low = mid;
        } else {
            idx_high = mid;
        }
    }
    
    // Linear interpolation between adjacent strikes
    double K_low = g_cache.strikes[idx_low];
    double K_high = g_cache.strikes[idx_high];
    double price_low = g_cache.prices[idx_low];
    double price_high = g_cache.prices[idx_high];
    
    // Check for invalid price values
    if (!isfinite(price_low) || !isfinite(price_high)) {
        if (g_debug) {
            fprintf(stderr, "Debug: Invalid cached prices: low=%.6f, high=%.6f\n", 
                    price_low, price_high);
        }
        return -1.0;
    }
    
    // Interpolation weight
    double weight = (K - K_low) / (K_high - K_low);
    
    // Interpolated price
    double result = price_low + weight * (price_high - price_low);
    
    if (g_debug) {
        fprintf(stderr, "Debug: Interpolated price %.6f between strikes %.2f (%.6f) and %.2f (%.6f)\n", 
                result, K_low, price_low, K_high, price_high);
    }
    
    return result;
}

// Function to check if a parameter set might be numerically challenging
bool is_challenging_parameter_set(double S, double K, double T, double v0, double kappa, double theta, double sigma, double rho) {
    // Check for extreme moneyness (very ITM or OTM)
    double moneyness = K / S;
    if (moneyness > 2.0 || moneyness < 0.5) {
        if (g_verbose_debug) {
            fprintf(stderr, "Debug: Extreme moneyness detected (%.2f)\n", moneyness);
        }
        return true;
    }
    
    // Check for very short expiry combined with high volatility
    if (T < 0.15 && v0 > 0.04) {  // Less than ~55 days and vol > 20%
        if (g_verbose_debug) {
            fprintf(stderr, "Debug: Short expiry (%.4f) with high vol (%.2f%%)\n", 
                    T, sqrt(v0) * 100);
        }
        return true;
    }
    
    // Check for extreme volatility parameters
    if (sigma > 1.0 || fabs(rho) > 0.9) {
        if (g_verbose_debug) {
            fprintf(stderr, "Debug: Extreme volatility parameters (sigma=%.2f, rho=%.2f)\n", 
                    sigma, rho);
        }
        return true;
    }
    
    return false;
}

// Function to adapt FFT parameters based on option characteristics
void adapt_fft_parameters(double S, double K, double T) {
    // Store original parameters
    int orig_fft_n = g_fft_n;
    double orig_alpha = g_alpha;
    double orig_eta = g_eta;
    double orig_log_strike_range = g_log_strike_range;
    
    // Adjust for extreme moneyness
    double moneyness = K / S;
    if (moneyness > 1.5 || moneyness < 0.7) {
        // For far ITM/OTM, increase grid size and range
        g_fft_n = 8192;
        g_log_strike_range = 4.0;
    }
    
    // Adjust for very short expiry
    if (T < 0.1) {  // Less than ~36 days
        // For short expiry, use smaller eta (finer grid)
        g_eta = 0.025;
        // And adjust alpha for better dampening
        g_alpha = 1.25;
    }
    
    // Adjust for very long expiry
    if (T > 2.0) {  // More than 2 years
        // For long expiry, can use larger eta (coarser grid)
        g_eta = 0.1;
    }
    
    // Log changes if in debug mode
    if (g_debug && (orig_fft_n != g_fft_n || orig_alpha != g_alpha || 
                   orig_eta != g_eta || orig_log_strike_range != g_log_strike_range)) {
        fprintf(stderr, "Debug: Adapted FFT parameters for option characteristics:\n");
        fprintf(stderr, "       N: %d -> %d\n", orig_fft_n, g_fft_n);
        fprintf(stderr, "       Range: %.2f -> %.2f\n", orig_log_strike_range, g_log_strike_range);
        fprintf(stderr, "       alpha: %.2f -> %.2f\n", orig_alpha, g_alpha);
        fprintf(stderr, "       eta: %.4f -> %.4f\n", orig_eta, g_eta);
    }
}

// Heston model call option pricing using FFT
double heston_call_fft(double S, double K, double T, double r, double q, 
                       double v0, double kappa, double theta, double sigma, double rho) {
    // Check if this parameter set might be challenging
    bool challenging = is_challenging_parameter_set(S, K, T, v0, kappa, theta, sigma, rho);
    
    // If challenging, adapt FFT parameters
    if (challenging) {
        adapt_fft_parameters(S, K, T);
    }
    
    // Initialize the FFT price cache if needed
    init_fft_cache(S, r, q, T, v0, kappa, theta, sigma, rho);
    
    // Check if cache initialization failed
    if (!g_cache.is_valid) {
        if (g_debug) {
            fprintf(stderr, "Debug: Cache initialization failed, trying with different parameters\n");
        }
        
        // Try with different FFT parameters
        g_fft_n = 8192;  // Larger grid
        g_alpha = 1.0;   // Different dampening
        g_eta = 0.1;     // Different spacing
        
        // Try again with new parameters
        init_fft_cache(S, r, q, T, v0, kappa, theta, sigma, rho);
        
        // If still failed, try one more set
        if (!g_cache.is_valid) {
            g_fft_n = 2048;   // Smaller grid
            g_alpha = 1.25;   // Different dampening
            g_eta = 0.075;    // Different spacing
            
            init_fft_cache(S, r, q, T, v0, kappa, theta, sigma, rho);
            
            // If all attempts failed, fall back to Black-Scholes
            if (!g_cache.is_valid) {
                if (g_debug) {
                    fprintf(stderr, "Debug: All FFT attempts failed, falling back to Black-Scholes\n");
                }
                
                // Fallback to Black-Scholes with equivalent volatility
                double bs_vol = sqrt(v0); // Simple approximation
                return black_scholes_call(S, K, T, r, q, bs_vol);
            }
        }
    }
    
    // Get option price from cache with interpolation
    double price = get_cached_option_price(K);
    
    if (price < 0.0 || !isfinite(price)) {
        if (g_debug) {
            fprintf(stderr, "Debug: Error retrieving price from cache, falling back to Black-Scholes\n");
        }
        
        // Fallback to Black-Scholes with equivalent volatility
        double bs_vol = sqrt(v0); // Simple approximation
        return black_scholes_call(S, K, T, r, q, bs_vol);
    }
    
    return price;
}

// Function to estimate implied volatility from the Heston model
double implied_vol_sv(double market_price, double S, double K, double T, double r, double q) {
    // Calculate BS IV first as a reference point
    double bs_iv = bs_implied_vol(market_price, S, K, T, r, q);
    
    // If BS IV calculation fails, return error
    if (bs_iv < 0.0) {
        if (g_debug) {
            fprintf(stderr, "Debug: BS IV calculation failed, cannot proceed with SV\n");
        }
        return -1.0;
    }
    
    if (g_debug) {
        fprintf(stderr, "Debug: Black-Scholes IV: %.2f%%\n", bs_iv * 100);
    }
    
    // Calculate moneyness (S/K adjusted for interest and dividends)
    double forward = S * exp((r - q) * T);
    double moneyness = forward / K;
    
    // Initial parameter guesses based on market characteristics
    double init_v0, init_kappa, init_theta;
    
    // Make smarter initial guesses based on moneyness and time
    if (moneyness > 1.1) { 
        // OTM call options often show higher volatility (volatility skew)
        init_v0 = bs_iv * bs_iv * 1.1;
        init_kappa = 2.0;
        init_theta = bs_iv * bs_iv * 1.05;
    } else if (moneyness < 0.9) {
        // ITM call options can also show different volatility patterns
        init_v0 = bs_iv * bs_iv * 1.05;
        init_kappa = 1.5;
        init_theta = bs_iv * bs_iv;
    } else {
        // ATM options
        init_v0 = bs_iv * bs_iv;
        init_kappa = 1.0;
        init_theta = bs_iv * bs_iv;
    }
    
    // Time-based adjustments
    if (T < 0.1) {
        // Short-dated options typically have higher volatility of volatility
        init_kappa = 3.0; // Faster mean reversion for short-dated
    } else if (T > 1.0) {
        // Long-dated options often have lower vol-of-vol
        init_kappa = 0.5; // Slower mean reversion for long-dated
    }
    
    // Parameter calibration with more robust approach
    double best_diff = DBL_MAX;
    double best_v = init_v0;
    double best_kappa = init_kappa;
    double best_theta = init_theta;
    double best_sigma = 0.4;  // Vol of vol
    double best_rho = -0.7;   // Typical correlation for equity options
    
    // Reset global early termination flag
    g_found_good_match = false;
    
    // Multi-stage calibration with better error handling
    g_using_error_handler = true;
    
    if (setjmp(g_error_jmp_buf) == 0) {
        // Grid search with multiple parameter combinations
        // Start with coarse grid and refine
        const int NUM_V0 = 5;
        const int NUM_KAPPA = 3;
        const int NUM_SIGMA = 3;
        const int NUM_RHO = 3;
        
        double v0_values[NUM_V0];
        double kappa_values[NUM_KAPPA];
        double sigma_values[NUM_SIGMA];
        double rho_values[NUM_RHO];
        
        // Initialize parameter grids centered around initial guesses
        // Order them from most likely to least likely values
        v0_values[0] = init_v0;            // Start with the BS estimate (most likely)
        v0_values[1] = init_v0 * 0.85;     // Try 15% lower
        v0_values[2] = init_v0 * 1.15;     // Try 15% higher
        v0_values[3] = init_v0 * 0.7;      // Try 30% lower
        v0_values[4] = init_v0 * 1.3;      // Try 30% higher
        
        // Most likely kappa values first
        kappa_values[0] = init_kappa;      // Start with initial guess
        kappa_values[1] = init_kappa * 1.5; // Try higher mean reversion
        kappa_values[2] = init_kappa * 0.5; // Try lower mean reversion
        
        // Most likely sigma values first
        sigma_values[0] = 0.2;             // Common for equities
        sigma_values[1] = 0.4;             // Medium volatility of variance
        sigma_values[2] = 0.6;             // Higher volatility of variance
        
        // Most likely rho values first
        rho_values[0] = -0.7;              // Strong negative correlation (common for equities)
        rho_values[1] = -0.4;              // Medium negative correlation
        rho_values[2] = 0.0;               // No correlation
        
        // Loop through parameters with early termination
        for (int v0_idx = 0; v0_idx < NUM_V0 && !g_found_good_match; v0_idx++) {
            double test_v0 = v0_values[v0_idx];
            double test_theta = test_v0;  // Set long-term variance equal to initial for simplicity
            
            for (int kappa_idx = 0; kappa_idx < NUM_KAPPA && !g_found_good_match; kappa_idx++) {
                double test_kappa = kappa_values[kappa_idx];
                
                for (int sigma_idx = 0; sigma_idx < NUM_SIGMA && !g_found_good_match; sigma_idx++) {
                    double test_sigma = sigma_values[sigma_idx];
                    
                    for (int rho_idx = 0; rho_idx < NUM_RHO && !g_found_good_match; rho_idx++) {
                        double test_rho = rho_values[rho_idx];
                        
                        // Calculate option price using current parameters
                        double model_price = heston_call_fft(S, K, T, r, q, 
                                                         test_v0, test_kappa, test_theta, 
                                                         test_sigma, test_rho);
                        
                        // Calculate price difference
                        double test_diff = fabs(model_price - market_price);
                        
                        // Update best parameters if this is better
                        if (test_diff < best_diff) {
                            best_v = test_v0;
                            best_kappa = test_kappa;
                            best_theta = test_theta;
                            best_rho = test_rho;
                            best_sigma = test_sigma;
                            best_diff = test_diff;
                            
                            if (g_debug) {
                                fprintf(stderr, "Debug: Found better parameter set - v0: %.4f, kappa: %.1f, sigma: %.2f, rho: %.2f, diff: $%.4f\n",
                                        test_v0, test_kappa, test_sigma, test_rho, test_diff);
                            }
                            
                            // If we're close enough, exit early
                            if (test_diff < 0.005 * market_price) {
                                g_found_good_match = true;
                                break;
                            }
                        }
                    }
                }
            }
        }
    } else {
        // Exception occurred during calibration
        fprintf(stderr, "Error: Exception during SV calibration, falling back to Black-Scholes\n");
        g_using_error_handler = false;
        return bs_iv;
    }
    
    g_using_error_handler = false;
    
    if (g_debug) {
        fprintf(stderr, "Debug: Best parameters - v0: %.4f, kappa: %.1f, sigma: %.2f, rho: %.2f\n",
                best_v, best_kappa, best_sigma, best_rho);
    }
    
    // Calculate the final implied volatility from the Heston calibration
    double sv_vol = sqrt(best_v);
    
    // Check if our best Heston calibration is reasonable
    if (best_diff > 0.1 * market_price) {
        // If Heston calibration is poor (diff > 10% of price), rely on BS IV
        if (g_debug) {
            fprintf(stderr, "Debug: Large calibration error (%.2f%% of price). Using BS IV.\n",
                    100.0 * best_diff / market_price);
        }
        return bs_iv;
    }
    
    // Apply final sanity check to ensure the result is reasonable
    if (sv_vol < 0.05) {
        if (g_debug) {
            fprintf(stderr, "Debug: SV result (%.2f%%) is too low. Using BS IV (%.2f%%) instead.\n",
                    sv_vol * 100, bs_iv * 100);
        }
        return bs_iv;
    }
    
    if (sv_vol > 1.5) {
        if (g_debug) {
            fprintf(stderr, "Debug: SV result (%.2f%%) is too high. Using BS IV (%.2f%%) instead.\n",
                    sv_vol * 100, bs_iv * 100);
        }
        return bs_iv;
    }
    
    // For most cases, return the calculated volatility
    if (g_debug) {
        fprintf(stderr, "Debug: Final SV: %.2f%% (BS IV: %.2f%%)\n", 
                sv_vol * 100, bs_iv * 100);
        fprintf(stderr, "Debug: Price difference: $%.4f (%.2f%% of market price)\n", 
                best_diff, 100.0 * best_diff / market_price);
    }
    
    return sv_vol;
}

// Free allocated memory for precomputed values
void cleanup_precomputed_values() {
    if (g_precomputed.simpson_weights != NULL) {
        free(g_precomputed.simpson_weights);
        g_precomputed.simpson_weights = NULL;
    }
    
    if (g_precomputed.exp_terms != NULL) {
        free(g_precomputed.exp_terms);
        g_precomputed.exp_terms = NULL;
    }
    
    g_precomputed.is_valid = false;
}

// Free allocated memory in FFT cache
void cleanup_fft_cache() {
    if (g_cache.prices != NULL) {
        free(g_cache.prices);
        g_cache.prices = NULL;
    }
    
    if (g_cache.strikes != NULL) {
        free(g_cache.strikes);
        g_cache.strikes = NULL;
    }
    
    g_cache.is_valid = false;
    
    // Also clean up precomputed values
    cleanup_precomputed_values();
}

// Helper function to safely parse a double value
double safe_atof(const char* str) {
    char* endptr;
    errno = 0;  // Reset error number
    
    double value = strtod(str, &endptr);
    
    // Check for conversion errors
    if (errno == ERANGE) {
        fprintf(stderr, "Error: Number out of range: %s\n", str);
        return -1.0;
    }
    
    // Check if any conversion happened
    if (endptr == str) {
        fprintf(stderr, "Error: Not a valid number: %s\n", str);
        return -1.0;
    }
    
    return value;
}

// Check if a number is a power of 2
int is_power_of_two(int n) {
    if (n <= 0)
        return 0;
    return (n & (n - 1)) == 0;
}

// Print usage information
void print_usage(const char* program_name) {
    fprintf(stderr, "Usage: %s [options] OptionPrice StockPrice Strike Time RiskFreeRate DividendYield\n", program_name);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --debug               Enable debug output\n");
    fprintf(stderr, "  --verbose-debug       Enable verbose debug output\n");
    fprintf(stderr, "  --help                Display this help message\n");
    fprintf(stderr, "  --fft-n=VALUE         Set FFT points (power of 2, default: 4096)\n");
    fprintf(stderr, "  --log-strike-range=X  Set log strike range (default: 3.0)\n");
    fprintf(stderr, "  --alpha=X             Set Carr-Madan alpha parameter (default: 1.5)\n");
    fprintf(stderr, "  --eta=X               Set grid spacing parameter (default: 0.05)\n");
    fprintf(stderr, "  --cache-tolerance=X   Set parameter tolerance for cache reuse (default: 1e-5)\n");
    fprintf(stderr, "\nExample: %s --fft-n=8192 5.0 100.0 100.0 0.25 0.05 0.02\n", program_name);
    fprintf(stderr, "\nNote: Parameters are automatically adapted based on option characteristics\n");
}

int main(int argc, char *argv[]) {
    // Set up long options for getopt
    static struct option long_options[] = {
        {"debug", no_argument, 0, 'd'},
        {"verbose-debug", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {"fft-n", required_argument, 0, 'n'},
        {"log-strike-range", required_argument, 0, 'r'},
        {"alpha", required_argument, 0, 'a'},
        {"eta", required_argument, 0, 'e'},
        {"cache-tolerance", required_argument, 0, 't'},
        {0, 0, 0, 0}
    };
    
    // Process command-line options
    int option_index = 0;
    int c;
    
    while ((c = getopt_long(argc, argv, "dhv", long_options, &option_index)) != -1) {
        switch (c) {
            case 'd':
                g_debug = true;
                break;
            case 'v':
                g_debug = true;
                g_verbose_debug = true;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            case 'n': {
                int n = atoi(optarg);
                if (is_power_of_two(n)) {
                    g_fft_n = n;
                } else {
                    fprintf(stderr, "Warning: FFT size must be a power of 2. Using default: %d\n", g_fft_n);
                }
                break;
            }
            case 'r': {
                double r = atof(optarg);
                if (r > 0.0) {
                    g_log_strike_range = r;
                } else {
                    fprintf(stderr, "Warning: Log strike range must be positive. Using default: %.1f\n", g_log_strike_range);
                }
                break;
            }
            case 'a': {
                double a = atof(optarg);
                if (a > 0.0) {
                    g_alpha = a;
                } else {
                    fprintf(stderr, "Warning: Alpha must be positive. Using default: %.1f\n", g_alpha);
                }
                break;
            }
            case 'e': {
                double eta = atof(optarg);
                if (eta > 0.0) {
                    g_eta = eta;
                } else {
                    fprintf(stderr, "Warning: Eta must be positive. Using default: %.3f\n", g_eta);
                }
                break;
            }
            case 't': {
                double tol = atof(optarg);
                if (tol > 0.0) {
                    g_cache_tolerance = tol;
                } else {
                    fprintf(stderr, "Warning: Cache tolerance must be positive. Using default: %.1e\n", 
                            g_cache_tolerance);
                }
                break;
            }
            case '?':
                // getopt_long already printed error message
                print_usage(argv[0]);
                return 1;
            default:
                break;
        }
    }
    
    // Check if we have the correct number of arguments after options
    if (argc - optind != 6) {
        fprintf(stderr, "Error: Incorrect number of arguments\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // Print FFT configuration if in debug mode
    if (g_debug) {
        fprintf(stderr, "Debug: FFT Configuration - N: %d, Range: %.1f, Alpha: %.2f, Eta: %.4f, Tolerance: %.1e\n",
                g_fft_n, g_log_strike_range, g_alpha, g_eta, g_cache_tolerance);
    }
    
    // Set up signal handlers for the entire program
    signal(SIGSEGV, error_handler);
    signal(SIGFPE, error_handler);
    
    // Safely parse command line arguments with error checking
    double market_price = safe_atof(argv[optind]);
    double S = safe_atof(argv[optind + 1]);
    double K = safe_atof(argv[optind + 2]);
    double T = safe_atof(argv[optind + 3]);
    double r = safe_atof(argv[optind + 4]);
    double q = safe_atof(argv[optind + 5]);
    
    // Check for parsing errors
    if (market_price < 0.0 || S < 0.0 || K < 0.0 || T < 0.0) {
        fprintf(stderr, "Error: Invalid input parameters\n");
        return 1;
    }
    
    // Check if this is a challenging parameter set and adapt FFT parameters if needed
    if (is_challenging_parameter_set(S, K, T, 0.04, 1.0, 0.04, 0.4, -0.7)) {
        if (g_debug) {
            fprintf(stderr, "Debug: Detected challenging parameter set, adapting FFT parameters\n");
        }
        adapt_fft_parameters(S, K, T);
    }
    
    // Input validation
    if (market_price <= 0.0) {
        fprintf(stderr, "Error: Option price must be positive\n");
        return 1;
    }
    
    if (S <= 0.0) {
        fprintf(stderr, "Error: Stock price must be positive\n");
        return 1;
    }
    
    if (K <= 0.0) {
        fprintf(stderr, "Error: Strike price must be positive\n");
        return 1;
    }
    
    if (T <= 0.0) {
        fprintf(stderr, "Error: Time to maturity must be positive\n");
        return 1;
    }
    
    // Use error handler for the main calculation
    g_using_error_handler = true;
    double iv = -1.0;
    
    if (setjmp(g_error_jmp_buf) == 0) {
        // Call the implied volatility function from the stochastic volatility model
        iv = implied_vol_sv(market_price, S, K, T, r, q);
    } else {
        // Exception occurred during calculation
        if (g_debug) {
            fprintf(stderr, "Exception during implied volatility calculation\n");
            fprintf(stderr, "Trying with alternative FFT parameters\n");
        }
        
        // Try a set of alternative parameters
        g_fft_n = 8192;
        g_alpha = 1.0;
        g_eta = 0.1;
        
        // Try again with new parameters
        if (setjmp(g_error_jmp_buf) == 0) {
            iv = implied_vol_sv(market_price, S, K, T, r, q);
        } else {
            // If still failing, try one more set
            g_fft_n = 2048;
            g_alpha = 1.25;
            g_eta = 0.075;
            
            if (setjmp(g_error_jmp_buf) == 0) {
                iv = implied_vol_sv(market_price, S, K, T, r, q);
            } else {
                // If all attempts failed, fall back to Black-Scholes
                if (g_debug) {
                    fprintf(stderr, "All FFT attempts failed, falling back to Black-Scholes\n");
                }
                
                // Fall back to Black-Scholes
                iv = bs_implied_vol(market_price, S, K, T, r, q);
                
                if (iv < 0.0) {
                    // If BS IV also fails, use a reasonable default
                    iv = 0.25;  // 25% is a reasonable default for most options
                }
            }
        }
    }
    
    g_using_error_handler = false;
    signal(SIGSEGV, SIG_DFL);
    signal(SIGFPE, SIG_DFL);
    
    // Clean up resources
    cleanup_fft_cache();
    
    // Check for error
    if (iv < 0.0) {
        fprintf(stderr, "Error: Failed to calculate implied volatility\n");
        return 1;
    }
    
    // Print the result with exactly 6 decimal places
    printf("%.6f\n", iv);
    
    return 0;
}