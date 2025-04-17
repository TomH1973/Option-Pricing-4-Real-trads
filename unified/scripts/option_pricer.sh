#!/bin/bash
#
# option_pricer.sh - Unified shell interface for option pricing tools
#

# Default values
MODEL="bs"           # bs (Black-Scholes) or heston
METHOD="analytic"    # analytic, quadrature, fft
OPTION_TYPE="call"   # call or put
TICKER="SPX"         # Default to S&P 500 index
CALCULATE_GREEKS=0   # Don't calculate Greeks by default
VERBOSE=0            # Don't show verbose output by default
USE_CACHED_DATA=0    # Don't use cached data by default
DATA_SOURCE="default" # Default data source

# Get script directory for relative path resolution
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
BINARY_DIR="$PROJECT_ROOT/unified/bin"
SCRIPT_PATH="$PROJECT_ROOT/unified/scripts"

# Error codes
E_SUCCESS=0
E_PARAM_ERR=1
E_FILE_NOT_FOUND=2
E_EXEC_ERR=3
E_DATA_ERR=4

# Help message
show_help() {
    cat << EOF
Usage: $(basename $0) [OPTIONS] SPOT_PRICE STRIKE_PRICE DAYS_TO_EXPIRY [OPTION_PRICE] [DIVIDEND_YIELD]

Price an option using the unified option pricing system.

Required parameters:
  SPOT_PRICE        Current price of the underlying asset
  STRIKE_PRICE      Strike price of the option
  DAYS_TO_EXPIRY    Days until option expiration

Optional parameters:
  OPTION_PRICE      Market price (for implied volatility calculation)
  DIVIDEND_YIELD    Annualized dividend yield (percentage)

Options:
  -h, --help              Show this help message
  -m, --model MODEL       Pricing model to use: 'bs' (Black-Scholes) or 'heston' (default: bs)
  -n, --method METHOD     Numerical method: 'analytic', 'quadrature', 'fft' (default: analytic)
  -t, --type TYPE         Option type: 'call' or 'put' (default: call)
  --ticker SYMBOL         Ticker symbol for market data (default: SPX)
  -g, --greeks            Calculate option Greeks
  -v, --verbose           Show verbose output
  -r, --rate RATE         Risk-free interest rate (percentage, default: from treasury)
  --vol VOLATILITY        Starting volatility (percentage, for calculation without market price)
  --data-source SOURCE    Market data source: 'default', 'alphavantage', 'finnhub', 'polygon' (default: default)
  --use-cached            Use cached market data (default: fetch fresh data)
  --auto-vol              Automatically determine historical volatility based on expiry
  --rate-term TERM        Term for risk-free rate: '1month', '3month', '6month', '1year', etc. (default: '3month')
  --fft-n N               Number of FFT points (for FFT method)
  --alpha VALUE           Alpha parameter (for FFT method)

Examples:
  $(basename $0) 4500 4600 30                        # Price SPX call option, 30 days to expiry
  $(basename $0) --ticker AAPL 180 175 45 2.50       # Calculate IV for AAPL call, market price $2.50
  $(basename $0) -m heston -n fft 100 110 60 1.5     # Price with Heston model using FFT
  $(basename $0) -t put --greeks 50 55 10 0.75       # Price put option and calculate Greeks
  $(basename $0) --ticker MSFT --auto-vol 300 310 45 # Price MSFT using auto-fetched market data and volatility
EOF
    exit $E_SUCCESS
}

# Function to validate numeric input
validate_numeric() {
    local val="$1"
    local param_name="$2"
    
    # Check if the value is a valid number
    if ! echo "$val" | grep -E '^-?[0-9]+(\.[0-9]+)?$' >/dev/null; then
        echo "Error: $param_name must be a number." >&2
        exit $E_PARAM_ERR
    fi
}

# Function to validate option type
validate_option_type() {
    if [[ "$OPTION_TYPE" != "call" && "$OPTION_TYPE" != "put" ]]; then
        echo "Error: Option type must be 'call' or 'put'." >&2
        exit $E_PARAM_ERR
    fi
}

# Function to validate model type
validate_model() {
    if [[ "$MODEL" != "bs" && "$MODEL" != "heston" ]]; then
        echo "Error: Pricing model must be 'bs' or 'heston'." >&2
        exit $E_PARAM_ERR
    fi
}

# Function to validate numerical method
validate_method() {
    if [[ "$METHOD" != "analytic" && "$METHOD" != "quadrature" && "$METHOD" != "fft" ]]; then
        echo "Error: Numerical method must be 'analytic', 'quadrature', or 'fft'." >&2
        exit $E_PARAM_ERR
    fi
    
    # Check for model-method compatibility
    if [[ "$MODEL" == "bs" && "$METHOD" != "analytic" ]]; then
        echo "Error: Black-Scholes model only supports analytic method." >&2
        exit $E_PARAM_ERR
    fi
    
    if [[ "$MODEL" == "heston" && "$METHOD" == "analytic" ]]; then
        echo "Error: Heston model does not support analytic method." >&2
        exit $E_PARAM_ERR
    fi
}

# Function to get market data for a ticker
get_market_data() {
    local ticker=$1
    local use_cached=$2
    local data_source=$3
    
    if [[ $VERBOSE -eq 1 ]]; then
        echo "Getting market data for $ticker..." >&2
    fi
    
    # Map data sources to numeric values used by the C code
    local source_num=0
    case "$data_source" in
        "alphavantage")
            source_num=1
            ;;
        "finnhub")
            source_num=2
            ;;
        "polygon")
            source_num=3
            ;;
        *)
            source_num=0  # Default
            ;;
    esac
    
    # Use our market data script
    local cmd="$SCRIPT_PATH/get_market_data.sh"
    
    # Add options
    if [[ $VERBOSE -eq 1 ]]; then
        cmd="$cmd --verbose"
    fi
    
    if [[ $use_cached -eq 1 ]]; then
        cmd="$cmd --use-cached"
    fi
    
    cmd="$cmd --source $source_num $ticker"
    
    if [[ $VERBOSE -eq 1 ]]; then
        echo "Executing: $cmd" >&2
    fi
    
    local result=$($cmd 2>/dev/null)
    local exit_code=$?
    
    if [[ $exit_code -ne 0 ]]; then
        if [[ $VERBOSE -eq 1 ]]; then
            echo "Failed to get market data for $ticker (error code: $exit_code)" >&2
        fi
        
        # If we failed, try with some default values for common tickers
        if [[ "$ticker" == "SPX" ]]; then
            echo "4700.50 1.42" # Sample spot price and dividend yield for S&P 500
            return $E_SUCCESS
        elif [[ "$ticker" == "AAPL" ]]; then
            echo "180.75 0.51" # Sample for Apple
            return $E_SUCCESS
        elif [[ "$ticker" == "MSFT" ]]; then
            echo "325.40 0.72" # Sample for Microsoft
            return $E_SUCCESS
        else
            echo "0 0" # Default values indicating we couldn't get data
            return $E_DATA_ERR
        fi
    fi
    
    # Get dividend yield also
    local div_cmd="$SCRIPT_PATH/get_market_data.sh --type dividend"
    
    if [[ $VERBOSE -eq 1 ]]; then
        div_cmd="$div_cmd --verbose"
    fi
    
    if [[ $use_cached -eq 1 ]]; then
        div_cmd="$div_cmd --use-cached"
    fi
    
    div_cmd="$div_cmd --source $source_num $ticker"
    
    local div_result=$($div_cmd 2>/dev/null)
    local div_exit_code=$?
    
    if [[ $div_exit_code -ne 0 ]]; then
        if [[ $VERBOSE -eq 1 ]]; then
            echo "Failed to get dividend yield for $ticker (error code: $div_exit_code)" >&2
        fi
        div_result="0"
    fi
    
    # Parse the result: combine price and dividend yield
    echo "$result $div_result"
    return $E_SUCCESS
}

# Function to get historical volatility based on expiry
get_historical_volatility() {
    local ticker=$1
    local days_to_expiry=$2
    local data_source=$3
    
    if [[ $VERBOSE -eq 1 ]]; then
        echo "Determining historical volatility period for $days_to_expiry days to expiry..." >&2
    fi
    
    # Determine appropriate lookback period based on option expiry
    local vol_period
    if [[ $days_to_expiry -le 7 ]]; then
        vol_period=10    # Use 10-day historical vol for very short term
    elif [[ $days_to_expiry -le 30 ]]; then
        vol_period=20    # Use 20-day for short term
    elif [[ $days_to_expiry -le 90 ]]; then
        vol_period=60    # Use 60-day for medium term
    elif [[ $days_to_expiry -le 180 ]]; then
        vol_period=90    # Use 90-day for longer term
    else
        vol_period=180   # Use 180-day for LEAPS
    fi
    
    if [[ $VERBOSE -eq 1 ]]; then
        echo "Using $vol_period-day historical volatility for $days_to_expiry-day option." >&2
    fi
    
    # Map data sources to numeric values used by the C code
    local source_num=0
    case "$data_source" in
        "alphavantage")
            source_num=1
            ;;
        "finnhub")
            source_num=2
            ;;
        "polygon")
            source_num=3
            ;;
        *)
            source_num=0  # Default
            ;;
    esac
    
    # Use our market data script to get historical volatility
    local cmd="$SCRIPT_PATH/get_market_data.sh --type volatility --days $vol_period"
    
    if [[ $VERBOSE -eq 1 ]]; then
        cmd="$cmd --verbose"
    fi
    
    cmd="$cmd --source $source_num $ticker"
    
    if [[ $VERBOSE -eq 1 ]]; then
        echo "Executing: $cmd" >&2
    fi
    
    local result=$($cmd 2>/dev/null)
    local exit_code=$?
    
    if [[ $exit_code -ne 0 ]]; then
        if [[ $VERBOSE -eq 1 ]]; then
            echo "Failed to get historical volatility for $ticker (error code: $exit_code)" >&2
        fi
        
        # If we failed, use reasonable default values for common indices
        if [[ "$ticker" == "SPX" ]]; then
            echo "0.15" # 15% for S&P 500
        elif [[ "$ticker" == "AAPL" ]]; then
            echo "0.25" # 25% for Apple
        elif [[ "$ticker" == "MSFT" ]]; then
            echo "0.22" # 22% for Microsoft
        else
            echo "0.30" # 30% default for stocks
        fi
        return $E_DATA_ERR
    fi
    
    # Parse the result
    echo "$result"
    return $E_SUCCESS
}

# Function to get risk-free rate
get_risk_free_rate() {
    local term=$1
    
    if [[ $VERBOSE -eq 1 ]]; then
        echo "Getting risk-free rate for $term term..." >&2
    fi
    
    # Use our market data script
    local cmd="$SCRIPT_PATH/get_market_data.sh --type rate --rate-term $term"
    
    if [[ $VERBOSE -eq 1 ]]; then
        cmd="$cmd --verbose"
    fi
    
    if [[ $VERBOSE -eq 1 ]]; then
        echo "Executing: $cmd" >&2
    fi
    
    local result=$($cmd 2>/dev/null)
    local exit_code=$?
    
    if [[ $exit_code -ne 0 ]]; then
        if [[ $VERBOSE -eq 1 ]]; then
            echo "Failed to get risk-free rate for $term (error code: $exit_code)" >&2
        fi
        
        # Use reasonable defaults based on current environment
        case "$term" in
            "1month")  echo "0.0175" ;; # 1.75%
            "3month")  echo "0.0185" ;; # 1.85%
            "6month")  echo "0.0195" ;; # 1.95%
            "1year")   echo "0.021"  ;; # 2.1%
            "2year")   echo "0.023"  ;; # 2.3%
            "5year")   echo "0.025"  ;; # 2.5%
            "10year")  echo "0.027"  ;; # 2.7%
            "30year")  echo "0.029"  ;; # 2.9%
            *)         echo "0.02"   ;; # Default 2%
        esac
        return $E_DATA_ERR
    fi
    
    # Parse the result
    echo "$result"
    return $E_SUCCESS
}

# Parse command line arguments
POSITIONAL=()
AUTO_VOLATILITY=0
RATE_TERM="3month"

while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
        -h|--help)
            show_help
            ;;
        -m|--model)
            MODEL="$2"
            shift 2
            ;;
        -n|--method)
            METHOD="$2"
            shift 2
            ;;
        -t|--type)
            OPTION_TYPE="$2"
            shift 2
            ;;
        --ticker)
            TICKER="$2"
            shift 2
            ;;
        -g|--greeks)
            CALCULATE_GREEKS=1
            shift
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        -r|--rate)
            RATE="$2"
            shift 2
            ;;
        --vol)
            VOLATILITY="$2"
            shift 2
            ;;
        --data-source)
            DATA_SOURCE="$2"
            shift 2
            ;;
        --use-cached)
            USE_CACHED_DATA=1
            shift
            ;;
        --auto-vol)
            AUTO_VOLATILITY=1
            shift
            ;;
        --rate-term)
            RATE_TERM="$2"
            shift 2
            ;;
        --fft-n)
            FFT_N="$2"
            shift 2
            ;;
        --alpha)
            ALPHA="$2"
            shift 2
            ;;
        *)
            POSITIONAL+=("$1")
            shift
            ;;
    esac
done

# Restore positional parameters
set -- "${POSITIONAL[@]}"

# Check for required arguments
if [ $# -lt 3 ]; then
    echo "Error: Missing required parameters." >&2
    echo "Run '$(basename $0) --help' for usage information." >&2
    exit $E_PARAM_ERR
fi

# Assign positional parameters
SPOT_PRICE="$1"
STRIKE_PRICE="$2"
DAYS_TO_EXPIRY="$3"
OPTION_PRICE="$4"
DIVIDEND_YIELD="$5"

# Validate numeric inputs
validate_numeric "$SPOT_PRICE" "Spot price"
validate_numeric "$STRIKE_PRICE" "Strike price"
validate_numeric "$DAYS_TO_EXPIRY" "Days to expiry"

if [ -n "$OPTION_PRICE" ]; then
    validate_numeric "$OPTION_PRICE" "Option price"
fi

if [ -n "$DIVIDEND_YIELD" ]; then
    validate_numeric "$DIVIDEND_YIELD" "Dividend yield"
fi

if [ -n "$RATE" ]; then
    validate_numeric "$RATE" "Interest rate"
fi

if [ -n "$VOLATILITY" ]; then
    validate_numeric "$VOLATILITY" "Volatility"
fi

# Validate option type, model, and method
validate_option_type
validate_model
validate_method

# Convert days to years
TIME_TO_EXPIRY=$(echo "scale=6; $DAYS_TO_EXPIRY / 365" | bc)

# If ticker is provided and no spot price/dividend yield, fetch market data
if [ -n "$TICKER" ] && { [ -z "$DIVIDEND_YIELD" ] || [ "$SPOT_PRICE" == "0" ]; }; then
    # Fetch market data if not using cached data
    if [ $USE_CACHED_DATA -eq 0 ]; then
        if [ $VERBOSE -eq 1 ]; then
            echo "Fetching market data for $TICKER from $DATA_SOURCE..."
        fi
        
        # Call the market data fetching function
        MARKET_DATA=$(get_market_data "$TICKER" $USE_CACHED_DATA $DATA_SOURCE)
        
        # Parse the result (simple space-separated format for now)
        if [ $? -eq $E_SUCCESS ]; then
            read -r FETCHED_SPOT FETCHED_DIV <<< "$MARKET_DATA"
            
            # Use fetched values if parameters weren't specified
            if [ "$SPOT_PRICE" == "0" ]; then
                SPOT_PRICE="$FETCHED_SPOT"
                if [ $VERBOSE -eq 1 ]; then
                    echo "Using spot price $SPOT_PRICE from market data" >&2
                fi
            fi
            
            if [ -z "$DIVIDEND_YIELD" ]; then
                DIVIDEND_YIELD="$FETCHED_DIV"
                if [ $VERBOSE -eq 1 ]; then
                    echo "Using dividend yield $DIVIDEND_YIELD from market data" >&2
                fi
            fi
        else
            echo "Warning: Failed to fetch market data. Using provided values." >&2
        fi
    else
        echo "Using cached market data (--use-cached flag is set)" >&2
    fi
fi

# If dividend yield is still not set, default to 0
if [ -z "$DIVIDEND_YIELD" ]; then
    DIVIDEND_YIELD="0"
fi

# If interest rate is not specified, fetch it for the given term
if [ -z "$RATE" ]; then
    FETCHED_RATE=$(get_risk_free_rate "$RATE_TERM")
    if [ $? -eq $E_SUCCESS ]; then
        RATE=$(echo "scale=6; $FETCHED_RATE * 100" | bc)
        if [ $VERBOSE -eq 1 ]; then
            echo "Using $RATE_TERM risk-free rate: ${RATE}%" >&2
        fi
    else
        RATE="5.0"  # Default to 5% if fetching fails
        echo "Warning: Could not fetch risk-free rate. Using default: ${RATE}%" >&2
    fi
fi

# If auto-volatility is enabled and no volatility is specified, calculate historical volatility
if [ $AUTO_VOLATILITY -eq 1 ] && [ -z "$VOLATILITY" ] && [ -n "$TICKER" ]; then
    FETCHED_VOL=$(get_historical_volatility "$TICKER" $DAYS_TO_EXPIRY $DATA_SOURCE)
    if [ $? -eq $E_SUCCESS ]; then
        VOLATILITY=$(echo "scale=2; $FETCHED_VOL * 100" | bc)
        if [ $VERBOSE -eq 1 ]; then
            echo "Using historical volatility for $TICKER: ${VOLATILITY}%" >&2
        fi
    else
        # If we have option price, we'll calculate IV, so no need for a default
        if [ -z "$OPTION_PRICE" ]; then
            VOLATILITY="20.0"  # Default to 20% if fetching fails
            echo "Warning: Could not determine historical volatility. Using default: ${VOLATILITY}%" >&2
        fi
    fi
fi

# Convert percentage values to decimal
RATE_DECIMAL=$(echo "scale=6; $RATE / 100" | bc)
DIVIDEND_DECIMAL=$(echo "scale=6; $DIVIDEND_YIELD / 100" | bc)

if [ -n "$VOLATILITY" ]; then
    VOLATILITY_DECIMAL=$(echo "scale=6; $VOLATILITY / 100" | bc)
else
    VOLATILITY_DECIMAL="0"  # Will trigger implied volatility calculation if market price is provided
fi

# Set model type numeric value for the binary
if [ "$MODEL" == "bs" ]; then
    MODEL_TYPE="0"  # Black-Scholes
else
    MODEL_TYPE="1"  # Heston
fi

# Set numerical method numeric value for the binary
case "$METHOD" in
    analytic)
        METHOD_TYPE="0"
        ;;
    quadrature)
        METHOD_TYPE="1"
        ;;
    fft)
        METHOD_TYPE="2"
        ;;
esac

# Set option type numeric value for the binary
if [ "$OPTION_TYPE" == "call" ]; then
    OPTION_TYPE_NUM="0"
else
    OPTION_TYPE_NUM="1"
fi

# Build the command arguments
ARGS=("$SPOT_PRICE" "$STRIKE_PRICE" "$TIME_TO_EXPIRY" "$RATE_DECIMAL" "$DIVIDEND_DECIMAL")
ARGS+=("$VOLATILITY_DECIMAL" "$OPTION_TYPE_NUM" "$MODEL_TYPE" "$METHOD_TYPE")

# Add option price if provided
if [ -n "$OPTION_PRICE" ]; then
    ARGS+=("$OPTION_PRICE")
else
    ARGS+=("0")  # Default to 0 if not provided
fi

# Add Greeks flag
ARGS+=("$CALCULATE_GREEKS")

# Add ticker if provided
if [ -n "$TICKER" ]; then
    ARGS+=("$TICKER")
else
    ARGS+=("")  # Empty string for no ticker
fi

# Output summary if verbose
if [ $VERBOSE -eq 1 ]; then
    echo -e "\n====== Option Pricing Parameters ======"
    echo "  Spot Price:      $SPOT_PRICE"
    echo "  Strike Price:    $STRIKE_PRICE"
    echo "  Days to Expiry:  $DAYS_TO_EXPIRY (${TIME_TO_EXPIRY} years)"
    echo "  Option Type:     $OPTION_TYPE"
    echo "  Model:           $MODEL"
    echo "  Method:          $METHOD"
    echo "  Interest Rate:   ${RATE}%"
    echo "  Dividend Yield:  ${DIVIDEND_YIELD}%"

    if [ -n "$VOLATILITY" ]; then
        echo "  Volatility:      ${VOLATILITY}%"
    fi

    if [ -n "$OPTION_PRICE" ]; then
        echo "  Market Price:    $OPTION_PRICE (will calculate implied volatility)"
    fi

    if [ $CALCULATE_GREEKS -eq 1 ]; then
        echo "  Greeks:          Will be calculated"
    fi

    if [ -n "$TICKER" ]; then
        echo "  Ticker Symbol:   $TICKER"
    fi
    echo "======================================"
fi

# Check if the binary exists
UNIFIED_BIN="$BINARY_DIR/unified_pricer"
if [ ! -f "$UNIFIED_BIN" ]; then
    echo "Error: Unified pricing binary not found at $UNIFIED_BIN" >&2
    exit $E_FILE_NOT_FOUND
fi

# Execute the command
"$UNIFIED_BIN" "${ARGS[@]}"
EXIT_CODE=$?

if [ $EXIT_CODE -ne 0 ]; then
    echo "Error: Option pricing calculation failed (exit code $EXIT_CODE)" >&2
    exit $E_EXEC_ERR
fi

exit $E_SUCCESS 