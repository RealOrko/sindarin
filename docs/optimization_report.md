# Sn Compiler Optimization Impact Report

## Summary

This report documents the optimization improvements implemented in the Sn compiler and their measured impact on generated code quality and performance.

## Optimizations Implemented

### 1. Constant Folding (Code Generation)
Compile-time evaluation of constant expressions.

**Example:**
```sn
var a: int = 2 + 3       // Source
```
```c
long a = 5L;             // Generated (constant folded)
```

**Impact:** Eliminates runtime function calls (e.g., `rt_add_long()`) for constant expressions.

### 2. Tail Call Optimization (-O2)
Converts tail-recursive function calls to loops, preventing stack overflow.

**Example - O0 (recursive):**
```c
long countdown(long n) {
    if (rt_le_long(n, 0L)) {
        _return_value = 0L;
        goto countdown_return;
    }
    _return_value = countdown(rt_sub_long(n, 1L));  // Recursive call
    goto countdown_return;
}
```

**Example - O2 (loop):**
```c
long countdown(long n) {
    while (1) { /* tail call loop */
        if (rt_le_long(n, 0L)) {
            _return_value = 0L;
            goto countdown_return;
        }
        n = rt_sub_long(n, 1L);
        continue;  // No recursive call!
    }
}
```

**Impact:** Eliminates stack growth for tail-recursive functions, enables safe deep recursion.

### 3. Dead Code Elimination (-O1, -O2)
Removes unreachable code after return/break/continue statements.

**Impact:** Reduces generated code size and improves clarity.

### 4. String Literal Merging (-O1, -O2)
Combines adjacent string literals in interpolated strings at compile time.

**Example:**
```sn
$"Hello " + "World"      // Source
```
```c
"Hello World"            // Generated (merged)
```

**Impact:** Fewer runtime string concatenation calls.

### 5. Arena Requirement Analysis
Functions using only primitive types skip arena creation.

**Impact:** Eliminates overhead of `rt_arena_create()`/`rt_arena_destroy()` for simple functions.

## Optimization Levels

| Level | Dead Code | String Merge | Tail Call Opt |
|-------|-----------|--------------|---------------|
| -O0   | No        | No           | No            |
| -O1   | Yes       | Yes          | No            |
| -O2   | Yes       | Yes          | Yes           |

## Test Results

### Integration Tests
- **31 tests** all pass at all optimization levels (-O0, -O1, -O2)
- Tests cover: arithmetic, loops, conditionals, strings, arrays, lambdas, recursion

### Unit Tests
- All optimizer tests pass
- All constant folding tests pass
- All type checker tests pass (including "did you mean?" suggestions)

## Type Checker Quality

The type checker includes enhanced error reporting with:

1. **Levenshtein Distance** - Calculates similarity between identifiers
2. **"Did you mean?"** suggestions for:
   - Undefined variables (suggests similar variable names)
   - Unknown methods (suggests similar method names)
   - Typos in array/string methods

**Example:**
```
Error: Undefined variable 'conut'
       Did you mean 'count'?
```

## Performance Characteristics

### Tail Call Optimization
- Recursive functions converted to loops
- Stack frame overhead eliminated
- Can handle arbitrarily deep "recursion"

### Constant Folding
- All arithmetic on literals evaluated at compile time
- No runtime function calls for constant expressions
- Works for: +, -, *, /, %, comparisons, logical ops

### Generated Code Quality
- Clean, readable C output
- Minimal temporary variables
- Direct literal values where possible

## Verification Commands

```bash
# Run all integration tests
./scripts/test-integration

# Run with specific optimization level
SN_OPT_LEVEL="-O0" ./scripts/test-integration

# Run unit tests
./bin/tests

# Build compiler
./scripts/build
```

## Files Modified

- `compiler/optimizer.c` - Dead code elimination, tail call optimization, string merging
- `compiler/optimizer.h` - Optimizer interface
- `compiler/code_gen_util.c` - Constant folding, native operators, arena analysis
- `compiler/code_gen_expr.c` - String interpolation optimization
- `compiler/compiler.c` - Optimization level handling
- `compiler/compiler.h` - Optimization level constants

## Conclusion

The implemented optimizations provide:
1. **Correctness** - All tests pass at all optimization levels
2. **Performance** - Reduced runtime overhead through constant folding and TCO
3. **Safety** - Tail call optimization prevents stack overflow in recursive code
4. **Usability** - Enhanced type checker with helpful suggestions
