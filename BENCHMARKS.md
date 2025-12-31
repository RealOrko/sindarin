# Benchmark Results

This document contains performance comparison results between Sindarin and other programming languages.

## Test Environment

- **Date**: 2025-12-31 06:35:43
- **Runs per benchmark**: 3 (median reported)

## Summary

All benchmarks were run 3 times per language, with the median time reported.

## Benchmark Results by Category

### 1. Fibonacci (CPU-bound, recursive fib(35))

Tests function call overhead with recursive algorithm.

| Language | Time (ms) |
|----------|-----------|
| sindarin | 25 |
| c | 26 |
| go | 49 |
| rust | 29 |
| java | 42 |
| csharp | 54 |
| python | 1607 |
| nodejs | 573 |

### 2. Prime Sieve (Memory + CPU, sieve up to 1,000,000)

Tests memory allocation and iteration performance.

| Language | Time (ms) |
|----------|-----------|
| sindarin | 14 |
| c | 2 |
| go | 2 |
| rust | 2 |
| java | 14 |
| csharp | 6 |
| python | 162 |
| nodejs | 24 |

### 3. String Operations (100,000 concatenations)

Tests string manipulation and substring search.

| Language | Time (ms) |
|----------|-----------|
| sindarin | 1150 |
| c | 1 |
| go | 1 |
| rust | 3 |
| java | 15 |
| csharp | 32 |
| python | 25 |
| nodejs | 6 |

### 4. Array Operations (1,000,000 integers)

Tests array creation, iteration, and in-place reversal.

| Language | Time (ms) |
|----------|-----------|
| sindarin | 13 |
| c | 2 |
| go | 4 |
| rust | 2 |
| java | 16 |
| csharp | 11 |
| python | 38 |
| nodejs | 77 |

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
