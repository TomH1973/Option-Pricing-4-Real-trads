#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Standard normal cumulative distribution function
double cdf(double x) {
    return 0.5 * (1.0 + erf(x / sqrt(2.0)));
}

// Black-Scholes call pricing
double bs_call(double S, double K, double T, double r, double sigma) {
    double d1 = (log(S/K) + (r + 0.5*sigma*sigma)*T) / (sigma*sqrt(T));
    double d2 = d1 - sigma * sqrt(T);
    return S * cdf(d1) - K * exp(-r*T) * cdf(d2);
}

// Implied volatility via bisection method
double implied_vol(double market_price, double S, double K, double T, double r) {
    double low = 1e-6, high = 5.0, mid, price;
    const double epsilon = 1e-6;
    
    for(int i = 0; i < 100; i++) {
        mid = (low + high) / 2.0;
        price = bs_call(S, K, T, r, mid);
        
        if(fabs(price - market_price) < epsilon)
            return mid;

        if(price > market_price)
            high = mid;
        else
            low = mid;
    }
    return mid;
}

int main(int argc, char *argv[]) {
    if(argc != 6) {
        fprintf(stderr, "Usage: %s OptionPrice StockPrice Strike TimeInYears RiskFreeRate\n", argv[0]);
        return 1;
    }

    double opt_price = atof(argv[1]);
    double S = atof(argv[2]);
    double K = atof(argv[3]);
    double T = atof(argv[4]);
    double r = atof(argv[5]);

    double iv = implied_vol(opt_price, S, K, T, r);
    printf("%.6f\n", iv);

    return 0;
}
