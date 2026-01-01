# Runtime Optimization: Function Inlining

This document describes the function inlining optimization implemented in the Sn runtime to reduce stack allocations and function call overhead.

## Implementation Status

**Implemented:** Option A - `static inline` in header with `#include "runtime.h"` in generated code.

### Inlined Functions (20 total)

| Category | Functions | Count |
|----------|-----------|-------|
| Long comparisons | `rt_eq_long`, `rt_ne_long`, `rt_lt_long`, `rt_le_long`, `rt_gt_long`, `rt_ge_long` | 6 |
| Double comparisons | `rt_eq_double`, `rt_ne_double`, `rt_lt_double`, `rt_le_double`, `rt_gt_double`, `rt_ge_double` | 6 |
| String comparisons | `rt_eq_string`, `rt_ne_string`, `rt_lt_string`, `rt_le_string`, `rt_gt_string`, `rt_ge_string` | 6 |
| Boolean operations | `rt_not_bool` | 1 |
| Array operations | `rt_array_length` | 1 |

### Changes Made

1. Added `static inline` function definitions to `runtime.h`
2. Added `RtArrayMetadata` typedef to `runtime.h` (for `rt_array_length`)
3. Added `#include <string.h>` to `runtime.h` (for `strcmp()` in string comparisons)
4. Modified code generator to `#include "runtime.h"` in generated C code
5. Added `-I<compiler_dir>` to GCC invocation for include path
6. Removed conflicting extern declarations from code generator
7. Updated Makefile to copy `runtime.h` to `bin/` directory

## What Is Inlined

### Numeric Comparisons

```c
static inline int rt_eq_long(long a, long b) { return a == b; }
static inline int rt_ne_long(long a, long b) { return a != b; }
static inline int rt_lt_long(long a, long b) { return a < b; }
static inline int rt_le_long(long a, long b) { return a <= b; }
static inline int rt_gt_long(long a, long b) { return a > b; }
static inline int rt_ge_long(long a, long b) { return a >= b; }

static inline int rt_eq_double(double a, double b) { return a == b; }
// ... etc
```

**Why inline:** Trivial one-liners called frequently in loops and conditionals. Eliminates ~2-5 CPU cycles per call.

### String Comparisons

```c
static inline int rt_eq_string(const char *a, const char *b) { return strcmp(a, b) == 0; }
static inline int rt_ne_string(const char *a, const char *b) { return strcmp(a, b) != 0; }
// ... etc
```

**Why inline:** Simple wrappers around `strcmp()`. Inlining eliminates function call overhead while still calling the optimized libc `strcmp()`.

### Boolean and Array Operations

```c
static inline int rt_not_bool(int a) { return !a; }

static inline size_t rt_array_length(void *arr) {
    if (arr == NULL) return 0;
    return ((RtArrayMetadata *)arr)[-1].size;
}
```

**Why inline:** `rt_not_bool` is trivial. `rt_array_length` is critical for loop performance - called in every `for` loop over arrays.

## What Is NOT Inlined (and Why)

### Arithmetic with Overflow Checking

```c
long rt_add_long(long a, long b)
{
    if ((b > 0 && a > LONG_MAX - b) || (b < 0 && a < LONG_MIN - b))
    {
        fprintf(stderr, "rt_add_long: overflow detected\n");
        exit(1);
    }
    return a + b;
}
```

| Function | Reason NOT to Inline |
|----------|---------------------|
| `rt_add_long`, `rt_sub_long`, `rt_mul_long`, `rt_div_long`, `rt_mod_long` | Overflow checks with `fprintf`/`exit` would bloat every arithmetic expression |
| `rt_add_double`, `rt_sub_double`, `rt_mul_double`, `rt_div_double` | Infinity checks with `fprintf`/`exit` |
| `rt_neg_long` | LONG_MIN overflow check |
| `rt_post_inc_long`, `rt_post_dec_long` | NULL and overflow checks |

**Better alternative:** Use `--unchecked` flag for performance-critical code. This generates native C operators `(a + b)` instead of function calls, eliminating both the call overhead AND the overflow checks.

### Complex Operations

| Function Category | Reason NOT to Inline |
|-------------------|---------------------|
| `rt_array_eq_*`, `rt_array_contains_*` | Iterate over arrays, complex logic |
| `rt_str_startsWith`, `rt_str_endsWith`, `rt_str_contains` | Multiple `strlen()` calls, branching |
| `rt_text_file_*`, `rt_binary_file_*` | I/O operations, error handling |
| `rt_path_exists`, `rt_path_is_file`, `rt_path_is_directory` | System calls (`stat()`) |
| `rt_time_is_before`, `rt_time_is_after`, `rt_time_equals` | NULL checks with `exit()` |
| Arena allocation functions | Complex logic, memory allocation fallbacks |

## Performance Impact

### Before (function calls)
```c
// Every comparison = function call
if (rt_lt_long(i, rt_array_length(arr))) { ... }
```

### After (inlined)
```c
// Comparisons compile to single CPU instructions
// Array length access is a single memory read
if (i < ((RtArrayMetadata *)arr)[-1].size) { ... }
```

**Estimated savings:**
- ~2-5 CPU cycles per comparison (eliminates stack frame setup, parameter passing)
- Better register allocation (compiler sees the full picture)
- Enables further optimizations (constant folding, dead code elimination)

## Alternative: Link-Time Optimization (LTO)

For additional performance, consider adding `-flto` to release builds:

```c
gcc -flto -O2 ...
```

This allows GCC to inline across translation units automatically, potentially inlining even functions that aren't marked `static inline`.

## Files Modified

- `src/runtime.h` - Added `static inline` definitions
- `src/runtime.c` - Removed duplicate implementations
- `src/code_gen.c` - Added `#include "runtime.h"`, removed conflicting externs
- `src/gcc_backend.c` - Added `-I<src_dir>` include path
- `src/Makefile` - Added rule to copy `runtime.h` to `bin/`
- `tests/unit/test_utils.h` - Updated expected output for unit tests
