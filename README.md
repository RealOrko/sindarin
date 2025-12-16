# 🐍 Sindarin (Sn) Programming Language

**Sindarin** is a statically-typed procedural programming language that compiles to C. It features clean arrow-based syntax, powerful string interpolation, and built-in array operations.

```
.sn source → 🔧 Sn Compiler → C code → 🏗️ GCC → executable
```

## ✨ Features

- 🎯 **Static typing** with explicit type annotations
- 🏹 **Arrow syntax** (`=>`) for clean, readable code blocks
- 📝 **String interpolation** with `$"Hello {name}!"`
- 📦 **Arrays** with built-in operations (push, pop, slice, etc.)
- 🔁 **Recursion** and standard control flow
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

### 📦 Arrays

#### Declaration & Initialization
```sn
var numbers: int[] = {1, 2, 3, 4, 5}
var names: str[] = {"alice", "bob", "charlie"}
var empty: int[] = {}
```

#### Array Operations
```sn
// Length
var size: int = len(numbers)

// Access by index
var first: int = numbers[0]

// Push (append)
numbers = push(6, numbers)

// Pop (remove last)
numbers = pop(numbers)

// Reverse
numbers = rev(numbers)

// Remove at index
numbers = rem(1, numbers)

// Insert at index
numbers = ins(99, 0, numbers)
```

#### Slicing 🔪
```sn
var arr: int[] = {1, 2, 3, 4, 5}

var slice1: int[] = arr[1..3]   // {2, 3}
var slice2: int[] = arr[2..]    // {3, 4, 5}
var slice3: int[] = arr[..2]    // {1, 2}
```

#### Method Syntax (Alternative)
```sn
var arr: int[]
arr.push(1)
arr.push(2)
var last: int = arr.pop()
arr.clear()
var size: int = arr.length
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

### 🔤 Strings as Character Arrays

Strings support array operations:

```sn
var text: str = "abc"
text = push('d', text)      // "abcd"
text = rev(text)            // "dcba"
var sub: str = text[1..3]   // "cb"
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

fn main(): void =>
  for var num: int = 1; num <= 20; num++ =>
    if is_prime(num) =>
      print($"{num} is prime! 🎉\n")
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
