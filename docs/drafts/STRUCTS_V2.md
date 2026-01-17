# Structs V2: SDK Implementation Progress

**Status:** Draft - Implementation in Progress
**Created:** 2026-01-17
**Extends:** [STRUCTS.md](STRUCTS.md)

---

## Overview

This document tracks the progress of implementing SDK types (Date, Time) using structs with methods, as a proof-of-concept for the struct system defined in STRUCTS.md.

The goal is to implement SDK types that:
1. Mirror the built-in types (Date, Time) exactly
2. Use regular structs with methods that delegate to C runtime functions
3. Demonstrate the struct method pattern can work outside the compiler core

---

## Implementation Approach

### Pattern: Regular Struct with C Adapters

After exploring multiple approaches, the working pattern is:

```sindarin
# 1. Regular struct (not native) with matching memory layout
struct SdkDate =>
    _days: int32

    # 2. Methods call native adapter functions
    static fn today(): SdkDate =>
        return sdk_date_today(arena)

    fn year(): int =>
        return sdk_date_year(self)

# 3. C adapter layer in #pragma source
#pragma source """
static inline SdkDate sdk_date_today(RtArena *arena) {
    RtDate *d = rt_date_today(arena);
    SdkDate result;
    result._days = d->days;
    return result;
}

static inline long long sdk_date_year(SdkDate *d) {
    RtDate rd = { .days = d->_days };
    return rt_date_get_year(&rd);
}
"""

# 4. Native fn declarations for the adapters
native fn sdk_date_today(a: *void): SdkDate
native fn sdk_date_year(d: *SdkDate): int
```

**Why this pattern:**
- Regular structs can be used in regular functions (unlike native structs)
- `#pragma source` provides the C adapter layer for type conversion
- Native fn declarations make the adapters callable from Sindarin
- The `arena` built-in provides access to the caller's arena for allocation
- Instance methods receive `self` as a pointer, enabling pass-by-reference

---

## Issues Discovered

### RESOLVED: Parser Bug with `native fn` in Struct Context

**Location:** `src/parser/parser_stmt_decl.c:809-833`

**Problem:** When parsing struct methods, `parser_is_method_start()` was incorrectly handling `native fn` declarations. The function called `parser_advance()` to peek at the next token but only restored `parser->current`, not the lexer state. This caused the lexer to be out of sync.

**Error:** `Expected 'fn' keyword (got 'getValue')` when parsing `native fn getValue(): int`

**Fix:** Simplified `parser_is_method_start()` to return `true` when `TOKEN_NATIVE` is seen in struct context without peeking ahead:

```c
static bool parser_is_method_start(Parser *parser)
{
    if (parser_check(parser, TOKEN_FN)) return true;
    if (parser_check(parser, TOKEN_STATIC)) return true;
    if (parser_check(parser, TOKEN_NATIVE))
    {
        /* In a struct body context, 'native' can only start a method (native fn).
         * We don't peek ahead because parser_advance() modifies lexer state
         * that cannot be easily restored. */
        return true;
    }
    return false;
}
```

**Status:** RESOLVED

---

### DOCUMENTED: Native Struct Usage Restriction

**Behavior:** Native structs can ONLY be used in native function contexts. This is by design.

```sindarin
native struct Buffer =>
    data: *byte
    length: int

# This works (inside native fn)
native fn use_buffer(): void =>
    var buf: Buffer = Buffer {}  # OK

# This fails (inside regular fn)
fn regular_use(): void =>
    var buf: Buffer = Buffer {}  # COMPILE ERROR: Native struct 'Buffer' can only be used in native function context
```

**Rationale:** This maintains the safety boundary - pointer types only exist in native contexts.

**Workaround:** Use regular structs with C adapter functions in `#pragma source` when you need to use struct types in regular functions.

**Status:** DOCUMENTED (by design, not a bug)

---

### UNRESOLVED: Import Mechanism Does Not Register Struct Types

**Problem:** When importing a module that defines a struct, the struct type is not properly available in the importing module during parsing.

**Example:**
```sindarin
# sdk/date.sn
struct SdkDate =>
    _days: int32
    static fn today(): SdkDate =>
        return sdk_date_today(arena)

# tests/sdk/test_date.sn
import "sdk/date.sn"

fn main(): void =>
    var d: SdkDate = SdkDate.today()  # ERROR: Type 'unknown' has no member 'today'
```

**Root Cause Analysis:**

1. The parser parses the main module first
2. When it encounters `SdkDate.today()`, it looks up the type `SdkDate`
3. At this point, the imported module hasn't been fully processed
4. The type lookup fails, resulting in a forward-reference "unknown" type
5. Later, when imports are merged (at line ~7598 in debug trace), the struct IS registered
6. But the lookups already happened earlier (at line ~257), so they got "unknown"

**Debug Evidence:**
```
[TRACE] Line 257: Looking up type 'SdkDate' - not found, creating forward reference
...
[TRACE] Line 7598: Registering struct type 'SdkDate' from imported module
```

**Impact:** SDK modules cannot be imported and used - the struct types are not visible.

**Workaround:** Embed the SDK code directly in the test file instead of importing.

**Potential Fix:** The compiler needs to process imports earlier, before parsing function bodies in the main module. This could involve:
1. A two-pass approach: first pass resolves imports, second pass parses bodies
2. Lazy type resolution during type checking instead of during parsing
3. Explicitly importing type definitions before parsing dependent code

**Status:** UNRESOLVED - Requires compiler modification

---

### UNRESOLVED: Forward Reference Type Resolution During Import

**Related to:** Import mechanism issue above

**Problem:** Types that are forward-referenced during parsing are never resolved to their actual definitions when those definitions come from imported modules.

**Technical Details:**
- The parser creates placeholder "forward reference" types for unknown type names
- These should be resolved during type checking
- However, the resolution doesn't happen for types that were defined in imported modules
- The type remains as "unknown" through code generation

**Status:** UNRESOLVED - Requires compiler modification

---

## Implementation Files

### sdk/date.sn

Complete implementation of `SdkDate` type following the pattern above. Compiles successfully on its own but cannot be imported due to the unresolved issues.

**Features implemented:**
- Static factory methods: `today()`, `fromYmd()`, `fromString()`, `fromEpochDays()`
- Getter methods: `year()`, `month()`, `day()`, `weekday()`, `dayOfYear()`, `epochDays()`, `daysInMonth()`
- Boolean methods: `isLeap()`, `isWeekend()`, `isWeekday()`
- Comparison methods: `isBefore()`, `isAfter()`, `equals()`, `diffDays()`
- Formatting methods: `format()`, `toIso()`, `toString()`
- Arithmetic methods: `addDays()`, `addWeeks()`, `addMonths()`, `addYears()`
- Boundary methods: `startOfMonth()`, `endOfMonth()`, `startOfYear()`, `endOfYear()`

### sdk/time.sn

Needs implementation following the same pattern as `sdk/date.sn`.

---

## Next Steps

1. **Fix import mechanism** - Register struct types from imports before parsing main module
2. **Implement sdk/time.sn** - Follow the same pattern as sdk/date.sn
3. **Create tests** - Once imports work, create comprehensive test files
4. **Validate memory model** - Ensure arena allocation matches built-in types

---

## References

- [STRUCTS.md](STRUCTS.md) - Original struct specification
- [INTEROP_V3.md](INTEROP_V3.md) - Related interop issues
- [../language/INTEROP.md](../language/INTEROP.md) - C interop documentation
