#!/bin/bash
#
# test_historical_prices.sh - Test script for historical prices functionality
#

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
UNIFIED_ROOT="$PROJECT_ROOT/unified"
BINARY_DIR="$UNIFIED_ROOT/bin"
PRICER_BIN="$BINARY_DIR/unified_pricer"
MARKET_TOOL="$BINARY_DIR/market_data_tool"

# Color codes
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if market_data_tool binary exists
if [ ! -x "$MARKET_TOOL" ]; then
    echo -e "${YELLOW}Note: $MARKET_TOOL not found. Will test with option_pricer.sh instead.${NC}"
    MARKET_TOOL="$UNIFIED_ROOT/scripts/get_market_data.sh"
fi

# Function to run a test and report result
run_test() {
    local test_name="$1"
    local command="$2"
    local expected_output="$3"
    
    echo -e "\n${YELLOW}Running test: ${test_name}${NC}"
    echo "Command: $command"
    
    # Run the command
    result=$(eval "$command" 2>&1)
    exit_code=$?
    
    # Check for errors
    if [ $exit_code -ne 0 ]; then
        echo -e "${RED}FAIL: Command failed with exit code $exit_code${NC}"
        echo "Output: $result"
        return 1
    fi
    
    # Check output if expected_output is provided
    if [ -n "$expected_output" ]; then
        if echo "$result" | grep -q "$expected_output"; then
            echo -e "${GREEN}PASS: Output contains expected value${NC}"
        else
            echo -e "${RED}FAIL: Output does not contain expected value${NC}"
            echo "Expected: $expected_output"
            echo "Actual: $result"
            return 1
        fi
    else
        echo -e "${GREEN}PASS: Command executed successfully${NC}"
    fi
    
    return 0
}

# Initialize counters
TESTS_TOTAL=0
TESTS_PASSED=0
TESTS_FAILED=0

# Function to record test result
record_test() {
    local result=$1
    
    TESTS_TOTAL=$((TESTS_TOTAL + 1))
    if [ $result -eq 0 ]; then
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
}

echo -e "${YELLOW}===============================================${NC}"
echo -e "${YELLOW}  Historical Prices Tests                     ${NC}"
echo -e "${YELLOW}===============================================${NC}"

# Test 1: Get historical prices for SPX (30 days)
run_test "Get SPX Historical Prices (30 days)" "$UNIFIED_ROOT/scripts/get_historical_prices.sh --verbose SPX 30" "historical prices"
record_test $?

# Test 2: Get historical prices with volatility calculation
run_test "Historical Prices with Volatility" "$UNIFIED_ROOT/scripts/get_historical_prices.sh --with-volatility --verbose AAPL 60" "Volatility"
record_test $?

# Test 3: Try invalid ticker
run_test "Invalid Ticker" "$UNIFIED_ROOT/scripts/get_historical_prices.sh INVALID_TICKER_XYZ 30 2>&1" "Error"
record_test $?

# Test 4: Test cache functionality
echo -e "\n${YELLOW}Testing cache functionality...${NC}"
run_test "Cache First Request" "time $UNIFIED_ROOT/scripts/get_historical_prices.sh --verbose SPX 30" "retrieved from API"
run_test "Cache Second Request" "time $UNIFIED_ROOT/scripts/get_historical_prices.sh --verbose SPX 30" "retrieved from cache"
record_test $?

# Print summary
echo -e "\n${YELLOW}===============================================${NC}"
echo -e "${YELLOW}  Test Summary                               ${NC}"
echo -e "${YELLOW}===============================================${NC}"
echo -e "Total tests:  $TESTS_TOTAL"
echo -e "${GREEN}Tests passed: $TESTS_PASSED${NC}"
if [ $TESTS_FAILED -gt 0 ]; then
    echo -e "${RED}Tests failed: $TESTS_FAILED${NC}"
    exit 1
else
    echo -e "Tests failed: $TESTS_FAILED"
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
fi 