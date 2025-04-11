#!/bin/bash

# Script to generate a volatility smile across multiple strikes
# Usage: ./generate_smile.sh

SPOT=100
DAYS=90
YIELD=0.01
RATE=0.05
BASE_VOL=0.2

echo "Strike,BS IV,SV IV"
for STRIKE in $(seq 80 5 120); do
    # First calculate a theoretical price using Black-Scholes with a base volatility
    PRICE=$(/home/usr0/projects/option_tools/calculate_iv_v2 --calc-price $SPOT $STRIKE $DAYS $RATE $YIELD $BASE_VOL)
    
    # Then calculate implied vols using both models
    RESULT=$(/home/usr0/projects/option_tools/price_option_v5.sh $SPOT $STRIKE $DAYS $PRICE $YIELD)
    
    # Extract the implied vols using grep and awk
    BS_IV=$(echo "$RESULT" | grep "Implied Volatility(BS)" | awk '{print $3}' | sed 's/%//')
    SV_IV=$(echo "$RESULT" | grep "Implied Volatility(SV)" | awk '{print $3}' | sed 's/%//')
    
    echo "$STRIKE,$BS_IV,$SV_IV"
done 