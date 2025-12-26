#!/bin/bash
cd /home/claude/workspace/testing/output

echo "=== Test Results Analysis ==="
echo ""

echo "=== SUCCESSFUL TESTS (compiled and built) ==="
for log in *.c.log; do
    name=$(basename "$log" .c.log)
    if [ -x "$name" ]; then
        echo "  $name"
    fi
done | sort

echo ""
echo "=== PARSER ERRORS (sn -> c failed) ==="
for log in *.c.log; do
    name=$(basename "$log" .c.log)
    content=$(cat "$log" 2>/dev/null)
    if echo "$content" | grep -q "Error at"; then
        echo "  $name"
        grep -m1 "Error at" "$log" | head -1 | sed 's/^/    /'
    fi
done | sort -u

echo ""
echo "=== TYPE ERRORS ==="
for log in *.c.log; do
    name=$(basename "$log" .c.log)
    content=$(cat "$log" 2>/dev/null)
    if echo "$content" | grep -q "Type error"; then
        echo "  $name"
        grep -m1 "Type error" "$log" | head -1 | sed 's/^/    /'
    fi
done | sort -u

echo ""
echo "=== GCC COMPILATION ERRORS (c -> exe failed) ==="
for log in *.c.log; do
    name=$(basename "$log" .c.log)
    # Check if .c exists but executable doesn't
    if [ -f "${name}.c" ] && [ ! -x "$name" ]; then
        content=$(cat "$log" 2>/dev/null)
        # Not a parser or type error
        if ! echo "$content" | grep -q "Error at" && ! echo "$content" | grep -q "Type error"; then
            echo "  $name"
        fi
    fi
done | sort

echo ""
echo "=== SUMMARY ==="
total=0
success=0
parser_err=0
type_err=0
gcc_err=0
for log in *.c.log; do
    total=$((total + 1))
    name=$(basename "$log" .c.log)
    content=$(cat "$log" 2>/dev/null)
    if [ -x "$name" ]; then
        success=$((success + 1))
    elif echo "$content" | grep -q "Error at"; then
        parser_err=$((parser_err + 1))
    elif echo "$content" | grep -q "Type error"; then
        type_err=$((type_err + 1))
    else
        gcc_err=$((gcc_err + 1))
    fi
done
echo "Total tests: $total"
echo "Successful: $success"
echo "Parser errors: $parser_err"
echo "Type errors: $type_err"
echo "GCC compilation errors: $gcc_err"
