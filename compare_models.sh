#!/bin/bash

# Script to compare all model versions
# Usage: ./compare_models.sh

SPOT=100
STRIKE=105
DAYS=30
PRICE=2.0
YIELD=0.02

# Run all versions and extract implied volatility
echo "Comparing model versions for $SPOT/$STRIKE/$DAYS option:"

# Black-Scholes
BS_IV=$(/home/usr0/projects/option_tools/calculate_iv_v2 $PRICE $SPOT $STRIKE $(echo "$DAYS/365" | bc -l) 0.05 $YIELD)
echo "Black-Scholes IV: $(echo "$BS_IV * 100" | bc -l)%"

# SV v3
SV3_RESULT=$(/home/usr0/projects/option_tools/price_option_v3.sh $SPOT $STRIKE $DAYS $PRICE $YIELD v3 2>/dev/null)
SV3_IV=$(echo "$SV3_RESULT" | grep "Implied Volatility(SV)" | awk '{print $3}')
echo "Stochastic Vol v3: $SV3_IV"

# SV v4
SV4_RESULT=$(/home/usr0/projects/option_tools/price_option_v4.sh $SPOT $STRIKE $DAYS $PRICE $YIELD 2>/dev/null)
SV4_IV=$(echo "$SV4_RESULT" | grep "Implied Volatility(SV)" | awk '{print $3}')
echo "Stochastic Vol v4: $SV4_IV"

# SV v5
SV5_RESULT=$(/home/usr0/projects/option_tools/price_option_v5.sh $SPOT $STRIKE $DAYS $PRICE $YIELD 2>/dev/null)
SV5_IV=$(echo "$SV5_RESULT" | grep "Implied Volatility(SV)" | awk '{print $3}')
echo "Stochastic Vol v5: $SV5_IV" 