# Exploratory Test Coverage Analysis

This document tracks test coverage for the Sn compiler's exploratory test suite (`compiler/tests/exploratory/`).

## Current Coverage Summary

**Total Tests:** 143 files

| Category | Coverage | Notes |
|----------|----------|-------|
| Basic Types (int, double, bool, char, str) | Good | Well covered |
| Long/Byte Types | Minimal | Few dedicated tests |
| Operators | Good | Some gaps in precedence |
| Arrays | Good | Slicing needs more coverage |
| Lambdas/Closures | Moderate | Complex scenarios missing |
| Memory Management | Moderate | Edge cases untested |
| Control Flow | Good | Nested break/continue gaps |
| Strings | Good | Methods well covered |
| File I/O | Moderate | Error paths untested |
| Functions | Good | Basic cases covered |
| Import System | None | No tests exist |
| Error Handling | Minimal | Runtime errors untested |

---

## Test Gaps by Category

### 1. Memory Management (High Priority)

**Why it matters:** Arena-based memory is the core of Sn's safety model. Bugs here cause use-after-free, leaks, or crashes.

**Missing tests:**
- [ ] Lambda capturing variable, mutating it across multiple calls
- [ ] Non-primitive attempting to escape private block (should error)
- [ ] Nested alternating private/shared blocks
- [ ] `as val` with array mutations (verify copy semantics)
- [ ] `as ref` mutation visibility across scopes
- [ ] Closure in shared loop modifying outer variable
- [ ] Function returning closure that captures local (arena lifetime)

**Example scenario not tested:**
```
fn make_counter(): fn(): int
    var count: int = 0
    return () =>
        count = count + 1
        return count

var c = make_counter()
print(c())  // 1
print(c())  // 2 - does mutation persist correctly?
```

### 2. Long and Byte Types (Medium Priority)

**Why it matters:** These types exist in the lexer/parser but have minimal test coverage.

**Missing tests:**
- [ ] `long` literal syntax (`42l`, `9223372036854775807l`)
- [ ] `long` arithmetic operations
- [ ] `long` overflow behavior
- [ ] `byte` literal syntax and range (0-255)
- [ ] `byte` arithmetic and conversions
- [ ] `byte[]` array operations
- [ ] Mixed `int`/`long` expressions

### 3. Lambda and Closure Edge Cases (High Priority)

**Why it matters:** Recent fixes to closure capture highlight this as a bug-prone area.

**Missing tests:**
- [ ] Recursive lambda (lambda calling itself via variable)
- [ ] Triple-nested lambdas with capture at each level
- [ ] Lambda capturing loop variable (closure-in-loop semantics)
- [ ] Array of lambdas with different captures
- [ ] Lambda with zero captures (pure function)
- [ ] Lambda parameter type inference in complex scenarios
- [ ] Higher-order: function taking function returning function

**Example scenario not tested:**
```
// Recursive lambda
var factorial: fn(int): int = (n: int): int =>
    if n <= 1 => return 1
    else => return n * factorial(n - 1)
```

### 4. Control Flow Edge Cases (Medium Priority)

**Missing tests:**
- [ ] `break` from deeply nested loop (3+ levels)
- [ ] `continue` in nested loops
- [ ] `if` without `else` (implicit behavior)
- [ ] Long else-if chains (10+ branches)
- [ ] `return` from inside nested blocks
- [ ] Empty loop body
- [ ] Loop with always-false condition

### 5. Array Operations (Medium Priority)

**Missing tests:**
- [ ] Multi-dimensional arrays (`int[][]`)
- [ ] Array of lambdas
- [ ] Complex slice expressions (`arr[f()..g()]`)
- [ ] Slice with negative indices (if supported)
- [ ] Spread in nested array literals
- [ ] Array mutation inside lambda
- [ ] Very large arrays (memory pressure)

### 6. String Operations (Low Priority)

**Missing tests:**
- [ ] All string methods comprehensive test
- [ ] Empty string edge cases
- [ ] Very long strings
- [ ] String comparison (`==`, `!=`)
- [ ] Interpolation with all types
- [ ] Nested interpolation edge cases
- [ ] Escape sequences in all contexts

### 7. File I/O Error Handling (Medium Priority)

**Missing tests:**
- [ ] Read non-existent file
- [ ] Write to read-only location
- [ ] Operations on closed file handle
- [ ] Empty file reading
- [ ] Binary file with null bytes
- [ ] Very large file handling
- [ ] Path edge cases (relative, absolute, special chars)

### 8. Function Features (Medium Priority)

**Missing tests:**
- [ ] Mutual recursion (A calls B calls A)
- [ ] Very deep recursion (stack limits)
- [ ] Function with many parameters (10+)
- [ ] Forward declaration with `shared` modifier
- [ ] Implicit return (function without return statement)

### 9. Import System (High Priority - No Coverage)

**Missing tests:**
- [ ] Basic import syntax
- [ ] Import visibility rules
- [ ] Circular import handling
- [ ] Multiple imports
- [ ] Private function visibility across modules

### 10. Error Handling and Runtime Safety (High Priority)

**Missing tests:**
- [ ] Division by zero (int and double)
- [ ] Array index out of bounds
- [ ] Null/nil handling
- [ ] Integer overflow behavior
- [ ] Stack overflow from recursion

---

## Recommended New Tests

### Batch 1: Memory Management
```
test_84_closure_mutation.sn      - Closure mutating captured var across calls
test_85_private_escape_error.sn  - Non-primitive escape from private (error case)
test_86_nested_private_shared.sn - Alternating nested memory contexts
test_87_as_val_array_copy.sn     - Verify array copy semantics with as val
test_88_closure_in_shared_loop.sn - Closure capturing in shared for loop
```

### Batch 2: Long and Byte Types
```
test_89_long_literals.sn         - Long integer literals and operations
test_90_long_overflow.sn         - Long type boundary behavior
test_91_byte_basics.sn           - Byte type operations
test_92_byte_array.sn            - Byte array operations
test_93_mixed_numeric_types.sn   - Int/long/double mixed expressions
```

### Batch 3: Lambda Edge Cases
```
test_94_recursive_lambda.sn      - Lambda calling itself
test_95_nested_lambda_capture.sn - Multi-level nested capture
test_96_lambda_no_capture.sn     - Pure lambdas with no captures
test_97_array_of_lambdas.sn      - Arrays containing lambda functions
test_98_lambda_returning_lambda.sn - Higher-order function patterns
```

### Batch 4: Control Flow
```
test_99_deep_break.sn            - Break from 4+ nested loops
test_100_nested_continue.sn      - Continue in nested loops
test_101_if_without_else.sn      - If statements without else
test_102_many_else_if.sn         - Long else-if chains
test_103_return_in_block.sn      - Return from nested blocks
```

### Batch 5: Error Handling
```
test_104_divide_by_zero.sn       - Division by zero behavior
test_105_array_bounds.sn         - Out of bounds access
test_106_deep_recursion.sn       - Stack depth limits
test_107_integer_overflow.sn     - Integer overflow behavior
```

---

## Running Tests

```bash
make test-explore              # Run all exploratory tests
make test-explore TEST=test_84 # Run specific test (if supported)
```

## Adding New Tests

1. Create `compiler/tests/exploratory/test_XX_name.sn`
2. Include expected output comments or assertions
3. Run `make test-explore` to verify
4. Update this document with coverage notes

---

## Coverage Metrics

Last updated: 2024-12-29

| Metric | Value |
|--------|-------|
| Total test files | 143 |
| Passing | 139 |
| Failing | 4 |
| Coverage estimate | ~70% |

**Known failing tests:**
- Check `compiler/tests/exploratory/output/` for failure logs
