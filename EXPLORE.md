# Exploratory Test Coverage Analysis

This document tracks test coverage for the Sn compiler's exploratory test suite (`compiler/tests/exploratory/`).

ALWAYS RUN COMPILATION AND BINARY OUTPUT WITH TIMEOUT TO AVOID CRASHING THE HOST MACHINE!!!

## Current Coverage Summary

**Total Tests:** 167 files (24 new tests added)

| Category | Coverage | Notes |
|----------|----------|-------|
| Basic Types (int, double, bool, char, str) | Good | Well covered |
| Long/Byte Types | **Good** | Now has dedicated tests (89-93) |
| Operators | Good | Some gaps in precedence |
| Arrays | Good | Slicing needs more coverage |
| Lambdas/Closures | **Good** | New tests added (94-98) |
| Memory Management | Moderate | Tests added but reveal bugs (84-88) |
| Control Flow | **Good** | Break/continue gaps filled (99-103) |
| Strings | Good | Methods well covered |
| File I/O | Moderate | Error paths untested |
| Functions | Good | Recursion tested (94, 106) |
| Import System | None | No tests exist |
| Error Handling | **Good** | New tests added (104-107) |

---

## Test Gaps by Category

### 1. Memory Management (High Priority)

**Why it matters:** Arena-based memory is the core of Sn's safety model. Bugs here cause use-after-free, leaks, or crashes.

**Tests implemented:**
- [x] Lambda capturing variable, mutating it across multiple calls (test_84) - **REVEALS BUG: mutations don't persist**
- [ ] Non-primitive attempting to escape private block (should error)
- [x] Nested alternating private/shared blocks (test_85) - **PASS**
- [x] `as val` with array mutations (verify copy semantics) (test_86) - **PASS**
- [ ] `as ref` mutation visibility across scopes
- [x] Closure in shared loop modifying outer variable (test_87) - **PASS but reveals closure mutation bug**
- [x] Function returning closure that captures local (arena lifetime) (test_88) - **PASS but reveals closure mutation bug**

**Known issues found:**
- Closure mutations don't persist between calls (tests 84, 87, 88)
- Early returns in functions leak arenas (test_103)

### 2. Long and Byte Types (Medium Priority)

**Why it matters:** These types exist in the lexer/parser but have minimal test coverage.

**Tests implemented:**
- [x] `long` literal syntax (`42l`, `9223372036854775807l`) (test_89) - **PASS**
- [x] `long` arithmetic operations (test_89) - **PASS**
- [x] `long` overflow behavior (test_90) - **PASS**
- [x] `byte` literal syntax and range (0-255) (test_91) - **PASS**
- [x] `byte` arithmetic and conversions (test_91) - **PASS**
- [x] `byte[]` array operations (test_92) - **PASS**
- [x] Mixed `int`/`long` expressions (test_93) - **PASS**

### 3. Lambda and Closure Edge Cases (High Priority)

**Why it matters:** Recent fixes to closure capture highlight this as a bug-prone area.

**Tests implemented:**
- [x] Recursive lambda (test_94) - **PASS** (uses regular recursive functions; recursive lambdas not supported)
- [x] Triple-nested lambdas with capture at each level (test_95) - **PASS**
- [ ] Lambda capturing loop variable (closure-in-loop semantics)
- [ ] Array of lambdas with different captures - **NOT SUPPORTED** (arrays of lambdas not supported)
- [x] Lambda with zero captures (pure function) (test_96) - **PASS**
- [ ] Lambda parameter type inference in complex scenarios
- [x] Higher-order: function taking function returning function (test_98) - **PASS**

**Known limitations:**
- Recursive lambdas not supported (variable not in scope during initialization)
- Arrays of lambda types not supported

### 4. Control Flow Edge Cases (Medium Priority)

**Tests implemented:**
- [x] `break` from deeply nested loop (3+ levels) (test_99) - **PASS**
- [x] `continue` in nested loops (test_100) - **PASS**
- [x] `if` without `else` (implicit behavior) (test_101) - **PASS**
- [x] Long else-if chains (test_102) - **PASS** (uses nested if-else syntax)
- [x] `return` from inside nested blocks (test_103) - **PASS but reveals memory leaks**
- [ ] Empty loop body
- [ ] Loop with always-false condition

**Known issues found:**
- Early returns in functions leak arena memory (test_103)
- `else if` syntax not supported; must use nested `else => if`

### 5. Array Operations (Medium Priority)

**Missing tests:**
- [ ] Multi-dimensional arrays (`int[][]`)
- [ ] Array of lambdas - **NOT SUPPORTED**
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

**Tests implemented:**
- [ ] Mutual recursion (A calls B calls A)
- [x] Very deep recursion (stack limits) (test_106) - **PASS**
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

**Tests implemented:**
- [x] Division by zero (int and double) (test_104) - **PASS** (with safe guard)
- [x] Array index out of bounds (test_105) - **PASS** (with safe getter)
- [ ] Null/nil handling
- [x] Integer overflow behavior (test_107) - **PASS**
- [x] Stack overflow from recursion (test_106) - **PASS** at moderate depths

---

## Recommended New Tests

### Batch 1: Memory Management ✅ IMPLEMENTED
```
test_84_closure_mutation.sn      - Closure mutating captured var across calls ✅
test_85_nested_private_shared.sn - Alternating nested memory contexts ✅
test_86_as_val_array_copy.sn     - Verify array copy semantics with as val ✅
test_87_closure_in_shared_loop.sn - Closure capturing in shared for loop ✅
test_88_make_counter.sn          - Function returning closure pattern ✅
```

### Batch 2: Long and Byte Types ✅ IMPLEMENTED
```
test_89_long_literals.sn         - Long integer literals and operations ✅
test_90_long_overflow.sn         - Long type boundary behavior ✅
test_91_byte_basics.sn           - Byte type operations ✅
test_92_byte_array.sn            - Byte array operations ✅
test_93_mixed_numeric_types.sn   - Int/long/double mixed expressions ✅
```

### Batch 3: Lambda Edge Cases ✅ IMPLEMENTED
```
test_94_recursive_lambda.sn      - Recursive functions with lambdas ✅
test_95_nested_lambda_capture.sn - Multi-level nested capture ✅
test_96_lambda_no_capture.sn     - Pure lambdas with no captures ✅
test_97_lambda_multiple.sn       - Multiple lambdas patterns ✅
test_98_lambda_returning_lambda.sn - Higher-order function patterns ✅
```

### Batch 4: Control Flow ✅ IMPLEMENTED
```
test_99_deep_break.sn            - Break from 4+ nested loops ✅
test_100_nested_continue.sn      - Continue in nested loops ✅
test_101_if_without_else.sn      - If statements without else ✅
test_102_many_else_if.sn         - Nested if-else chains ✅
test_103_return_in_block.sn      - Return from nested blocks ✅
```

### Batch 5: Error Handling ✅ IMPLEMENTED
```
test_104_divide_by_zero.sn       - Division by zero behavior ✅
test_105_array_bounds.sn         - Out of bounds access ✅
test_106_deep_recursion.sn       - Stack depth limits ✅
test_107_integer_overflow.sn     - Integer overflow behavior ✅
```

---

## Bugs Found During Testing

### Critical
1. **Closure mutation doesn't persist** (test_84, test_87, test_88)
   - When a closure mutates a captured variable, the mutation doesn't persist between calls
   - Counter closures return same value instead of incrementing

2. **Arena memory leaks on early return** (test_103)
   - Functions that return early from loops/nested blocks leak arena allocations
   - AddressSanitizer detects these leaks

### Moderate
3. **Nested functions cause C compilation errors** (various tests)
   - Nested function definitions with same name as captured variables cause C code issues

### Language Limitations Discovered
- `else if` not supported; must use `else => if`
- Recursive lambdas not supported (variable not in scope during initialization)
- Arrays of lambda types not supported

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

Last updated: 2025-12-29

| Metric | Value |
|--------|-------|
| Total test files | 167 |
| Passing | ~155 |
| Revealing bugs | 4 |
| Coverage estimate | ~85% |

**Tests revealing compiler bugs:**
- test_84, test_87, test_88: Closure mutation issues
- test_103: Arena memory leaks on early return
