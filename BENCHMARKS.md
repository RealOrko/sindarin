# Benchmark Results

Performance comparison between Sindarin and other programming languages.

## Executive Summary

Sindarin delivers **compiled-language performance** while offering high-level language ergonomics. By compiling to optimized C code, Sindarin consistently ranks among the fastest languages tested.

### Key Highlights

| Metric | Result |
|--------|--------|
| **Fastest benchmark** | String Operations (1ms) - #1 overall |
| **vs Python** | 6-59x faster |
| **vs Node.js** | 4-22x faster |
| **vs Java** | 1.6-13x faster |
| **vs C (baseline)** | 0.3-1.0x (competitive) |

### Performance Tier

```
S-Tier  ████  C, Rust
A-Tier  ███   Sindarin, C#, Go
B-Tier  ██    Java, Node.js
C-Tier  █     Python
```

Sindarin sits comfortably in the **A-Tier**, trading blows with established compiled languages while significantly outperforming interpreted languages.

---

## Test Environment

- **Date**: 2026-01-02
- **Runs per benchmark**: 3 (median reported)
- **Sindarin**: Compiled to C via GCC with `-O2` optimization

---

## Benchmark Results

### 1. Fibonacci (Recursive fib(35))

*Tests function call overhead and recursion performance.*

```
sindarin  ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   25ms
c         ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   23ms  (fastest)
go        █░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   48ms
rust      ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   28ms
java      █░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   41ms
csharp    █░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   54ms
python    ████████████████████████████████████████ 1470ms
nodejs    ███████████████░░░░░░░░░░░░░░░░░░░░░░░░░  558ms
```

| Rank | Language | Time | vs Sindarin |
|:----:|----------|-----:|:-----------:|
| 1 | C | 23ms | 0.92x |
| **2** | **Sindarin** | **25ms** | **1.00x** |
| 3 | Rust | 28ms | 1.12x |
| 4 | Java | 41ms | 1.64x |
| 5 | Go | 48ms | 1.92x |
| 6 | C# | 54ms | 2.16x |
| 7 | Node.js | 558ms | 22.3x |
| 8 | Python | 1470ms | 58.8x |

---

### 2. Prime Sieve (Sieve of Eratosthenes to 1,000,000)

*Tests memory allocation and iteration performance.*

```
sindarin  █░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    6ms
c         ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    3ms
go        █░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    4ms
rust      ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    2ms  (fastest)
java      ████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   15ms
csharp    █░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    4ms
python    ████████████████████████████████████████  133ms
nodejs    ██████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   22ms
```

| Rank | Language | Time | vs Sindarin |
|:----:|----------|-----:|:-----------:|
| 1 | Rust | 2ms | 0.33x |
| 2 | C | 3ms | 0.50x |
| 3 | Go | 4ms | 0.67x |
| 4 | C# | 4ms | 0.67x |
| **5** | **Sindarin** | **6ms** | **1.00x** |
| 6 | Java | 15ms | 2.50x |
| 7 | Node.js | 22ms | 3.67x |
| 8 | Python | 133ms | 22.2x |

---

### 3. String Operations (100,000 concatenations)

*Tests string manipulation and memory management.*

```
sindarin  █░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    1ms  (fastest)
c         █░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    1ms
go        █░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    1ms
rust      ███░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    3ms
java      ████████████████░░░░░░░░░░░░░░░░░░░░░░░░   13ms
csharp    ████████████████████████████████████████   31ms
python    ████████████████████████░░░░░░░░░░░░░░░░   19ms
nodejs    ███████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    6ms
```

| Rank | Language | Time | vs Sindarin |
|:----:|----------|-----:|:-----------:|
| **1** | **Sindarin** | **1ms** | **1.00x** |
| 2 | C | 1ms | 1.00x |
| 3 | Go | 1ms | 1.00x |
| 4 | Rust | 3ms | 3.00x |
| 5 | Node.js | 6ms | 6.00x |
| 6 | Java | 13ms | 13.0x |
| 7 | Python | 19ms | 19.0x |
| 8 | C# | 31ms | 31.0x |

---

### 4. Array Operations (1,000,000 integers)

*Tests array creation, iteration, and in-place reversal.*

```
sindarin  ███░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    6ms
c         █░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    2ms
go        ███░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    6ms
rust      ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    1ms  (fastest)
java      █████████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   16ms
csharp    ██████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   11ms
python    ███████████████████░░░░░░░░░░░░░░░░░░░░░   35ms
nodejs    ████████████████████████████████████████   71ms
```

| Rank | Language | Time | vs Sindarin |
|:----:|----------|-----:|:-----------:|
| 1 | Rust | 1ms | 0.17x |
| 2 | C | 2ms | 0.33x |
| **3** | **Sindarin** | **6ms** | **1.00x** |
| 4 | Go | 6ms | 1.00x |
| 5 | C# | 11ms | 1.83x |
| 6 | Java | 16ms | 2.67x |
| 7 | Python | 35ms | 5.83x |
| 8 | Node.js | 71ms | 11.8x |

---

## Overall Rankings

Average rank across all benchmarks:

| Rank | Language | Avg Position | Strengths |
|:----:|----------|:------------:|-----------|
| 1 | C | 1.8 | Fastest baseline, low-level control |
| 2 | Rust | 2.2 | Memory safety + speed |
| **3** | **Sindarin** | **2.8** | **String ops champion, high-level syntax** |
| 4 | Go | 3.8 | Fast compilation, good concurrency |
| 5 | Java | 5.5 | JIT warmup needed |
| 6 | C# | 5.8 | Good all-rounder |
| 7 | Node.js | 6.8 | V8 optimization |
| 8 | Python | 7.5 | Interpreted overhead |

---

## Compilation Notes

| Language | Compiler/Runtime | Flags |
|----------|------------------|-------|
| Sindarin | GCC (via C) | `-O2` |
| C | GCC | `-O2` |
| Rust | rustc | `-O` (release) |
| Go | go build | default |
| Java | javac | default |
| C# | dotnet | `-c Release` |
| Python | CPython 3.x | interpreter |
| Node.js | V8 | default |

---

## Validation

All implementations produce identical results:

- Fibonacci(35) = 9,227,465
- Primes up to 1,000,000 = 78,498
- String length = 500,000; Occurrences = 100,000
- Array sum = 499,999,500,000
