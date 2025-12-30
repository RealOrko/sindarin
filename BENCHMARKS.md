# Benchmark Results

This document contains performance comparison results between Sindarin and other programming languages.

## Test Environment

- **Date**: 2025-12-30 10:06:10
- **Runs per benchmark**: 3 (median reported)

## Summary

All benchmarks were run 3 times per language, with the median time reported.

## Benchmark Results by Category

### 1. Fibonacci (CPU-bound, recursive fib(35))

Tests function call overhead with recursive algorithm.

| Language | Time (ms) |
|----------|-----------|
| sindarin | 0 |
| c | 23 |
| go | 53 |
| rust | 35 |
| java | 50 |
| csharp | 61 |
| python | 2021 |
| nodejs | 828 |

### 2. Prime Sieve (Memory + CPU, sieve up to 1,000,000)

Tests memory allocation and iteration performance.

| Language | Time (ms) |
|----------|-----------|
| sindarin | 0 |
| c | 3 |
| go | 3 |
| rust | 2 |
| java | 16 |
| csharp | 5 |
| python | 161 |
| nodejs | 44 |

### 3. String Operations (100,000 concatenations)

Tests string manipulation and substring search.

| Language | Time (ms) |
|----------|-----------|
| sindarin | 0 |
| c | 1 |
| go | 2 |
| rust | 3 |
| java | 15 |
| csharp | 20 |
| python | 13 |
| nodejs | 10 |

### 4. Array Operations (1,000,000 integers)

Tests array creation, iteration, and in-place reversal.

| Language | Time (ms) |
|----------|-----------|
| sindarin | 0 |
| c | 2 |
| go | 5 |
| rust | 2 |
| java | 25 |
| csharp | 6 |
| python | 37 |
| nodejs | 155 |

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
