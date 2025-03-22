#!/bin/bash

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

# Fetch dividend yield (mockup, replace with actual data sources)
# fetch_dividend_yield() {
# API_KEY="4ca82c74108a21558687ff2eb3cdbf7d"
# SERIES_ID="SP500DIVYIELD"
# RESPONSE=$(curl -s "https://api.stlouisfed.org/fred/series/observations?series_id=$SERIES_ID&api_key=$API_KEY&file_type=json")
# YIELD=$(echo "$RESPONSE" | grep -oP '"value":"\K[0-9.]+' | tail -1)

# echo "1.343"
# }

# Check arguments
if [ $# -ne 5 ]; then
  echo "Usage: $0 UNDERLYING_PRICE STRIKE DAYS_TO_EXPIRY OPTION_PRICE UNDERLYING_YIELD"
  echo "Example: $0 5616 5600 9 118.5 0.013"
  exit 1
fi

UNDERLYING_PRICE=$1
STRIKE=$2
DAYS_TO_EXPIRY=$3
OPTION_PRICE=$4
UNDERLYING_YIELD=$5

# Convert days to expiry to years
T=$(echo "$DAYS_TO_EXPIRY / 365.25" | bc -l)

# Get risk-free rate dynamically
RISK_FREE_RATE=$(fetch_risk_free_rate)
if [[ -z $RISK_FREE_RATE ]]; then
  echo "Failed to fetch risk-free rate."
  exit 1
fi

# DIVIDEND_YIELD=$(fetch_dividend_yield)
# Q=$(echo "$DIVIDEND_YIELD / 100" | bc -l)

R=$(echo "$RISK_FREE_RATE / 100" | bc -l)

# Calculate implied volatility using the C program
IVBS=$(/home/usr0/projects/option_tools/calculate_iv_v2 "$OPTION_PRICE" "$UNDERLYING_PRICE" "$STRIKE" "$T" "$R" "$UNDERLYING_YIELD")
if [[ $? -ne 0 ]]; then
  echo "Implied volatility calculation (BS) failed."
  exit 1
fi

IVSV=$(/home/usr0/projects/option_tools/calculate_sv_v2 "$OPTION_PRICE" "$UNDERLYING_PRICE" "$STRIKE" "$T" "$R" "$UNDERLYING_YIELD")
if [[ $? -ne 0 ]]; then
  echo "Implied volatility calculation (SV) failed."
  exit 1
fi

echo   "========= Option Pricing Summary ========="
printf "%-25s %12.2f\n" "Underlying Price:" "$UNDERLYING_PRICE"
printf "%-25s %12.2f\n" "Strike:" "$STRIKE"
printf "%-25s %16.6f\n" "Time to expiry (yrs):" "$T"
printf "%-25s %14.4f%%\n" "Risk-Free Rate:" "$(echo "$R*100" | bc -l)"
printf "%-25s %14.4f%%\n" "Underlying Yield:" "$(echo "$UNDERLYING_YIELD*100" | bc -l)"
printf "%-25s %14.4f\n" "Market Option Price:" "$OPTION_PRICE"
printf "%-25s %12.2f%%\n" "Implied Volatility(BS):" "$(echo "$IVBS"*100 | bc -l)"
printf "%-25s %12.2f%%\n" "Implied Volatility(SV):" "$(echo "$IVSV"*100 | bc -l)"
echo   "=========================================="
