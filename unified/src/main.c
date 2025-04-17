#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../include/option_types.h"
#include "../include/option_pricing.h"
#include "../include/error_handling.h"
#include "../include/market_data.h"

/**
 * Print usage information
 */
static void print_usage(const char* program_name) {
    printf("Usage: %s [options] SPOT_PRICE STRIKE_PRICE TIME_TO_EXPIRY RISK_FREE_RATE DIVIDEND_YIELD VOLATILITY OPTION_TYPE MODEL_TYPE METHOD_TYPE [MARKET_PRICE] [CALCULATE_GREEKS] [TICKER]\n", program_name);
    printf("\n");
    printf("Required parameters:\n");
    printf("  SPOT_PRICE       Current price of the underlying asset\n");
    printf("  STRIKE_PRICE     Strike price of the option\n");
    printf("  TIME_TO_EXPIRY   Time to expiration in years\n");
    printf("  RISK_FREE_RATE   Risk-free interest rate (decimal format, e.g., 0.05 for 5%%)\n");
    printf("  DIVIDEND_YIELD   Dividend yield (decimal format, e.g., 0.02 for 2%%)\n");
    printf("  VOLATILITY       Initial volatility (decimal format, e.g., 0.2 for 20%%)\n");
    printf("  OPTION_TYPE      0 for call, 1 for put\n");
    printf("  MODEL_TYPE       0 for Black-Scholes, 1 for Heston\n");
    printf("  METHOD_TYPE      0 for analytic, 1 for quadrature, 2 for FFT\n");
    printf("\n");
    printf("Optional parameters:\n");
    printf("  MARKET_PRICE     Market price for implied volatility calculation (0 to skip)\n");
    printf("  CALCULATE_GREEKS 1 to calculate Greeks, 0 to skip\n");
    printf("  TICKER           Ticker symbol for market data lookup\n");
    printf("\n");
    printf("Alternative usage for market data retrieval:\n");
    printf("  %s --get-market-data TICKER [DATA_SOURCE]\n", program_name);
    printf("    TICKER          Ticker symbol to look up\n");
    printf("    DATA_SOURCE     Data source (0: default, 1: Alpha Vantage, 2: Finnhub, 3: Polygon)\n");
    printf("\n");
    printf("Alternative usage for historical prices retrieval:\n");
    printf("  %s --get-historical-prices TICKER DAYS [DATA_SOURCE]\n", program_name);
    printf("    TICKER          Ticker symbol to look up\n");
    printf("    DAYS            Number of days of historical data (1-365)\n");
    printf("    DATA_SOURCE     Data source (0: default, 1: Alpha Vantage, 2: Finnhub, 3: Polygon)\n");
}

/**
 * Parse command-line arguments
 */
static int parse_arguments(
    int argc,
    char* argv[],
    double* spot_price,
    double* strike_price,
    double* time_to_expiry,
    double* risk_free_rate,
    double* dividend_yield,
    double* volatility,
    OptionType* option_type,
    ModelType* model_type,
    NumericalMethod* method,
    double* market_price,
    GreeksFlags* greeks_flags,
    char** ticker_symbol
) {
    /* Check for minimum number of arguments */
    if (argc < 10) {
        fprintf(stderr, "Error: Not enough arguments provided.\n");
        print_usage(argv[0]);
        return 0;
    }
    
    /* Parse required arguments */
    *spot_price = atof(argv[1]);
    *strike_price = atof(argv[2]);
    *time_to_expiry = atof(argv[3]);
    *risk_free_rate = atof(argv[4]);
    *dividend_yield = atof(argv[5]);
    *volatility = atof(argv[6]);
    *option_type = (OptionType)atoi(argv[7]);
    *model_type = (ModelType)atoi(argv[8]);
    *method = (NumericalMethod)atoi(argv[9]);
    
    /* Initialize optional arguments with defaults */
    *market_price = 0.0;
    memset(greeks_flags, 0, sizeof(GreeksFlags));
    *ticker_symbol = NULL;
    
    /* Parse optional arguments if provided */
    if (argc > 10) {
        *market_price = atof(argv[10]);
    }
    
    if (argc > 11) {
        int calculate_greeks = atoi(argv[11]);
        if (calculate_greeks) {
            /* Set all Greeks flags to 1 */
            greeks_flags->delta = 1;
            greeks_flags->gamma = 1;
            greeks_flags->theta = 1;
            greeks_flags->vega = 1;
            greeks_flags->rho = 1;
        }
    }
    
    if (argc > 12 && argv[12][0] != '\0') {
        *ticker_symbol = argv[12];
    }
    
    return 1; /* Successful parsing */
}

/**
 * Format and print a PricingResult structure
 */
static void print_result(
    const PricingResult* result,
    double spot_price,
    double strike_price,
    double time_to_expiry,
    OptionType option_type,
    ModelType model_type,
    const GreeksFlags* greeks_flags
) {
    const char* option_type_str = (option_type == OPTION_CALL) ? "Call" : "Put";
    const char* model_type_str = (model_type == MODEL_BLACK_SCHOLES) ? "Black-Scholes" : "Heston";
    
    printf("\n============== OPTION PRICING RESULT ==============\n");
    printf("Option Type:        %s\n", option_type_str);
    printf("Pricing Model:      %s\n", model_type_str);
    printf("Spot Price:         %.2f\n", spot_price);
    printf("Strike Price:       %.2f\n", strike_price);
    printf("Time to Expiry:     %.6f years\n", time_to_expiry);
    printf("\n");
    
    if (result->error_code != 0) {
        printf("ERROR: %s (code: %d)\n", get_error_message(result->error_code), result->error_code);
        return;
    }
    
    printf("Option Price:       %.6f\n", result->price);
    
    if (result->implied_volatility > 0) {
        printf("Implied Volatility: %.2f%%\n", result->implied_volatility * 100);
    }
    
    if (greeks_flags->delta || greeks_flags->gamma || greeks_flags->theta || 
        greeks_flags->vega || greeks_flags->rho) {
        printf("\n---------------- Greeks ----------------\n");
        
        if (greeks_flags->delta) {
            printf("Delta:              %.6f\n", result->delta);
        }
        
        if (greeks_flags->gamma) {
            printf("Gamma:              %.6f\n", result->gamma);
        }
        
        if (greeks_flags->theta) {
            printf("Theta:              %.6f\n", result->theta);
        }
        
        if (greeks_flags->vega) {
            printf("Vega:               %.6f\n", result->vega);
        }
        
        if (greeks_flags->rho) {
            printf("Rho:                %.6f\n", result->rho);
        }
    }
    
    printf("=================================================\n");
}

/**
 * Main function - Entry point for the unified option pricing system
 */
int main(int argc, char* argv[]) {
    double spot_price, strike_price, time_to_expiry;
    double risk_free_rate, dividend_yield, volatility, market_price;
    OptionType option_type;
    ModelType model_type;
    NumericalMethod method;
    GreeksFlags greeks_flags;
    char* ticker_symbol;
    PricingResult result;
    int ret_code;
    
    /* Check for market data retrieval mode */
    if (argc >= 3 && strcmp(argv[1], "--get-market-data") == 0) {
        const char* ticker = argv[2];
        DataSource source = DATA_SOURCE_DEFAULT;
        
        /* Parse data source if provided */
        if (argc >= 4) {
            source = (DataSource)atoi(argv[3]);
        }
        
        /* Initialize market data module */
        ret_code = market_data_init(NULL);
        if (ret_code != 0) {
            fprintf(stderr, "Error: Failed to initialize market data module: %s\n", 
                    get_error_message(ret_code));
            return 1;
        }
        
        /* Get spot price */
        int error_code;
        double price = get_current_price(ticker, source, &error_code);
        if (error_code != ERROR_SUCCESS) {
            fprintf(stderr, "Error retrieving price: %s\n", get_error_message(error_code));
            market_data_cleanup();
            return 1;
        }
        
        /* Get dividend yield */
        double yield = get_dividend_yield(ticker, source, &error_code);
        if (error_code != ERROR_SUCCESS) {
            /* Non-fatal, just set to 0 */
            yield = 0.0;
        }
        
        /* Output results in a format the shell script can parse */
        printf("%.6f %.6f\n", price, yield);
        
        /* Cleanup and exit */
        market_data_cleanup();
        return 0;
    }
    
    /* Check for historical prices retrieval mode */
    if (argc >= 4 && strcmp(argv[1], "--get-historical-prices") == 0) {
        const char* ticker = argv[2];
        int days = atoi(argv[3]);
        DataSource source = DATA_SOURCE_DEFAULT;
        
        /* Parse data source if provided */
        if (argc >= 5) {
            source = (DataSource)atoi(argv[4]);
        }
        
        /* Validate days parameter */
        if (days <= 0 || days > 365) {
            fprintf(stderr, "Error: Days parameter must be between 1 and 365\n");
            return 1;
        }
        
        /* Initialize market data module */
        ret_code = market_data_init(NULL);
        if (ret_code != 0) {
            fprintf(stderr, "Error: Failed to initialize market data module: %s\n", 
                    get_error_message(ret_code));
            return 1;
        }
        
        /* Prepare arrays for prices and dates */
        double* prices = NULL;
        char** dates = NULL;
        int error_code;
        
        /* Get historical prices */
        int count = get_historical_prices(ticker, days, source, &prices, &dates, &error_code);
        
        if (error_code != ERROR_SUCCESS || count <= 0) {
            fprintf(stderr, "Error retrieving historical prices: %s\n", 
                    get_error_message(error_code));
            market_data_cleanup();
            return 1;
        }
        
        /* Output the results */
        for (int i = 0; i < count; i++) {
            printf("%s,%.6f\n", dates[i], prices[i]);
        }
        
        /* Free memory */
        for (int i = 0; i < count; i++) {
            free(dates[i]);
        }
        free(prices);
        free(dates);
        
        /* Cleanup and exit */
        market_data_cleanup();
        return 0;
    }
    
    /* Parse command-line arguments for normal option pricing mode */
    if (!parse_arguments(argc, argv, &spot_price, &strike_price, &time_to_expiry,
                         &risk_free_rate, &dividend_yield, &volatility,
                         &option_type, &model_type, &method,
                         &market_price, &greeks_flags, &ticker_symbol)) {
        return 1;
    }
    
    /* Initialize market data module if a ticker symbol is provided */
    if (ticker_symbol != NULL && *ticker_symbol) {
        ret_code = market_data_init(NULL);
        if (ret_code != 0) {
            fprintf(stderr, "Warning: Failed to initialize market data module: %s\n", 
                    get_error_message(ret_code));
            /* Continue without market data */
        }
    }
    
    /* Call the option pricing function */
    ret_code = price_option(
        spot_price, strike_price, time_to_expiry,
        risk_free_rate, dividend_yield, volatility,
        option_type, model_type, method,
        market_price, greeks_flags, ticker_symbol,
        &result
    );
    
    /* Cleanup market data module if initialized */
    if (ticker_symbol != NULL && *ticker_symbol) {
        market_data_cleanup();
    }
    
    /* Check for errors */
    if (ret_code != 0) {
        fprintf(stderr, "Error: %s\n", get_error_message(ret_code));
        return 1;
    }
    
    /* Print the result */
    print_result(&result, spot_price, strike_price, time_to_expiry,
                 option_type, model_type, &greeks_flags);
    
    return 0;
} 