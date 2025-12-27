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

### State Methods

#### isEof()

Returns true if at end of file.

```sindarin
while !f.isEof() =>
  var line: str = f.readLine()
  process(line)
```

#### hasChars()

Returns true if more characters are available.

```sindarin
while f.hasChars() =>
  var ch: int = f.readChar()
```

#### close()

Closes the file handle. Always close files when done.

```sindarin
var f: TextFile = TextFile.open("data.txt")
// ... operations ...
f.close()
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

#### flush()

Forces buffered data to be written to disk.

```sindarin
f.writeByte(42)
f.flush()  // Ensure it's written
```

#### close()

Closes the file handle.

---

## Byte Array Methods

Byte arrays have special methods for encoding and conversion.

### toString()

Converts bytes to a string (UTF-8 decoding).

```sindarin
var bytes: byte[] = {72, 101, 108, 108, 111}
var text: str = bytes.toString()  // "Hello"
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

### Explicit Closing

For long-running operations, close files explicitly:

```sindarin
for path in filePaths =>
  var f: TextFile = TextFile.open(path)
  process(f)
  f.close()  // Don't wait for arena cleanup
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

---

## See Also

- [README.md](../README.md) - Language overview with file I/O examples
- [MEMORY.md](MEMORY.md) - Arena memory management details
- [ARRAYS.md](ARRAYS.md) - Array operations including byte arrays
- [samples/lib/fileio.sn](../samples/lib/fileio.sn) - File I/O demo code
- [samples/lib/bytes.sn](../samples/lib/bytes.sn) - Byte type demo code
- [IO.md](../IO.md) - Design document with implementation details
