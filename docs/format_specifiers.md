# Format Specifiers for String Interpolation

## Overview

Format specifiers allow precise control over how values are formatted within interpolated strings.
The syntax follows a pattern inspired by Python/C# format specifiers: `{expression:format}`.

## Syntax

```
$"text {expression:format_spec} more text"
```

Where `format_spec` follows the pattern: `[[fill]align][sign][#][0][width][.precision][type]`

## Supported Format Types

### Numeric Types

| Type | Description | Example | Output |
|------|-------------|---------|--------|
| `d` | Decimal integer | `{42:d}` | `42` |
| `x` | Lowercase hex | `{255:x}` | `ff` |
| `X` | Uppercase hex | `{255:X}` | `FF` |
| `o` | Octal | `{8:o}` | `10` |
| `b` | Binary | `{5:b}` | `101` |
| `f` | Fixed-point float | `{3.14159:.2f}` | `3.14` |
| `e` | Scientific notation | `{1234.5:e}` | `1.234500e+03` |
| `E` | Scientific (uppercase) | `{1234.5:E}` | `1.234500E+03` |
| `%` | Percentage | `{0.25:%}` | `25.000000%` |

### Width and Padding

| Spec | Description | Example | Output |
|------|-------------|---------|--------|
| `5d` | Minimum width 5 | `{42:5d}` | `   42` |
| `05d` | Zero-padded width 5 | `{42:05d}` | `00042` |
| `-5d` | Left-aligned width 5 | `{42:-5d}` | `42   ` |

### Precision

| Spec | Description | Example | Output |
|------|-------------|---------|--------|
| `.2f` | 2 decimal places | `{3.14159:.2f}` | `3.14` |
| `.4f` | 4 decimal places | `{3.14159:.4f}` | `3.1416` |
| `.0f` | No decimal places | `{3.7:.0f}` | `4` |

### String Formatting

| Spec | Description | Example | Output |
|------|-------------|---------|--------|
| `s` | String (default) | `{"hello":s}` | `hello` |
| `10s` | Min width 10 | `{"hi":10s}` | `hi        ` |
| `-10s` | Right-padded | `{"hi":-10s}` | `        hi` |
| `.5s` | Max 5 chars | `{"hello world":.5s}` | `hello` |

### Boolean Formatting

| Spec | Description | Example | Output |
|------|-------------|---------|--------|
| (none) | Default | `{true}` | `true` |

## Implementation Details

### AST Changes

The `InterpolExpr` structure is extended to store format specifiers:

```c
typedef struct {
    Expr **parts;           // Expression parts
    char **format_specs;    // Format specifier for each part (NULL if none)
    int part_count;
} InterpolExpr;
```

### Lexer Changes

The lexer preserves the colon and format specifier as part of the expression content.
Format specifiers are recognized as: `:` followed by format characters until `}`.

### Parser Changes

When parsing interpolation expressions, the parser:
1. Extracts the expression up to `:` or `}`
2. If `:` is found, extracts the format specifier
3. Stores both in the AST node

### Code Generation

Format specifiers are passed to runtime formatting functions:

```c
char *rt_format_int(RtArena *arena, long value, const char *format);
char *rt_format_double(RtArena *arena, double value, const char *format);
char *rt_format_string(RtArena *arena, const char *value, const char *format);
```

## Examples

```sn
var pi: double = 3.14159
var count: int = 42
var name: str = "Alice"

// Basic formatting
print($"Pi is approximately {pi:.2f}\n")           // Pi is approximately 3.14
print($"Count: {count:05d}\n")                     // Count: 00042
print($"Hex: {count:x}\n")                         // Hex: 2a

// Width and alignment
print($"Name: {name:10s}!\n")                      // Name: Alice     !
print($"Right: {count:>5d}\n")                     // Right:    42

// Combined
print($"Value: {count:+05d}\n")                    // Value: +0042
```

## Future Enhancements

1. Custom format functions for user-defined types
2. Locale-aware formatting (thousands separators, decimal points)
3. Date/time formatting when those types are added
