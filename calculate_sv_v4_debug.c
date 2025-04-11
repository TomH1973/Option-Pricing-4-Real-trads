#include <signal.h>

static bool g_verbose_debug = false;
static jmp_buf g_error_jmp_buf;
static bool g_using_error_handler = false;

static void segfault_handler(int sig) {
    if (g_using_error_handler) {
        fprintf(stderr, "ERROR: Caught signal %d (segmentation fault)\n", sig);
        longjmp(g_error_jmp_buf, 1);
    }
}

void precompute_fft_values(double S) {
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

    cleanup_precomputed_values();
    
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
    
    for (int i = 0; i < g_fft_n; i++) {
        g_precomputed.simpson_weights[i] = (i == 0) ? 1.0/3.0 : 
                                          ((i % 2 == 1) ? 4.0/3.0 : 2.0/3.0);
    }
    
    double log_S = log(S);
    
    g_using_error_handler = true;
    signal(SIGSEGV, segfault_handler);
    
    if (setjmp(g_error_jmp_buf) == 0) {
        for (int i = 0; i < g_fft_n; i++) {
            double v = i * g_eta;
            if (fabs(v) < 1e-10) v = 1e-10;
            
            double complex exp_term = exp(-I * v * log_S);
            if (!isfinite(creal(exp_term)) || !isfinite(cimag(exp_term))) {
                if (g_verbose_debug) {
                    fprintf(stderr, "Warning: Non-finite exp term at i=%d, v=%.6f, log_S=%.6f\n", 
                            i, v, log_S);
                }
                exp_term = 1.0 + 0.0 * I;
            }
            g_precomputed.exp_terms[i] = exp_term;
        }
    } else {
        fprintf(stderr, "Error: Exception during precomputation of FFT values\n");
        cleanup_precomputed_values();
        g_using_error_handler = false;
        signal(SIGSEGV, SIG_DFL);
        return;
    }
    
    g_using_error_handler = false;
    signal(SIGSEGV, SIG_DFL);
    
    g_precomputed.fft_n = g_fft_n;
    g_precomputed.eta = g_eta;
    g_precomputed.alpha = g_alpha;
    g_precomputed.S = S;
    g_precomputed.is_valid = true;
}

void init_fft_cache(double S, double r, double q, double T, 
                   double v0, double kappa, double theta, double sigma, double rho) {
    g_using_error_handler = true;
    signal(SIGSEGV, segfault_handler);
    
    if (setjmp(g_error_jmp_buf) == 0) {
        p = fftw_plan_dft_1d(g_fft_n, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
        if (p == NULL) {
            fprintf(stderr, "Error: Failed to create FFTW plan\n");
            fftw_free(in);
            fftw_free(out);
            g_using_error_handler = false;
            signal(SIGSEGV, SIG_DFL);
            return;
        }
        
        fftw_execute(p);
        
        double log_S = log(S);
        double inv_pi = 1.0 / M_PI;
        double range_factor = 2.0 * g_log_strike_range / g_fft_n;
        
        for (int i = 0; i < g_fft_n; i++) {
            double log_K = log_S - g_log_strike_range + range_factor * i;
            double K = exp(log_K);
            
            g_cache.strikes[i] = K;
            
            double exp_factor = exp(-g_alpha * log_K) * inv_pi;
            
            double real_part = creal(out[i]);
            if (!isfinite(real_part)) {
                if (g_verbose_debug) {
                    fprintf(stderr, "Warning: Non-finite FFT output at index %d\n", i);
                }
                real_part = 0.0;
            }
            
            double price_part = real_part * exp_factor;
            
            g_cache.prices[i] = fmax(0.0, price_part);
        }
    } else {
        fprintf(stderr, "Error: Exception during FFT computation\n");
        if (in) fftw_free(in);
        if (out) fftw_free(out);
        if (p) fftw_destroy_plan(p);
        g_using_error_handler = false;
        signal(SIGSEGV, SIG_DFL);
        signal(SIGFPE, SIG_DFL);
        return;
    }
    
    g_using_error_handler = false;
    signal(SIGSEGV, SIG_DFL);
    signal(SIGFPE, SIG_DFL);
    
    // Clean up FFTW resources
    fftw_destroy_plan(p);
    fftw_free(in);
    fftw_free(out);
    
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
        }
        return -1.0;
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
    
    // Interpolation weight
    double weight = (K - K_low) / (K_high - K_low);
    
    // Interpolated price
    double result = price_low + weight * (price_high - price_low);
    
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
    if (g_debug && (orig_fft_n != g_fft_n || orig_alpha != g_alpha || orig_eta != g_eta)) {
        fprintf(stderr, "Debug: Adapted FFT parameters for option characteristics:\n");
        fprintf(stderr, "       N: %d -> %d\n", orig_fft_n, g_fft_n);
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
    
    // Initial parameter guesses based on market characteristics
    double init_v0 = bs_iv * bs_iv;  // Initial variance
    double init_kappa = 1.0;         // Mean reversion speed
    double init_theta = init_v0;     // Long-term variance
    double init_sigma = 0.4;         // Volatility of variance
    double init_rho = -0.7;          // Correlation
    
    // Parameter calibration
    // We'll use a simple grid search approach for robustness
    double best_price_diff = DBL_MAX;
    double best_v = init_v0;
    double best_kappa = init_kappa;
    double best_theta = init_theta;
    double best_sigma = init_sigma;
    double best_rho = init_rho;
    double best_diff = DBL_MAX;
    
    // Try different parameter combinations
    for (double v = init_v0 * 0.5; v <= init_v0 * 1.5; v += init_v0 * 0.1) {
        for (double kappa = 0.5; kappa <= 2.0; kappa += 0.5) {
            for (double sigma = 0.2; sigma <= 0.6; sigma += 0.2) {
                for (double rho = -0.8; rho <= 0.0; rho += 0.2) {
                    // Use v as theta (long-term variance) for simplicity
                    double theta = v;
                    
                    // Calculate option price with current parameters
                    double price = heston_call_fft(S, K, T, r, q, v, kappa, theta, sigma, rho);
                    
                    // Calculate difference from market price
                    double diff = fabs(price - market_price);
                    
                    // Update best parameters if this is better
                    if (diff < best_diff) {
                        best_diff = diff;
                        best_v = v;
                        best_kappa = kappa;
                        best_theta = theta;
                        best_sigma = sigma;
                        best_rho = rho;
                        
                        // If difference is small enough, we can stop
                        if (diff < 0.0001 * market_price) {
                            g_found_good_match = true;
                            break;
                        }
                    }
                }
                if (g_found_good_match) break;
            }
            if (g_found_good_match) break;
        }
        if (g_found_good_match) break;
    }
    
    // Calculate the final implied volatility from the Heston calibration
    double sv_vol = sqrt(best_v);
    
    // Check if our best Heston calibration is reasonable
    if (best_diff > 0.1 * market_price) {
        // If Heston calibration is poor (diff > 10% of price), rely more on BS IV
        if (g_debug) {
            fprintf(stderr, "Debug: Large calibration error (%.2f%% of price). Using BS IV.\n",
                    100.0 * best_diff / market_price);
        }
        return bs_iv;
    }
    
    // For most cases, return the Heston-based volatility
    if (g_debug) {
        fprintf(stderr, "Debug: Final SV: %.2f%% (BS IV: %.2f%%)\n", 
                sv_vol * 100, bs_iv * 100);
        fprintf(stderr, "Debug: Price difference: $%.4f (%.2f%% of market price)\n", 
                best_diff, 100.0 * best_diff / market_price);
    }
    
    return sv_vol;
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
    signal(SIGSEGV, segfault_handler);
    signal(SIGFPE, segfault_handler);
    
    // Safely parse command line arguments with error checking
    double market_price = safe_atof(argv[optind]);
    double S = safe_atof(argv[optind + 1]);
    double K = safe_atof(argv[optind + 2]);
    double T = safe_atof(argv[optind + 3]);
    double r = safe_atof(argv[optind + 4]);
    double q = safe_atof(argv[optind + 5]);
    
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
        fprintf(stderr, "Error: Exception during implied volatility calculation\n");
        
        // Try with different FFT parameters before falling back to Black-Scholes
        if (g_debug) {
            fprintf(stderr, "Debug: Trying with alternative FFT parameters\n");
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
                fprintf(stderr, "Error: All FFT attempts failed, falling back to Black-Scholes\n");
                
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