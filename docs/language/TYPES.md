# Types in Sindarin

Sindarin is a statically-typed language with explicit type annotations. This document covers all primitive and built-in types.

## Primitive Types

### int

64-bit signed integer.

```sindarin
var count: int = 42
var negative: int = -7
var zero: int = 0
```

Range: -9,223,372,036,854,775,808 to 9,223,372,036,854,775,807

### long

64-bit signed integer (same as `int`, with explicit suffix).

```sindarin
var big: long = 42L
var value: long = 1000000000L
```

### double

64-bit floating-point number (IEEE 754 double precision).

```sindarin
var pi: double = 3.14159
var negative: double = -2.5
var scientific: double = 1.5e10
```

### bool

Boolean type with values `true` and `false`.

```sindarin
var active: bool = true
var done: bool = false
```

Boolean operators:
- `&&` - logical AND
- `||` - logical OR
- `!` - logical NOT

```sindarin
if hasTicket && hasID =>
  print("Entry allowed\n")

if isAdmin || isModerator =>
  print("Can moderate\n")

if !isBlocked =>
  print("Access granted\n")
```

### char

Single character, enclosed in single quotes.

```sindarin
var letter: char = 'A'
var digit: char = '5'
var newline: char = '\n'
```

Escape sequences work the same as in strings:
- `'\n'` - newline
- `'\t'` - tab
- `'\\'` - backslash
- `'\''` - single quote

### byte

Unsigned 8-bit integer (0-255). Used for binary data operations.

```sindarin
var b: byte = 255
var zero: byte = 0
```

**Note:** Hex literals (like `0xFF`) are not yet implemented. Use decimal values.

Bytes implicitly convert to integers for arithmetic:

```sindarin
var b1: byte = 100
var b2: byte = 50
var sum: int = b1 + b2      // 150
var product: int = b1 * b2  // 5000 (exceeds byte range, int handles it)
```

### str

String type for text data. See [STRINGS.md](STRINGS.md) for full documentation.

```sindarin
var greeting: str = "Hello, World!"
var name: str = "Sindarin"
```

### void

Used for functions that don't return a value.

```sindarin
fn greet(name: str): void =>
  print($"Hello, {name}!\n")
```

## Array Types

Arrays are declared with `type[]` syntax:

```sindarin
var numbers: int[] = {1, 2, 3, 4, 5}
var names: str[] = {"alice", "bob", "charlie"}
var bytes: byte[] = {72, 101, 108, 108, 111}
var empty: int[] = {}
```

See [ARRAYS.md](ARRAYS.md) for full documentation on array operations.

## Built-in Types

### TextFile

Handle for text-oriented file operations. Works with `str` values.

```sindarin
// Static methods
var content: str = TextFile.readAll("data.txt")
TextFile.writeAll("output.txt", content)
var exists: bool = TextFile.exists("data.txt")

// Instance methods
var f: TextFile = TextFile.open("data.txt")
var line: str = f.readLine()
f.close()
```

See [FILE_IO.md](FILE_IO.md) for full documentation.

### BinaryFile

Handle for binary file operations. Works with `byte` and `byte[]` values.

```sindarin
// Static methods
var data: byte[] = BinaryFile.readAll("image.bin")
BinaryFile.writeAll("copy.bin", data)

// Instance methods
var f: BinaryFile = BinaryFile.open("data.bin")
var b: int = f.readByte()
var bytes: byte[] = f.readBytes(10)
f.close()
```

See [FILE_IO.md](FILE_IO.md) for full documentation.

### Time

Represents date and time values. See [TIME.md](TIME.md) for full documentation.

```sindarin
var now: Time = Time.now()
var formatted: str = now.format("%Y-%m-%d %H:%M:%S")
```

## Type Summary Table

| Type | Description | Example Literals |
|------|-------------|------------------|
| `int` | 64-bit signed integer | `42`, `-7`, `0` |
| `long` | 64-bit signed integer | `42L`, `1000L` |
| `double` | 64-bit floating-point | `3.14`, `-2.5`, `1.5e10` |
| `bool` | Boolean | `true`, `false` |
| `char` | Single character | `'A'`, `'\n'`, `'\t'` |
| `byte` | Unsigned 8-bit (0-255) | `255`, `0` |
| `str` | String | `"hello"`, `$"Hi {name}"` |
| `void` | No return value | (function returns only) |
| `type[]` | Array of type | `{1, 2, 3}`, `{"a", "b"}` |
| `TextFile` | Text file handle | (from `TextFile.open()`) |
| `BinaryFile` | Binary file handle | (from `BinaryFile.open()`) |
| `Time` | Date/time value | (from `Time.now()`) |

## Type Annotations

### Variable Declarations

Variables require explicit type annotations:

```sindarin
var name: str = "Sindarin"
var count: int = 42
var pi: double = 3.14159
var active: bool = true
```

### Function Parameters and Returns

Functions require type annotations for parameters and return types:

```sindarin
fn add(a: int, b: int): int =>
  return a + b

fn greet(name: str): void =>
  print($"Hello, {name}!\n")

fn factorial(n: int): int =>
  if n <= 1 =>
    return 1
  return n * factorial(n - 1)
```

### Array Types

Array type annotations use the `type[]` syntax:

```sindarin
fn sum(numbers: int[]): int =>
  var total: int = 0
  for n in numbers =>
    total = total + n
  return total

fn getNames(): str[] =>
  return {"alice", "bob", "charlie"}
```

## Type Conversion

### Implicit Conversions

- `byte` to `int` (for arithmetic operations)

### Explicit Conversions

String to bytes:
```sindarin
var bytes: byte[] = "Hello".toBytes()
```

Bytes to string:
```sindarin
var text: str = bytes.toString()
```

## See Also

- [STRINGS.md](STRINGS.md) - String methods and interpolation
- [ARRAYS.md](ARRAYS.md) - Array operations
- [FILE_IO.md](FILE_IO.md) - TextFile and BinaryFile types
- [TIME.md](TIME.md) - Time type operations
- [MEMORY.md](MEMORY.md) - Memory management and type lifetimes
