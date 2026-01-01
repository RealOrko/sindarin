# Benchmark Results

Performance comparison between Sindarin and other programming languages.

## Executive Summary

Sindarin delivers **compiled-language performance** while offering high-level language ergonomics. By compiling to optimized C code, Sindarin consistently ranks among the fastest languages tested.

### Key Highlights

| Metric | Result |
|--------|--------|
| **Fastest benchmark** | Prime Sieve (9ms) - #5 overall |
| **vs Python** | 4-65x faster |
| **vs Node.js** | 2-24x faster |
| **vs Java** | 1.7-2x faster |
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

- **Date**: 2026-01-01
- **Runs per benchmark**: 3 (median reported)
- **Sindarin**: Compiled to C via GCC with `-O2` optimization

---

## Benchmark Results

### 1. Fibonacci (Recursive fib(35))

*Tests function call overhead and recursion performance.*

```
sindarin  ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   24ms
c         ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   23ms  (fastest)
go        █░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   48ms
rust      ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   30ms
java      █░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   42ms
csharp    █░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   55ms
python    ████████████████████████████████████████ 1553ms
nodejs    ██████████████░░░░░░░░░░░░░░░░░░░░░░░░░░  579ms
```

| Rank | Language | Time | vs Sindarin |
|:----:|----------|-----:|:-----------:|
| 1 | C | 23ms | 0.96x |
| **2** | **Sindarin** | **24ms** | **1.00x** |
| 3 | Rust | 30ms | 1.25x |
| 4 | Java | 42ms | 1.75x |
| 5 | Go | 48ms | 2.00x |
| 6 | C# | 55ms | 2.29x |
| 7 | Node.js | 579ms | 24.1x |
| 8 | Python | 1553ms | 64.7x |

---

### 2. Prime Sieve (Sieve of Eratosthenes to 1,000,000)

*Tests memory allocation and iteration performance.*

```
sindarin  ██░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    9ms
c         ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    3ms
go        ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    2ms  (fastest)
rust      ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    2ms
java      ███░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   15ms
csharp    █░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    4ms
python    ████████████████████████████████████████  155ms
nodejs    █████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   22ms
```

| Rank | Language | Time | vs Sindarin |
|:----:|----------|-----:|:-----------:|
| 1 | Go | 2ms | 0.22x |
| 2 | Rust | 2ms | 0.22x |
| 3 | C | 3ms | 0.33x |
| 4 | C# | 4ms | 0.44x |
| **5** | **Sindarin** | **9ms** | **1.00x** |
| 6 | Java | 15ms | 1.67x |
| 7 | Node.js | 22ms | 2.44x |
| 8 | Python | 155ms | 17.2x |

---

### 3. String Operations (100,000 concatenations)

*Tests string manipulation and memory management.*

```
sindarin  ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    0ms  (fastest)
c         ██░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    2ms
go        ██░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    2ms
rust      ███░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    3ms
java      █████████████████░░░░░░░░░░░░░░░░░░░░░░░   14ms
csharp    ████████████████████████████████████████   32ms
python    █████████████████████████░░░░░░░░░░░░░░░   20ms
nodejs    ███████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    6ms
```

| Rank | Language | Time | vs Sindarin |
|:----:|----------|-----:|:-----------:|
| **1** | **Sindarin** | **0ms** | **1.00x** |
| 2 | C | 2ms | N/A |
| 3 | Go | 2ms | N/A |
| 4 | Rust | 3ms | N/A |
| 5 | Node.js | 6ms | N/A |
| 6 | Java | 14ms | N/A |
| 7 | Python | 20ms | N/A |
| 8 | C# | 32ms | N/A |

---

### 4. Array Operations (1,000,000 integers)

*Tests array creation, iteration, and in-place reversal.*

```
sindarin  ████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    9ms
c         ██░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    4ms
go        ███░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    7ms
rust      █░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    2ms  (fastest)
java      ████████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   16ms
csharp    ██████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   12ms
python    ██████████████████░░░░░░░░░░░░░░░░░░░░░░   36ms
nodejs    ████████████████████████████████████████   80ms
```

| Rank | Language | Time | vs Sindarin |
|:----:|----------|-----:|:-----------:|
| 1 | Rust | 2ms | 0.22x |
| 2 | C | 4ms | 0.44x |
| 3 | Go | 7ms | 0.78x |
| **4** | **Sindarin** | **9ms** | **1.00x** |
| 5 | C# | 12ms | 1.33x |
| 6 | Java | 16ms | 1.78x |
| 7 | Python | 36ms | 4.00x |
| 8 | Node.js | 80ms | 8.89x |

---

## Overall Rankings

Average rank across all benchmarks:

| Rank | Language | Avg Position | Strengths |
|:----:|----------|:------------:|-----------|
| 1 | C | 2.0 | Fastest baseline, low-level control |
| 2 | Rust | 2.5 | Memory safety + speed |
| **3** | **Sindarin** | **3.0** | **String ops champion, high-level syntax** |
| 4 | Go | 3.0 | Fast compilation, good concurrency |
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
