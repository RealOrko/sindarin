# Callback-Based Streaming in Sindarin (DRAFT)

**Status:** Draft
**Created:** 2026-01-12
**Prerequisites:** None (works with current language features)
**Future:** May be promoted to SDK once compression module is implemented

---

## Overview

Sindarin supports callback-based streaming using existing language features:
- First-class functions and closures
- Type aliases for function signatures
- Chunked file I/O methods

This pattern enables memory-efficient processing of large data without loading everything into memory.

---

## Core Types

```sindarin
# A function that produces chunks of data
# Returns empty array to signal end of stream
type ChunkReader = fn(): byte[]

# A function that consumes chunks of data
type ChunkWriter = fn(chunk: byte[]): void

# A function that transforms chunks
type ChunkTransform = fn(chunk: byte[]): byte[]

# A predicate for filtering
type ChunkFilter = fn(chunk: byte[]): bool
```

---

## Basic Patterns

### Stream Processing

```sindarin
# Process a stream from reader to writer
fn processStream(reader: ChunkReader, writer: ChunkWriter): void =>
    while true =>
        var chunk: byte[] = reader()
        if chunk.length == 0 =>
            break
        writer(chunk)
```

### Stream Transformation

```sindarin
# Process with transformation
fn transformStream(reader: ChunkReader, transform: ChunkTransform, writer: ChunkWriter): void =>
    while true =>
        var chunk: byte[] = reader()
        if chunk.length == 0 =>
            break
        var transformed: byte[] = transform(chunk)
        writer(transformed)
```

### Stream with Accumulator

```sindarin
# Process and accumulate a result
fn reduceStream(reader: ChunkReader, initial: int, reducer: fn(int, byte[]): int): int =>
    var acc: int = initial
    while true =>
        var chunk: byte[] = reader()
        if chunk.length == 0 =>
            break
        acc = reducer(acc, chunk)
    return acc
```

---

## File I/O Examples

### Copy File (Streaming)

```sindarin
fn copyFile(src: str, dest: str): bool =>
    var input: BinaryFile = BinaryFile.open(src)
    var output: BinaryFile = BinaryFile.open(dest)

    processStream(
        fn(): byte[] =>
            if input.hasBytes() =>
                return input.readBytes(4096)
            return {},
        fn(chunk: byte[]): void =>
            output.writeBytes(chunk)
    )

    input.close()
    output.close()
    return true
```

### Calculate File Hash/Checksum

```sindarin
fn calculateChecksum(path: str): int =>
    var file: BinaryFile = BinaryFile.open(path)

    var checksum: int = reduceStream(
        fn(): byte[] =>
            if file.hasBytes() =>
                return file.readBytes(8192)
            return {},
        0,
        fn(acc: int, chunk: byte[]): int =>
            # Simple checksum - sum all bytes
            var sum: int = acc
            for b in chunk =>
                sum = sum + (b as int)
            return sum
    )

    file.close()
    return checksum
```

### Count Bytes Matching Criteria

```sindarin
fn countNonZeroBytes(path: str): int =>
    var file: BinaryFile = BinaryFile.open(path)

    var count: int = reduceStream(
        fn(): byte[] =>
            if file.hasBytes() =>
                return file.readBytes(4096)
            return {},
        0,
        fn(acc: int, chunk: byte[]): int =>
            var c: int = 0
            for b in chunk =>
                if b != 0 =>
                    c = c + 1
            return acc + c
    )

    file.close()
    return count
```

---

## Text Streaming Examples

### Line-by-Line Processing

```sindarin
type LineReader = fn(): str
type LineWriter = fn(line: str): void

fn processLines(reader: LineReader, writer: LineWriter): void =>
    while true =>
        var line: str = reader()
        if line.length == 0 =>
            break
        writer(line)

fn filterLines(path: str, predicate: fn(str): bool): str[] =>
    var file: TextFile = TextFile.open(path)
    var result: str[] = {}

    processLines(
        fn(): str =>
            if file.hasLines() =>
                return file.readLine()
            return "",
        fn(line: str): void =>
            if predicate(line) =>
                result.push(line)
    )

    file.close()
    return result

# Usage: find all lines containing "error"
var errors: str[] = filterLines("app.log", fn(line: str): bool =>
    return line.contains("error")
)
```

### Transform Lines

```sindarin
fn transformFile(src: str, dest: str, transform: fn(str): str): void =>
    var input: TextFile = TextFile.open(src)
    var output: TextFile = TextFile.open(dest)

    processLines(
        fn(): str =>
            if input.hasLines() =>
                return input.readLine()
            return "",
        fn(line: str): void =>
            output.writeLine(transform(line))
    )

    input.close()
    output.close()

# Usage: uppercase all lines
transformFile("input.txt", "output.txt", fn(line: str): str =>
    return line.toUpper()
)
```

---

## Progress Reporting

### With Byte Counter

```sindarin
fn copyWithProgress(src: str, dest: str, onProgress: fn(int): void): void =>
    var input: BinaryFile = BinaryFile.open(src)
    var output: BinaryFile = BinaryFile.open(dest)
    var total: int = 0

    processStream(
        fn(): byte[] =>
            if input.hasBytes() =>
                return input.readBytes(4096)
            return {},
        fn(chunk: byte[]): void =>
            output.writeBytes(chunk)
            total = total + chunk.length
            onProgress(total)
    )

    input.close()
    output.close()

# Usage
copyWithProgress("large.bin", "copy.bin", fn(bytes: int): void =>
    print($"\rCopied {bytes} bytes...")
)
print("\nDone!\n")
```

### With Percentage

```sindarin
fn copyWithPercentage(src: str, dest: str): void =>
    var size: int = BinaryFile.size(src)
    var copied: int = 0
    var lastPercent: int = -1

    var input: BinaryFile = BinaryFile.open(src)
    var output: BinaryFile = BinaryFile.open(dest)

    processStream(
        fn(): byte[] =>
            if input.hasBytes() =>
                return input.readBytes(16384)
            return {},
        fn(chunk: byte[]): void =>
            output.writeBytes(chunk)
            copied = copied + chunk.length
            var percent: int = (copied * 100) / size
            if percent != lastPercent =>
                lastPercent = percent
                print($"\rProgress: {percent}%")
    )

    input.close()
    output.close()
    print("\nComplete!\n")
```

---

## Chaining Transforms

### Pipeline Pattern

```sindarin
# Chain multiple transforms together
fn chainTransforms(transforms: (ChunkTransform)[]): ChunkTransform =>
    return fn(chunk: byte[]): byte[] =>
        var result: byte[] = chunk
        for t in transforms =>
            result = t(result)
        return result

# Usage
var pipeline: ChunkTransform = chainTransforms({
    fn(chunk: byte[]): byte[] => encryptChunk(chunk),
    fn(chunk: byte[]): byte[] => compressChunk(chunk)
})

transformStream(reader, pipeline, writer)
```

### Tee (Write to Multiple Destinations)

```sindarin
fn teeWriter(writers: (ChunkWriter)[]): ChunkWriter =>
    return fn(chunk: byte[]): void =>
        for w in writers =>
            w(chunk)

# Usage: write to file and calculate checksum simultaneously
var checksum: int = 0
var output: BinaryFile = BinaryFile.open("output.bin")

var tee: ChunkWriter = teeWriter({
    fn(chunk: byte[]): void =>
        output.writeBytes(chunk),
    fn(chunk: byte[]): void =>
        for b in chunk =>
            checksum = checksum + (b as int)
})

processStream(reader, tee)
output.close()
print($"Checksum: {checksum}\n")
```

---

## Memory Considerations

Streaming patterns must align with Sindarin's arena-based memory model. Understanding arena lifetime, escape behavior, and the `shared`/`private` modifiers is essential for writing efficient and correct streaming code.

**Key principle:** Every block creates an arena. Data escaping to outer scopes is promoted (copied). Long-running operations must manage memory explicitly.

### Arena-Friendly Patterns

Chunks are allocated in the current arena. For long-running streams, use `private` blocks to limit memory growth:

```sindarin
fn processLargeFile(path: str): void =>
    var file: BinaryFile = BinaryFile.open(path)
    var totalProcessed: int = 0

    while file.hasBytes() =>
        private =>
            # Each iteration's chunks are freed after the block
            var chunk: byte[] = file.readBytes(65536)
            processChunk(chunk)
            totalProcessed = totalProcessed + chunk.length

    file.close()
    print($"Processed {totalProcessed} bytes\n")
```

### Avoiding Memory Accumulation

```sindarin
# BAD: Accumulates all chunks in memory
fn badPattern(path: str): byte[] =>
    var file: BinaryFile = BinaryFile.open(path)
    var all: byte[] = {}
    while file.hasBytes() =>
        var chunk: byte[] = file.readBytes(4096)
        all = all.concat(chunk)  # Memory grows unbounded!
    file.close()
    return all

# GOOD: Process chunks without accumulation
fn goodPattern(path: str, handler: ChunkWriter): void =>
    var file: BinaryFile = BinaryFile.open(path)
    while file.hasBytes() =>
        private =>
            var chunk: byte[] = file.readBytes(4096)
            handler(chunk)
            # chunk is freed here
    file.close()
```

### When to Use `shared` vs `private` vs Default

| Scenario | Recommendation | Why |
|----------|---------------|-----|
| Building result collection | `shared` loop | Avoid promotion overhead |
| Processing then discarding | Default loop | Auto-cleanup per iteration |
| Large temporary data | `private` block | Guaranteed cleanup, bounded memory |
| Performance-critical accumulation | `shared` block | Minimize arena overhead |
| Unknown iteration count | Default or `private` | Prevent unbounded growth |

### Escape Behavior in Callbacks

When using closures that capture outer variables, be aware of escape semantics:

```sindarin
fn processWithResult(path: str): int =>
    var total: int = 0  # Primitive - no arena concerns

    processStream(
        fn(): byte[] => readNextChunk(),
        fn(chunk: byte[]): void =>
            # chunk is in the callback's arena
            total = total + chunk.length  # Primitive escapes safely
            # Don't try to store chunk in outer scope!
    )

    return total
```

```sindarin
fn collectChunks(path: str): byte[][] =>
    var chunks: byte[][] = {}  # Outer scope - function arena

    processStream(
        fn(): byte[] => readNextChunk(),
        fn(chunk: byte[]): void =>
            # chunk must be promoted to outer arena
            chunks.push(chunk)  # Works - chunk promoted automatically
    )

    return chunks  # All chunks promoted to caller's arena
```

### Arena Hierarchy in Streaming

```
Caller's Arena
  └── processStream() Arena
        ├── reader/writer closures (capture outer references)
        └── Per-chunk Arena (if default loop)
              └── chunk allocation (freed after handler returns)
```

With `shared`:
```
Caller's Arena
  └── processStream() Arena (shared with caller)
        ├── reader/writer closures
        └── chunks (all in caller's arena - no per-chunk arena)
```

---

## Future: Compression Integration

Once the compression module is implemented, streaming will integrate naturally:

```sindarin
# Future API (requires structs for full streaming)
import "sdk/compression/gzip" as gzip

fn compressFile(src: str, dest: str): void =>
    var input: BinaryFile = BinaryFile.open(src)
    var output: BinaryFile = BinaryFile.open(dest)

    gzip.compressStream(
        fn(): byte[] =>
            if input.hasBytes() =>
                return input.readBytes(16384)
            return {},
        fn(chunk: byte[]): void =>
            output.writeBytes(chunk)
    )

    input.close()
    output.close()
```

### Simple Compression (Works Today)

For one-shot compression of data that fits in memory:

```sindarin
# This pattern works today with existing interop
import "sdk/compression/gzip" as gzip

var data: byte[] = BinaryFile.readAll("input.bin")
var compressed: byte[] = gzip.compress(data)
BinaryFile.writeAll("output.gz", compressed)
```

---

## Error Handling Patterns

### With Result Tracking

```sindarin
fn processStreamSafe(reader: ChunkReader, writer: ChunkWriter): bool =>
    var success: bool = true
    var chunksProcessed: int = 0

    while true =>
        var chunk: byte[] = reader()
        if chunk.length == 0 =>
            break

        # Could add error checking here
        writer(chunk)
        chunksProcessed = chunksProcessed + 1

    return success
```

### With Error Callback

```sindarin
type ErrorHandler = fn(msg: str): void

fn processStreamWithErrors(
    reader: ChunkReader,
    writer: ChunkWriter,
    onError: ErrorHandler
): int =>
    var processed: int = 0

    while true =>
        var chunk: byte[] = reader()
        if chunk.length == 0 =>
            break

        # Process and track
        writer(chunk)
        processed = processed + chunk.length

    return processed
```

---

## Summary

| Pattern | Use Case |
|---------|----------|
| `processStream` | Simple read-write pipeline |
| `transformStream` | Apply transformation to each chunk |
| `reduceStream` | Accumulate result across chunks |
| `chainTransforms` | Compose multiple transformations |
| `teeWriter` | Write to multiple destinations |
| `private` blocks | Memory-efficient long streams |

The callback-based streaming pattern provides:
- Memory efficiency for large files
- Composable operations
- Progress reporting capability
- Integration with arena memory model

No language changes required - this works with existing Sindarin features.

---

## See Also

### Critical References
- [MEMORY.md](../language/MEMORY.md) - **Essential:** Arena memory model, `shared`, `private`, escape behavior
- [LAMBDAS.md](../language/LAMBDAS.md) - Lambda and closure documentation, capture semantics

### Related Documentation
- [FILE_IO.md](../language/FILE_IO.md) - File I/O methods (`readBytes`, `hasBytes`, etc.)
- [ARRAYS.md](../language/ARRAYS.md) - Array operations used in streaming

### Related Drafts
- [STRUCTS.md](STRUCTS.md) - Struct support (needed for full compression streaming with zlib)
