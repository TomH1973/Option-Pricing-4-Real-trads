#!/bin/bash
#
# test_ticker_option_pricing.sh - Test script for ticker-based option pricing
#

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
UNIFIED_ROOT="$PROJECT_ROOT/unified"
SCRIPTS_DIR="$UNIFIED_ROOT/scripts"
PRICER_SCRIPT="$SCRIPTS_DIR/option_pricer.sh"

# Color codes for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print test header
print_header() {
    echo -e "\n${YELLOW}===============================================${NC}"
    echo -e "${YELLOW}$1${NC}"
    echo -e "${YELLOW}===============================================${NC}"
}

# Function to run a test
run_test() {
    local test_name="$1"
    local command="$2"
    local expected_result="$3"
    
    echo -e "\n${BLUE}Running test: ${test_name}${NC}"
    echo -e "Command: ${command}"
    
    # Execute the command and capture output and exit code
    result=$(eval "$command" 2>&1)
    exit_code=$?
    
    # Check exit code if expected_result is not provided
    if [ -z "$expected_result" ]; then
        if [ $exit_code -eq 0 ]; then
            echo -e "${GREEN}PASS: Command executed successfully${NC}"
        else
            echo -e "${RED}FAIL: Command failed with exit code $exit_code${NC}"
            echo -e "Output: $result"
            return 1
        fi
    # Otherwise check for expected string in output
    elif echo "$result" | grep -q "$expected_result"; then
        echo -e "${GREEN}PASS: Expected result found in output${NC}"
    else
        echo -e "${RED}FAIL: Expected result not found in output${NC}"
        echo -e "Expected to find: $expected_result"
        echo -e "Actual output: $result"
        return 1
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

# Check if the script exists
if [ ! -x "$PRICER_SCRIPT" ]; then
    echo -e "${RED}Error: Option pricer script not found at $PRICER_SCRIPT${NC}"
    exit 1
fi

print_header "Test 1: Basic ticker lookup with SPX"
run_test "SPX Call Option" "$PRICER_SCRIPT --verbose --ticker SPX 0 4800 30" "Ticker Symbol:   SPX"
record_test $?

print_header "Test 2: Ticker with auto-volatility"
run_test "AAPL with Auto-Vol" "$PRICER_SCRIPT --verbose --ticker AAPL --auto-vol 180 185 45" "Using historical volatility"
record_test $?

print_header "Test 3: Risk-free rate fetching"
run_test "Risk-free Rate" "$PRICER_SCRIPT --verbose --ticker SPX --rate-term 10year 4700 4750 90" "risk-free rate"
record_test $?

print_header "Test 4: Put option with Greeks"
run_test "Put with Greeks" "$PRICER_SCRIPT --verbose --ticker MSFT -t put --greeks 320 330 60" "Greeks:          Will be calculated"
record_test $?

print_header "Test 5: Market Data Retrieval"
run_test "Get Market Data" "$SCRIPTS_DIR/get_market_data.sh --verbose SPX" "Current Price"
record_test $?

print_header "Test 6: Historical Volatility"
run_test "Historical Volatility" "$SCRIPTS_DIR/get_market_data.sh --verbose --type volatility --days 30 SPX" "Historical Volatility"
record_test $?

print_header "Test 7: Dividend Yield"
run_test "Dividend Yield" "$SCRIPTS_DIR/get_market_data.sh --verbose --type dividend AAPL" "Dividend Yield"
record_test $?

print_header "Test 8: Implied Volatility Calculation"
run_test "IV Calculation" "$PRICER_SCRIPT --verbose --ticker SPX 4700 4750 30 150" "Implied Volatility"
record_test $?

print_header "Test 9: Heston Model with FFT"
run_test "Heston FFT" "$PRICER_SCRIPT --verbose -m heston -n fft --ticker SPX 4700 4750 60" "Heston"
record_test $?

print_header "Test 10: Invalid Ticker"
run_test "Invalid Ticker" "$SCRIPTS_DIR/get_market_data.sh --verbose INVALID_TICKER_XYZ" "Error"
record_test $?

# Print summary
print_header "TEST SUMMARY"
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