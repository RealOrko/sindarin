# 🐍 Sindarin (Sn) Programming Language

**Sindarin** is a statically-typed procedural programming language that compiles to C. It features clean arrow-based syntax, powerful string interpolation, and built-in array operations.

```
.sn source → 🔧 Sn Compiler → C code → 🏗️ GCC → executable
```

## ✨ Features

- 🎯 **Static typing** with explicit type annotations
- 🏹 **Arrow syntax** (`=>`) for clean, readable code blocks
- 📝 **String interpolation** with `$"Hello {name}!"`
- 📦 **Arrays** with built-in operations (push, pop, slice, join, etc.)
- 🔤 **String methods** (toUpper, toLower, trim, split, replace, etc.)
- 🔁 **Control flow** with for, for-each, while, break, continue
- ⚡ **Boolean operators** (`&&`, `||`, `!`)
- 📚 **Module imports** for code organization

## 🚀 Quick Start

### Building the Compiler

```bash
./scripts/build.sh          # Build compiler + run tests
```

### Compiling a Program

```bash
bin/sn samples/main.sn -o output.c
gcc output.c bin/arena.o bin/debug.o bin/runtime.o -o myprogram
./myprogram
```

### Running Samples

```bash
./scripts/run.sh            # Run samples/main.sn
```

## 📖 Language Guide

### 🔢 Data Types

| Type | Description | Example |
|------|-------------|---------|
| `int` | Integer numbers | `42`, `-7` |
| `double` | Floating-point | `3.14159` |
| `str` | Strings | `"hello"` |
| `char` | Single character | `'A'` |
| `bool` | Boolean | `true`, `false` |
| `type[]` | Arrays | `int[]`, `str[]` |

### 📝 Variables

```sn
var name: str = "Sindarin"
var count: int = 42
var pi: double = 3.14159
var active: bool = true
var letter: char = 'S'
```

### 🎯 Functions

Functions use the `fn` keyword with arrow syntax:

```sn
fn greet(name: str): void =>
  print($"Hello, {name}!\n")

fn add(a: int, b: int): int =>
  return a + b

fn factorial(n: int): int =>
  if n <= 1 =>
    return 1
  return n * factorial(n - 1)
```

### 🔀 Control Flow

#### If-Else
```sn
if condition =>
  // do something
else =>
  // do something else
```

#### Boolean Operators
```sn
if hasTicket && hasID =>
  print("Entry allowed\n")

if isAdmin || isModerator =>
  print("Can moderate\n")

if !isBlocked =>
  print("Access granted\n")
```

#### While Loop
```sn
var i: int = 0
while i < 10 =>
  print($"{i}\n")
  i = i + 1
```

#### For Loop
```sn
for var i: int = 0; i < 10; i++ =>
  print($"{i}\n")
```

#### For-Each Loop
```sn
var names: str[] = {"alice", "bob", "charlie"}
for name in names =>
  print($"Hello, {name}!\n")
```

#### Break and Continue
```sn
for var i: int = 0; i < 10; i++ =>
  if i == 5 =>
    break           // Exit loop early
  if i % 2 == 0 =>
    continue        // Skip to next iteration
  print($"{i}\n")
```

### 💬 String Interpolation

Use `$` prefix to embed expressions in strings:

```sn
var name: str = "World"
var count: int = 42
print($"Hello, {name}! The answer is {count}.\n")
```

Works with all types:
```sn
var pi: double = 3.14
var flag: bool = true
print($"Pi is {pi}, flag is {flag}\n")
```

### 🔤 String Methods

```sn
var text: str = "  Hello World  "

// Case conversion
var upper: str = text.toUpper()     // "  HELLO WORLD  "
var lower: str = text.toLower()     // "  hello world  "

// Trimming
var trimmed: str = text.trim()      // "Hello World"

// Substring
var sub: str = "Hello".substring(0, 3)  // "Hel"

// Search
var idx: int = "hello".indexOf("ll")    // 2
var has: bool = "hello".contains("ell") // true
var starts: bool = "hello".startsWith("he")  // true
var ends: bool = "hello".endsWith("lo")      // true

// Replace
var replaced: str = "hello".replace("l", "L")  // "heLLo"

// Split
var parts: str[] = "a,b,c".split(",")   // {"a", "b", "c"}

// Length
var size: int = "hello".length          // 5

// Method chaining
var result: str = "  HELLO  ".trim().toLower()  // "hello"
```

### 📦 Arrays

#### Declaration & Initialization
```sn
var numbers: int[] = {1, 2, 3, 4, 5}
var names: str[] = {"alice", "bob", "charlie"}
var empty: int[] = {}
```

#### Array Methods
```sn
var arr: int[] = {1, 2, 3, 4, 5}

// Length
var size: int = arr.length      // 5

// Access (including negative indexing)
var first: int = arr[0]         // 1
var last: int = arr[-1]         // 5

// Modify
arr.push(6)                     // Append: {1, 2, 3, 4, 5, 6}
var popped: int = arr.pop()     // Remove last, returns 6
arr.insert(99, 2)               // Insert at index: {1, 2, 99, 3, 4, 5}
arr.remove(2)                   // Remove at index: {1, 2, 3, 4, 5}
arr.reverse()                   // In-place: {5, 4, 3, 2, 1}
arr.clear()                     // Empty the array

// Search
var idx: int = arr.indexOf(3)   // 2
var has: bool = arr.contains(3) // true

// Copy and combine
var copy: int[] = arr.clone()
var combined: int[] = arr.concat({6, 7, 8})

// Join (for string output)
var nums: int[] = {1, 2, 3}
var joined: str = nums.join(", ")  // "1, 2, 3"
```

#### Slicing
```sn
var arr: int[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}

// Range slicing
var slice1: int[] = arr[2..5]   // {2, 3, 4}
var slice2: int[] = arr[..3]    // {0, 1, 2}
var slice3: int[] = arr[7..]    // {7, 8, 9}
var copy: int[] = arr[..]       // Full copy

// Step slicing
var evens: int[] = arr[..:2]    // {0, 2, 4, 6, 8}
var odds: int[] = arr[1..:2]    // {1, 3, 5, 7, 9}

// Negative indexing in slices
var lastTwo: int[] = arr[-2..]  // {8, 9}
```

### 📚 Imports

Split code across files:

```sn
// utils.sn
fn helper(): void =>
  print("I'm a helper!\n")

// main.sn
import "utils"

fn main(): void =>
  helper()
```

## 🏗️ Architecture

```
┌─────────┐    ┌─────────┐    ┌──────────────┐    ┌──────────┐
│ lexer.c │ →  │ parser.c│ →  │type_checker.c│ →  │code_gen.c│
└─────────┘    └─────────┘    └──────────────┘    └──────────┘
     ↓              ↓               ↓                   ↓
   tokens          AST          typed AST            C code
```

## 🧪 Testing

```bash
./scripts/test.sh             # Unit tests
./scripts/integration_test.sh # Integration tests
```

## 📁 Project Structure

```
├── compiler/              # 🔧 Compiler source code
│   ├── main.c             # Entry point
│   ├── lexer.c/h          # Tokenizer
│   ├── parser.c/h         # AST builder
│   ├── type_checker.c/h   # Static type checking
│   ├── code_gen.c/h       # C code generator
│   ├── runtime.c/h        # Runtime library
│   ├── arena.c/h          # Memory management
│   └── tests/             # Unit tests
│       └── integration/   # Integration tests (.sn files)
├── samples/               # 📝 Example .sn programs
├── scripts/               # 🛠️ Build & run scripts
│   ├── build.sh           # Full build + tests
│   ├── run.sh             # Run main.sn
│   ├── test.sh            # Unit tests
│   └── integration_test.sh
├── bin/                   # 📦 Compiled outputs
│   ├── sn                 # Compiler binary
│   ├── tests              # Test runner
│   ├── *.o                # Object files for linking
│   └── *.d                # Dependency files
└── CLAUDE.md              # Project instructions
```

## 📜 Example Program

```sn
fn is_prime(n: int): bool =>
  if n <= 1 =>
    return false
  var i: int = 2
  while i * i <= n =>
    if n % i == 0 =>
      return false
    i = i + 1
  return true

fn find_primes(limit: int): int[] =>
  var primes: int[] = {}
  for var n: int = 2; n <= limit; n++ =>
    if is_prime(n) =>
      primes.push(n)
  return primes

fn main(): void =>
  var primes: int[] = find_primes(50)
  print($"Found {primes.length} primes: {primes.join(\", \")}\n")

  // Filter primes > 20
  print("Primes greater than 20:\n")
  for p in primes =>
    if p > 20 =>
      print($"  {p}\n")
```

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Run tests with `./scripts/test.sh`
4. Submit a pull request

## 📄 License

MIT License - feel free to use, modify, and distribute!

---

*🧝 Named after the Elvish language from Tolkien's legendarium*
