#!/bin/bash
#
# Basic test script for the unified option pricing system
#

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
UNIFIED_ROOT="$PROJECT_ROOT/unified"
SCRIPTS_DIR="$UNIFIED_ROOT/scripts"

# Test Parameters
TICKER="SPX"
SPOT_PRICE="4700.50"
STRIKE_PRICE="4750"
DAYS_TO_EXPIRY="30"
OPTION_PRICE="67.25"
DIVIDEND_YIELD="1.42"
RATE="5.25"

# Color codes for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
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
    
    echo -e "\n${YELLOW}Running test: ${test_name}${NC}"
    echo -e "Command: ${command}"
    
    # Execute the command and capture output and exit code
    result=$($command 2>&1)
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

# Test 1: Check if option_pricer.sh exists and is executable
print_header "Test 1: Check if option_pricer.sh exists and is executable"
if [ -x "$SCRIPTS_DIR/option_pricer.sh" ]; then
    echo -e "${GREEN}PASS: option_pricer.sh exists and is executable${NC}"
    record_test 0
else
    echo -e "${RED}FAIL: option_pricer.sh not found or not executable${NC}"
    record_test 1
fi

# Test 2: Run option_pricer.sh with --help
print_header "Test 2: Run option_pricer.sh with --help"
run_test "Help text" "$SCRIPTS_DIR/option_pricer.sh --help" "Usage:"
record_test $?

# Test 3: Run option_pricer.sh with SPX default
print_header "Test 3: Run option_pricer.sh with SPX default"
run_test "SPX Default" "$SCRIPTS_DIR/option_pricer.sh $SPOT_PRICE $STRIKE_PRICE $DAYS_TO_EXPIRY" "Ticker Symbol:   SPX"
record_test $?

# Test 4: Run option_pricer.sh with invalid parameters
print_header "Test 4: Run option_pricer.sh with invalid parameters"
run_test "Invalid Parameters" "$SCRIPTS_DIR/option_pricer.sh invalid $STRIKE_PRICE $DAYS_TO_EXPIRY" "Error: Spot price must be a number"
record_test $?

# Test 5: Run option_pricer.sh with all parameters
print_header "Test 5: Run option_pricer.sh with all parameters"
run_test "All Parameters" "$SCRIPTS_DIR/option_pricer.sh --ticker $TICKER -m heston -n fft -t put --greeks -r $RATE --vol 20 $SPOT_PRICE $STRIKE_PRICE $DAYS_TO_EXPIRY $OPTION_PRICE $DIVIDEND_YIELD" "Greeks:          Will be calculated"
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