# Sindarin Language Overview

Sindarin is a statically-typed procedural programming language that compiles to C. It features clean arrow-based syntax, powerful string interpolation, and built-in array operations.

```
.sn source → Sn Compiler → C code → GCC → executable
```

## Philosophy

### Design Principles

1. **Explicit Over Implicit** - All types are explicitly annotated. No type inference means code is always clear about what types are being used.

2. **Safety First** - Panic on errors rather than returning null or error codes. This keeps code clean and avoids pervasive null checks.

3. **Simple Memory Model** - Arena-based memory management with clear ownership semantics. No manual malloc/free, no garbage collector pauses.

4. **Clean Syntax** - Arrow-based blocks (`=>`) provide consistent, readable structure. No curly braces for blocks.

5. **Batteries Included** - Built-in string methods, array operations, and file I/O. Common tasks don't require external libraries.

6. **C Interoperability** - Compiles to readable C code. Easy to integrate with existing C libraries and tools.

### Why Sindarin?

- **Learning**: Clear syntax and explicit types make it easy to understand what code does
- **Scripting**: Built-in file I/O and string processing for automation tasks
- **Performance**: Compiles to native code via C with no runtime overhead
- **Simplicity**: Small language with consistent rules, easy to master

## Syntax Overview

### Arrow Blocks

Sindarin uses `=>` to introduce code blocks instead of curly braces:

```sindarin
fn greet(name: str): void =>
  print($"Hello, {name}!\n")

if condition =>
  doSomething()
else =>
  doOtherThing()

while running =>
  processNext()
```

### Variables

Variables are declared with `var` and require type annotations:

```sindarin
var name: str = "Sindarin"
var count: int = 42
var pi: double = 3.14159
var active: bool = true
var letter: char = 'S'
```

### Functions

Functions use the `fn` keyword with explicit parameter and return types:

```sindarin
fn add(a: int, b: int): int =>
  return a + b

fn factorial(n: int): int =>
  if n <= 1 =>
    return 1
  return n * factorial(n - 1)
```

### String Interpolation

Embed expressions in strings with `$"..."` syntax:

```sindarin
var name: str = "World"
var count: int = 42
print($"Hello, {name}! Count is {count}.\n")
```

### Arrays

Arrays use curly braces for literals and have built-in methods:

```sindarin
var numbers: int[] = {1, 2, 3, 4, 5}
numbers.push(6)
var first: int = numbers[0]
var last: int = numbers[-1]
var slice: int[] = numbers[1..4]
```

### Control Flow

```sindarin
// If-else
if condition =>
  doSomething()
else =>
  doOtherThing()

// While loop
while i < 10 =>
  process(i)
  i = i + 1

// For loop
for var i: int = 0; i < 10; i++ =>
  print($"{i}\n")

// For-each loop
for item in items =>
  process(item)
```

### Boolean Operators

```sindarin
if hasTicket && hasID =>
  print("Entry allowed\n")

if isAdmin || isModerator =>
  print("Can moderate\n")

if !isBlocked =>
  print("Access granted\n")
```

## Quick Examples

### Hello World

```sindarin
fn main(): void =>
  print("Hello, World!\n")
```

### FizzBuzz

```sindarin
fn main(): void =>
  for var i: int = 1; i <= 100; i++ =>
    if i % 15 == 0 =>
      print("FizzBuzz\n")
    else if i % 3 == 0 =>
      print("Fizz\n")
    else if i % 5 == 0 =>
      print("Buzz\n")
    else =>
      print($"{i}\n")
```

### Prime Finder

```sindarin
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
```

### File Processing

```sindarin
fn main(): void =>
  // Read file and count lines
  var content: str = TextFile.readAll("data.txt")
  var lines: str[] = content.splitLines()

  var nonEmpty: int = 0
  for line in lines =>
    if !line.isBlank() =>
      nonEmpty = nonEmpty + 1

  print($"Total lines: {lines.length}\n")
  print($"Non-empty lines: {nonEmpty}\n")
```

### String Processing

```sindarin
fn main(): void =>
  var text: str = "  Hello, World!  "

  // Method chaining
  var cleaned: str = text.trim().toLower()
  print($"Cleaned: '{cleaned}'\n")

  // Splitting and joining
  var words: str[] = "apple,banana,cherry".split(",")
  var sentence: str = words.join(" and ")
  print($"Fruits: {sentence}\n")
```

## Module System

Split code across files with imports:

```sindarin
// utils.sn
fn helper(): void =>
  print("I'm a helper!\n")

// main.sn
import "utils"

fn main(): void =>
  helper()
```

## Memory Management

Sindarin uses arena-based memory with optional control:

```sindarin
// Shared function - uses caller's arena
fn helper(a: int, b: int) shared: int =>
  return a + b

// Private block - isolated arena, freed on exit
private =>
  var temp: int[] = {1, 2, 3}
  // temp freed here

// Value copy semantics
var original: int[] = {1, 2, 3}
var copy: int[] as val = original  // Independent copy
```

See [MEMORY.md](MEMORY.md) for full documentation.

## Compilation

```bash
# Compile to executable
bin/sn source.sn -o program
./program

# Emit C code only
bin/sn source.sn --emit-c -o output.c

# Debug build with symbols
bin/sn source.sn -g -o program
```

## Documentation Index

- [TYPES.md](TYPES.md) - Primitive and built-in types
- [STRINGS.md](STRINGS.md) - String methods and interpolation
- [ARRAYS.md](ARRAYS.md) - Array operations and slicing
- [LAMBDAS.md](LAMBDAS.md) - Lambda expressions and closures
- [FILE_IO.md](FILE_IO.md) - TextFile and BinaryFile operations
- [DATE.md](DATE.md) - Calendar date operations
- [TIME.md](TIME.md) - Date and time operations
- [RANDOM.md](RANDOM.md) - Random number generation
- [UUID.md](UUID.md) - Universally unique identifiers
- [ENVIRONMENT.md](ENVIRONMENT.md) - Environment variables
- [MEMORY.md](MEMORY.md) - Arena memory management
- [THREADING.md](THREADING.md) - Threading with `&` spawn and `!` sync
- [INTEROP.md](INTEROP.md) - C interoperability and native functions
