# Interop V2: Proposed Improvements

This document outlines proposed improvements to Sindarin's C interop system based on real-world experience implementing the `sdk/compression.sn` zlib bindings.

## Summary of Proposals

| Feature | Priority | Complexity | Impact |
|---------|----------|------------|--------|
| Expression-bodied functions | High | Low | Cleaner constants, helpers |
| Numeric type casting | High | Medium | Eliminates workarounds |
| Multi-line literals (arrays, structs) | Medium | Low | Better readability |
| Multi-line strings | Medium | Medium | SQL, HTML, JSON embedding |
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

### Overflow Behavior

Numeric casts follow the same overflow semantics as arithmetic operations:

- **Default**: Checked - runtime error on overflow/truncation that loses data
- **`--unchecked`**: No checking, C-style truncation (faster)
- **`-O2`**: Implies unchecked arithmetic

```sindarin
var big: int = 1000
var b: byte = big as byte  // Runtime error in checked mode (1000 > 255)
                           // Silently truncates to 232 in unchecked mode
```

Widening conversions (e.g., `byte` to `int`) never overflow and are always safe.

### Implementation Notes

- Type checker change in `type_checker_expr.c`
- Check if operand is numeric type and target is numeric type
- Code gen emits checked or unchecked cast based on compiler flags
- Checked: `rt_checked_cast_to_byte(value)` (runtime validation)
- Unchecked: `(uint8_t)(value)` (C-style truncation)

---

## 3. Multi-line Array and Struct Literals

### Problem

Array and struct literals must be on a single line:

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

This makes test data, large static arrays, and complex struct initialization hard to read.

### Proposal

Allow array and struct literals to span multiple lines when inside braces:

```sindarin
// Arrays
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

// Structs
var config: Config = Config {
    name: "myapp",
    port: 8080,
    debug: true
}

var point: Point = Point {
    x: 10.5,
    y: 20.3
}

// Nested structs
var server: Server = Server {
    config: Config {
        name: "web",
        port: 443
    },
    handlers: {handler1, handler2}
}
```

### Implementation Notes

- Lexer/parser change
- When inside `{` for array/struct literal, ignore newlines until matching `}`
- Similar to how many languages handle multi-line expressions in brackets
- Comments inside multi-line literals should be allowed

---

## 4. `sizeof()` Builtin

### Problem

Some C APIs require knowing type sizes. Currently no way to get this:

```sindarin
native fn sort_integers(arr: int[]): void =>
    qsort(arr as ref, arr.length, 8, cmp)  // Hardcoded 8 = sizeof(int64_t)
```

### Solution

Add `sizeof()` builtin that delegates directly to C's `sizeof` operator.

### On Types

```sindarin
sizeof(int)      // 8 (on 64-bit)
sizeof(byte)     // 1
sizeof(double)   // 8
sizeof(bool)     // 1
sizeof(*int)     // 8 (pointer size)
sizeof(MyStruct) // Size of struct including padding
```

### On Expressions

```sindarin
var x: int = 42
sizeof(x)        // 8 (size of int)

var p: Point = Point { x: 1.0, y: 2.0 }
sizeof(p)        // 16 (size of Point struct)
```

Note: Like C, `sizeof(expr)` determines the type at compile time - the expression is not evaluated.

### Usage with C APIs

```sindarin
native fn sort_integers(arr: int[]): void =>
    qsort(arr as ref, arr.length, sizeof(int), cmp)
```

For arrays, use `.length` to get element count - `sizeof()` returns the size of the array pointer type.

### Implementation Notes

- Parser recognizes `sizeof` as builtin (with or without parentheses)
- Delegates directly to C's `sizeof` operator
- Works on types: `sizeof(int)`, `sizeof(MyStruct)`
- Works on expressions: `sizeof(x)`, `sizeof x`

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

### Solution

Require explicit cast using `as byte`. This prevents implicit narrowing bugs and depends on numeric type casting (section 2):

```sindarin
fn fill_pattern(size: int): byte[] =>
    var data: byte[size]
    var i: int = 0
    while i < size =>
        data[i] = (i % 256) as byte
        i = i + 1
    return data
```

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

---

## 6. Multi-line Strings

### Problem

No way to write multi-line strings without manual `\n` escapes:

```sindarin
var sql: str = "SELECT * FROM users\nWHERE active = true\nORDER BY name"
```

### Solution: Pipe Block Strings

Use `|` to start an indentation-based string block. This aligns with Sindarin's design philosophy - the string block is defined by indentation, just like code blocks.

### Syntax

```sindarin
var sql: str = |
    SELECT * FROM users
    WHERE active = true
    ORDER BY name

// With interpolation
var query: str = $|
    SELECT * FROM {table}
    WHERE id = {id}
```

#### Rules

1. `|` or `$|` followed by newline starts a block string
2. All subsequent lines with greater indentation than the `|` line are included
3. The block ends at the first line with equal or less indentation (dedent)
4. Common leading whitespace is stripped (based on least-indented content line)
5. Trailing newline is included (consistent with typical file content)

#### Examples

```sindarin
fn get_query(): str =>
    var sql: str = |
        SELECT *
        FROM users
        WHERE active = true
    return sql
    // sql = "SELECT *\nFROM users\nWHERE active = true\n"

fn get_html(title: str): str =>
    return $|
        <html>
            <head><title>{title}</title></head>
            <body>Hello</body>
        </html>
    // Returns "<html>\n    <head>..." with inner indent preserved

fn config(): str =>
    var json: str = |
        {
            "name": "app",
            "version": 1
        }
    return json
    // json = "{\n    \"name\": \"app\",\n    \"version\": 1\n}\n"
```

#### Edge Cases

```sindarin
// Empty lines preserve relative indentation
var text: str = |
    First paragraph.

    Second paragraph.
// text = "First paragraph.\n\nSecond paragraph.\n"

// Deeper indentation preserved relative to base
var code: str = |
    fn example():
        if true:
            return 1
// code = "fn example():\n    if true:\n        return 1\n"

// Single line (least useful but valid)
var one: str = |
    single line
// one = "single line\n"
```

### Implementation Notes

- Lexer recognizes `|` followed by newline as start of block string
- Track indentation level of the `|` line
- Collect lines until dedent
- Calculate common indent from collected lines
- Strip common indent from all lines
- Join with newlines, add trailing newline
- For `$|`, parse interpolation expressions within the block

---

## References

- Current interop documentation: `docs/language/INTEROP.md`
- `as ref` implementation: commit `530fefa`
- zlib SDK implementation: `sdk/compression.sn`
