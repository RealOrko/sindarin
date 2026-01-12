#!/bin/bash
# Unified test runner for Sn compiler
# Usage: run_tests.sh <test-type> <compiler-binary>
# Test types: integration, integration-errors, explore, explore-errors

set -e

# Cross-platform timeout command
# macOS uses gtimeout from coreutils, Linux has timeout built-in
if command -v timeout &> /dev/null; then
    TIMEOUT_CMD="timeout"
elif command -v gtimeout &> /dev/null; then
    TIMEOUT_CMD="gtimeout"
else
    echo "Warning: No timeout command found. Tests may hang on infinite loops."
    TIMEOUT_CMD=""
fi

# Wrapper function for timeout
run_with_timeout() {
    local seconds="$1"
    shift
    if [ -n "$TIMEOUT_CMD" ]; then
        $TIMEOUT_CMD "${seconds}s" "$@"
    else
        "$@"
    fi
}

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BOLD='\033[1m'
NC='\033[0m'

# Arguments
TEST_TYPE="${1:-integration}"
SN="${2:-bin/sn}"

# Excluded tests (space-separated list via environment variable)
# Example: SN_EXCLUDE_TESTS="test_foo test_bar" ./scripts/run_tests.sh
EXCLUDE_TESTS="${SN_EXCLUDE_TESTS:-}"

# Directories
# Use SN_TEST_DIR if set, otherwise try /tmp. If /tmp has noexec, fall back to ./build/test_runner
if [ -n "$SN_TEST_DIR" ]; then
    TEMP_DIR="$SN_TEST_DIR/sn_test_runner_$$"
elif mount | grep -q "on /tmp.*noexec"; then
    # /tmp is mounted noexec, use a directory within the project
    TEMP_DIR="$(pwd)/build/test_runner_$$"
else
    TEMP_DIR="/tmp/sn_test_runner_$$"
fi
mkdir -p "$TEMP_DIR"

# Cleanup on exit
cleanup() {
    rm -rf "$TEMP_DIR"
}
trap cleanup EXIT

# Determine test configuration based on type
case "$TEST_TYPE" in
    integration)
        TEST_DIR="tests/integration"
        PATTERN="*.sn"
        EXPECT_COMPILE_FAIL=false
        TITLE="Integration Tests"
        ;;
    integration-errors)
        TEST_DIR="tests/integration/errors"
        PATTERN="*.sn"
        EXPECT_COMPILE_FAIL=true
        TITLE="Integration Error Tests (expected compile failures)"
        ;;
    explore)
        TEST_DIR="tests/exploratory"
        PATTERN="test_*.sn"
        EXPECT_COMPILE_FAIL=false
        TITLE="Exploratory Tests"
        ;;
    explore-errors)
        TEST_DIR="tests/exploratory/errors"
        PATTERN="*.sn"
        EXPECT_COMPILE_FAIL=true
        TITLE="Exploratory Error Tests (expected compile failures)"
        ;;
    sdk)
        TEST_DIR="tests/sdk"
        PATTERN="test_*.sn"
        EXPECT_COMPILE_FAIL=false
        TITLE="SDK Tests"
        ;;
    *)
        echo "Unknown test type: $TEST_TYPE"
        echo "Valid types: integration, integration-errors, explore, explore-errors, sdk"
        exit 1
        ;;
esac

# Print header
echo ""
echo -e "${BOLD}${TITLE}${NC}"
echo "============================================================"

# Counters
passed=0
failed=0
skipped=0

# Run tests
for test_file in "$TEST_DIR"/$PATTERN; do
    [ -f "$test_file" ] || continue

    test_name=$(basename "$test_file" .sn)

    # Check if test is excluded
    if [ -n "$EXCLUDE_TESTS" ] && echo "$EXCLUDE_TESTS" | grep -qw "$test_name"; then
        printf "  %-45s ${YELLOW}SKIP${NC} (excluded)\n" "$test_name"
        skipped=$((skipped + 1))
        continue
    fi

    expected_file="${test_file%.sn}.expected"
    panic_file="${test_file%.sn}.panic"
    exe_file="$TEMP_DIR/$test_name"
    output_file="$TEMP_DIR/$test_name.out"
    compile_err="$TEMP_DIR/$test_name.compile_err"

    printf "  %-45s " "$test_name"

    if $EXPECT_COMPILE_FAIL; then
        # Error test: should fail to compile
        if [ ! -f "$expected_file" ]; then
            printf "${YELLOW}SKIP${NC} (no .expected)\n"
            skipped=$((skipped + 1))
            continue
        fi

        if $SN "$test_file" -o "$exe_file" -l 1 2>"$compile_err"; then
            printf "${RED}FAIL${NC} (should not compile)\n"
            failed=$((failed + 1))
            continue
        fi

        if grep -qF "$(head -1 "$expected_file")" "$compile_err"; then
            printf "${GREEN}PASS${NC}\n"
            passed=$((passed + 1))
        else
            printf "${RED}FAIL${NC} (wrong error)\n"
            echo "    Expected: $(head -1 "$expected_file")"
            echo "    Got:      $(head -1 "$compile_err" | sed 's/.*error.0m: //')"
            failed=$((failed + 1))
        fi
    else
        # Positive test: should compile and run
        if [ ! -f "$expected_file" ]; then
            # For explore tests, .expected is optional
            if [ "$TEST_TYPE" = "explore" ]; then
                : # Continue without expected file
            else
                printf "${YELLOW}SKIP${NC} (no .expected)\n"
                skipped=$((skipped + 1))
                continue
            fi
        fi

        # Compile
        compile_timeout=10
        if ! run_with_timeout ${compile_timeout} $SN "$test_file" -o "$exe_file" -l 1 -g -O0 2>"$compile_err"; then
            printf "${RED}FAIL${NC} (compile error)\n"
            head -3 "$compile_err" | sed 's/^/    /'
            failed=$((failed + 1))
            continue
        fi

        # Run (capture exit code without triggering set -e)
        run_timeout=30
        [ "$TEST_TYPE" = "integration" ] && run_timeout=5

        run_exit_code=0
        ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" run_with_timeout ${run_timeout} "$exe_file" > "$output_file" 2>&1 || run_exit_code=$?

        # Check for expected panic
        if [ -f "$panic_file" ]; then
            if [ $run_exit_code -eq 0 ]; then
                printf "${RED}FAIL${NC} (expected panic)\n"
                failed=$((failed + 1))
                continue
            fi
        else
            if [ $run_exit_code -ne 0 ]; then
                printf "${RED}FAIL${NC} (exit code: $run_exit_code)\n"
                head -3 "$output_file" | sed 's/^/    /'
                failed=$((failed + 1))
                continue
            fi
        fi

        # Compare output if expected file exists
        if [ -f "$expected_file" ]; then
            if diff -q "$output_file" "$expected_file" > /dev/null 2>&1; then
                printf "${GREEN}PASS${NC}\n"
                passed=$((passed + 1))
            else
                printf "${RED}FAIL${NC} (output mismatch)\n"
                echo "    Expected: $(head -1 "$expected_file")"
                echo "    Got:      $(head -1 "$output_file")"
                failed=$((failed + 1))
            fi
        else
            # No expected file but ran successfully
            printf "${GREEN}PASS${NC}\n"
            passed=$((passed + 1))
        fi
    fi
done

# Print summary
echo ""
echo "------------------------------------------------------------"
printf "Results: ${GREEN}%d passed${NC}, ${RED}%d failed${NC}, ${YELLOW}%d skipped${NC}\n" $passed $failed $skipped

# Exit with error if any tests failed
if [ $failed -gt 0 ]; then
    exit 1
fi
