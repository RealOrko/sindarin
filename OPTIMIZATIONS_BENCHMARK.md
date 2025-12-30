# Benchmark Optimizations

Workarounds to improve benchmark performance using current Sindarin features. For compiler-level improvements, see `OPTIMIZATIONS_COMPILER.md`.

## Quick Wins

### 1. Use `--unchecked` Flag

Run benchmarks without overflow checking:

```bash
bin/sn benchmark.sn -o benchmark --unchecked -O2
```

This uses native C operators instead of runtime-checked functions.

**Impact**: 2-4x improvement on arithmetic-heavy benchmarks (Fibonacci).

### 2. Use Array Join for String Building

**Problem**: Repeated `+` on strings is O(n²).

```sindarin
// SLOW: Each + copies the entire string
var result: str = ""
for var i: int = 0; i < 100000; i++ =>
    result = result + "Hello"
```

**Workaround**: Build an array and join:

```sindarin
// FASTER: O(n) array pushes + single join
var parts: str[] = {}
for var i: int = 0; i < 100000; i++ =>
    parts.push("Hello")
var result: str = parts.join("")
```

**Impact**: 10-50x improvement on string concatenation loops.

### 3. Cache Array Length

**Problem**: `len()` called repeatedly in loops.

```sindarin
// len(arr) called each iteration
while left < right =>
    var temp: int = arr[left]
    // ...
```

**Workaround**: Cache length before loop:

```sindarin
var length: int = len(arr)
var right: int = length - 1
while left < right =>
    // ...
```

**Impact**: Minor improvement, but cleaner code.

### 4. Use Index Loops for Performance-Critical Code

Test whether index-based loops are faster than for-each:

```sindarin
// For-each (may have slight overhead)
for num in arr =>
    total = total + num

// Index-based (direct array access)
for var i: int = 0; i < len(arr); i++ =>
    total = total + arr[i]
```

Profile both to determine which is faster for your use case.

## Benchmark-Specific Notes

### Fibonacci
- Already optimal algorithm
- Use `--unchecked` for best results
- Tests function call overhead (recursive) and loop performance (iterative)

### Prime Sieve
- **Cannot be optimized** with current language features
- Requires sized array allocation (see `OPTIMIZATIONS_COMPILER.md`)
- Current bottleneck: 1M individual `.push()` calls

### String Operations
- Use array + join pattern (see above)
- This is the only significant optimization available today

### Array Operations
- **Cannot be optimized** with current language features
- Requires sized array allocation
- Current bottleneck: 1M individual `.push()` calls

## Summary

| Optimization | Available Today | Impact |
|--------------|-----------------|--------|
| `--unchecked` flag | Yes | 2-4x on arithmetic |
| Array join for strings | Yes | 10-50x on string concat |
| Cache array length | Yes | Minor |
| Sized array allocation | No (see COMPILER.md) | Would be 100x+ |
| `str.append()` method | No (see COMPILER.md) | Would be 100x+ |
