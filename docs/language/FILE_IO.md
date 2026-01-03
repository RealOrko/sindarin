# File I/O in Sindarin

Sindarin provides two built-in types for file operations: `TextFile` for text-based file handling and `BinaryFile` for raw byte operations. Both types integrate with Sindarin's arena-based memory management.

## The `byte` Type

Before diving into file I/O, it's important to understand the `byte` primitive type used for binary operations.

```sindarin
// byte is an unsigned 8-bit value (0-255)
var b: byte = 255
var zero: byte = 0

// Byte arrays use curly braces like other arrays
var data: byte[] = {72, 101, 108, 108, 111}  // ASCII for "Hello"
var empty: byte[] = {}
```

**Note:** Hex literals (like `0xFF`) are not yet implemented. Use decimal values (0-255).

### Byte to Int Conversion

Bytes implicitly convert to integers for arithmetic:

```sindarin
var b1: byte = 100
var b2: byte = 50
var sum: int = b1 + b2      // 150
var product: int = b1 * b2  // 5000 (exceeds byte range, int handles it)
```

---

## TextFile

`TextFile` is used for reading and writing text content. It works with `str` values.

### Static Methods

These methods operate directly on file paths without opening a file handle:

#### TextFile.readAll(path)

Reads the entire file content as a string.

```sindarin
var content: str = TextFile.readAll("data.txt")
print(content)
```

#### TextFile.writeAll(path, content)

Writes a string to a file, creating or overwriting it.

```sindarin
TextFile.writeAll("output.txt", "Hello, World!\nLine 2")
```

#### TextFile.exists(path)

Checks if a file exists.

```sindarin
if TextFile.exists("config.txt") =>
  var config: str = TextFile.readAll("config.txt")
else =>
  print("Config file not found\n")
```

#### TextFile.copy(source, dest)

Copies a file to a new location.

```sindarin
TextFile.copy("original.txt", "backup.txt")
```

#### TextFile.move(source, dest)

Moves or renames a file.

```sindarin
TextFile.move("old_name.txt", "new_name.txt")
```

#### TextFile.delete(path)

Deletes a file.

```sindarin
TextFile.delete("temp.txt")
```

### Instance Methods

For streaming operations, open a file handle:

#### TextFile.open(path)

Opens a file and returns a handle.

```sindarin
var f: TextFile = TextFile.open("data.txt")
// ... use the file ...
f.close()
```

#### readLine()

Reads the next line (without the trailing newline).

```sindarin
var f: TextFile = TextFile.open("data.txt")
while !f.isEof() =>
  var line: str = f.readLine()
  print($"{line}\n")
f.close()
```

#### readAll()

Reads all remaining content.

```sindarin
var f: TextFile = TextFile.open("data.txt")
var firstLine: str = f.readLine()
var rest: str = f.readAll()  // Everything after the first line
f.close()
```

#### readLines()

Reads all remaining lines into an array.

```sindarin
var f: TextFile = TextFile.open("data.txt")
var lines: str[] = f.readLines()
for line in lines =>
  print($"Line: {line}\n")
f.close()
```

#### readWord()

Reads the next whitespace-delimited token.

```sindarin
var f: TextFile = TextFile.open("numbers.txt")
while f.hasChars() =>
  var word: str = f.readWord()
  print($"Word: {word}\n")
f.close()
```

#### readChar()

Reads a single character (returns -1 at EOF).

```sindarin
var f: TextFile = TextFile.open("data.txt")
var ch: int = f.readChar()
while ch != -1 =>
  print($"Char: {ch}\n")
  ch = f.readChar()
f.close()
```

#### writeLine(text)

Writes a string followed by a newline.

```sindarin
var f: TextFile = TextFile.open("output.txt")
f.writeLine("First line")
f.writeLine("Second line")
f.close()
```

#### write(text)

Writes a string without a trailing newline.

```sindarin
f.write("Hello")
f.write(" ")
f.write("World")  // Outputs: "Hello World"
```

#### writeChar(ch)

Writes a single character.

```sindarin
f.writeChar('A')
```

#### print(text) / println(text)

Formatted write methods (uses string interpolation syntax).

```sindarin
f.print($"Value: {x}")
f.println($"Line: {i}")  // Adds newline
```

### State Methods

#### isEof()

Returns true if at end of file.

```sindarin
while !f.isEof() =>
  var line: str = f.readLine()
  process(line)
```

#### hasChars() / hasWords() / hasLines()

Returns true if more content is available.

```sindarin
while f.hasChars() =>
  var ch: int = f.readChar()

while f.hasWords() =>
  var word: str = f.readWord()

while f.hasLines() =>
  var line: str = f.readLine()
```

#### position()

Gets the current byte position in the file.

```sindarin
var pos: int = f.position()
```

#### seek(pos)

Seeks to a byte position in the file.

```sindarin
f.seek(0)  // Return to start
f.seek(pos)  // Go to saved position
```

#### rewind()

Returns to the beginning of the file.

```sindarin
f.rewind()
```

#### flush()

Forces buffered data to be written to disk.

```sindarin
f.writeLine("Important data")
f.flush()  // Ensure it's written
```

#### close()

Closes the file handle. Always close files when done.

```sindarin
var f: TextFile = TextFile.open("data.txt")
// ... operations ...
f.close()
```

### Properties

```sindarin
var p: str = f.path   // Full file path
var n: str = f.name   // Filename only (without directory)
var s: int = f.size   // File size in bytes
```

---

## BinaryFile

`BinaryFile` is used for reading and writing raw bytes.

### Static Methods

#### BinaryFile.readAll(path)

Reads the entire file as a byte array.

```sindarin
var data: byte[] = BinaryFile.readAll("image.bin")
print($"File size: {data.length} bytes\n")
```

#### BinaryFile.writeAll(path, data)

Writes a byte array to a file.

```sindarin
var header: byte[] = {0, 1, 2, 3}
BinaryFile.writeAll("output.bin", header)
```

#### BinaryFile.exists(path)

Checks if a file exists.

```sindarin
if BinaryFile.exists("data.bin") =>
  var data: byte[] = BinaryFile.readAll("data.bin")
```

#### BinaryFile.copy(source, dest)

Copies a binary file.

```sindarin
BinaryFile.copy("original.bin", "backup.bin")
```

#### BinaryFile.move(source, dest)

Moves or renames a binary file.

```sindarin
BinaryFile.move("temp.bin", "final.bin")
```

#### BinaryFile.delete(path)

Deletes a binary file.

```sindarin
BinaryFile.delete("temp.bin")
```

### Instance Methods

#### BinaryFile.open(path)

Opens a binary file and returns a handle.

```sindarin
var f: BinaryFile = BinaryFile.open("data.bin")
// ... use the file ...
f.close()
```

#### readByte()

Reads a single byte (returns -1 at EOF).

```sindarin
var f: BinaryFile = BinaryFile.open("data.bin")
var b: int = f.readByte()
while b != -1 =>
  print($"Byte: {b}\n")
  b = f.readByte()
f.close()
```

#### readBytes(count)

Reads up to `count` bytes into an array.

```sindarin
var f: BinaryFile = BinaryFile.open("data.bin")
var header: byte[] = f.readBytes(4)
var body: byte[] = f.readBytes(100)
f.close()
```

#### readAll()

Reads all remaining bytes.

```sindarin
var f: BinaryFile = BinaryFile.open("data.bin")
var allData: byte[] = f.readAll()
f.close()
```

#### readInto(buffer)

Reads into an existing byte buffer, returns number of bytes read.

```sindarin
var buffer: byte[] = byte[1024]{}
var n: int = f.readInto(buffer)
```

#### writeByte(value)

Writes a single byte.

```sindarin
var f: BinaryFile = BinaryFile.open("output.bin")
f.writeByte(255)
f.writeByte(0)
f.close()
```

#### writeBytes(data)

Writes a byte array.

```sindarin
var f: BinaryFile = BinaryFile.open("output.bin")
var data: byte[] = {1, 2, 3, 4, 5}
f.writeBytes(data)
f.close()
```

### State Methods

#### isEof()

Returns true if at end of file.

#### hasBytes()

Returns true if more bytes are available.

#### position() / seek(pos) / rewind()

Position management, same as TextFile.

#### flush()

Forces buffered data to be written to disk.

```sindarin
f.writeByte(42)
f.flush()  // Ensure it's written
```

#### close()

Closes the file handle.

### Properties

```sindarin
var p: str = f.path   // Full file path
var n: str = f.name   // Filename only
var s: int = f.size   // File size in bytes
```

---

## Byte Array Methods

Byte arrays have special methods for encoding and conversion.

### toString()

Converts bytes to a string (UTF-8 decoding).

```sindarin
var bytes: byte[] = {72, 101, 108, 108, 111}
var text: str = bytes.toString()  // "Hello"
```

### toStringLatin1()

Converts bytes to a string using Latin-1/ISO-8859-1 encoding.

```sindarin
var bytes: byte[] = {72, 101, 108, 108, 111}
var text: str = bytes.toStringLatin1()
```

### toHex()

Encodes bytes as a hexadecimal string.

```sindarin
var bytes: byte[] = {72, 101, 108, 108, 111}
var hex: str = bytes.toHex()  // "48656c6c6f"
```

### toBase64()

Encodes bytes as a Base64 string.

```sindarin
var bytes: byte[] = {77, 97, 110}
var b64: str = bytes.toBase64()  // "TWFu"
```

### String to Bytes

Convert a string to its byte representation:

```sindarin
var text: str = "Hello"
var bytes: byte[] = text.toBytes()  // {72, 101, 108, 108, 111}
```

### Bytes Static Methods

#### Bytes.fromHex(hexString)

Decodes a hexadecimal string to bytes.

```sindarin
var bytes: byte[] = Bytes.fromHex("48656c6c6f")
var text: str = bytes.toString()  // "Hello"
```

#### Bytes.fromBase64(base64String)

Decodes a Base64 string to bytes.

```sindarin
var bytes: byte[] = Bytes.fromBase64("SGVsbG8=")
var text: str = bytes.toString()  // "Hello"
```

---

## Standard Streams

Built-in file handles for console I/O:

```sindarin
// Read line from standard input
var line: str = Stdin.readLine()

// Write to standard output
Stdout.write("Hello")
Stdout.writeLine("Hello")

// Write to standard error
Stderr.write("Error!")
Stderr.writeLine("Error!")
```

### Convenience Functions

Global functions for simple console I/O:

```sindarin
// Read line from stdin
var input: str = readLine()

// Write to stdout
print("output")
println("output")

// Write to stderr
printErr("error")
printErrLn("error")
```

---

## Path Utilities

Static methods for path manipulation:

```sindarin
// Extract directory portion
var dir: str = Path.directory("/home/user/file.txt")   // "/home/user"

// Extract filename
var name: str = Path.filename("/home/user/file.txt")   // "file.txt"

// Extract extension (without dot)
var ext: str = Path.extension("/home/user/file.txt")   // "txt"

// Join path components
var full: str = Path.join("/home", "user", "file.txt") // "/home/user/file.txt"

// Resolve to absolute path
var abs: str = Path.absolute("./file.txt")             // "/current/dir/file.txt"

// Check path existence and type
var exists: bool = Path.exists("/some/path")
var isFile: bool = Path.isFile("/some/path")
var isDir: bool = Path.isDirectory("/some/path")
```

---

## Directory Operations

```sindarin
// List files in directory
var files: str[] = Directory.list("/home/user")

// List files recursively
var files: str[] = Directory.listRecursive("/home/user")

// Create directory (creates parents if needed)
Directory.create("/new/path")

// Delete directory (must be empty, panics otherwise)
Directory.delete("/old/path")

// Delete directory and all contents
Directory.deleteRecursive("/old/path")
```

---

## Memory Management

File handles integrate with Sindarin's arena-based memory management.

### Automatic Cleanup

Files are automatically closed when their arena is destroyed:

```sindarin
fn processFile(path: str): str =>
  var f: TextFile = TextFile.open(path)
  var content: str = f.readAll()
  // f is automatically closed when function returns
  return content
```

### Private Blocks

Use private blocks for guaranteed cleanup:

```sindarin
var result: str = ""
private =>
  var f: TextFile = TextFile.open("data.txt")
  result = f.readAll()
  // f is GUARANTEED to close here, even on early return
```

**Important:** File handles opened within a private block cannot escape:

```sindarin
var f: TextFile    // Outer variable

private =>
  f = TextFile.open("data.txt")    // COMPILE ERROR: File cannot escape private block
```

### Shared Functions

Shared functions use the caller's arena:

```sindarin
fn processFile(f: TextFile) shared: str =>
  // Uses caller's arena - no new arena created
  // f remains open (caller owns it)
  return f.readLine()
```

### Explicit Closing

For long-running operations, close files explicitly:

```sindarin
for path in filePaths =>
  var f: TextFile = TextFile.open(path)
  process(f)
  f.close()  // Don't wait for arena cleanup
```

### Loop Iteration Behavior

**Non-shared loops** (default): Per-iteration arena cleanup

```sindarin
for path in paths =>
  var f: TextFile = TextFile.open(path)
  process(f)
  // f.close() called automatically at iteration end
```

**Shared loops**: File persists across iterations

```sindarin
var f: TextFile = TextFile.open("log.txt")
for item in items shared =>
  f.writeLine(item)    // Same file handle, no re-open
f.close()
```

---

## Common Patterns

### Process File Line by Line

```sindarin
var f: TextFile = TextFile.open("data.txt")
while !f.isEof() =>
  var line: str = f.readLine()
  if !line.isBlank() =>
    processLine(line)
f.close()
```

### Read and Process All Lines

```sindarin
var content: str = TextFile.readAll("data.txt")
var lines: str[] = content.splitLines()
for line in lines =>
  if !line.isBlank() =>
    print($"Processing: {line}\n")
```

### Copy File with Modification

```sindarin
var content: str = TextFile.readAll("input.txt")
var modified: str = content.replace("old", "new")
TextFile.writeAll("output.txt", modified)
```

### Binary File Header Check

```sindarin
var f: BinaryFile = BinaryFile.open("file.bin")
var header: byte[] = f.readBytes(4)
if header.toHex() == "89504e47" =>
  print("PNG file detected\n")
f.close()
```

### Write CSV File

```sindarin
var f: TextFile = TextFile.open("output.csv")
f.writeLine("Name,Age,City")
f.writeLine("Alice,30,Boston")
f.writeLine("Bob,25,Seattle")
f.close()
```

### Read Words from File

```sindarin
var content: str = TextFile.readAll("document.txt")
var words: str[] = content.splitWhitespace()
print($"Word count: {words.length}\n")
```

### Parse CSV File

```sindarin
fn parseCsv(path: str): str[][] =>
  var content: str = TextFile.readAll(path)
  var lines: str[] = content.splitLines()
  var result: str[][] = str[][]{}

  for line in lines =>
    if line.isBlank() =>
      continue
    var fields: str[] = line.split(",")
    var trimmed: str[] = str[]{}
    for field in fields =>
      trimmed.push(field.trim())
    result.push(trimmed)

  return result
```

### Log File Analyzer

```sindarin
fn analyzeLog(path: str): void =>
  var f: TextFile = TextFile.open(path)
  var errorCount: int = 0
  var warnCount: int = 0

  while f.hasLines() =>
    var line: str = f.readLine()
    if line.contains("[ERROR]") =>
      errorCount = errorCount + 1
    else if line.contains("[WARN]") =>
      warnCount = warnCount + 1

  f.close()
  println($"Errors: {errorCount}, Warnings: {warnCount}")
```

---

## Threading

File operations work naturally with Sindarin's threading model. Use `&` to spawn I/O in background threads and `!` to synchronize.

### Parallel File Reads

```sindarin
var f1: str = &TextFile.readAll("file1.txt")
var f2: str = &TextFile.readAll("file2.txt")
var f3: str = &TextFile.readAll("file3.txt")

// Wait for all to complete
[f1, f2, f3]!

print($"Total: {f1.length + f2.length + f3.length} bytes\n")
```

### Background Write

```sindarin
// Fire and forget - write happens in background
&TextFile.writeAll("backup.txt", data)

// Continue with other work...
```

### Inline Sync

```sindarin
// Spawn and immediately wait
var content: str = &TextFile.readAll("large.txt")!

// Use in expressions
var total: int = &countLines("a.txt")! + &countLines("b.txt")!
```

### Memory Semantics

File handles and data follow the frozen reference rules:

```sindarin
var data: str = "content to write"
var result: void = &TextFile.writeAll("out.txt", data)

data = "modified"  // COMPILE ERROR: data frozen by thread

result!            // sync releases freeze
data = "modified"  // OK
```

---

## Error Handling

File operations panic on errors (file not found, permission denied, etc.). Always check existence before operations that require existing files:

```sindarin
var path: str = "config.txt"
if TextFile.exists(path) =>
  var config: str = TextFile.readAll(path)
  processConfig(config)
else =>
  print("Warning: Config file not found, using defaults\n")
  useDefaults()
```

Examples of panic conditions:
- `TextFile.open()` - File doesn't exist or permission denied
- `TextFile.delete()` - File doesn't exist or permission denied
- `f.readLine()` - I/O error during read
- `f.seek(pos)` - Invalid position

Future versions may introduce a `Result` type for recoverable error handling.

---

## Design Decisions

This section documents the design rationale for the file I/O system.

### Principles

1. **Consistency** - Method naming follows existing conventions (camelCase like arrays/strings)
2. **Simplicity** - No mode flags, files are always read/write capable
3. **Safety** - Panic on errors rather than returning null
4. **Clarity** - Separate types for text (`TextFile`) and binary (`BinaryFile`) operations
5. **Ergonomics** - Convenience methods for common operations

### Arena Lifecycle

File handles are bound to the arena in which they are opened:

| Context | Arena Behavior | File Behavior |
|---------|----------------|---------------|
| Function entry | New arena created | Files opened here close on function exit |
| `private =>` block | New isolated arena | Files **guaranteed** to close at block end |
| `shared` function | Uses caller's arena | Files persist in caller's scope |
| Loop iteration (non-shared) | Per-iteration arena | File opens/closes each iteration |
| Loop iteration (shared) | Parent arena reused | File persists across iterations |

### Escaping Rules

File handles follow these escaping rules:

| Scenario | Allowed? | Behavior |
|----------|----------|----------|
| Return file from function | Yes | Handle promoted to caller's arena |
| Assign to outer variable | Yes (non-private) | Handle ownership transfers |
| Assign to outer variable from `private` | **No** | Compile-time error |
| Pass to function | Yes | Reference passed, caller retains ownership |
| Pass to `shared` function | Yes | Same handle, no ownership change |
| Capture in closure | Yes | Handle lifetime extends to closure lifetime |

### Return Value Promotion

```sindarin
fn openConfig(): TextFile =>
  var f: TextFile = TextFile.open("config.txt")
  return f    // Handle promoted to caller's arena

fn main(): int =>
  var config: TextFile = openConfig()
  // config is valid here - we own it now
  var data: str = config.readAll()
  config.close()
  return 0
```

---

## Implementation Notes

### Static Method Syntax

Implementing `TextFile.open()` requires parser support for static method calls on type names:

```
TypeName.methodName(args...)
```

The parser must recognize type names (`TextFile`, `BinaryFile`, `Path`, `Directory`, `Bytes`, `Stdin`, `Stdout`, `Stderr`) in expression position followed by `.`.

### Type Checking

New type kinds required:

```c
typedef enum {
    // ... existing types
    TYPE_BYTE,
    TYPE_TEXT_FILE,
    TYPE_BINARY_FILE,
} TypeKind;
```

### Code Generation

File operations map to runtime functions:

| Sindarin | C Runtime |
|----------|-----------|
| `TextFile.open(path)` | `rt_text_file_open(arena, path)` |
| `f.readLine()` | `rt_text_file_read_line(arena, f)` |
| `f.writeLine(s)` | `rt_text_file_write_line(f, s)` |
| `f.close()` | `rt_text_file_close(f)` |

### C Runtime Structures

```c
typedef struct RtTextFile {
    FILE *fp;
    char *path;
    bool is_open;
} RtTextFile;

typedef struct RtBinaryFile {
    FILE *fp;
    char *path;
    bool is_open;
} RtBinaryFile;
```

### Arena Integration

File handles are tracked via a linked list in the runtime arena:

```c
typedef struct RtFileHandle {
    FILE *fp;
    char *path;
    bool is_open;
    struct RtFileHandle *next;
} RtFileHandle;

typedef struct RtArena {
    // ... existing fields
    RtFileHandle *open_files;    // Head of file handle list
} RtArena;

void rt_arena_destroy(RtArena *arena) {
    // Close all open files first
    RtFileHandle *fh = arena->open_files;
    while (fh) {
        if (fh->is_open && fh->fp) {
            fclose(fh->fp);
            fh->is_open = false;
        }
        fh = fh->next;
    }
    // ... existing arena cleanup
}
```

### Handle Promotion

When a file handle escapes its scope, it must be promoted to the parent arena:

```c
RtTextFile *rt_text_file_promote(RtArena *dest, RtTextFile *src) {
    // Allocate new handle in destination arena
    RtTextFile *promoted = rt_arena_alloc(dest, sizeof(RtTextFile));
    promoted->fp = src->fp;
    promoted->path = rt_arena_promote_string(dest, src->path);
    promoted->is_open = src->is_open;

    // Add to destination arena's file list
    promoted->next = dest->open_files;
    dest->open_files = promoted;

    // Remove from source arena's file list (don't close on source destroy)
    rt_arena_untrack_file(src_arena, src);

    return promoted;
}
```

---

## See Also

- [THREADING.md](THREADING.md) - Threading model (`&` spawn, `!` sync)
- [MEMORY.md](MEMORY.md) - Arena memory management details
- [ARRAYS.md](ARRAYS.md) - Array operations including byte arrays
