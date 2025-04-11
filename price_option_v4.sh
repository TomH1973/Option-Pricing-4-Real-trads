#!/bin/bash

# Enhanced version of price_option.sh with configurable FFT parameters
# for the stochastic volatility model

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
  echo "  --fft-n=VALUE          Set FFT points (power of 2, default: 4096)"
  echo "  --log-strike-range=X   Set log strike range (default: 3.0)"
  echo "  --alpha=X              Set Carr-Madan alpha parameter (default: 1.5)"
  echo "  --eta=X                Set grid spacing parameter (default: 0.05)"
  echo "  --cache-tolerance=X    Set cache tolerance parameter (default: 1e-5)"
  echo "  --debug                Enable debug output"
  echo "  --help                 Display this help message"
  echo "Example: $0 --fft-n=8192 5616 5600 9 118.5 0.013"
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
    --cache-tolerance=*)
      CACHE_TOL="${arg#*=}"
      FFT_PARAMS="$FFT_PARAMS --cache-tolerance=$CACHE_TOL"
      ;;
    --debug)
      DEBUG_FLAG="--debug"
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

# Calculate implied volatility using the enhanced FFT-based model
if [[ -n "$DEBUG_FLAG" ]]; then
  echo "Running with FFT parameters: $FFT_PARAMS"
fi

IVSV=$(/home/usr0/projects/option_tools/calculate_sv_v4 $DEBUG_FLAG $FFT_PARAMS "$OPTION_PRICE" "$UNDERLYING_PRICE" "$STRIKE" "$T" "$R" "$Q")
if [[ $? -ne 0 ]]; then
  echo "Implied volatility calculation (SV) failed."
  exit 1
fi

# Display results with FFT parameter info
echo   "=============== Option Pricing Summary ==============="
printf "%-25s %12.2f\n" "Underlying Price:" "$UNDERLYING_PRICE"
printf "%-25s %12.2f\n" "Strike:" "$STRIKE"
printf "%-25s %16.6f\n" "Time to expiry (yrs):" "$T"
printf "%-25s %14.4f%%\n" "Risk-Free Rate:" "$(echo "$R*100" | bc -l)"
printf "%-25s %14.4f%%\n" "Underlying Yield:" "$(echo "$Q*100" | bc -l)"
printf "%-25s %14.4f\n" "Market Option Price:" "$OPTION_PRICE"
printf "%-25s %12.2f%%\n" "Implied Volatility(BS):" "$(echo "$IVBS"*100 | bc -l)"
printf "%-25s %12.2f%%\n" "Implied Volatility(SV):" "$(echo "$IVSV"*100 | bc -l)"
printf "%-25s %s\n" "SV Model Used:" "Stochastic Volatility (FFT-V4)"

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
if [[ -n "$CACHE_TOL" ]]; then
  printf "%-25s %s\n" "Cache Tolerance:" "$CACHE_TOL"
fi
echo   "======================================================"
