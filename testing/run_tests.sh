#!/bin/bash
# Run all exploratory tests and capture results

COMPILER="../bin/sn"
OUTPUT_DIR="output"
RESULTS_FILE="test_results.txt"

mkdir -p "$OUTPUT_DIR"
echo "=== Sindarin Exploratory Test Results ===" > "$RESULTS_FILE"
echo "Date: $(date)" >> "$RESULTS_FILE"
echo "" >> "$RESULTS_FILE"

for test_file in test_*.sn; do
    test_name=$(basename "$test_file" .sn)
    c_file="$OUTPUT_DIR/${test_name}.c"
    exe_file="$OUTPUT_DIR/${test_name}"

    echo "Testing: $test_name"
    echo "========================================" >> "$RESULTS_FILE"
    echo "Test: $test_name" >> "$RESULTS_FILE"
    echo "========================================" >> "$RESULTS_FILE"

    # Compile .sn to .c
    compile_output=$(timeout 5s strace -o "$c_file.strace" $COMPILER "$test_file" -o "$c_file" 2>&1)
    compile_status=$?

    if [ $compile_status -ne 0 ]; then
        echo "  COMPILE FAILED (sn -> c)"
        echo "Status: COMPILE FAILED (sn -> c)" >> "$RESULTS_FILE"
        echo "Output:" >> "$RESULTS_FILE"
        echo "$compile_output" >> "$RESULTS_FILE"
        echo "" >> "$RESULTS_FILE"
        continue
    fi

    # Compile .c to executable
    gcc_output=$(gcc -fsanitize=address -o "$exe_file" "$c_file" ../bin/arena.o ../bin/debug.o ../bin/runtime.o 2>&1)
    gcc_status=$?

    if [ $gcc_status -ne 0 ]; then
        echo "  COMPILE FAILED (c -> exe)"
        echo "Status: COMPILE FAILED (c -> exe)" >> "$RESULTS_FILE"
        echo "GCC Output:" >> "$RESULTS_FILE"
        echo "$gcc_output" >> "$RESULTS_FILE"
        echo "" >> "$RESULTS_FILE"
        continue
    fi

    # Run the executable
    # run_output=$("$exe_file" 2>&1)
    # run_status=$?

    # if [ $run_status -ne 0 ]; then
    #     echo "  RUNTIME ERROR (exit code: $run_status)"
    #     echo "Status: RUNTIME ERROR (exit: $run_status)" >> "$RESULTS_FILE"
    # else
    #     echo "  PASSED"
    #     echo "Status: PASSED" >> "$RESULTS_FILE"
    # fi

    # echo "Output:" >> "$RESULTS_FILE"
    # echo "$run_output" >> "$RESULTS_FILE"
    # echo "" >> "$RESULTS_FILE"
done

echo ""
echo "Results written to $RESULTS_FILE"
