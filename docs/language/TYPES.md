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

### Date

Represents calendar dates without time components. See [DATE.md](DATE.md) for full documentation.

```sindarin
var today: Date = Date.today()
var formatted: str = today.format("YYYY-MM-DD")
```

### Time

Represents date and time values. See [TIME.md](TIME.md) for full documentation.

```sindarin
var now: Time = Time.now()
var formatted: str = now.format("YYYY-MM-DD HH:mm:ss")
```

### Process

Handle for spawning and managing external processes.

```sindarin
// Run a command
var result: Process = Process.run("ls")
print($"Exit code: {result.exitCode}\n")
print($"Output: {result.stdout}\n")

// Run with arguments
var p: Process = Process.run("grep", {"pattern", "file.txt"})
if p.exitCode == 0 =>
  print(p.stdout)
else =>
  print($"Error: {p.stderr}\n")
```

Properties:
- `exitCode: int` - Process exit code (0 typically means success, 127 if command not found)
- `stdout: str` - Captured standard output
- `stderr: str` - Captured standard error

## Utility Namespaces

Sindarin provides several utility namespaces with static methods for common operations.

### Path

File system path manipulation utilities.

```sindarin
// Extract path components
var dir: str = Path.directory("/home/user/file.txt")   // "/home/user"
var name: str = Path.filename("/home/user/file.txt")   // "file.txt"
var ext: str = Path.extension("/home/user/file.txt")   // "txt"

// Join paths
var full: str = Path.join("/home", "user", "file.txt") // "/home/user/file.txt"

// Resolve to absolute path
var abs: str = Path.absolute("./relative/path")

// Check path properties
if Path.exists("config.json") =>
  if Path.isFile("config.json") =>
    print("Found config file\n")

if Path.isDirectory("./data") =>
  print("Data directory exists\n")
```

Methods:
| Method | Signature | Description |
|--------|-----------|-------------|
| `directory` | `(path: str): str` | Extract directory portion |
| `filename` | `(path: str): str` | Extract filename with extension |
| `extension` | `(path: str): str` | Extract extension (without dot) |
| `join` | `(path1: str, path2: str, ...): str` | Join 2-3 path components |
| `absolute` | `(path: str): str` | Resolve to absolute path |
| `exists` | `(path: str): bool` | Check if path exists |
| `isFile` | `(path: str): bool` | Check if path is a regular file |
| `isDirectory` | `(path: str): bool` | Check if path is a directory |

### Directory

Directory manipulation utilities.

```sindarin
// List directory contents
var files: str[] = Directory.list("./src")
for f in files =>
  print($"{f}\n")

// List recursively (includes subdirectories)
var allFiles: str[] = Directory.listRecursive("./project")

// Create and delete directories
Directory.create("./output")
Directory.delete("./empty_dir")           // Must be empty
Directory.deleteRecursive("./temp")       // Deletes contents too
```

Methods:
| Method | Signature | Description |
|--------|-----------|-------------|
| `list` | `(path: str): str[]` | List files in directory (non-recursive) |
| `listRecursive` | `(path: str): str[]` | List all files recursively |
| `create` | `(path: str): void` | Create a directory |
| `delete` | `(path: str): void` | Delete an empty directory |
| `deleteRecursive` | `(path: str): void` | Delete directory and all contents |

### Stdin

Standard input stream utilities.

```sindarin
// Read input
var line: str = Stdin.readLine()
var word: str = Stdin.readWord()
var ch: int = Stdin.readChar()

// Check input availability
while Stdin.hasLines() =>
  var input: str = Stdin.readLine()
  print($"Got: {input}\n")

if !Stdin.isEof() =>
  print("More input available\n")
```

Methods:
| Method | Signature | Description |
|--------|-----------|-------------|
| `readLine` | `(): str` | Read a line from stdin |
| `readWord` | `(): str` | Read a whitespace-delimited word |
| `readChar` | `(): int` | Read a single character (returns char code) |
| `hasChars` | `(): bool` | Check if characters are available |
| `hasLines` | `(): bool` | Check if lines are available |
| `isEof` | `(): bool` | Check if at end of input |

### Stdout

Standard output stream utilities.

```sindarin
Stdout.write("Hello ")
Stdout.writeLine("World!")
Stdout.flush()
```

Methods:
| Method | Signature | Description |
|--------|-----------|-------------|
| `write` | `(text: str): void` | Write text without newline |
| `writeLine` | `(text: str): void` | Write text with newline |
| `flush` | `(): void` | Flush output buffer |

### Stderr

Standard error stream utilities.

```sindarin
Stderr.write("Warning: ")
Stderr.writeLine("Something went wrong")
Stderr.flush()
```

Methods:
| Method | Signature | Description |
|--------|-----------|-------------|
| `write` | `(text: str): void` | Write text without newline |
| `writeLine` | `(text: str): void` | Write text with newline |
| `flush` | `(): void` | Flush error buffer |

### Bytes

Byte array conversion utilities.

```sindarin
// Convert hex string to bytes
var bytes: byte[] = Bytes.fromHex("48656c6c6f")  // "Hello" in hex

// Convert Base64 string to bytes
var decoded: byte[] = Bytes.fromBase64("SGVsbG8=")  // "Hello" in Base64
```

Methods:
| Method | Signature | Description |
|--------|-----------|-------------|
| `fromHex` | `(hex: str): byte[]` | Convert hex string to byte array |
| `fromBase64` | `(b64: str): byte[]` | Convert Base64 string to byte array |

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
| `Date` | Calendar date value | (from `Date.today()`) |
| `Time` | Date/time value | (from `Time.now()`) |
| `Process` | Process execution result | (from `Process.run()`) |

### Utility Namespaces

| Namespace | Description |
|-----------|-------------|
| `Path` | File system path manipulation |
| `Directory` | Directory listing and management |
| `Stdin` | Standard input stream |
| `Stdout` | Standard output stream |
| `Stderr` | Standard error stream |
| `Bytes` | Byte array conversions |

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
- [DATE.md](DATE.md) - Date type operations
- [TIME.md](TIME.md) - Time type operations
- [MEMORY.md](MEMORY.md) - Memory management and type lifetimes
