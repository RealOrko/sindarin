# File I/O Variance Testing Gap Analysis

This document identifies missing variance testing scenarios for file I/O operations based on IO.md specifications and current test coverage.

## Current Coverage Summary

### Integration Tests (compiler/tests/integration/)
- `test_textfile_static.sn` - Static methods: writeAll, exists, readAll, copy, move, delete
- `test_textfile_reading.sn` - Reading: readLine, readAll, readLines, readWord, readChar
- `test_textfile_writing.sn` - Writing: write, writeLine, writeChar, print, println
- `test_textfile_state.sn` - State: path, name, size, position, seek, rewind, isEof, hasChars, hasLines, hasWords, flush
- `test_binary_file.sn` - BinaryFile: all static and instance methods
- `test_byte_array.sn` - byte type and array operations
- `test_byte_encoding.sn` - toHex, toString, toBase64, toStringLatin1
- `test_string_bytes.sn` - toBytes, Bytes.fromHex, Bytes.fromBase64
- `test_path.sn` - Path utilities
- `test_directory.sn` - Directory operations
- `test_stdout_stderr.sn` - Standard streams

### Exploratory Tests (testing/)
**NO file I/O tests exist in the testing/ directory** - All file I/O testing is only in integration tests.

---

## GAP 1: Missing TextFile Edge Cases / Variance Testing

### 1.1 Empty File Handling
- [ ] `TextFile.readAll()` on empty file - should return ""
- [ ] `TextFile.readLines()` on empty file - should return empty array
- [ ] `f.readLine()` on empty file - should return ""
- [ ] `f.readWord()` on empty file - should return ""
- [ ] `f.readChar()` on empty file - should return -1 (EOF)
- [ ] `f.hasLines()` on empty file - should return false
- [ ] `f.hasChars()` on empty file - should return false
- [ ] `f.hasWords()` on empty file - should return false

### 1.2 Single Character / Line Files
- [ ] File with single character (no newline)
- [ ] File with single character followed by newline
- [ ] File with single newline only
- [ ] File with multiple consecutive newlines

### 1.3 Large File Handling
- [ ] Reading files larger than buffer size (memory stress test)
- [ ] Many lines (1000+ lines)
- [ ] Very long lines (>4KB single line)

### 1.4 Special Content
- [ ] File containing only whitespace
- [ ] File with leading/trailing whitespace
- [ ] File with null bytes (text file handling)
- [ ] File with unicode/UTF-8 characters
- [ ] File with mixed line endings (\n, \r\n, \r)

### 1.5 Error Conditions (Panic Cases)
- [ ] `TextFile.open()` on non-existent file
- [ ] `TextFile.open()` on directory
- [ ] `TextFile.readAll()` on non-existent file
- [ ] `TextFile.delete()` on non-existent file
- [ ] `TextFile.delete()` on directory
- [ ] `TextFile.copy()` with non-existent source
- [ ] `TextFile.move()` with non-existent source
- [ ] `f.seek()` with negative position
- [ ] `f.seek()` beyond file size (allowed but next read returns EOF)
- [ ] Operations after `f.close()`

### 1.6 Path Edge Cases
- [ ] Relative paths (./file.txt)
- [ ] Absolute paths (/tmp/file.txt)
- [ ] Paths with spaces ("/tmp/my file.txt")
- [ ] Paths with special characters
- [ ] Very long path names
- [ ] `f.name` when path includes directories (should extract just filename)

### 1.7 Read-After-Write
- [ ] Write data, seek to beginning, read it back
- [ ] Append to file (multiple writes)
- [ ] Write, close, reopen, read

### 1.8 readInto() Method
**NOT TESTED AT ALL**
- [ ] Basic `readInto(buffer)` usage
- [ ] readInto with buffer larger than remaining content
- [ ] readInto with empty buffer
- [ ] Multiple readInto calls

---

## GAP 2: Missing BinaryFile Edge Cases / Variance Testing

### 2.1 Empty File Handling
- [ ] `BinaryFile.readAll()` on empty file
- [ ] `f.readByte()` on empty file
- [ ] `f.readBytes(n)` on empty file
- [ ] `f.readBytes(n)` when n > remaining bytes

### 2.2 Boundary Values
- [ ] byte value 0 (null byte)
- [ ] byte value 255 (max value)
- [ ] All byte values 0-255 round-trip

### 2.3 Large Binary Files
- [ ] Binary files >64KB
- [ ] Reading large chunks

### 2.4 readInto() Method
**NOT TESTED AT ALL**
- [ ] Basic `readInto(buffer)` usage
- [ ] Partial buffer fills

### 2.5 Error Conditions
- [ ] `BinaryFile.open()` on non-existent file
- [ ] `BinaryFile.delete()` on non-existent file
- [ ] Operations after close

---

## GAP 3: Missing byte Type Variance Testing

### 3.1 byte Literal Forms
- [ ] `byte = 0xFF` (hex literal)
- [ ] `byte = 'A'` (char literal conversion)
- [ ] byte from integer expression

### 3.2 byte Arithmetic
- [ ] byte + byte (overflow behavior)
- [ ] byte comparisons
- [ ] byte to int conversion
- [ ] int to byte conversion (truncation)

### 3.3 byte Array Edge Cases
- [ ] Empty byte array operations
- [ ] Single byte array
- [ ] Byte array slicing
- [ ] Negative index access
- [ ] Out-of-bounds access (panic)

---

## GAP 4: Missing Encoding Utilities Variance Testing

### 4.1 toHex() Edge Cases
- [ ] Lowercase output verification (should be "48656c6c6f" not "48656C6C6F")
- [ ] Verify each byte maps to exactly 2 hex chars

### 4.2 Bytes.fromHex() Edge Cases
- [ ] Odd-length hex string (error or truncate?)
- [ ] Invalid hex characters
- [ ] Mixed case input handling
- [ ] Whitespace in hex string

### 4.3 toBase64() Edge Cases
- [ ] Verify padding correctness
- [ ] URL-safe base64 (not implemented?)

### 4.4 Bytes.fromBase64() Edge Cases
- [ ] Invalid base64 characters
- [ ] Missing/incorrect padding
- [ ] Whitespace in base64 string
- [ ] URL-safe base64 decoding

### 4.5 toString() Edge Cases
- [ ] Invalid UTF-8 sequences
- [ ] Partial UTF-8 sequences
- [ ] Embedded null bytes

---

## GAP 5: Missing Path Utilities Variance Testing

### 5.1 Path.directory()
- [ ] Empty string input
- [ ] Just filename (no directory)
- [ ] Root path only ("/" -> "")
- [ ] Windows-style paths (if supported)

### 5.2 Path.filename()
- [ ] Empty string
- [ ] Path ending in /
- [ ] Just "/" (root)

### 5.3 Path.extension()
- [ ] No extension
- [ ] Multiple dots (file.tar.gz)
- [ ] Hidden files (.bashrc)
- [ ] Trailing dot (file.)

### 5.4 Path.join()
- [ ] Empty components
- [ ] Multiple consecutive slashes
- [ ] Trailing slashes
- [ ] More than 3 components

### 5.5 Path.absolute()
**NOT TESTED AT ALL**
- [ ] Basic relative to absolute conversion
- [ ] Already absolute path
- [ ] Path with ".." components
- [ ] Path with "." components

---

## GAP 6: Missing Directory Operations Variance Testing

### 6.1 Directory.create()
- [ ] Already exists (should not error)
- [ ] Parent doesn't exist (should create parents)
- [ ] Permission denied (panic)

### 6.2 Directory.delete()
- [ ] Non-empty directory (should panic)
- [ ] Non-existent directory (panic)
- [ ] Not a directory (panic)

### 6.3 Directory.list() and listRecursive()
- [ ] Empty directory
- [ ] Hidden files (dotfiles)
- [ ] Symbolic links
- [ ] Permission denied on subdirectory

---

## GAP 7: Missing Standard Streams Testing

### 7.1 Stdin
**NOT TESTED** (interactive input)
- [ ] Stdin.readLine() basic
- [ ] readLine() global function

### 7.2 printErr/printErrLn
- [ ] Verify output goes to stderr not stdout

---

## GAP 8: Missing String Methods for Text Processing

Per IO.md, these new string methods are required but may not be fully tested:

### High Priority (required for file I/O usage)
- [ ] `.splitWhitespace()` - TESTED in test_string_split.sn
- [ ] `.splitLines()` - TESTED in test_string_split.sn
- [ ] `.isBlank()` - TESTED in test_string_split.sn

### Medium Priority (useful for parsing)
- [ ] `.isNumeric()` - NOT TESTED
- [ ] `.toInt()` - NOT TESTED
- [ ] `.toDouble()` - NOT TESTED
- [ ] `.repeat(n)` - NOT TESTED
- [ ] `.countLines()` - NOT TESTED
- [ ] `.countWords()` - NOT TESTED

### Lower Priority (formatting)
- [ ] `.padStart(n, ch)` - NOT TESTED
- [ ] `.padEnd(n, ch)` - NOT TESTED
- [ ] `.trimStart()` - NOT TESTED
- [ ] `.trimEnd()` - NOT TESTED

---

## GAP 9: Memory Management / Arena Integration

### 9.1 Private Block File Cleanup
- [ ] File opened in `private =>` block auto-closes on exit
- [ ] Verify file handle cannot escape private block

### 9.2 Shared Function Behavior
- [ ] File passed to shared function stays open
- [ ] String/array returned from shared function uses caller arena

### 9.3 Loop Behavior
- [ ] Non-shared loop: file closes each iteration
- [ ] Shared loop: file persists across iterations

### 9.4 Return Value Promotion
- [ ] Function returning TextFile - handle promoted to caller arena
- [ ] Function returning BinaryFile - handle promoted to caller arena

---

## Test Scenarios to Add to test-explore

The following test files should be added to `testing/` for exploratory variance testing:

1. `test_file_io_empty.sn` - Empty file handling for both TextFile and BinaryFile
2. `test_file_io_edge_content.sn` - Special content (whitespace, unicode, line endings)
3. `test_file_io_large.sn` - Large file stress tests
4. `test_file_io_errors.sn` - Error conditions and panics
5. `test_file_io_paths.sn` - Path edge cases (spaces, special chars, relative/absolute)
6. `test_file_io_readinto.sn` - readInto method testing
7. `test_byte_boundaries.sn` - Byte type boundary values and arithmetic
8. `test_encoding_edge.sn` - Encoding edge cases (invalid input)
9. `test_arena_files.sn` - Memory management with file handles
10. `test_string_parsing.sn` - New string methods for parsing

---

## Priority Ranking

**Critical (Required for robust file I/O)**
1. Empty file handling
2. Error conditions (panics)
3. readInto method (completely missing)
4. Path.absolute() (completely missing)

**High (Common edge cases)**
1. Single character/line files
2. Unicode content
3. Mixed line endings
4. Encoding edge cases

**Medium (Stress and robustness)**
1. Large files
2. Long paths
3. Arena integration tests

**Low (Nice to have)**
1. String parsing methods
2. Byte arithmetic
