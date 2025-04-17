#!/bin/bash

# Advanced version of price_option.sh for the stochastic volatility model
# Uses calculate_sv_v6 which implements enhanced calibration to reduce Black-Scholes fallbacks
# and provides more accurate Heston model calibration

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
  echo "  --no-bs-fallback       Disable Black-Scholes fallback mechanism"
  echo "  --max-attempts=N       Set maximum calibration attempts (default: 3)"
  echo "  --help                 Display this help message"
  echo ""
  echo "Note: FFT parameters are automatically adapted based on option characteristics"
  echo "This version (v6) uses enhanced calibration to reduce Black-Scholes fallbacks"
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
DEBUG_FLAG=""
SV_EXTRA_FLAGS=""
args=()
MAX_ATTEMPTS=""

for arg in "$@"; do
  case "$arg" in
    --debug)
      DEBUG_FLAG="--debug"
      ;;
    --verbose-debug)
      DEBUG_FLAG="--verbose-debug"
      ;;
    --no-bs-fallback)
      SV_EXTRA_FLAGS="$SV_EXTRA_FLAGS --no-bs-fallback"
      ;;
    --max-attempts=*)
      MAX_ATTEMPTS="${arg#*=}"
      if [[ "$MAX_ATTEMPTS" =~ ^[0-9]+$ ]]; then
        SV_EXTRA_FLAGS="$SV_EXTRA_FLAGS --max-attempts=$MAX_ATTEMPTS"
      else
        echo "Error: Invalid max attempts value: $MAX_ATTEMPTS"
        print_usage
        exit 1
      fi
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

# Calculate implied volatility using the enhanced SV model (v6)
IVSV_OUTPUT=$(/home/usr0/projects/option_tools/calculate_sv_v6 $DEBUG_FLAG $SV_EXTRA_FLAGS "$OPTION_PRICE" "$UNDERLYING_PRICE" "$STRIKE" "$T" "$R" "$Q" 2>&1)
IVSV_EXIT_CODE=$?
if [[ $IVSV_EXIT_CODE -ne 0 ]]; then
  echo "Implied volatility calculation (SV) failed with exit code: $IVSV_EXIT_CODE"
  exit 1
fi

# Check if BS fallback was used
USED_BS_FALLBACK=false
if echo "$IVSV_OUTPUT" | grep -q "falling back to Black-Scholes"; then
  USED_BS_FALLBACK=true
fi

# Extract the final IV value (last line of output)
IVSV=$(echo "$IVSV_OUTPUT" | tail -n 1)

# Calculate moneyness for display purposes
MONEYNESS=$(echo "scale=4; $STRIKE / $UNDERLYING_PRICE" | bc -l)
MONEYNESS_PCT=$(echo "scale=2; $MONEYNESS * 100" | bc -l)

# Determine if SV model matched BS model
BS_SV_DIFF=$(echo "scale=6; ($IVBS - $IVSV) / $IVBS * 100" | bc -l)
BS_SV_DIFF_ABS=$(echo $BS_SV_DIFF | tr -d '-')

# Calculate SV vs BS difference for display
if (( $(echo "$BS_SV_DIFF_ABS < 1.0" | bc -l) )); then
  SV_MATCH_STATUS="Identical (< 1%)"
elif (( $(echo "$BS_SV_DIFF_ABS < 5.0" | bc -l) )); then
  SV_MATCH_STATUS="Very Similar (< 5%)"
elif (( $(echo "$BS_SV_DIFF_ABS < 10.0" | bc -l) )); then
  SV_MATCH_STATUS="Similar (< 10%)"
elif (( $(echo "$BS_SV_DIFF_ABS < 15.0" | bc -l) )); then
  SV_MATCH_STATUS="Divergent (> 10%)"
else
  SV_MATCH_STATUS="Highly Divergent (> 15%)"
fi

# Display results with option characteristics info
echo   "================= Option Pricing Summary ================="
printf "%-25s %12.2f\n" "Underlying Price:" "$UNDERLYING_PRICE"
printf "%-25s %12.2f\n" "Strike:" "$STRIKE"
printf "%-25s %16.4f (%.1f%%)\n" "Moneyness (K/S):" "$MONEYNESS" "$MONEYNESS_PCT"
printf "%-25s %16.6f\n" "Time to expiry (yrs):" "$T"
printf "%-25s %14.4f%%\n" "Risk-Free Rate:" "$(echo "$R*100" | bc -l)"
printf "%-25s %14.4f%%\n" "Underlying Yield:" "$(echo "$Q*100" | bc -l)"
printf "%-25s %14.4f\n" "Market Option Price:" "$OPTION_PRICE"
printf "%-25s %12.2f%%\n" "Implied Volatility(BS):" "$(echo "$IVBS"*100 | bc -l)"
printf "%-25s %12.2f%%\n" "Implied Volatility(SV):" "$(echo "$IVSV"*100 | bc -l)"
printf "%-25s %s\n" "SV Model Used:" "Enhanced Stochastic Volatility (V6)"
printf "%-25s %s\n" "SV vs BS Difference:" "$SV_MATCH_STATUS"
if [[ "$USED_BS_FALLBACK" == "true" ]]; then
  printf "%-25s %s\n" "BS Fallback:" "Yes (SV calibration failed, using BS model)"
fi
echo   "=========================================================="

# If SV and BS models are highly divergent, print additional information
if (( $(echo "$BS_SV_DIFF_ABS > 15.0" | bc -l) )); then
  echo ""
  if (( $(echo "$IVSV > $IVBS" | bc -l) )); then
    echo "Note: SV model is indicating significantly higher volatility than BS model."
    echo "      This could suggest the presence of volatility skew or jumps in the underlying."
  else
    echo "Note: SV model is indicating significantly lower volatility than BS model."
    echo "      This could suggest mean reversion effects or other stochastic features."
  fi
fi