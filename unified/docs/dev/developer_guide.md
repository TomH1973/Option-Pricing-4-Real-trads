# Developer Guide - Unified Option Pricing System

This guide is intended for developers who want to understand, modify, or extend the Unified Option Pricing System.

## Table of Contents

1. [System Architecture](#system-architecture)
2. [Directory Structure](#directory-structure)
3. [Building the System](#building-the-system)
4. [Adding a New Option Pricing Model](#adding-a-new-option-pricing-model)
5. [Adding a New Market Data Source](#adding-a-new-market-data-source)
6. [Error Handling Conventions](#error-handling-conventions)
7. [Testing](#testing)
8. [Code Style Guide](#code-style-guide)
9. [Documentation](#documentation)

## System Architecture

The Unified Option Pricing System follows a modular architecture:

```
                  ┌─────────────────┐
                  │ Shell Interface │
                  └────────┬────────┘
                           │
                           ▼
           ┌─────────────────────────────┐
           │      Unified API Layer      │
           └──┬─────────────┬────────────┘
              │             │
    ┌─────────▼──────┐   ┌──▼────────────┐
    │ Model Adapters │   │ Market Data   │
    └─────────┬──────┘   │ Integration   │
              │          └───────┬───────┘
    ┌─────────▼──────┐          │
    │Legacy Binaries │   ┌──────▼───────┐
    └────────────────┘   │External APIs │
                         └──────────────┘
```

The system is designed with the following key components:

1. **Shell Interface Layer**: Shell scripts that provide a user-friendly command-line interface.
2. **Unified API Layer**: C functions that standardize option pricing operations.
3. **Model Adapters**: Adapters for different pricing models (Black-Scholes, Heston, etc.).
4. **Market Data Integration**: Functions for retrieving and caching market data.
5. **Legacy Binary Integration**: Communication with existing binaries for backward compatibility.
6. **External API Integration**: Interaction with external market data sources.

## Directory Structure

```
unified/
├── bin/            # Compiled binaries
├── docs/           # Documentation
│   ├── api/        # API documentation
│   ├── dev/        # Developer documentation
│   ├── examples/   # Usage examples
│   └── user/       # User guides
├── include/        # Header files
│   ├── option_types.h
│   ├── error_handling.h
│   ├── option_pricing.h
│   ├── market_data.h
│   ├── black_scholes_adapter.h
│   ├── heston_adapter.h
│   └── path_resolution.h
├── obj/            # Object files (generated during build)
├── scripts/        # Shell scripts
│   ├── option_pricer.sh
│   ├── get_market_data.sh
│   └── get_historical_prices.sh
├── src/            # Source files
│   ├── main.c
│   ├── option_pricing.c
│   ├── error_handling.c
│   ├── path_resolution.c
│   ├── market_data.c
│   ├── black_scholes_adapter.c
│   └── heston_adapter.c
└── tests/          # Test scripts and data
    ├── test_basic.sh
    ├── test_market_data.sh
    ├── test_ticker_option_pricing.sh
    └── test_historical_prices.sh
```

## Building the System

### Prerequisites

Ensure you have the following development packages installed:

```bash
# For Debian/Ubuntu
sudo apt-get install build-essential libfftw3-dev libcurl4-openssl-dev libjansson-dev

# For Fedora/RHEL
sudo dnf install gcc make fftw-devel libcurl-devel jansson-devel
```

### Build Commands

To build the system:

```bash
cd unified
make -f Makefile.unified
```

To clean and rebuild:

```bash
make -f Makefile.unified clean
make -f Makefile.unified
```

To install the binaries:

```bash
make -f Makefile.unified install
```

### Makefile Structure

The `Makefile.unified` contains several targets:

- `all`: Builds the entire system
- `clean`: Removes all generated files
- `install`: Installs the binaries
- `test`: Runs the test suite
- `check`: Runs memory leak checks using Valgrind

## Adding a New Option Pricing Model

To add a new pricing model to the system, follow these steps:

1. **Create adapter header file** in `include/`:

```c
// include/new_model_adapter.h
#ifndef NEW_MODEL_ADAPTER_H
#define NEW_MODEL_ADAPTER_H

#include "option_types.h"

int price_with_new_model(
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

// Additional function declarations...

#endif /* NEW_MODEL_ADAPTER_H */
```

2. **Create adapter implementation** in `src/`:

```c
// src/new_model_adapter.c
#include "../include/new_model_adapter.h"
#include "../include/error_handling.h"
#include "../include/path_resolution.h"

// Implementation details...
```

3. **Update ModelType enumeration** in `include/option_types.h`:

```c
typedef enum {
    MODEL_BLACK_SCHOLES = 0,
    MODEL_HESTON = 1,
    MODEL_NEW_MODEL = 2,  // Add your new model
    MODEL_DEFAULT = MODEL_BLACK_SCHOLES
} ModelType;
```

4. **Update option_pricing.c** to use your model:

```c
// In price_option function
switch (model_type) {
    case MODEL_BLACK_SCHOLES:
        // Existing Black-Scholes code...
        break;
    case MODEL_HESTON:
        // Existing Heston code...
        break;
    case MODEL_NEW_MODEL:
        ret = price_with_new_model(
            spot_price, strike_price, time_to_expiry, 
            risk_free_rate, dividend_yield, volatility,
            option_type, method, market_price, result
        );
        break;
    default:
        // Error handling...
}
```

5. **Update shell interface** in `scripts/option_pricer.sh`:

```bash
# Add new option in the parse_args function
case "$1" in
    # Existing cases...
    "--model" | "-m")
        shift
        case "$1" in
            "bs" | "black-scholes")
                MODEL="bs"
                ;;
            "heston")
                MODEL="heston"
                ;;
            "new-model")  # Add your model
                MODEL="new-model"
                ;;
            *)
                echo "Error: Invalid model: $1"
                exit 1
                ;;
        esac
        shift
        ;;
    # More cases...
esac

# Update the model selection logic
if [ "$MODEL" = "new-model" ]; then
    MODEL_TYPE=2  # Match the enum value
fi
```

6. **Add tests** for your new model in the `tests/` directory.

## Adding a New Market Data Source

To add a new market data source:

1. **Update DataSource enumeration** in `include/market_data.h`:

```c
typedef enum {
    DATA_SOURCE_DEFAULT = 0,
    DATA_SOURCE_ALPHAVANTAGE = 1,
    DATA_SOURCE_FINNHUB = 2,
    DATA_SOURCE_POLYGON = 3,
    DATA_SOURCE_NEW_SOURCE = 4  // Add your new source
} DataSource;
```

2. **Add API key handling** in `market_data.c`:

```c
// Add a global variable for the API key
static char* new_source_api_key = NULL;

// Update the set_api_key function
int set_api_key(DataSource source, const char* api_key) {
    switch (source) {
        // Existing cases...
        case DATA_SOURCE_NEW_SOURCE:
            if (new_source_api_key) {
                free(new_source_api_key);
            }
            new_source_api_key = strdup(api_key);
            return 0;
        default:
            return ERROR_INVALID_DATA_SOURCE;
    }
}

// Update the market_data_init function to load the key from config
// Look for NEW_SOURCE_API_KEY in the config file
```

3. **Implement the API request and parsing functions**:

```c
static double parse_price_new_source(const char* json_data, const char* ticker) {
    // Implementation to parse your API response
}

// Update the get_current_price function to use your new source
double get_current_price(const char* ticker, DataSource source, int* error_code) {
    // Existing code...
    
    switch (source) {
        // Existing cases...
        case DATA_SOURCE_NEW_SOURCE:
            if (!new_source_api_key) {
                set_error(error_code_ptr, ERROR_API_KEY_NOT_SET);
                return -1.0;
            }
            
            // Construct the API URL for your source
            snprintf(url, sizeof(url), "https://api.new-source.com/v1/quote?symbol=%s&apikey=%s",
                     ticker, new_source_api_key);
            
            // Make the request and handle the response
            // ...
            
            return parse_price_new_source(response.data, ticker);
        default:
            // Error handling...
    }
}

// Implement similar changes for other functions (get_dividend_yield, get_historical_prices, etc.)
```

4. **Update shell scripts** to support the new data source:

```bash
# In scripts/get_market_data.sh
case "$SOURCE" in
    "0") SOURCE_NAME="Default" ;;
    "1") SOURCE_NAME="Alpha Vantage" ;;
    "2") SOURCE_NAME="Finnhub" ;;
    "3") SOURCE_NAME="Polygon" ;;
    "4") SOURCE_NAME="New Source" ;;  # Add your source
    *) echo "Error: Invalid data source"; exit 1 ;;
esac
```

5. **Add tests** for your new data source in the `tests/` directory.

## Error Handling Conventions

The system uses standardized error codes defined in `error_handling.h`. When adding new functionality:

1. **Use existing error codes** where appropriate.
2. **Define new error codes** for unique error conditions.
3. **Set and check errors consistently**:

```c
if (error_condition) {
    set_error(ERROR_CODE);
    if (error_code_ptr) {
        *error_code_ptr = ERROR_CODE;
    }
    return error_return_value;
}
```

4. **Add error descriptions** for new error codes:

```c
// In error_handling.c
const char* get_error_description(int error_code) {
    switch (error_code) {
        // Existing cases...
        case NEW_ERROR_CODE:
            return "Description of the new error";
        default:
            return "Unknown error";
    }
}
```

## Testing

### Unit Tests

Create unit tests for new C functions in the appropriate test files.

### Shell Script Tests

For new functionality, add test cases to existing test scripts or create new ones:

```bash
#!/bin/bash
# tests/test_new_feature.sh

# Test setup...

# Test function
run_test() {
    local test_name="$1"
    local command="$2"
    local expected_result="$3"
    
    echo "Running test: ${test_name}"
    result=$($command 2>&1)
    exit_code=$?
    
    # Check results...
}

# Run tests
run_test "Test New Feature" "./scripts/option_pricer.sh --new-feature arg" "Expected output"

# Test cleanup...
```

### Memory Testing

Use Valgrind to check for memory leaks:

```bash
valgrind --leak-check=full --show-leak-kinds=all ./bin/unified_pricer --test-args
```

## Code Style Guide

Follow these conventions for consistency:

1. **Indentation**: 4 spaces, no tabs.
2. **Naming**:
   - Functions: `lower_case_with_underscores`
   - Constants/Macros: `UPPER_CASE_WITH_UNDERSCORES`
   - Variables: `lower_case_with_underscores`
   - Struct/Enum types: `CamelCase`
3. **Comments**:
   - Use Doxygen-style comments for functions and structures.
   - Add inline comments for complex logic.
4. **Error Handling**:
   - Check all function returns for errors.
   - Free all allocated memory.
   - Use consistent error reporting.
5. **Memory Management**:
   - Check all memory allocations.
   - Free memory in the reverse order of allocation.
   - Use helper functions for common allocation patterns.

## Documentation

When adding new features, update:

1. **API Documentation** in `docs/api/` for new functions/types.
2. **User Guide** in `docs/user/` for user-facing features.
3. **Developer Guide** in `docs/dev/` for implementation details.
4. **Examples** in `docs/examples/` to demonstrate usage.
5. **Header Files** with Doxygen-style comments for new functions.

Ensure your documentation answers:
- What does the feature do?
- How do users use it?
- What parameters/options are available?
- What errors might occur and how to handle them?
- Are there any performance considerations? 