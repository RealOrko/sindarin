# Sindarin Compiler Test Issues

Analysis of compiler errors from `testing/output/*.c.log` and runtime issues from `testing/output/*.c.strace` files.

**Date:** 2025-12-19
**Total Tests Analyzed:** 121

---

## Summary

| Category | Count | Severity |
|----------|-------|----------|
| Compiler Crashes (SEGV) | 3 | Critical |
| Type Checking Errors | 6 | High |
| Parser Errors | 15 | High |
| Memory Leaks | 121 | Medium |

---

## 1. Critical: Compiler Crashes (SEGV)

The compiler crashes with a segmentation fault at `type_checker_stmt.c:361` when processing certain source files. The crash occurs due to a null pointer dereference (reading address `0x8`).

### Affected Tests

| Test File | Source Line in Crash |
|-----------|---------------------|
| test_15_functions.sn | type_checker_stmt.c:361 |
| test_17_return_values.sn | type_checker_stmt.c:361 |
| test_19_function_params.sn | type_checker_stmt.c:361 |

### Stack Trace (from AddressSanitizer)
```
AddressSanitizer: SEGV on unknown address 0x000000000008
The signal is caused by a READ memory access.
Hint: address points to the zero page.
    #0 in type_check_stmt /home/gavin/code/sindarin/compiler/type_checker_stmt.c:361
    #1 in type_check_module /home/gavin/code/sindarin/compiler/type_checker.c:12
    #2 in compiler_compile /home/gavin/code/sindarin/compiler/compiler.c:159
    #3 in main /home/gavin/code/sindarin/compiler/main.c:15
```

### Root Cause Analysis
The source files appear to be valid Sindarin code using standard function definitions and calls. The crash suggests the type checker is accessing a null AST node or uninitialized pointer when processing function-related constructs.

---

## 2. High: Type Checking Errors

### 2.1 Invalid Types for Arithmetic Operations (int/double promotion)

| Test File | Line | Error |
|-----------|------|-------|
| test_02_double_literals.sn | 28 | `var mixed: double = pi * 2` - int literal not promoted to double |

**Expected Behavior:** Integer literals should automatically promote to double when used in arithmetic with double operands.

### 2.2 Argument Type Mismatch in Function Calls

| Test File | Lines | Error |
|-----------|-------|-------|
| test_32_array_types.sn | 53 | Argument type mismatch in call |
| test_42_as_val.sn | 41 | Argument type mismatch in call |
| test_47_string_array_ops.sn | 12, 13, 49 | Argument type mismatch in call |

### 2.3 Mixed Type Expression Errors

| Test File | Lines | Errors |
|-----------|-------|--------|
| test_60_mixed_types.sn | 14, 17, 20, 23, 28-30, 34, 37, 41, 71 | Multiple type errors including: |
| | | - Invalid types for `+` operator |
| | | - Invalid types for arithmetic operator |
| | | - Type mismatch in comparison |
| | | - Invalid expression in interpolated string |

**Expected Behavior:** The language should support mixed int/double arithmetic with automatic type promotion.

---

## 3. High: Parser Errors

### 3.1 Array Element Assignment Not Recognized

The parser rejects array element assignment (`arr[index] = value`) as an invalid assignment target.

| Test File | Line | Expression |
|-----------|------|------------|
| test_06_arrays_basic.sn | 37 | `nums[0] = 100` |
| test_26_array_basic.sn | 21 | `nums[0] = 99` |
| test_29_array_negative_idx.sn | 23 | `nums[0] = 99` |

**Error Message:** `Error at '100': Invalid assignment target`

### 3.2 Reserved Keyword Conflicts

The parser rejects identifiers that conflict with reserved keywords:

| Keyword | Test Files | Lines |
|---------|------------|-------|
| `val` | test_07_arrays_methods.sn | 20 |
| | test_15_nested_loops.sn | 47 |
| | test_23_scope_shadowing.sn | 62 |
| | test_25_boundary_values.sn | 48 |
| | test_56_deep_nesting.sn | 27 |
| `long` | test_33_string_basic.sn | 29 |
| `double` | test_24_complex_expressions.sn | 41 |

**Note:** These test files use common variable names that happen to be reserved keywords in the language. The test files should be updated to use different variable names.

### 3.3 Multi-line Lambda Expression Parsing

Lambdas with multi-line bodies fail to parse when the body doesn't immediately follow the arrow.

| Test File | Line | Error |
|-----------|------|-------|
| test_22_lambda_infer.sn | 38 | Expected expression after newline |
| test_23_lambda_capture.sn | 25 | Expected expression after newline |
| test_46_lambda_returning_array.sn | 7 | Expected expression after newline |
| test_nested_lambda.sn | 4 | Expected expression after newline |

**Example Problem Code:**
```sindarin
var abs_val: fn(int): int = fn(x): int =>
    if x < 0 =>  // <-- Parser fails here
        return 0 - x
    return x
```

### 3.4 Shared Keyword in Foreach Loops

The `shared` modifier is not recognized in foreach loop iteration variables.

| Test File | Line | Error |
|-----------|------|-------|
| test_18_private_shared.sn | 41 | `Expected '=>' after for-each iterable` |

### 3.5 Inconsistent Indentation Errors

Several test files have indentation issues that the parser rejects:

| Test File | Line |
|-----------|------|
| test_15_nested_loops.sn | 58 |
| test_18_private_shared.sn | 57 |
| test_33_string_basic.sn | 47 |
| test_56_deep_nesting.sn | 50 |

### 3.6 Invalid Expression in String Interpolation

Certain expressions are rejected inside interpolated strings:

| Test File | Line | Expression |
|-----------|------|------------|
| test_20_operator_combos.sn | 52 | `{--5}` (double unary minus) |

**Error Message:** `Error at '--': Expected expression`

---

## 4. Medium: Memory Leaks

**All 121 compiled tests** show memory leak warnings from LeakSanitizer:

```
==PID==LeakSanitizer has encountered a fatal error.
==PID==HINT: For debugging, try setting environment variable LSAN_OPTIONS=verbosity=1:log_threads=1
==PID==HINT: LeakSanitizer does not work under ptrace
```

**Result:** All tests exit with code 1.

**Affected Component:** The `bin/sn` compiler binary has memory leaks in its core allocation routines.

---

## Test File Validity Summary

### Tests with VALID Sindarin Syntax (Compiler/Type Checker Bug)

These tests appear to use valid language constructs but fail due to compiler issues:

- test_02_double_literals.sn - Valid int/double promotion
- test_06_arrays_basic.sn - Valid array element assignment
- test_15_functions.sn - Valid function definitions
- test_17_return_values.sn - Valid return statements
- test_19_function_params.sn - Valid function parameters
- test_46_lambda_returning_array.sn - Valid lambda with array return

### Tests with INVALID Sindarin Syntax (Test Bug)

These tests use reserved keywords as variable names or have syntax issues:

- test_07_arrays_methods.sn - Uses `val` as variable name
- test_15_nested_loops.sn - Uses `val` as variable name, indentation issues
- test_23_scope_shadowing.sn - Uses `val` as parameter name
- test_24_complex_expressions.sn - Uses `double` as variable name
- test_25_boundary_values.sn - Uses `val` as variable name
- test_33_string_basic.sn - Uses `long` as variable name
- test_56_deep_nesting.sn - Uses `val` as variable name

---

## Recommendations

### Critical Priority
1. **Fix null pointer dereference in `type_checker_stmt.c:361`** - The type checker crashes when processing function definitions/calls in certain patterns.

### High Priority
2. **Implement array element assignment parsing** - `arr[i] = value` should be a valid lvalue.
3. **Fix int-to-double type promotion** - Mixed arithmetic should work with automatic promotion.
4. **Fix multi-line lambda parsing** - Lambda bodies should support newlines after `=>`.

### Medium Priority
5. **Address memory leaks in the compiler** - LeakSanitizer reports fatal errors on every compilation.
6. **Update test files** - Rename variables that conflict with reserved keywords (`val`, `long`, `double`).

### Low Priority
7. **Support `shared` modifier in foreach loops** if this is intended behavior.
8. **Allow unary operators in interpolation** - `${--x}` should parse correctly.
