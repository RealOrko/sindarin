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
var b: byte = 255b
var zero: byte = 0b
```

**Note:** Hex literals (like `0xFF`) are not yet implemented. Use decimal values with the `b` or `B` suffix.

Bytes implicitly convert to integers for arithmetic:

```sindarin
var b1: byte = 100
var b2: byte = 50
var sum: int = b1 + b2      // 150
var product: int = b1 * b2  // 5000 (exceeds byte range, int handles it)
```

### int32

32-bit signed integer. Used primarily for C interoperability.

```sindarin
var small: int32 = 42i32
var negative: int32 = -100i32
```

Range: -2,147,483,648 to 2,147,483,647

### uint

64-bit unsigned integer. Used for sizes and C interoperability.

```sindarin
var size: uint = 18446744073709551615u  // Max uint64
var count: uint = 0u
```

Range: 0 to 18,446,744,073,709,551,615

### uint32

32-bit unsigned integer. Used primarily for C interoperability.

```sindarin
var flags: uint32 = 255u32
var mask: uint32 = 0u32
```

Range: 0 to 4,294,967,295

### float

32-bit floating-point number (IEEE 754 single precision). Used for C interoperability when `double` precision is not needed.

```sindarin
var precise: float = 3.14f
var small: float = 0.001f
```

## Numeric Literal Suffixes

Sindarin supports type suffixes on numeric literals to explicitly specify the type:

| Suffix | Type | Example |
|--------|------|---------|
| (none) | `int` | `42` |
| `l` or `L` | `long` | `42l`, `42L` |
| `b` or `B` | `byte` | `255b`, `255B` |
| (none) or `d` or `D` | `double` | `3.14`, `3.14d`, `3.14D` |
| `f` or `F` | `float` | `3.14f`, `3.14F` |
| `u` or `U` | `uint` | `1000u`, `1000U` |
| `u32` or `U32` | `uint32` | `42u32`, `42U32` |
| `i32` or `I32` | `int32` | `42i32`, `42I32` |

### Why Use Suffixes?

Sindarin does not implicitly widen integer types. Without suffixes, you must match the declared type exactly:

```sindarin
var x: long = 100     // ERROR: int literal cannot assign to long
var x: long = 100l    // OK: explicit long literal

var y: float = 3.14   // ERROR: double literal cannot assign to float
var y: float = 3.14f  // OK: explicit float literal

var z: byte = 255     // ERROR: int literal cannot assign to byte
var z: byte = 255b    // OK: explicit byte literal
```

### Suffixes in Expressions

Literals with suffixes work naturally in expressions:

```sindarin
var sum: long = 10l + 20L           // long + long = long
var product: uint = 100u * 2u       // uint * uint = uint
var result: float = 1.5f + 2.5f     // float + float = float
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

## Pointer Types

Pointer types are used for C interoperability. They can only be used in `native` functions.

### Syntax

```sindarin
*T       // pointer to T
**T      // pointer to pointer to T
*void    // void pointer (opaque data)
```

### Null Pointers

The `nil` constant represents a null pointer:

```sindarin
var p: *int = nil

if p != nil =>
    // use pointer
```

`nil` is only valid for pointer types.

### Opaque Types

Opaque types represent C structures that cannot be inspected from Sindarin:

```sindarin
type FILE = opaque

native fn fopen(path: str, mode: str): *FILE
native fn fclose(f: *FILE): int
```

See [INTEROP.md](INTEROP.md) for full documentation on pointer types and C interoperability.

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

### Random

Random number generator with secure defaults. See [RANDOM.md](RANDOM.md) for full documentation.

```sindarin
// Static methods use OS entropy (no setup required)
var dice: int = Random.int(1, 6)
var coin: bool = Random.bool()
var pick: str = Random.choice({"red", "green", "blue"})

// Instance methods for reproducible sequences
var rng: Random = Random.createWithSeed(42)
var value: int = rng.int(1, 100)
```

Static methods:
- `int(min, max)`, `long(min, max)`, `double(min, max)` - Random values in range
- `bool()`, `byte()`, `bytes(count)` - Random boolean, byte, or byte array
- `gaussian(mean, stddev)` - Sample from normal distribution
- `choice(array)`, `shuffle(array)`, `sample(array, count)` - Collection operations
- `create()`, `createWithSeed(seed)` - Create Random instances

### UUID

Universally unique identifier type. See [UUID.md](UUID.md) for full documentation.

```sindarin
// Generate a time-ordered UUID (v7) - recommended
var id: UUID = UUID.create()
print($"New record: {id}\n")

// Parse from string
var parsed: UUID = UUID.fromString("01912345-6789-7abc-8def-0123456789ab")

// Deterministic UUID from namespace + name
var ns: UUID = UUID.namespaceUrl()
var userId: UUID = UUID.v5(ns, "https://myapp.com/users/alice")
```

Static methods:
- `create()`, `v7()` - Generate time-ordered UUIDv7
- `v4()` - Generate random UUIDv4
- `v5(namespace, name)` - Deterministic UUID from namespace + name
- `fromString(str)`, `fromHex(str)`, `fromBase64(str)` - Parse from string formats
- `nil()`, `max()` - Special UUID values

Instance methods:
- `.version()`, `.variant()`, `.isNil()` - UUID properties
- `.timestamp()`, `.time()` - Time extraction (v7 only)
- `.toString()`, `.toHex()`, `.toBase64()`, `.toBytes()` - Conversion

### TcpListener

A TCP socket that listens for incoming connections. See [NETWORK_IO.md](NETWORK_IO.md) for full documentation.

```sindarin
var server: TcpListener = TcpListener.bind(":8080")
print($"Listening on port {server.port}\n")

var client: TcpStream = server.accept()
// handle client...
server.close()
```

Static methods:
- `bind(address)` - Create listener bound to address

Instance methods:
- `.accept()` - Wait for and accept a connection
- `.close()` - Close the listener

Properties:
- `.port` - Bound port number

### TcpStream

A TCP connection for bidirectional communication. See [NETWORK_IO.md](NETWORK_IO.md) for full documentation.

```sindarin
var conn: TcpStream = TcpStream.connect("example.com:80")
conn.writeLine("GET / HTTP/1.0")
conn.writeLine("")
var response: byte[] = conn.readAll()
conn.close()
```

Static methods:
- `connect(address)` - Connect to remote address

Instance methods:
- `.read(maxBytes)` - Read up to maxBytes
- `.readAll()` - Read until connection closes
- `.readLine()` - Read until newline
- `.write(data)` - Write bytes
- `.writeLine(text)` - Write string + newline
- `.close()` - Close the connection

Properties:
- `.remoteAddress` - Remote peer address

### UdpSocket

A UDP socket for connectionless datagram communication. See [NETWORK_IO.md](NETWORK_IO.md) for full documentation.

```sindarin
var socket: UdpSocket = UdpSocket.bind(":9000")
socket.sendTo("Hello".toBytes(), "127.0.0.1:9001")

var data: byte[]
var sender: str
data, sender = socket.receiveFrom(1024)
socket.close()
```

Static methods:
- `bind(address)` - Create socket bound to address

Instance methods:
- `.sendTo(data, address)` - Send datagram
- `.receiveFrom(maxBytes)` - Receive datagram and sender address
- `.close()` - Close the socket

Properties:
- `.port` - Bound port number

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
| `long` | 64-bit signed integer | `42l`, `42L` |
| `int32` | 32-bit signed integer | `42i32`, `42I32` |
| `uint` | 64-bit unsigned integer | `42u`, `42U` |
| `uint32` | 32-bit unsigned integer | `42u32`, `42U32` |
| `double` | 64-bit floating-point | `3.14`, `3.14d`, `1.5e10` |
| `float` | 32-bit floating-point | `3.14f`, `3.14F` |
| `bool` | Boolean | `true`, `false` |
| `char` | Single character | `'A'`, `'\n'`, `'\t'` |
| `byte` | Unsigned 8-bit (0-255) | `255b`, `255B` |
| `str` | String | `"hello"`, `$"Hi {name}"` |
| `void` | No return value | (function returns only) |
| `type[]` | Array of type | `{1, 2, 3}`, `{"a", "b"}` |
| `*T` | Pointer to T | `nil` (native functions only) |
| `TextFile` | Text file handle | (from `TextFile.open()`) |
| `BinaryFile` | Binary file handle | (from `BinaryFile.open()`) |
| `Date` | Calendar date value | (from `Date.today()`) |
| `Time` | Date/time value | (from `Time.now()`) |
| `Process` | Process execution result | (from `Process.run()`) |
| `UUID` | Universally unique identifier | (from `UUID.create()`) |
| `Random` | Random number generator | (from `Random.create()`) |
| `TcpListener` | TCP server socket | (from `TcpListener.bind()`) |
| `TcpStream` | TCP connection | (from `TcpStream.connect()`) |
| `UdpSocket` | UDP socket | (from `UdpSocket.bind()`) |

### Utility Namespaces

| Namespace | Description |
|-----------|-------------|
| `Path` | File system path manipulation |
| `Directory` | Directory listing and management |
| `Stdin` | Standard input stream |
| `Stdout` | Standard output stream |
| `Stderr` | Standard error stream |
| `Bytes` | Byte array conversions |
| `Interceptor` | Function interception for debugging and mocking |

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

### Explicit Conversions with `as`

Sindarin supports explicit type casting between numeric types using the `as` keyword:

```sindarin
var x: int = 42
var b: byte = x as byte      // int to byte
var d: double = x as double  // int to double
var f: float = d as float    // double to float
```

#### Supported Numeric Conversions

All numeric types can be cast to each other:

| From | To |
|------|-----|
| `int` | `byte`, `uint`, `int32`, `uint32`, `long`, `double`, `float`, `char` |
| `byte` | `int`, `uint`, `int32`, `uint32`, `long`, `double`, `float`, `char` |
| `uint` | `int`, `byte`, `int32`, `uint32`, `long`, `double`, `float`, `char` |
| `int32` | `int`, `byte`, `uint`, `uint32`, `long`, `double`, `float`, `char` |
| `uint32` | `int`, `byte`, `uint`, `int32`, `long`, `double`, `float`, `char` |
| `long` | `int`, `byte`, `uint`, `int32`, `uint32`, `double`, `float`, `char` |
| `double` | `int`, `byte`, `uint`, `int32`, `uint32`, `long`, `float`, `char` |
| `float` | `int`, `byte`, `uint`, `int32`, `uint32`, `long`, `double`, `char` |
| `char` | `int`, `byte`, `uint`, `int32`, `uint32`, `long`, `double`, `float` |
| `bool` | `int`, `byte`, `uint`, `int32`, `uint32`, `long`, `double`, `float` |

#### Truncation and Precision

When casting to a smaller type, values may be truncated:

```sindarin
var big: int = 1000
var small: byte = big as byte  // Truncates to 232 (1000 mod 256)

var pi: double = 3.7
var truncated: int = pi as int // Truncates to 3 (no rounding)
```

#### Boolean to Numeric

Boolean values convert to numeric types:

```sindarin
var t: bool = true
var f: bool = false
var t_int: int = t as int     // 1
var f_int: int = f as int     // 0
var t_double: double = t as double  // 1.0
```

#### Character Conversions

Characters can be cast to/from numeric types using their character code:

```sindarin
var c: char = 'A'
var code: int = c as int      // 65

var num: int = 66
var letter: char = num as char // 'B'
```

#### Chained Casts

Casts can be chained:

```sindarin
var orig: double = 123.456
var result: byte = (orig as int) as byte  // 123
```

#### Casts in Expressions

Casts have high precedence and work in expressions:

```sindarin
var x: double = 10.5
var y: double = 3.5
var sum: int = (x as int) + (y as int)  // 13
```

### String and Byte Conversions

String to bytes:
```sindarin
var bytes: byte[] = "Hello".toBytes()
```

Bytes to string:
```sindarin
var text: str = bytes.toString()
```

## sizeof Operator

The `sizeof` operator returns the size in bytes of a type or expression. It delegates directly to C's `sizeof` operator.

### Syntax

Parentheses are optional:

```sindarin
sizeof(int)     // 8
sizeof int      // 8
sizeof(x)       // size of variable x
sizeof x        // size of variable x
```

### On Primitive Types

```sindarin
sizeof(int)     // 8
sizeof(long)    // 8
sizeof(double)  // 8
sizeof(float)   // 4
sizeof(byte)    // 1
sizeof(bool)    // 1
sizeof(char)    // 1
sizeof(int32)   // 4
sizeof(uint)    // 8
sizeof(uint32)  // 4
```

### On Variables

`sizeof` returns the size of the variable's type:

```sindarin
var x: int = 42
var d: double = 3.14
var b: byte = 255b

sizeof(x)       // 8
sizeof(d)       // 8
sizeof(b)       // 1
```

### On Structs

Returns the struct size including padding:

```sindarin
struct Point =>
    x: double
    y: double

sizeof(Point)   // 16 (2 * 8 bytes)

var p: Point = Point { x: 1.0, y: 2.0 }
sizeof(p)       // 16
```

### On Arrays

For array variables, `sizeof` returns the pointer size (8 bytes), not the array contents size. Use `.length` to get the element count:

```sindarin
var arr: int[] = {1, 2, 3, 4, 5}
sizeof(arr)     // 8 (pointer size)
arr.length      // 5 (element count)
```

### In Expressions

`sizeof` can be used in expressions:

```sindarin
var total: int = sizeof(int) + sizeof(double)  // 16
var count: int = 1024 / sizeof(int)            // 128
```

### In Native Functions

In native functions, `sizeof` also works on pointer types:

```sindarin
native fn example(): void =>
    sizeof(*int)    // 8
    sizeof(*void)   // 8
```

## See Also

- [STRINGS.md](STRINGS.md) - String methods and interpolation
- [ARRAYS.md](ARRAYS.md) - Array operations
- [STRUCTS.md](STRUCTS.md) - Struct declarations and fields
- [FILE_IO.md](FILE_IO.md) - TextFile and BinaryFile types
- [NETWORK_IO.md](NETWORK_IO.md) - TCP and UDP networking
- [DATE.md](DATE.md) - Date type operations
- [TIME.md](TIME.md) - Time type operations
- [RANDOM.md](RANDOM.md) - Random number generation
- [UUID.md](UUID.md) - Universally unique identifiers
- [SDK Environment](../sdk/env.md) - Environment variables (SDK)
- [MEMORY.md](MEMORY.md) - Memory management and type lifetimes
- [INTEROP.md](INTEROP.md) - C interoperability and pointer types
- [INTERCEPTORS.md](INTERCEPTORS.md) - Function interception
