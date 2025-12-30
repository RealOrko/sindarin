# Benchmark Results

This document contains performance comparison results between Sindarin and other programming languages.

## Test Environment

- **Date**: 2025-12-30 20:54:53
- **Runs per benchmark**: 3 (median reported)

## Summary

All benchmarks were run 3 times per language, with the median time reported.

## Benchmark Results by Category

### 1. Fibonacci (CPU-bound, recursive fib(35))

Tests function call overhead with recursive algorithm.

| Language | Time (ms) |
|----------|-----------|
| sindarin | 173 |
| c | 27 |
| go | 51 |
| rust | 33 |
| java | 43 |
| csharp | 57 |
| python | 1481 |
| nodejs | 557 |

### 2. Prime Sieve (Memory + CPU, sieve up to 1,000,000)

Tests memory allocation and iteration performance.

| Language | Time (ms) |
|----------|-----------|
| sindarin | 7157 |
| c | 3 |
| go | 2 |
| rust | 2 |
| java | 12 |
| csharp | 4 |
| python | 139 |
| nodejs | 23 |

### 3. String Operations (100,000 concatenations)

Tests string manipulation and substring search.

| Language | Time (ms) |
|----------|-----------|
| sindarin | 1103 |
| c | 2 |
| go | 2 |
| rust | 6 |
| java | 15 |
| csharp | 33 |
| python | 21 |
| nodejs | 7 |

### 4. Array Operations (1,000,000 integers)

Tests array creation, iteration, and in-place reversal.

| Language | Time (ms) |
|----------|-----------|
| sindarin | 5185 |
| c | 4 |
| go | 7 |
| rust | 3 |
| java | 17 |
| csharp | 15 |
| python | 38 |
| nodejs | 76 |

## Notes

- **Sindarin** compiles to C via GCC with `-O2` optimization
- **C** compiled with GCC `-O2`
- **Rust** compiled with `rustc -O` (release mode)
- **Go** uses default `go build` settings
- **Java** uses `javac` with default settings
- **C#** compiled with `dotnet -c Release`
- **Python** uses CPython 3.x interpreter
- **Node.js** uses V8 JavaScript engine

## Validation

All benchmark implementations produce the expected output values:
- Fibonacci(35) = 9,227,465
- Fibonacci(50) = 12,586,269,025
- Primes up to 1,000,000 = 78,498
- String length = 500,000; Occurrences = 100,000
- Array sum = 499,999,500,000
