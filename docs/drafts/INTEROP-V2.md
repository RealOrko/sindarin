# Interop V2: Proposed Improvements

This document outlines proposed improvements to Sindarin's C interop system based on real-world experience implementing the `sdk/compression.sn` zlib bindings.

## Summary of Proposals

| Feature | Priority | Complexity | Impact |
|---------|----------|------------|--------|
| Expression-bodied functions | High | Low | Cleaner constants, helpers |
| Numeric type casting | High | Medium | Eliminates workarounds |
| Multi-line array literals | Medium | Low | Better test data, readability |
| `sizeof()` builtin | Medium | Low | Essential for some C APIs |
| Byte array assignment | Low | Medium | Dynamic byte buffer population |

---

## 1. Expression-Bodied Functions

### Problem

Single-expression functions require verbose multi-line syntax:

```sindarin
fn Z_OK(): int =>
    return 0

fn Z_BEST_SPEED(): int =>
    return 1

fn Z_DATA_ERROR(): int =>
    return -3

fn double(x: int): int =>
    return x * 2
```

This is especially painful for "constant" functions that just return a value.

### Proposal

Allow expression directly after `=>` on the same line, with implicit return:

```sindarin
fn Z_OK(): int => 0
fn Z_BEST_SPEED(): int => 1
fn Z_DATA_ERROR(): int => -3

fn double(x: int): int => x * 2
fn is_even(x: int): bool => x % 2 == 0
fn clamp(x: int, lo: int, hi: int): int => min(max(x, lo), hi)
```

### Grammar Change

```
function_def := 'fn' IDENT '(' params ')' ':' type '=>' (expression | block)
```

After `=>`, if the next token is on the same line and starts an expression, parse as implicit return. Otherwise, expect indented block.

### Implementation Notes

- Parser change in `parser_stmt.c`
- After parsing `=>`, check if next token is on same line
- If same line and is expression start token, parse expression and wrap in implicit return
- If newline/indent follows, parse block as usual
- Works for both `fn` and `native fn`

### Native Function Examples

Expression-bodied syntax works for native functions too:

```sindarin
// Native function declarations (no body) - unchanged
native fn compress(dest: *byte, destLen: uint as ref, source: *byte, sourceLen: uint): int

// Native functions with simple bodies
native fn get_errno(): int => __errno_location() as val
native fn null_byte(): *byte => nil

// Useful for thin wrappers
native fn compress_bound(len: int): int => compressBound(len as uint) as int
```

### Precedent

- Kotlin: `fun double(x: Int) = x * 2`
- Scala: `def double(x: Int) = x * 2`
- C# expression-bodied: `int Double(int x) => x * 2;`
- Rust closures: `|x| x * 2`

---

## 2. Numeric Type Casting

### Problem

No way to explicitly convert between numeric types. Must use intermediate variables:

```sindarin
native fn compress_to(source: byte[], dest: byte[]): int =>
    var destLen: uint = dest.length
    var result: int = compress(dest as ref, destLen, source as ref, source.length)
    if result != 0 =>
        return -1

    // WORKAROUND: Can't do "return destLen as int"
    var written: int = destLen
    return written
```

The `as` operator only works with `any` types for boxing/unboxing.

### Proposal

Extend `as` to support numeric type conversions:

```sindarin
native fn compress_to(source: byte[], dest: byte[]): int =>
    var destLen: uint = dest.length
    var result: int = compress(dest as ref, destLen, source as ref, source.length)
    if result != 0 =>
        return -1
    return destLen as int  // Now works!
```

### Supported Conversions

```sindarin
// Integer widening (always safe)
var b: byte = 255
var i: int = b as int        // 255

// Integer narrowing (may truncate)
var i: int = 1000
var b: byte = i as byte      // 232 (truncated)

// Signed/unsigned conversion
var i: int = -1
var u: uint = i as uint      // Large positive number

// Float/int conversion
var d: double = 3.14
var i: int = d as int        // 3 (truncated)
var f: float = 2.5
var i: int = f as int        // 2

// Int to float (may lose precision for large values)
var i: int = 42
var d: double = i as double  // 42.0
```

### Alternative: Constructor Syntax

Instead of extending `as`, use type-as-function syntax:

```sindarin
var i: int = int(destLen)
var b: byte = byte(value)
var d: double = double(count)
```

### Recommendation

Extend `as` operator - it's more consistent with existing `as val` and `as ref` patterns.

### Implementation Notes

- Type checker change in `type_checker_expr.c`
- Check if operand is numeric type and target is numeric type
- Code gen emits C cast: `(int64_t)(destLen)`

---

## 3. Multi-line Array Literals

### Problem

Array literals must be on a single line:

```sindarin
// This fails to parse:
var data: byte[] = {
    72, 101, 108, 108, 111,
    32, 87, 111, 114, 108,
    100, 33
}

// Must write:
var data: byte[] = {72, 101, 108, 108, 111, 32, 87, 111, 114, 108, 100, 33}
```

This makes test data and large static arrays hard to read.

### Proposal

Allow array literals to span multiple lines when inside braces:

```sindarin
var data: byte[] = {
    72, 101, 108, 108, 111,   // "Hello"
    32, 87, 111, 114, 108,    // " Worl"
    100, 33                    // "d!"
}

var matrix: int[][] = {
    {1, 2, 3},
    {4, 5, 6},
    {7, 8, 9}
}
```

### Implementation Notes

- Lexer/parser change
- When inside `{` for array literal, ignore newlines until matching `}`
- Similar to how many languages handle multi-line expressions in brackets

---

## 4. `sizeof()` Builtin

### Problem

Some C APIs require knowing type sizes at runtime. Currently no way to get this:

```sindarin
// qsort signature
native fn qsort(base: *void, count: int, size: int, cmp: Comparator): void

native fn sort_integers(arr: int[]): void =>
    // PROBLEM: How to get sizeof(int)?
    qsort(arr as ref, arr.length, 8, cmp)  // Hardcoded 8 = sizeof(int64_t)
```

### Proposal

Add `sizeof()` builtin that returns size in bytes:

```sindarin
native fn sort_integers(arr: int[]): void =>
    qsort(arr as ref, arr.length, sizeof(int), cmp)
```

### Supported Forms

```sindarin
sizeof(int)      // 8 (on 64-bit)
sizeof(byte)     // 1
sizeof(double)   // 8
sizeof(bool)     // 1
sizeof(*int)     // 8 (pointer size)
sizeof(MyStruct) // Size of struct including padding
```

### Restriction

Only allowed in `native fn` bodies where low-level size information is meaningful.

### Implementation Notes

- Parser recognizes `sizeof` as builtin
- Type checker validates type argument exists
- Code gen emits `sizeof(c_type_name)`

---

## 5. Byte Array Element Assignment

### Problem

Cannot assign computed values to byte array elements in regular functions:

```sindarin
fn fill_pattern(size: int): byte[] =>
    var data: byte[size]
    var i: int = 0
    while i < size =>
        // ERROR: Can't convert int to byte
        data[i] = i % 256
        i = i + 1
    return data
```

This works in `native fn` because type checking is relaxed, but not in regular functions.

### Current Workaround

Use native function or pre-compute byte array literals.

### Proposal

Allow implicit narrowing conversion when assigning to byte array elements, with defined truncation semantics:

```sindarin
fn fill_pattern(size: int): byte[] =>
    var data: byte[size]
    var i: int = 0
    while i < size =>
        data[i] = i % 256  // Implicitly truncates to byte
        i = i + 1
    return data
```

### Alternative

Require explicit cast (ties into proposal #2):

```sindarin
data[i] = (i % 256) as byte
```

### Recommendation

Require explicit cast - implicit narrowing can hide bugs. This makes proposal #2 (numeric casting) the prerequisite.

---

## Implementation Priority

### Phase 1: Quick Wins
1. **Expression-bodied functions** - Parser-only change, high impact
2. **Multi-line array literals** - Lexer change, improves readability

### Phase 2: Type System
3. **Numeric type casting** - Type checker + code gen, enables byte assignment
4. **Byte array assignment** - Depends on #3

### Phase 3: Native Enhancements
5. **sizeof() builtin** - Parser + type checker + code gen

---

## Example: Improved zlib SDK

With all proposals implemented, the SDK would be cleaner:

```sindarin
#pragma link "z"

// Constants as expression-bodied functions
fn Z_OK(): int => 0
fn Z_STREAM_END(): int => 1
fn Z_DATA_ERROR(): int => -3
fn Z_MEM_ERROR(): int => -4
fn Z_BUF_ERROR(): int => -5

fn Z_BEST_SPEED(): int => 1
fn Z_BEST_COMPRESSION(): int => 9
fn Z_DEFAULT_COMPRESSION(): int => -1

// Low-level bindings
native fn compress(dest: *byte, destLen: uint as ref, source: *byte, sourceLen: uint): int
native fn uncompress(dest: *byte, destLen: uint as ref, source: *byte, sourceLen: uint): int
native fn compressBound(sourceLen: uint): uint

// High-level wrapper - much cleaner with numeric casting
native fn compress_to(source: byte[], dest: byte[]): int =>
    var destLen: uint = dest.length
    var result: int = compress(dest as ref, destLen, source as ref, source.length)
    if result != 0 =>
        return -1
    return destLen as int  // Direct cast instead of intermediate variable

// Utility functions as one-liners
fn max_compressed_size(len: int): int => compressBound(len as uint) as int
fn is_error(code: int): bool => code < 0
fn compression_ratio(orig: int, comp: int): double => (comp * 100.0) / orig
```

---

## Open Questions

1. **Should `sizeof()` work on expressions or only types?**
   - Types only is simpler and matches C
   - Expressions would require evaluating the type at compile time

2. **Should numeric casts be checked at runtime for overflow?**
   - Unchecked (like C) is simpler and faster
   - Checked would be safer but adds overhead
   - Could tie into `--unchecked` flag

3. **Should multi-line support extend to other constructs?**
   - Function call arguments spanning lines?
   - Struct literals?
   - Would require broader parser changes

---

## References

- Current interop documentation: `docs/language/INTEROP.md`
- `as ref` implementation: commit `530fefa`
- zlib SDK implementation: `sdk/compression.sn`
