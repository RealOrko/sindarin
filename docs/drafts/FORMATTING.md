# String Formatting in Sindarin

> **DRAFT - PARTIALLY IMPLEMENTED** - Core formatting functionality is implemented. This document describes both the current implementation and planned enhancements.

Format specifiers allow precise control over how values are formatted within interpolated strings. The syntax follows a pattern inspired by Python/C# format specifiers: `{expression:format}`.

---

## Overview

```sindarin
var pi: double = 3.14159
var count: int = 42
var name: str = "Alice"

// Precision formatting
print($"Pi is approximately {pi:.2f}\n")    // Pi is approximately 3.14

// Width and padding
print($"Count: {count:05d}\n")              // Count: 00042

// Base conversion
print($"Hex: {count:x}\n")                  // Hex: 2a

// String width
print($"Name: {name:10s}!\n")               // Name: Alice     !
```

---

## Syntax

Format specifiers appear after a colon inside interpolation braces:

```
$"text {expression:format_spec} more text"
```

The format specification follows the pattern: `[[fill]align][sign][#][0][width][.precision][type]`

---

## Numeric Formatting

### Integer Types

| Specifier | Description | Example | Output |
|-----------|-------------|---------|--------|
| `d` | Decimal integer | `{42:d}` | `42` |
| `x` | Lowercase hex | `{255:x}` | `ff` |
| `X` | Uppercase hex | `{255:X}` | `FF` |
| `o` | Octal | `{8:o}` | `10` |
| `b` | Binary | `{5:b}` | `101` |

```sindarin
var num: int = 255

print($"Decimal: {num:d}\n")    // Decimal: 255
print($"Hex: {num:x}\n")        // Hex: ff
print($"Octal: {num:o}\n")      // Octal: 377
print($"Binary: {num:b}\n")     // Binary: 11111111
```

### Floating-Point Types

| Specifier | Description | Example | Output |
|-----------|-------------|---------|--------|
| `f` | Fixed-point | `{3.14159:.2f}` | `3.14` |
| `e` | Scientific notation | `{1234.5:e}` | `1.234500e+03` |
| `E` | Scientific (uppercase) | `{1234.5:E}` | `1.234500E+03` |
| `%` | Percentage | `{0.25:%}` | `25.000000%` |

```sindarin
var pi: double = 3.14159265359

print($"Default: {pi:f}\n")      // Default: 3.141593
print($"2 decimals: {pi:.2f}\n") // 2 decimals: 3.14
print($"4 decimals: {pi:.4f}\n") // 4 decimals: 3.1416
print($"No decimals: {pi:.0f}\n") // No decimals: 3

var ratio: double = 0.756
print($"Percentage: {ratio:%}\n") // Percentage: 75.600000%
```

---

## Width and Padding

Control the minimum width and padding character for formatted output.

| Specifier | Description | Example | Output |
|-----------|-------------|---------|--------|
| `5d` | Minimum width 5 | `{42:5d}` | `   42` |
| `05d` | Zero-padded width 5 | `{42:05d}` | `00042` |
| `-5d` | Left-aligned width 5 | `{42:-5d}` | `42   ` |

```sindarin
var n: int = 42

// Right-aligned (default)
print($"[{n:5d}]\n")     // [   42]

// Zero-padded
print($"[{n:05d}]\n")    // [00042]

// Left-aligned
print($"[{n:-5d}]\n")    // [42   ]

// Combined with base conversion
print($"[{n:08x}]\n")    // [0000002a]
```

---

## String Formatting

| Specifier | Description | Example | Output |
|-----------|-------------|---------|--------|
| `s` | String (default) | `{"hello":s}` | `hello` |
| `10s` | Min width 10 | `{"hi":10s}` | `hi        ` |
| `-10s` | Right-aligned | `{"hi":-10s}` | `        hi` |
| `.5s` | Max 5 chars | `{"hello world":.5s}` | `hello` |

```sindarin
var name: str = "Alice"
var title: str = "Hello World"

// Minimum width (left-aligned by default for strings)
print($"[{name:10s}]\n")      // [Alice     ]

// Right-aligned
print($"[{name:-10s}]\n")     // [     Alice]

// Truncation
print($"[{title:.5s}]\n")     // [Hello]

// Combined width and truncation
print($"[{title:10.5s}]\n")   // [Hello     ]
```

---

## Boolean Formatting

Booleans format as `true` or `false` by default.

```sindarin
var flag: bool = true
print($"Value: {flag}\n")   // Value: true
```

---

## Common Patterns

### Tabular Output

```sindarin
fn printTable(items: str[], values: int[]): void =>
    print("Item          Value\n")
    print("----          -----\n")
    for var i: int = 0; i < items.length; i++ =>
        print($"{items[i]:12s}  {values[i]:5d}\n")

// Usage
var items: str[] = {"Apples", "Oranges", "Bananas"}
var counts: int[] = {42, 7, 156}
printTable(items, counts)
// Output:
// Item          Value
// ----          -----
// Apples           42
// Oranges           7
// Bananas         156
```

### Hex Dump

```sindarin
fn hexDump(data: byte[]): void =>
    for var i: int = 0; i < data.length; i++ =>
        var b: int = data[i]
        print($"{b:02x} ")
        if (i + 1) % 16 == 0 =>
            print("\n")
    print("\n")

// Usage
var data: byte[] = {72, 101, 108, 108, 111}
hexDump(data)  // 48 65 6c 6c 6f
```

### Progress Display

```sindarin
fn showProgress(current: int, total: int): void =>
    var percent: double = current * 100.0 / total
    print($"\rProgress: {percent:5.1f}% ({current:4d}/{total:4d})")

// Usage
for var i: int = 0; i <= 100; i++ =>
    showProgress(i, 100)
    Time.sleep(50)
```

### Currency Formatting

```sindarin
fn formatCurrency(amount: double): str =>
    return $"${amount:.2f}"

// Usage
var price: double = 19.99
print($"Total: {formatCurrency(price)}\n")  // Total: $19.99
```

### Zero-Padded IDs

```sindarin
fn formatId(id: int): str =>
    return $"ID-{id:06d}"

// Usage
print($"{formatId(42)}\n")     // ID-000042
print($"{formatId(12345)}\n")  // ID-012345
```

---

## Implementation Checklist

### Implemented
- [x] Format specifier syntax in string interpolation (`{expr:spec}`)
- [x] Integer formatting (`d`, `x`, `X`, `o`, `b`)
- [x] Floating-point formatting (`f`, `e`, `E`, `%`)
- [x] Width and padding (`05d`, `-5d`)
- [x] Precision for floats (`.2f`)
- [x] String width and truncation (`10s`, `.5s`)

### Needs Testing/Documentation
- [ ] Comprehensive test coverage for edge cases
- [ ] Negative width values
- [ ] Zero precision
- [ ] Empty format strings

### Planned Enhancements
- [ ] Alignment specifiers (`<`, `>`, `^` for left/right/center)
- [ ] Sign specifiers (`+`, `-`, ` ` for always/negative-only/space)
- [ ] Thousands separator (`,` or `_`)
- [ ] Custom fill characters
- [ ] Date/time formatting integration with `Date` and `Time` types
- [ ] Locale-aware formatting

---

## Revision History

| Date | Changes |
|------|---------|
| 2024-12-30 | Initial draft with format specifier syntax |
| 2025-01-02 | Restructured to match language documentation style |

---

## See Also

- [STRINGS.md](../language/STRINGS.md) - String methods and interpolation basics
- [TYPES.md](../language/TYPES.md) - Primitive types and their default formatting
- [DATE.md](../language/DATE.md) - Date type (future formatting integration)
- [TIME.md](../language/TIME.md) - Time type (future formatting integration)
