#!/bin/bash

# Robust version of price_option.sh with error recovery and fallback mechanisms
# Uses calculate_sv_v4_debug for enhanced error handling

# Fetch market data 
fetch_risk_free_rate() {
  RATE=$(curl -s "https://www.cnbc.com/quotes/US10Y" -H "User-Agent: Mozilla/5.0" | grep -oP '(?<=<span class="QuoteStrip-lastPrice">)[0-9.]+'| head -1)
  if [[ -z "$RATE" ]]; then
  echo "Warning: Could not fetch risk-free rate, defaulting to 4.32 %." >&2
  echo "4.32"
  else
    echo "$RATE"
  fi
}

print_usage() {
  echo "Usage: $0 [options] UNDERLYING_PRICE STRIKE DAYS_TO_EXPIRY OPTION_PRICE UNDERLYING_YIELD"
  echo "Options:"
  echo "  --debug                Enable debug output"
  echo "  --verbose-debug        Enable verbose debug output"
  echo "  --fft-n=VALUE          Set initial FFT points (power of 2, default: 4096)"
  echo "  --log-strike-range=X   Set initial log strike range (default: 3.0)"
  echo "  --alpha=X              Set initial Carr-Madan alpha parameter (default: 1.5)"
  echo "  --eta=X                Set initial grid spacing parameter (default: 0.05)"
  echo "  --help                 Display this help message"
  echo ""
  echo "Note: This script includes error recovery and will try multiple parameter sets if needed"
  echo "Example: $0 5616 5600 9 118.5 0.013"
}

# Check for help flag first
for arg in "$@"; do
  if [[ "$arg" == "--help" ]] || [[ "$arg" == "-h" ]]; then
    print_usage
    exit 0
  fi
done

# Process options
FFT_PARAMS=""
DEBUG_FLAG=""
args=()

for arg in "$@"; do
  case "$arg" in
    --fft-n=*)
      FFT_N="${arg#*=}"
      FFT_PARAMS="$FFT_PARAMS --fft-n=$FFT_N"
      ;;
    --log-strike-range=*)
      LOG_RANGE="${arg#*=}"
      FFT_PARAMS="$FFT_PARAMS --log-strike-range=$LOG_RANGE"
      ;;
    --alpha=*)
      ALPHA="${arg#*=}"
      FFT_PARAMS="$FFT_PARAMS --alpha=$ALPHA"
      ;;
    --eta=*)
      ETA="${arg#*=}"
      FFT_PARAMS="$FFT_PARAMS --eta=$ETA"
      ;;
    --debug)
      DEBUG_FLAG="--debug"
      ;;
    --verbose-debug)
      DEBUG_FLAG="--verbose-debug"
      ;;
    --help)
      print_usage
      exit 0
      ;;
    --*)
      echo "Error: Unknown option: $arg"
      print_usage
      exit 1
      ;;
    *)
      # Not an option, add to arguments list
      args+=("$arg")
      ;;
  esac
done

# Check if we have the correct number of arguments
if [ ${#args[@]} -ne 5 ]; then
  echo "Error: Incorrect number of arguments"
  print_usage
  exit 1
fi

UNDERLYING_PRICE=${args[0]}
STRIKE=${args[1]}
DAYS_TO_EXPIRY=${args[2]}
OPTION_PRICE=${args[3]}
UNDERLYING_YIELD=${args[4]}

# Convert days to expiry to years
T=$(echo "$DAYS_TO_EXPIRY / 365.25" | bc -l)

# Get risk-free rate dynamically
RISK_FREE_RATE=$(fetch_risk_free_rate)
if [[ -z $RISK_FREE_RATE ]]; then
  echo "Failed to fetch risk-free rate."
  exit 1
fi

# Convert rates to decimal
R=$(echo "$RISK_FREE_RATE / 100" | bc -l)
Q=$UNDERLYING_YIELD

# Calculate implied volatility using the Black-Scholes model
IVBS=$(/home/usr0/projects/option_tools/calculate_iv_v2 "$OPTION_PRICE" "$UNDERLYING_PRICE" "$STRIKE" "$T" "$R" "$Q")
if [[ $? -ne 0 ]]; then
  echo "Implied volatility calculation (BS) failed."
  exit 1
fi

# Calculate implied volatility using the robust debug implementation
if [[ -n "$DEBUG_FLAG" ]]; then
  echo "Running with parameters: $FFT_PARAMS $DEBUG_FLAG"
fi

IVSV=$(/home/usr0/projects/option_tools/calculate_sv_v4_debug $DEBUG_FLAG $FFT_PARAMS "$OPTION_PRICE" "$UNDERLYING_PRICE" "$STRIKE" "$T" "$R" "$Q")
SV_RESULT=$?

if [[ $SV_RESULT -ne 0 ]]; then
  echo "Warning: Robust SV calculation failed. Falling back to Black-Scholes."
  IVSV=$IVBS
  FALLBACK_USED="Yes (Black-Scholes)"
else
  FALLBACK_USED="No"
fi

# Calculate moneyness for display purposes
MONEYNESS=$(echo "scale=4; $STRIKE / $UNDERLYING_PRICE" | bc -l)

# Display results with error handling info
echo   "=============== Option Pricing Summary ==============="
printf "%-25s %12.2f\n" "Underlying Price:" "$UNDERLYING_PRICE"
printf "%-25s %12.2f\n" "Strike:" "$STRIKE"
printf "%-25s %16.4f\n" "Moneyness (K/S):" "$MONEYNESS"
printf "%-25s %16.6f\n" "Time to expiry (yrs):" "$T"
printf "%-25s %14.4f%%\n" "Risk-Free Rate:" "$(echo "$R*100" | bc -l)"
printf "%-25s %14.4f%%\n" "Underlying Yield:" "$(echo "$Q*100" | bc -l)"
printf "%-25s %14.4f\n" "Market Option Price:" "$OPTION_PRICE"
printf "%-25s %12.2f%%\n" "Implied Volatility(BS):" "$(echo "$IVBS"*100 | bc -l)"
printf "%-25s %12.2f%%\n" "Implied Volatility(SV):" "$(echo "$IVSV"*100 | bc -l)"
printf "%-25s %s\n" "SV Model Used:" "Stochastic Volatility (FFT-V4 Debug)"
printf "%-25s %s\n" "Fallback Used:" "$FALLBACK_USED"

# Display FFT parameters if any were specified
if [[ -n "$FFT_N" ]]; then
  printf "%-25s %s\n" "FFT Points:" "$FFT_N"
fi
if [[ -n "$LOG_RANGE" ]]; then
  printf "%-25s %s\n" "Log Strike Range:" "$LOG_RANGE"
fi
if [[ -n "$ALPHA" ]]; then
  printf "%-25s %s\n" "Alpha Parameter:" "$ALPHA"
fi
if [[ -n "$ETA" ]]; then
  printf "%-25s %s\n" "Eta Parameter:" "$ETA"
fi
echo   "======================================================"