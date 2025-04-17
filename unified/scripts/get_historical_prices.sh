#!/bin/bash
#
# get_historical_prices.sh - Script to retrieve historical price data for a ticker
#
# Usage: ./get_historical_prices.sh [OPTIONS] TICKER DAYS
#
# Options:
#   --verbose             Show detailed output
#   --with-volatility     Also calculate historical volatility
#   --source SOURCE       Data source (0: default, 1: Alpha Vantage, 2: Finnhub, 3: Polygon)
#   --output FORMAT       Output format (csv, json) - default: csv
#   --help                Display this help message
#

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
UNIFIED_ROOT="$PROJECT_ROOT/unified"
BINARY_DIR="$UNIFIED_ROOT/bin"
PRICER_BIN="$BINARY_DIR/unified_pricer"

# Defaults
VERBOSE=0
WITH_VOLATILITY=0
SOURCE=0
OUTPUT_FORMAT="csv"

# Print usage
usage() {
    echo "Usage: $0 [OPTIONS] TICKER DAYS"
    echo ""
    echo "Options:"
    echo "  --verbose             Show detailed output"
    echo "  --with-volatility     Also calculate historical volatility"
    echo "  --source SOURCE       Data source (0: default, 1: Alpha Vantage, 2: Finnhub, 3: Polygon)"
    echo "  --output FORMAT       Output format (csv, json) - default: csv"
    echo "  --help                Display this help message"
    exit 1
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --verbose)
            VERBOSE=1
            shift
            ;;
        --with-volatility)
            WITH_VOLATILITY=1
            shift
            ;;
        --source)
            if [[ -n "$2" && "$2" =~ ^[0-3]$ ]]; then
                SOURCE="$2"
                shift 2
            else
                echo "Error: Source must be a number between 0 and 3"
                exit 1
            fi
            ;;
        --output)
            if [[ -n "$2" && "$2" =~ ^(csv|json)$ ]]; then
                OUTPUT_FORMAT="$2"
                shift 2
            else
                echo "Error: Output format must be 'csv' or 'json'"
                exit 1
            fi
            ;;
        --help)
            usage
            ;;
        -*)
            echo "Error: Unknown option: $1"
            usage
            ;;
        *)
            break
            ;;
    esac
done

# Check required arguments
if [[ $# -lt 2 ]]; then
    echo "Error: Missing required arguments"
    usage
fi

TICKER="$1"
DAYS="$2"

# Validate arguments
if [[ ! "$DAYS" =~ ^[0-9]+$ ]]; then
    echo "Error: DAYS must be a positive integer"
    exit 1
fi

if [[ $DAYS -lt 1 || $DAYS -gt 365 ]]; then
    echo "Error: DAYS must be between 1 and 365"
    exit 1
fi

# Execute the historical price retrieval
if [[ $VERBOSE -eq 1 ]]; then
    echo "Retrieving $DAYS days of historical prices for $TICKER..."
    echo "Using data source: $SOURCE"
fi

# Create temp files for results
TEMP_DIR=$(mktemp -d)
PRICES_FILE="$TEMP_DIR/prices.txt"
DATES_FILE="$TEMP_DIR/dates.txt"
RESULT_FILE="$TEMP_DIR/result.txt"

# Call the unified pricer with the historical prices option
COMMAND="$PRICER_BIN --get-historical-prices $TICKER $DAYS $SOURCE"

if [[ $VERBOSE -eq 1 ]]; then
    echo "Executing: $COMMAND"
fi

# Execute the command
RESULT=$($COMMAND 2>&1)
EXIT_CODE=$?

if [[ $EXIT_CODE -ne 0 ]]; then
    if [[ $VERBOSE -eq 1 ]]; then
        echo "Error retrieving historical prices:"
        echo "$RESULT"
    else
        echo "Error: Failed to retrieve historical prices for $TICKER"
    fi
    rm -rf "$TEMP_DIR"
    exit $EXIT_CODE
fi

# Parse the results
if [[ $VERBOSE -eq 1 ]]; then
    if [[ -f "$PRICES_FILE" && -f "$DATES_FILE" ]]; then
        echo "Successfully retrieved historical prices from API"
    else
        echo "Retrieved historical prices from cache"
    fi
fi

# Process the output format
if [[ "$OUTPUT_FORMAT" == "csv" ]]; then
    echo "Date,Price" > "$RESULT_FILE"
    paste -d, "$DATES_FILE" "$PRICES_FILE" >> "$RESULT_FILE"
else # json
    echo "{" > "$RESULT_FILE"
    echo "  \"ticker\": \"$TICKER\"," >> "$RESULT_FILE"
    echo "  \"days\": $DAYS," >> "$RESULT_FILE"
    echo "  \"prices\": [" >> "$RESULT_FILE"
    
    # Combine dates and prices into JSON format
    LINE_COUNT=$(wc -l < "$DATES_FILE")
    CURRENT_LINE=0
    
    while IFS='' read -r DATE && IFS='' read -r PRICE <&3; do
        CURRENT_LINE=$((CURRENT_LINE + 1))
        if [[ $CURRENT_LINE -eq $LINE_COUNT ]]; then
            echo "    {\"date\": \"$DATE\", \"price\": $PRICE}" >> "$RESULT_FILE"
        else
            echo "    {\"date\": \"$DATE\", \"price\": $PRICE}," >> "$RESULT_FILE"
        fi
    done < "$DATES_FILE" 3< "$PRICES_FILE"
    
    echo "  ]" >> "$RESULT_FILE"
    
    # Add volatility if requested
    if [[ $WITH_VOLATILITY -eq 1 ]]; then
        VOL_COMMAND="$PRICER_BIN --get-volatility $TICKER $DAYS $SOURCE"
        VOLATILITY=$($VOL_COMMAND 2>/dev/null)
        
        if [[ -n "$VOLATILITY" && "$VOLATILITY" != "Error"* ]]; then
            echo "  ,\"volatility\": $VOLATILITY" >> "$RESULT_FILE"
        fi
    fi
    
    echo "}" >> "$RESULT_FILE"
fi

# Output the result
cat "$RESULT_FILE"

# Clean up
rm -rf "$TEMP_DIR"

exit 0 