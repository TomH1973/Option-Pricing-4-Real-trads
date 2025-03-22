#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>

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

// Complex characteristic function for Heston model
double complex cf_heston(double complex s, double S, double v0, double kappa, 
                         double theta, double sigma, double rho, double r, 
                         double q, double T) {
    double complex u = s - 0.5;
    double complex d = csqrt((rho * sigma * u * I - kappa) * 
                     (rho * sigma * u * I - kappa) - 
                     sigma * sigma * (u * I) * (u * I - I));
    double complex g = (kappa - rho * sigma * u * I - d) / 
                     (kappa - rho * sigma * u * I + d);
    
    double complex A = (r - q) * u * I * T + 
                      kappa * theta * ((kappa - rho * sigma * u * I - d) * T - 
                      2.0 * clog((1.0 - g * cexp(-d * T)) / (1.0 - g))) / 
                      (sigma * sigma);
                      
    double complex B = (kappa - rho * sigma * u * I - d) * 
                      (1.0 - cexp(-d * T)) / 
                      (sigma * sigma * (1.0 - g * cexp(-d * T)));
                      
    return cexp(A + B * v0 + I * u * clog(S));
}

// Price a European call option using the Heston model with numerical integration
double heston_call(double S, double K, double v0, double kappa, double theta, 
                  double sigma, double rho, double r, double q, double T) {
    int n = 2048;  // Number of integration points
    double eta = 0.25;  // Spacing parameter
    double b = 1000.0;  // Upper bound for integration
    double integration_result = 0.0;
    
    // Simpson's rule for numerical integration
    for (int j = 0; j < n; j++) {
        double u = j * eta;
        double weight = (j == 0) ? 1.0 / 3.0 : ((j == n - 1) ? 1.0 / 3.0 : 
                        ((j % 2 == 1) ? 4.0 / 3.0 : 2.0 / 3.0));
        
        double complex integrand = cexp(-I * u * clog(K)) * 
                                 cf_heston(u, S, v0, kappa, theta, sigma, 
                                          rho, r, q, T) / (I * u);
        
        integration_result += weight * creal(integrand);
    }
    
    integration_result *= eta / M_PI;
    
    // Price from the integration result
    double call_price = S * exp(-q * T) - K * exp(-r * T) / 2.0 + 
                       exp(-r * T) * integration_result;
    
    return call_price > 0 ? call_price : 0;
}

// Implied volatility calculation for stochastic volatility model via bisection
double implied_params(double market_price, double S, double K, double T, 
                     double r, double q) {
    // Default Heston parameters
    double v0_default = 0.04;  // Initial variance (~20% volatility)
    double kappa_default = 2.0;  // Mean reversion speed
    double theta_default = 0.04;  // Long-term variance
    double sigma_default = 0.3;  // Volatility of variance
    double rho_default = -0.7;  // Correlation
    
    // We'll calibrate only initial variance v0, keeping other parameters fixed
    double low = 0.0001;  // Lower bound for v0
    double high = 0.25;   // Upper bound for v0
    double mid, price;
    double epsilon = 1e-6;
    int max_iter = 100;
    
    double fa = heston_call(S, K, low, kappa_default, theta_default, 
                           sigma_default, rho_default, r, q, T) - market_price;
    double fb = heston_call(S, K, high, kappa_default, theta_default, 
                           sigma_default, rho_default, r, q, T) - market_price;
    
    if (fa * fb > 0) {
        fprintf(stderr, "Initial interval doesn't bracket a root.\n");
        return -1;
    }
    
    for (int i = 0; i < max_iter; i++) {
        mid = (low + high) / 2.0;
        price = heston_call(S, K, mid, kappa_default, theta_default, 
                           sigma_default, rho_default, r, q, T);
        double fm = price - market_price;
        
        if (fabs(fm) < epsilon) {
            return sqrt(mid);  // Return equivalent Black-Scholes volatility
        }
        
        if (fa * fm < 0) {
            high = mid;
            fb = fm;
        } else {
            low = mid;
            fa = fm;
        }
    }
    
    // If it didn't converge, return best approximation
    return sqrt(mid);  // Return in terms of volatility not variance
}

int main(int argc, char *argv[]) {
    if (argc != 7) {
        fprintf(stderr, "Usage: %s OptionPrice StockPrice Strike Time RiskFreeRate DividendYield\n", argv[0]);
        return 1;
    }
    
    double market_price = atof(argv[1]);
    double S = atof(argv[2]);
    double K = atof(argv[3]);
    double T = atof(argv[4]);
    double r = atof(argv[5]);
    double q = atof(argv[6]);
    
    double implied_vol = implied_params(market_price, S, K, T, r, q);
    
    if (implied_vol < 0) {
        fprintf(stderr, "Implied parameter calculation failed.\n");
        return 1;
    }
    
    printf("%.6f\n", implied_vol);
    return 0;
}