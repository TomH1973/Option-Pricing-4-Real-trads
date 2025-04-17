/**
 * market_data_tool.c
 * Standalone command-line tool for retrieving market data
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../include/market_data.h"

void print_usage(const char* program_name) {
    printf("Usage: %s [operation] [parameters]\n\n", program_name);
    printf("Operations:\n");
    printf("  price TICKER [SOURCE]            Get current price for a ticker\n");
    printf("  dividend TICKER [SOURCE]         Get dividend yield for a ticker\n");
    printf("  volatility TICKER DAYS [SOURCE]  Get historical volatility for a ticker\n");
    printf("  rate TERM                        Get risk-free rate for a term\n");
    printf("\n");
    printf("Parameters:\n");
    printf("  TICKER   Ticker symbol (e.g., AAPL, MSFT, SPX)\n");
    printf("  SOURCE   Data source (0: default, 1: Alpha Vantage, 2: Finnhub, 3: Polygon)\n");
    printf("  DAYS     Number of days for historical volatility calculation\n");
    printf("  TERM     Rate term (0: 1-month, 1: 3-month, 2: 6-month, 3: 1-year,\n");
    printf("           4: 2-year, 5: 5-year, 6: 10-year, 7: 30-year)\n");
}

int main(int argc, char** argv) {
    // Check arguments
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    // Initialize market data module
    if (market_data_init(NULL) != 0) {
        fprintf(stderr, "Error: Failed to initialize market data module\n");
        return 1;
    }
    
    const char* operation = argv[1];
    int error_code = 0;
    
    // Handle different operations
    if (strcmp(operation, "price") == 0) {
        // Check arguments for price operation
        if (argc < 3) {
            fprintf(stderr, "Error: Missing ticker parameter\n");
            print_usage(argv[0]);
            market_data_cleanup();
            return 1;
        }
        
        const char* ticker = argv[2];
        DataSource source = DATA_SOURCE_DEFAULT;
        
        if (argc >= 4) {
            source = (DataSource)atoi(argv[3]);
        }
        
        double price = get_current_price(ticker, source, &error_code);
        
        if (error_code != 0) {
            fprintf(stderr, "Error %d retrieving price for %s\n", error_code, ticker);
            market_data_cleanup();
            return error_code;
        }
        
        printf("%.6f\n", price);
    }
    else if (strcmp(operation, "dividend") == 0) {
        // Check arguments for dividend operation
        if (argc < 3) {
            fprintf(stderr, "Error: Missing ticker parameter\n");
            print_usage(argv[0]);
            market_data_cleanup();
            return 1;
        }
        
        const char* ticker = argv[2];
        DataSource source = DATA_SOURCE_DEFAULT;
        
        if (argc >= 4) {
            source = (DataSource)atoi(argv[3]);
        }
        
        double yield = get_dividend_yield(ticker, source, &error_code);
        
        if (error_code != 0) {
            fprintf(stderr, "Error %d retrieving dividend yield for %s\n", error_code, ticker);
            market_data_cleanup();
            return error_code;
        }
        
        printf("%.6f\n", yield);
    }
    else if (strcmp(operation, "volatility") == 0) {
        // Check arguments for volatility operation
        if (argc < 4) {
            fprintf(stderr, "Error: Missing ticker or days parameter\n");
            print_usage(argv[0]);
            market_data_cleanup();
            return 1;
        }
        
        const char* ticker = argv[2];
        int days = atoi(argv[3]);
        DataSource source = DATA_SOURCE_DEFAULT;
        
        if (argc >= 5) {
            source = (DataSource)atoi(argv[4]);
        }
        
        double volatility = get_historical_volatility(ticker, days, source, &error_code);
        
        if (error_code != 0) {
            fprintf(stderr, "Error %d retrieving historical volatility for %s\n", error_code, ticker);
            market_data_cleanup();
            return error_code;
        }
        
        printf("%.6f\n", volatility);
    }
    else if (strcmp(operation, "rate") == 0) {
        // Check arguments for rate operation
        if (argc < 3) {
            fprintf(stderr, "Error: Missing term parameter\n");
            print_usage(argv[0]);
            market_data_cleanup();
            return 1;
        }
        
        RateTerm term = (RateTerm)atoi(argv[2]);
        
        double rate = get_risk_free_rate(term, &error_code);
        
        if (error_code != 0) {
            fprintf(stderr, "Error %d retrieving risk-free rate\n", error_code);
            market_data_cleanup();
            return error_code;
        }
        
        printf("%.6f\n", rate);
    }
    else {
        fprintf(stderr, "Error: Unknown operation '%s'\n", operation);
        print_usage(argv[0]);
        market_data_cleanup();
        return 1;
    }
    
    // Clean up
    market_data_cleanup();
    return 0;
} 