# Sindarin I/O System Design

This document outlines the design for file I/O and streaming in the Sindarin language.

ALWAYS EXECUTE COMPILATION AND BINARIES WITH 'timeout' TO AVOID CRASHING THE COMPUTER.

## Design Principles

1. **Consistency** - Method naming follows existing conventions (camelCase like arrays/strings)
2. **Simplicity** - No mode flags, files are always read/write capable
3. **Safety** - Panic on errors rather than returning null
4. **Clarity** - Separate types for text (`TextFile`) and binary (`BinaryFile`) operations
5. **Ergonomics** - Convenience methods for common operations

## New Types

### `byte`

A new primitive type representing an unsigned 8-bit value (0-255).

```sn
var b: byte = 255               # Decimal literal
var b2: byte = 0                # Zero value

# Byte arrays (use curly braces)
var buffer: byte[] = {}         # Empty array
var data: byte[] = {72, 101, 108, 108, 111}  # ASCII for "Hello"
```

**Note:** Hex literals (0xFF) are not yet implemented. Use decimal values (0-255).

**C mapping:** `unsigned char`

### `TextFile`

Handle for text-oriented file operations. Works with `str` and `char` types.

### `BinaryFile`

Handle for binary file operations. Works with `byte` and `byte[]` types.

---

## TextFile API

### Static Methods

```sn
# Open file for reading and writing (panics if file doesn't exist or can't be opened)
var f: TextFile = TextFile.open("data.txt")

# Check if file exists without opening
var exists: bool = TextFile.exists("data.txt")

# One-shot read entire file contents
var content: str = TextFile.readAll("data.txt")

# One-shot write entire file (creates or overwrites)
TextFile.writeAll("out.txt", content)

# Delete file (panics if fails)
TextFile.delete("old.txt")

# Copy file to new location
TextFile.copy("src.txt", "dst.txt")

# Move/rename file
TextFile.move("old.txt", "new.txt")
```

### Reading Methods

```sn
# Read single character
var ch: char = f.readChar()

# Read whitespace-delimited token
var word: str = f.readWord()

# Read single line (strips trailing newline)
var line: str = f.readLine()

# Read all remaining content as string
var all: str = f.readAll()

# Read all remaining lines as array
var lines: str[] = f.readLines()

# Read into character buffer, returns number of chars read
var n: int = f.readInto(buffer)
```

### Writing Methods

```sn
# Write single character
f.writeChar(ch)

# Write string
f.write(text)

# Write string followed by newline
f.writeLine(text)

# Formatted write (uses string interpolation syntax)
f.print($"Value: {x}")

# Formatted write followed by newline
f.println($"Value: {x}")
```

### State Methods

```sn
# Check if more data is available
var hasMore: bool = f.hasChars()
var hasMore: bool = f.hasWords()
var hasMore: bool = f.hasLines()

# Check if at end of file
var eof: bool = f.isEof()

# Get current byte position in file
var pos: int = f.position()

# Seek to byte position
f.seek(pos)

# Return to beginning of file
f.rewind()

# Force buffered data to disk
f.flush()
```

### Properties

```sn
# Full file path
var p: str = f.path

# Filename only (without directory)
var n: str = f.name

# File size in bytes
var s: int = f.size
```

### Lifecycle

```sn
# Explicit close (optional - files auto-close with arena)
f.close()
```

---

## BinaryFile API

### Static Methods

```sn
var f: BinaryFile = BinaryFile.open("data.bin")
var exists: bool = BinaryFile.exists("data.bin")
var data: byte[] = BinaryFile.readAll("data.bin")
BinaryFile.writeAll("out.bin", data)
BinaryFile.delete("old.bin")
BinaryFile.copy("src.bin", "dst.bin")
BinaryFile.move("old.bin", "new.bin")
```

### Reading Methods

```sn
# Read single byte
var b: byte = f.readByte()

# Read N bytes into new array
var bytes: byte[] = f.readBytes(count)

# Read all remaining bytes
var all: byte[] = f.readAll()

# Read into byte buffer, returns number of bytes read
var n: int = f.readInto(buffer)
```

### Writing Methods

```sn
# Write single byte
f.writeByte(b)

# Write byte array
f.writeBytes(data)
```

### State Methods

```sn
var hasMore: bool = f.hasBytes()
var eof: bool = f.isEof()
var pos: int = f.position()
f.seek(pos)
f.rewind()
f.flush()
```

### Properties

```sn
var p: str = f.path
var n: str = f.name
var s: int = f.size
```

### Lifecycle

```sn
f.close()
```

---

## Byte Array Extensions

Additional methods for `byte[]` beyond standard array operations:

```sn
# Convert to string (UTF-8 decoding)
var s: str = bytes.toString()

# Convert to string (Latin-1/ISO-8859-1 decoding)
var s: str = bytes.toStringLatin1()

# Encode to hexadecimal string
var hex: str = bytes.toHex()          # "48656c6c6f"

# Encode to Base64 string
var b64: str = bytes.toBase64()       # "SGVsbG8="
```

### String to Bytes

```sn
# Encode string to UTF-8 bytes
var bytes: byte[] = str.toBytes()
```

### Static Byte Utilities

```sn
# Decode hexadecimal string to bytes
var bytes: byte[] = Bytes.fromHex("48656c6c6f")

# Decode Base64 string to bytes
var bytes: byte[] = Bytes.fromBase64("SGVsbG8=")
```

---

## Standard Streams

Built-in file handles for console I/O:

```sn
# Read line from standard input
var line: str = Stdin.readLine()

# Write to standard output
Stdout.write("Hello")
Stdout.writeLine("Hello")

# Write to standard error
Stderr.write("Error!")
Stderr.writeLine("Error!")
```

### Convenience Functions

Global functions for simple console I/O:

```sn
# Read line from stdin
var input: str = readLine()

# Write to stdout (these may already exist)
print("output")
println("output")

# Write to stderr
printErr("error")
printErrLn("error")
```

---

## Path Utilities

Static methods for path manipulation:

```sn
# Extract directory portion
var dir: str = Path.directory("/home/user/file.txt")   # "/home/user"

# Extract filename
var name: str = Path.filename("/home/user/file.txt")   # "file.txt"

# Extract extension (without dot)
var ext: str = Path.extension("/home/user/file.txt")   # "txt"

# Join path components
var full: str = Path.join("/home", "user", "file.txt") # "/home/user/file.txt"

# Resolve to absolute path
var abs: str = Path.absolute("./file.txt")             # "/current/dir/file.txt"

# Check path existence and type
var exists: bool = Path.exists("/some/path")
var isFile: bool = Path.isFile("/some/path")
var isDir: bool = Path.isDirectory("/some/path")
```

---

## Directory Operations

```sn
# List files in directory
var files: str[] = Directory.list("/home/user")

# List files recursively
var files: str[] = Directory.listRecursive("/home/user")

# Create directory (creates parents if needed)
Directory.create("/new/path")

# Delete directory (must be empty, panics otherwise)
Directory.delete("/old/path")

# Delete directory and all contents
Directory.deleteRecursive("/old/path")
```

---

## Memory Management Rules

File handles follow the same lifecycle and escaping rules as other arena-allocated resources (strings, arrays). This section formalizes the "rules of the road" for file handle memory management.

### Arena Lifecycle

File handles are bound to the arena in which they are opened:

| Context | Arena Behavior | File Behavior |
|---------|----------------|---------------|
| Function entry | New arena created | Files opened here close on function exit |
| `private =>` block | New isolated arena | Files **guaranteed** to close at block end |
| `shared` function | Uses caller's arena | Files persist in caller's scope |
| Loop iteration (non-shared) | Per-iteration arena | File opens/closes each iteration |
| Loop iteration (shared) | Parent arena reused | File persists across iterations |

### Shared vs Private Blocks

**Private Blocks** - Isolation and Guaranteed Cleanup

```sn
var result: str = ""

private =>
    var f: TextFile = TextFile.open("data.txt")
    result = f.readAll()          # String promoted to outer scope
    # f is automatically closed here - GUARANTEED
    # f cannot escape this block (compile error if assigned to outer variable)
```

Private blocks create an isolated arena. File handles opened within:
- Are **always** closed when the block exits
- **Cannot** be assigned to variables in outer scopes (compile-time error)
- Provide deterministic cleanup even on early return or panic

**Shared Blocks** - Arena Reuse

```sn
fn processFile(f: TextFile) shared: str =>
    # Uses caller's arena - no new arena created
    # f remains open (caller owns it)
    return f.readLine()
```

Shared functions/blocks:
- Reuse the caller's arena
- Do not close files passed to them
- Allocate strings/arrays in caller's arena (they escape automatically)

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

**Return Value Promotion:**

```sn
fn openConfig(): TextFile =>
    var f: TextFile = TextFile.open("config.txt")
    return f    # Handle promoted to caller's arena

fn main(): int =>
    var config: TextFile = openConfig()
    # config is valid here - we own it now
    var data: str = config.readAll()
    config.close()
    return 0
```

**Private Block Restriction:**

```sn
var f: TextFile    # Outer variable

private =>
    f = TextFile.open("data.txt")    # COMPILE ERROR: File cannot escape private block
```

This restriction ensures private blocks provide guaranteed cleanup.

### Eager Disposal

While files auto-close with their arena, explicit `close()` is recommended for:

1. **Long-running scopes** - Don't hold files open unnecessarily
2. **Resource contention** - Other processes may need access
3. **Many files in loop** - Close each before opening next

```sn
# Bad: Holds all files open until function exits
fn processAll(paths: str[]): void =>
    for path in paths =>
        var f: TextFile = TextFile.open(path)
        process(f)
        # f stays open until loop ends (shared) or iteration ends (non-shared)

# Good: Explicit early disposal
fn processAll(paths: str[]): void =>
    for path in paths =>
        var f: TextFile = TextFile.open(path)
        process(f)
        f.close()    # Immediate release
```

### Loop Iteration Behavior

**Non-shared loops** (default): Per-iteration arena cleanup

```sn
for path in paths =>
    var f: TextFile = TextFile.open(path)
    process(f)
    # f.close() called automatically at iteration end
```

**Shared loops**: File persists across iterations

```sn
var f: TextFile = TextFile.open("log.txt")
for item in items shared =>
    f.writeLine(item)    # Same file handle, no re-open
f.close()
```

### Arena Integration (Implementation)

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

### Handle Promotion (Implementation)

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

## Error Handling

All file operations that can fail will **panic** with a descriptive error message rather than returning null or an error code.

Examples of panic conditions:
- `TextFile.open()` - File doesn't exist or permission denied
- `TextFile.delete()` - File doesn't exist or permission denied
- `f.readLine()` - I/O error during read
- `f.seek(pos)` - Invalid position

This keeps code clean and avoids pervasive null checks. Future versions may introduce a `Result` type for recoverable error handling.

---

## Implementation Notes

### Static Method Syntax

Implementing `TextFile.open()` requires parser support for static method calls on type names:

```
TypeName.methodName(args...)
```

This is distinct from instance method calls:

```
expression.methodName(args...)
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

---

## Implementation Order

### Phase 1: Foundation

1. **`byte` type** - Add primitive type to lexer, parser, type checker, code gen
2. **Byte arrays** - `byte[]` support with existing array infrastructure
3. **Static method parsing** - `TypeName.method()` syntax in parser

### Phase 2: Core File I/O

4. **`TextFile` type** - Type definition, arena tracking
5. **TextFile static methods** - `open`, `exists`, `readAll`, `writeAll`
6. **TextFile instance methods** - `readLine`, `readAll`, `write`, `writeLine`, `close`
7. **TextFile state methods** - `hasLines`, `hasChars`, `isEof`, `position`, `seek`

### Phase 3: Binary and Streams

8. **`BinaryFile` type** - Parallel to TextFile
9. **BinaryFile methods** - `readByte`, `readBytes`, `writeByte`, `writeBytes`
10. **Standard streams** - `Stdin`, `Stdout`, `Stderr` as global instances
11. **Console functions** - `readLine()`, `print()`, `println()`

### Phase 4: Text Processing

12. **String splitting** - `split`, `splitWhitespace`, `splitLines`
13. **String joining** - Array `.join()` method
14. **Additional string methods** - As needed for file processing

### Phase 5: Utilities

15. **`Path` utilities** - Static path manipulation
16. **`Directory`** - Listing and manipulation
17. **`Bytes` utilities** - `fromHex`, `fromBase64`

### Existing String Methods

The following string methods already exist in Sindarin:

| Method | Status |
|--------|--------|
| `.length` | Exists |
| `.substring(start, end)` | Exists |
| `.indexOf(str)` | Exists |
| `.contains(str)` | Exists |
| `.startsWith(str)` | Exists |
| `.endsWith(str)` | Exists |
| `.trim()` | Exists |
| `.toUpper()` | Exists |
| `.toLower()` | Exists |
| `.replace(old, new)` | Exists |
| `.charAt(index)` | Exists |
| `.split(delim)` | Exists |

### New String Methods Required

| Method | Priority | Status | Purpose |
|--------|----------|--------|---------|
| `.splitWhitespace()` | High | **Implemented** | Word tokenization |
| `.splitLines()` | High | **Implemented** | Line processing |
| `.isBlank()` | High | **Implemented** | Empty/whitespace check |
| `.isNumeric()` | Medium | Not implemented | Validation |
| `.toInt()` | Medium | Not implemented | Parsing |
| `.toDouble()` | Medium | Not implemented | Parsing |
| `.repeat(n)` | Medium | Not implemented | String building |
| `.padStart(n, ch)` | Low | Not implemented | Formatting |
| `.padEnd(n, ch)` | Low | Not implemented | Formatting |

---

## Usage Examples

### Read File Line by Line

```sn
var f: TextFile = TextFile.open("data.txt")
while f.hasLines() {
    var line: str = f.readLine()
    println(line)
}
f.close()
```

### One-Shot File Read

```sn
var content: str = TextFile.readAll("config.txt")
println(content)
```

### Write Output File

```sn
var f: TextFile = TextFile.open("output.txt")
f.writeLine("Header")
for i in 0..10 {
    f.println($"Line {i}")
}
f.close()
```

### Copy Binary File

```sn
var data: byte[] = BinaryFile.readAll("image.png")
BinaryFile.writeAll("copy.png", data)
```

### Process Binary Data

```sn
var f: BinaryFile = BinaryFile.open("data.bin")
var header: byte[] = f.readBytes(4)
if header.toHex() == "89504e47" {
    println("PNG file detected")
}
f.close()
```

### Interactive Input

```sn
Stdout.write("Enter name: ")
var name: str = Stdin.readLine()
println($"Hello, {name}!")
```

---

## Text Processing Functions

Comprehensive string methods for text manipulation, essential for processing file contents.

### Splitting and Joining

```sn
# Split string by delimiter
var parts: str[] = text.split(",")              # Split on comma
var parts: str[] = text.split(", ")             # Split on comma-space
var words: str[] = text.splitWhitespace()       # Split on any whitespace
var lines: str[] = text.splitLines()            # Split on newlines (\n, \r\n, \r)

# Split with limit
var parts: str[] = text.split(",", 3)           # At most 3 parts

# Join array into string
var csv: str = parts.join(",")                  # "a,b,c"
var spaced: str = words.join(" ")               # "hello world"
var lines: str = items.join("\n")               # Multi-line string
```

### Searching and Matching

```sn
# Find substring position (-1 if not found)
var pos: int = text.indexOf("needle")
var pos: int = text.indexOf("needle", startFrom)
var pos: int = text.lastIndexOf("needle")

# Check containment
var has: bool = text.contains("search")
var starts: bool = text.startsWith("prefix")
var ends: bool = text.endsWith("suffix")

# Count occurrences
var n: int = text.count("pattern")
var n: int = text.countLines()
var n: int = text.countWords()
```

### Extraction and Slicing

```sn
# Substring extraction
var sub: str = text.substring(start, end)       # From start to end (exclusive)
var sub: str = text.substring(start)            # From start to end of string
var first: str = text.take(n)                   # First n characters
var last: str = text.takeLast(n)                # Last n characters
var rest: str = text.drop(n)                    # Skip first n characters
var rest: str = text.dropLast(n)                # Skip last n characters

# Character access
var ch: char = text.charAt(index)
var first: char = text.first()
var last: char = text.last()

# Line/word extraction
var line: str = text.lineAt(n)                  # Get nth line (0-indexed)
var word: str = text.wordAt(n)                  # Get nth word
```

### Trimming and Padding

```sn
# Remove whitespace
var trimmed: str = text.trim()                  # Both ends
var trimmed: str = text.trimStart()             # Leading only
var trimmed: str = text.trimEnd()               # Trailing only

# Remove specific characters
var trimmed: str = text.trim(".,!?")            # Remove punctuation from ends
var trimmed: str = text.trimStart("0")          # Remove leading zeros

# Padding
var padded: str = text.padStart(10)             # Pad with spaces to length 10
var padded: str = text.padStart(10, '0')        # Pad with zeros
var padded: str = text.padEnd(10)
var padded: str = text.padEnd(10, '.')
var centered: str = text.center(20)             # Center in 20-char field
```

### Case Conversion

```sn
var upper: str = text.toUpper()                 # "HELLO WORLD"
var lower: str = text.toLower()                 # "hello world"
var title: str = text.toTitle()                 # "Hello World"
var camel: str = text.toCamelCase()             # "helloWorld"
var snake: str = text.toSnakeCase()             # "hello_world"
var kebab: str = text.toKebabCase()             # "hello-world"
var pascal: str = text.toPascalCase()           # "HelloWorld"
```

### Replacement

```sn
# Replace occurrences
var new: str = text.replace("old", "new")       # Replace first
var new: str = text.replaceAll("old", "new")    # Replace all

# Character replacement
var new: str = text.replaceChar('a', 'b')       # Replace all 'a' with 'b'

# Remove substrings
var new: str = text.remove("pattern")           # Remove first occurrence
var new: str = text.removeAll("pattern")        # Remove all occurrences
```

### Repetition and Concatenation

```sn
var repeated: str = text.repeat(3)              # "abcabcabc"
var sep: str = "-".repeat(40)                   # "----------------------------------------"

# Concatenation (+ operator works, but also:)
var combined: str = strings.concat(a, b, c)     # Efficient multi-concat
```

### Comparison

```sn
var same: bool = a.equals(b)                    # Case-sensitive equality
var same: bool = a.equalsIgnoreCase(b)          # Case-insensitive
var cmp: int = a.compareTo(b)                   # -1, 0, or 1

# Numeric comparison for version strings etc.
var cmp: int = a.compareNatural(b)              # "file2" < "file10"
```

### Validation and Testing

```sn
var empty: bool = text.isEmpty()                # Length == 0
var blank: bool = text.isBlank()                # Empty or only whitespace
var alpha: bool = text.isAlpha()                # Only letters
var numeric: bool = text.isNumeric()            # Only digits
var alphaNum: bool = text.isAlphanumeric()      # Letters and digits
var upper: bool = text.isUpper()                # All uppercase
var lower: bool = text.isLower()                # All lowercase
var whitespace: bool = text.isWhitespace()      # Only whitespace
```

### Parsing

```sn
# Parse to numbers (panics on invalid input)
var n: int = text.toInt()
var n: double = text.toDouble()

# Parse with validation
var valid: bool = text.isInt()
var valid: bool = text.isDouble()

# Parse boolean
var b: bool = text.toBool()                     # "true"/"false", "1"/"0", "yes"/"no"
```

### Character Iteration

```sn
# Get characters as array
var chars: []char = text.toChars()

# Iterate characters
for ch in text.chars() =>
    println(ch)

# Iterate with index
for i, ch in text.charsIndexed() =>
    println($"{i}: {ch}")
```

### Encoding and Escaping

```sn
# Escape for various contexts
var escaped: str = text.escapeHtml()            # &lt;div&gt;
var escaped: str = text.escapeJson()            # \"quoted\"
var escaped: str = text.escapeUrl()             # hello%20world
var escaped: str = text.escapeCsv()             # "field,with,commas"

# Unescape
var raw: str = text.unescapeHtml()
var raw: str = text.unescapeJson()
var raw: str = text.unescapeUrl()
```

### Formatting

```sn
# Number formatting in strings
var formatted: str = n.format()                 # "1,234,567"
var formatted: str = n.format(2)                # "1234.57" (2 decimal places)

# String interpolation (existing feature)
var msg: str = $"Hello, {name}!"
var msg: str = $"Value: {n:,.2f}"               # With format specifier (future)
```

### Regular Expressions (Future)

```sn
# Basic regex support (future consideration)
var matches: bool = text.matches("^[a-z]+$")
var found: str[] = text.findAll("[0-9]+")
var replaced: str = text.replaceRegex("[0-9]+", "X")
```

---

## Complete Text Processing Example

```sn
# Parse a CSV file
fn parseCsv(path: str): str[][] =>
    var content: str = TextFile.readAll(path)
    var lines: str[] = content.splitLines()
    var result: str[][] = str[][]{}

    for line in lines =>
        if line.isBlank() =>
            continue
        var fields: str[] = line.split(",")
        # Trim whitespace from each field
        var trimmed: str[] = str[]{}
        for field in fields =>
            trimmed.push(field.trim())
        result.push(trimmed)

    return result

# Find and replace in file
fn findReplace(path: str, find: str, replace: str): void =>
    var content: str = TextFile.readAll(path)
    var updated: str = content.replaceAll(find, replace)
    TextFile.writeAll(path, updated)

# Word frequency counter
fn wordFrequency(path: str): void =>
    var content: str = TextFile.readAll(path)
    var words: str[] = content.toLower().splitWhitespace()

    # Would need a Map type for proper implementation
    # For now, just count total
    println($"Total words: {words.length}")

# Log file analyzer
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
