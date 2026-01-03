#!/bin/bash
# Threading Test Runner for Sindarin
# Runs all test_thread_*.sn tests (positive and negative)
# Returns 0 if all tests pass, 1 otherwise

# Don't use set -e since we expect some commands to fail

# Configuration
BIN_DIR="${BIN_DIR:-bin}"
TEST_DIR="${TEST_DIR:-tests/integration}"
ERROR_TEST_DIR="${ERROR_TEST_DIR:-tests/integration/errors}"
TEMP_DIR="${TEMP_DIR:-/tmp/sn_thread_tests}"
COMPILER="${BIN_DIR}/sn"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Counters
passed=0
failed=0
skipped=0

# Ensure temp directory exists
mkdir -p "$TEMP_DIR"

echo "=========================================="
echo "Sindarin Threading Test Runner"
echo "=========================================="
echo ""

# Check if compiler exists
if [ ! -x "$COMPILER" ]; then
    echo -e "${RED}Error: Compiler not found at $COMPILER${NC}"
    echo "Run 'make build' first"
    exit 1
fi

echo -e "${BLUE}Running Positive Tests (should compile and run)${NC}"
echo "------------------------------------------"

# Run positive threading tests
for test_file in "$TEST_DIR"/test_thread_*.sn; do
    [ -f "$test_file" ] || continue

    test_name=$(basename "$test_file" .sn)
    expected_file="${test_file%.sn}.expected"
    panic_file="${test_file%.sn}.panic"
    exe_file="$TEMP_DIR/$test_name"
    output_file="$TEMP_DIR/${test_name}.out"
    compile_err="$TEMP_DIR/${test_name}.compile_err"

    printf "  %-40s " "$test_name"

    # Check for expected file
    if [ ! -f "$expected_file" ]; then
        echo -e "${YELLOW}SKIP${NC} (no .expected file)"
        skipped=$((skipped + 1))
        continue
    fi

    # Compile
    if ! "$COMPILER" "$test_file" -o "$exe_file" -l 1 -g -O0 2>"$compile_err"; then
        echo -e "${RED}FAIL${NC} (compilation error)"
        head -5 "$compile_err"
        failed=$((failed + 1))
        continue
    fi

    # Run
    ASAN_OPTIONS=detect_leaks=0 timeout 5s "$exe_file" > "$output_file" 2>&1
    run_exit_code=$?

    # Check if panic expected
    if [ -f "$panic_file" ]; then
        if [ $run_exit_code -eq 0 ]; then
            echo -e "${RED}FAIL${NC} (expected panic but succeeded)"
            failed=$((failed + 1))
            continue
        fi
    else
        if [ $run_exit_code -ne 0 ]; then
            echo -e "${RED}FAIL${NC} (runtime error, exit code: $run_exit_code)"
            head -5 "$output_file"
            failed=$((failed + 1))
            continue
        fi
    fi

    # Compare output
    if diff -q "$output_file" "$expected_file" > /dev/null 2>&1; then
        echo -e "${GREEN}PASS${NC}"
        passed=$((passed + 1))
    else
        echo -e "${RED}FAIL${NC} (output mismatch)"
        echo "  Expected:"
        head -3 "$expected_file" | sed 's/^/    /'
        echo "  Got:"
        head -3 "$output_file" | sed 's/^/    /'
        failed=$((failed + 1))
    fi
done

echo ""
echo -e "${BLUE}Running Negative Tests (should fail compilation)${NC}"
echo "------------------------------------------"

# Run negative threading tests (compile errors)
for test_file in "$ERROR_TEST_DIR"/test_thread_*.sn; do
    [ -f "$test_file" ] || continue

    test_name=$(basename "$test_file" .sn)
    expected_file="${test_file%.sn}.expected"
    compile_err="$TEMP_DIR/${test_name}.compile_err"

    printf "  %-40s " "$test_name"

    # Check for expected file
    if [ ! -f "$expected_file" ]; then
        echo -e "${YELLOW}SKIP${NC} (no .expected file)"
        skipped=$((skipped + 1))
        continue
    fi

    # Compile (should fail)
    if "$COMPILER" "$test_file" -o "$TEMP_DIR/$test_name" -l 1 2>"$compile_err"; then
        echo -e "${RED}FAIL${NC} (expected compile error but succeeded)"
        failed=$((failed + 1))
        continue
    fi

    # Check error message matches
    expected_error=$(head -1 "$expected_file")
    if grep -qF "$expected_error" "$compile_err"; then
        echo -e "${GREEN}PASS${NC}"
        passed=$((passed + 1))
    else
        echo -e "${RED}FAIL${NC} (error message mismatch)"
        echo "  Expected:"
        echo "    $expected_error"
        echo "  Got:"
        head -3 "$compile_err" | sed 's/^/    /'
        failed=$((failed + 1))
    fi
done

# Cleanup
rm -rf "$TEMP_DIR"

# Summary
echo ""
echo "=========================================="
echo "Threading Test Results"
echo "=========================================="
total=$((passed + failed + skipped))
echo -e "Total:   $total"
echo -e "Passed:  ${GREEN}$passed${NC}"
echo -e "Failed:  ${RED}$failed${NC}"
echo -e "Skipped: ${YELLOW}$skipped${NC}"
echo ""

if [ $failed -gt 0 ]; then
    echo -e "${RED}THREADING TESTS FAILED${NC}"
    exit 1
else
    echo -e "${GREEN}ALL THREADING TESTS PASSED${NC}"
    exit 0
fi
