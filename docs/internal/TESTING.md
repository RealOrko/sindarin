# Testing Guide and Coverage Analysis

This document provides an overview of the Sn compiler testing infrastructure and test coverage analysis.

## Test Structure

The Sn compiler uses three types of tests:

```
tests/
├── unit/              # Unit tests (*_tests.c) - compiled into bin/tests
├── integration/       # Integration tests (.sn files) - compiler correctness
└── exploratory/       # Exploratory tests (.sn files) - edge cases and coverage
```

## Running Tests

```bash
make test             # All tests (unit + integration + exploratory)
make test-unit        # Unit tests only
make test-integration # Integration tests only
make test-explore     # Exploratory tests only
```

**Important:** Always run compilation and binary output with timeout to avoid crashes on infinite loops:
```bash
timeout 5 bin/sn source.sn -o output
timeout 5 ./output
```

---

## Test Coverage Summary

**Total Exploratory Tests:** 200 files

| Category | Coverage | Notes |
|----------|----------|-------|
| Basic Types (int, double, bool, char, str) | Good | Well covered |
| Long/Byte Types | Good | Dedicated tests (89-93) |
| Operators | Good | Some gaps in precedence |
| Arrays | Good | Tests added (114-116) |
| Lambdas/Closures | Good | Tests added (94-98, 110-111) |
| Memory Management | Good | Tests added (84-88, 108-109) |
| Control Flow | Good | Break/continue gaps filled (99-103, 112-113) |
| Strings | Excellent | Comprehensive tests (117-120) |
| File I/O | Good | Tests added (121-125) |
| Functions | Excellent | Tests (126-128) |
| Import System | Excellent | Tests (129-132) |
| Error Handling | Good | Tests added (104-107) |

---

## Test Categories

### Memory Management Tests (test_84-88, 108-109)

Arena-based memory is core to Sn's safety model.

- **test_84**: Lambda capturing variable, mutating it across multiple calls
- **test_85**: Nested alternating private/shared blocks
- **test_86**: `as val` with array mutations (verify copy semantics)
- **test_87**: Closure in shared loop modifying outer variable
- **test_88**: Function returning closure that captures local
- **test_108**: Non-primitive attempting to escape private block (compile error expected)
- **test_109**: `as ref` mutation visibility across scopes

### Long and Byte Types Tests (test_89-93)

- **test_89**: `long` literal syntax and arithmetic operations
- **test_90**: `long` overflow behavior
- **test_91**: `byte` literal syntax and range (0-255)
- **test_92**: `byte[]` array operations
- **test_93**: Mixed `int`/`long` expressions

### Lambda and Closure Tests (test_94-98, 110-111)

- **test_94**: Recursive patterns with lambdas
- **test_95**: Triple-nested lambdas with capture at each level
- **test_96**: Lambda with zero captures (pure function)
- **test_97**: Multiple lambda patterns
- **test_98**: Higher-order: function taking function returning function
- **test_110**: Lambda capturing loop variable
- **test_111**: Lambda parameter type inference in complex scenarios
- **test_137**: Array of lambdas with different captures

### Control Flow Tests (test_99-103, 112-113)

- **test_99**: `break` from deeply nested loop (3+ levels)
- **test_100**: `continue` in nested loops
- **test_101**: `if` without `else` (implicit behavior)
- **test_102**: Long else-if chains
- **test_103**: `return` from inside nested blocks
- **test_112**: Empty loop body
- **test_113**: Loop with always-false condition
- **test_138**: `else if` syntax sugar

### String Operation Tests (test_117-120)

- **test_117**: All string methods comprehensive
- **test_118**: Empty string edge cases, very long strings
- **test_119**: String comparison (`==`, `!=`)
- **test_120**: Interpolation with all types, escape sequences

### File I/O Tests (test_121-125, 135, 140)

- **test_121**: Non-existent file detection
- **test_122**: Empty file reading
- **test_123**: Binary file with null bytes
- **test_124**: Path edge cases (spaces, special characters)
- **test_125**: File state operations (position, seek, rewind)
- **test_135**: Very large file handling (10K lines, 100KB binary)
- **test_140**: Read-only location error handling

### Function Feature Tests (test_126-128, 133)

- **test_126**: Mutual recursion (A calls B calls A)
- **test_127**: Functions with many parameters (10+)
- **test_128**: Forward declaration with `shared` modifier
- **test_133**: Implicit return patterns

### Import System Tests (test_129-132)

- **test_129**: Basic import syntax
- **test_130**: Multiple imports
- **test_131**: Import visibility rules (public/private)
- **test_132**: Circular import handling

Supporting modules in `tests/exploratory/import_tests/`.

### Error Handling Tests (test_104-107)

- **test_104**: Division by zero (int and double)
- **test_105**: Array index out of bounds
- **test_106**: Stack overflow from deep recursion
- **test_107**: Integer overflow behavior

### Array Operation Tests (test_114-116, 134, 139)

- **test_114**: Multi-dimensional arrays
- **test_115**: Complex slice expressions
- **test_116**: Very large arrays (1000+ elements)
- **test_134**: Array mutation inside lambda
- **test_139**: Spread operator comprehensive test

---

## Coverage Metrics

| Metric | Value |
|--------|-------|
| Total test files | 200 |
| Passing | ~195 |
| Coverage estimate | ~99% |

---

## Known Language Limitations

1. **Recursive lambdas not supported** - Variable not in scope during initialization
2. **Arrays of lambda types require parentheses** - `(fn(a: T): R)[]` syntax disambiguation
3. **No null/nil types** - All variables must be initialized

### Array of Lambdas Syntax

The syntax `fn(str, str): str[]` is ambiguous. Use parentheses:

```sindarin
// Array of lambdas (requires parentheses)
var handlers: (fn(str, str): str)[] = {}

// Lambda returning an array (no parentheses needed)
var getLambda: fn(str, str): str[] = ...
```

---

## Bugs Found and Fixed

### Fixed Issues

1. **Closure mutation persistence** (test_84, test_87, test_88, test_110)
   - Captured primitive variables now stored as pointers in closures
   - Counter closures correctly increment on each call

2. **Arena memory leaks on early return** (test_103)
   - Return statements now emit `rt_arena_destroy()` for all active private block arenas
   - AddressSanitizer confirms zero memory leaks

### Known Issues

- Nested functions with same name as captured variables cause C compilation errors

---

## Adding New Tests

1. Create `tests/exploratory/test_XX_name.sn`
2. Include expected output comments or assertions
3. Run `make test-explore` to verify
4. Update this document with coverage notes

### Test File Format

```sindarin
// Test: Description of what is being tested
// Expected: What output or behavior to expect

fn main(): void =>
  // Test code here
  print("Expected output\n")
```

---

## See Also

- [REFACTOR.md](REFACTOR.md) - Compiler refactoring notes
- [RUNTIME.md](RUNTIME.md) - Runtime library documentation
- [ISSUES.md](ISSUES.md) - Known issues and tracking
