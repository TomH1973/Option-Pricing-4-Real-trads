#!/bin/bash

# Enhanced version of price_option.sh that supports multiple volatility models
# including the new FFT-based stochastic volatility implementation

# Fetch market data (mockup, replace with actual data sources)
fetch_risk_free_rate() {
  RATE=$(curl -s "https://www.cnbc.com/quotes/US10Y" -H "User-Agent: Mozilla/5.0" | grep -oP '(?<=<span class="QuoteStrip-lastPrice">)[0-9.]+'| head -1)
  if [[ -z "$RATE" ]]; then
  echo "Warning: Could not fetch risk-free rate, defaulting to 4.32 %." >&2
  echo "4.32"
  else
    echo "$RATE"
  fi
}

# Check arguments
if [ $# -lt 5 ]; then
  echo "Usage: $0 UNDERLYING_PRICE STRIKE DAYS_TO_EXPIRY OPTION_PRICE UNDERLYING_YIELD [sv-model]"
  echo "Options:"
  echo "  sv-model: Stochastic volatility model to use (v2 or v3). Default: v3"
  echo "Example: $0 5616 5600 9 118.5 0.013 v3"
  exit 1
fi

UNDERLYING_PRICE=$1
STRIKE=$2
DAYS_TO_EXPIRY=$3
OPTION_PRICE=$4
UNDERLYING_YIELD=$5

# Default to v3 model, but allow user to specify
SV_MODEL="v3"
if [ $# -ge 6 ]; then
  SV_MODEL=$6
fi

# Validate SV model choice
if [ "$SV_MODEL" != "v2" ] && [ "$SV_MODEL" != "v3" ]; then
  echo "Error: Invalid stochastic volatility model. Use 'v2' or 'v3'."
  exit 1
fi

# Check if chosen model is available
if [ "$SV_MODEL" = "v3" ] && [ ! -f ./calculate_sv_v3 ]; then
  echo "Warning: FFT-based model (v3) not found. Falling back to v2." >&2
  SV_MODEL="v2"
fi

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

# Calculate implied volatility using the selected stochastic volatility model
if [ "$SV_MODEL" = "v3" ]; then
  IVSV=$(/home/usr0/projects/option_tools/calculate_sv_v3 "$OPTION_PRICE" "$UNDERLYING_PRICE" "$STRIKE" "$T" "$R" "$Q")
else
  IVSV=$(/home/usr0/projects/option_tools/calculate_sv_v2 "$OPTION_PRICE" "$UNDERLYING_PRICE" "$STRIKE" "$T" "$R" "$Q")
fi

if [[ $? -ne 0 ]]; then
  echo "Implied volatility calculation (SV) failed."
  exit 1
fi

# Display model details based on the model used
SV_MODEL_DESC="Stochastic Volatility (Heston)"
if [ "$SV_MODEL" = "v3" ]; then
  SV_MODEL_DESC="Stochastic Volatility (FFT)"
fi

echo   "=============== Option Pricing Summary ==============="
printf "%-25s %12.2f\n" "Underlying Price:" "$UNDERLYING_PRICE"
printf "%-25s %12.2f\n" "Strike:" "$STRIKE"
printf "%-25s %16.6f\n" "Time to expiry (yrs):" "$T"
printf "%-25s %14.4f%%\n" "Risk-Free Rate:" "$(echo "$R*100" | bc -l)"
printf "%-25s %14.4f%%\n" "Underlying Yield:" "$(echo "$Q*100" | bc -l)"
printf "%-25s %14.4f\n" "Market Option Price:" "$OPTION_PRICE"
printf "%-25s %12.2f%%\n" "Implied Volatility(BS):" "$(echo "$IVBS"*100 | bc -l)"
printf "%-25s %12.2f%%\n" "Implied Volatility(SV):" "$(echo "$IVSV"*100 | bc -l)"
printf "%-25s %s\n" "SV Model Used:" "$SV_MODEL_DESC"
echo   "======================================================"
