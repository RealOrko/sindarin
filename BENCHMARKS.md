# Benchmark Results

Performance comparison between Sindarin and other programming languages.

## Executive Summary

Sindarin delivers **compiled-language performance** while offering high-level language ergonomics. By compiling to optimized C code, Sindarin consistently ranks among the fastest languages tested.

### Key Highlights

| Metric | Result |
|--------|--------|
| **Fastest benchmark** | String Operations (1ms) - #1 overall |
| **vs Python** | 4-54x faster |
| **vs Node.js** | 4-20x faster |
| **vs Java** | 1.5-15x faster |
| **vs C (baseline)** | 0.2-1.0x (competitive) |

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

- **Date**: 2026-01-03
- **Runs per benchmark**: 3 (median reported)
- **Sindarin**: Compiled to C via GCC with `-O2` optimization

---

## Benchmark Results

### 1. Fibonacci (Recursive fib(35))

*Tests function call overhead and recursion performance.*

```
sindarin  ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   27ms
c         ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   24ms  (fastest)
go        █░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   48ms
rust      ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   28ms
java      █░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   41ms
csharp    █░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   54ms
python    ████████████████████████████████████████ 1470ms
nodejs    ██████████████░░░░░░░░░░░░░░░░░░░░░░░░░░  544ms
```

| Rank | Language | Time | vs Sindarin |
|:----:|----------|-----:|:-----------:|
| 1 | C | 24ms | 0.89x |
| **2** | **Sindarin** | **27ms** | **1.00x** |
| 3 | Rust | 28ms | 1.04x |
| 4 | Java | 41ms | 1.52x |
| 5 | Go | 48ms | 1.78x |
| 6 | C# | 54ms | 2.00x |
| 7 | Node.js | 544ms | 20.1x |
| 8 | Python | 1470ms | 54.4x |

---

### 2. Prime Sieve (Sieve of Eratosthenes to 1,000,000)

*Tests memory allocation and iteration performance.*

```
sindarin  █░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    6ms
c         ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    2ms  (fastest)
go        ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    2ms
rust      ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    2ms
java      ███░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   13ms
csharp    █░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    4ms
python    ████████████████████████████████████████  138ms
nodejs    ██████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   22ms
```

| Rank | Language | Time | vs Sindarin |
|:----:|----------|-----:|:-----------:|
| 1 | C | 2ms | 0.33x |
| 2 | Go | 2ms | 0.33x |
| 3 | Rust | 2ms | 0.33x |
| 4 | C# | 4ms | 0.67x |
| **5** | **Sindarin** | **6ms** | **1.00x** |
| 6 | Java | 13ms | 2.17x |
| 7 | Node.js | 22ms | 3.67x |
| 8 | Python | 138ms | 23.0x |

---

### 3. String Operations (100,000 concatenations)

*Tests string manipulation and memory management.*

```
sindarin  █░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    1ms  (fastest)
c         █░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    1ms
go        █░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    1ms
rust      ███░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    3ms
java      ███████████████████░░░░░░░░░░░░░░░░░░░░░   15ms
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
| 6 | Java | 15ms | 15.0x |
| 7 | Python | 19ms | 19.0x |
| 8 | C# | 31ms | 31.0x |

---

### 4. Array Operations (1,000,000 integers)

*Tests array creation, iteration, and in-place reversal.*

```
sindarin  ████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    8ms
c         █░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    2ms
go        ██░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    5ms
rust      ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    1ms  (fastest)
java      ███████████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   20ms
csharp    ██████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   11ms
python    ███████████████████░░░░░░░░░░░░░░░░░░░░░   35ms
nodejs    ████████████████████████████████████████   71ms
```

| Rank | Language | Time | vs Sindarin |
|:----:|----------|-----:|:-----------:|
| 1 | Rust | 1ms | 0.12x |
| 2 | C | 2ms | 0.25x |
| 3 | Go | 5ms | 0.62x |
| **4** | **Sindarin** | **8ms** | **1.00x** |
| 5 | C# | 11ms | 1.38x |
| 6 | Java | 20ms | 2.50x |
| 7 | Python | 35ms | 4.38x |
| 8 | Node.js | 71ms | 8.88x |

---

## Overall Rankings

Average rank across all benchmarks:

| Rank | Language | Avg Position | Strengths |
|:----:|----------|:------------:|-----------|
| 1 | C | 1.5 | Fastest baseline, low-level control |
| 2 | Rust | 2.8 | Memory safety + speed |
| **3** | **Sindarin** | **3.0** | **String ops champion, high-level syntax** |
| 4 | Go | 3.2 | Fast compilation, good concurrency |
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
