#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Cumulative distribution function for standard normal distribution
double cdf(double x) {
    return 0.5 * (1.0 + erf(x / sqrt(2.0)));
}

// Black-Scholes European call option pricing
double bs_call(double S, double K, double T, double r, double sigma) {
    double d1 = (log(S/K) + (r + 0.5*sigma*sigma)*T) / (sigma*sqrt(T));
    double d2 = d1 - sigma * sqrt(T);
    return S * 0.5 * (1.0 + erf(d1 / sqrt(2.0))) - K * exp(-r*T) * 0.5 * (1.0 + erf(d2 / sqrt(2.0)));
}

// Implied volatility via bisection method (corrected)
double implied_vol(double market_price, double S, double K, double T, double r) {
    double low = 1e-6, high = 5.0, mid;
    double epsilon = 1e-8;
    int max_iter = 100;

    double fa = bs_call(S, K, T, r, low) - market_price;
    double fb = bs_call(S, K, T, r, high) - market_price;

    if (fa * fb > 0) {
        fprintf(stderr, "Initial interval doesn't bracket a root.\n");
        return -1;
    }

    for(int i = 0; i < max_iter; i++) {
        mid = (low + high) / 2.0;
        double fm = bs_call(S, K, T, r, mid) - market_price;

        if(fabs(fm) < epsilon) {
            return mid;  // Found solution
        }

        if(fa * fm < 0) {
            high = mid;
            fb = fm;
        } else {
            low = mid;
            fa = fm;
        }
    }

    // If it didn't converge, return best approximation
    return mid;
}

int main(int argc, char *argv[]) {
    if(argc != 6) {
        fprintf(stderr, "Usage: %s OptionPrice StockPrice Strike Time RiskFreeRate\n", argv[0]);
        return 1;
    }

    double market_price = atof(argv[1]);
    double S = atof(argv[2]);
    double K = atof(argv[3]);
    double T = atof(argv[4]);
    double r = atof(argv[5]);

    double iv = implied_vol(market_price, S, K, T, r);

    if(iv < 0) {
        fprintf(stderr, "Implied volatility calculation failed.\n");
        return 1;
    }

    printf("Implied volatility: %.6f\n", iv);
    return 0;
}
