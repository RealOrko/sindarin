# Sindarin (Sn) Programming Language

**Sindarin** is a statically-typed procedural programming language that compiles to C. It features clean arrow-based syntax, powerful string interpolation, and built-in array operations.

```
.sn source → Sn Compiler → C code → GCC → executable
```

## Features

- **Static typing** with explicit type annotations
- **Arrow syntax** (`=>`) for clean, readable code blocks
- **String interpolation** with `$"Hello {name}!"`
- **Arrays** with built-in operations (push, pop, slice, join, etc.)
- **String methods** (toUpper, toLower, trim, split, splitLines, isBlank, etc.)
- **File I/O** with TextFile and BinaryFile types
- **Control flow** with for, for-each, while, break, continue
- **Module imports** for code organization
- **Arena memory** with shared/private scopes and copy semantics
- **C interoperability** with native functions, pointers, and callbacks

## Quick Start

### Build the Compiler

```bash
# Configure and build with CMake
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Verify
bin/sn --version
```

See [docs/language/BUILDING.md](docs/language/BUILDING.md) for detailed instructions for Linux, macOS, and Windows.

### Compile a Program

```bash
bin/sn samples/main.sn -o myprogram    # Compile to executable
./myprogram                             # Run it

# Or emit C code only:
bin/sn samples/main.sn --emit-c -o output.c
```

## Example

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
  var primes: int[] = {}
  for var n: int = 2; n <= 50; n++ =>
    if is_prime(n) =>
      primes.push(n)
  print($"Primes: {primes.join(\", \")}\n")
```

## Documentation

See [docs/README.md](docs/README.md) for the full documentation index.

### Getting Started

| Document | Description |
|----------|-------------|
| [BUILDING](docs/language/BUILDING.md) | Build instructions for Linux, macOS, Windows |
| [OVERVIEW](docs/language/OVERVIEW.md) | Language philosophy, syntax overview, examples |

### Language Reference

| Document | Description |
|----------|-------------|
| [TYPES](docs/language/TYPES.md) | Primitive and built-in types |
| [STRINGS](docs/language/STRINGS.md) | String methods and interpolation |
| [ARRAYS](docs/language/ARRAYS.md) | Array operations and slicing |
| [LAMBDAS](docs/language/LAMBDAS.md) | Lambda expressions and closures |
| [FILE_IO](docs/language/FILE_IO.md) | TextFile and BinaryFile operations |
| [NETWORK_IO](docs/language/NETWORK_IO.md) | TCP and UDP networking |
| [DATE](docs/language/DATE.md) | Calendar date operations |
| [TIME](docs/language/TIME.md) | Date and time operations |
| [RANDOM](docs/language/RANDOM.md) | Random number generation |
| [UUID](docs/language/UUID.md) | UUID generation and manipulation |
| [ENVIRONMENT](docs/language/ENVIRONMENT.md) | Environment variable access |
| [MEMORY](docs/language/MEMORY.md) | Arena memory management |
| [THREADING](docs/language/THREADING.md) | Threading with spawn and sync |
| [PROCESSES](docs/language/PROCESSES.md) | Process execution and output capture |
| [NAMESPACES](docs/language/NAMESPACES.md) | Namespaced imports for collision resolution |
| [INTEROP](docs/language/INTEROP.md) | C interoperability and native functions |

## Architecture

```
Source (.sn)
    ↓
Lexer → Parser → Type Checker → Optimizer → Code Gen → GCC
    ↓
Executable
```

See `src/` for compiler implementation details.

## Testing

```bash
bin/tests                              # Unit tests
./scripts/run_tests.sh integration     # Integration tests
./scripts/run_tests.sh explore         # Exploratory tests
```

On Windows, use the PowerShell test runner:

```powershell
.\scripts\run_integration_test.ps1 -TestType integration -All
.\scripts\run_integration_test.ps1 -TestType explore -All
```

## Project Structure

```
├── src/           # Compiler source code
├── tests/         # Unit, integration, and exploratory tests
├── samples/       # Example .sn programs
├── docs/          # Documentation
├── benchmark/     # Performance benchmarks
├── bin/           # Compiled outputs (sn, tests, runtime)
├── build/         # CMake build directory
└── CMakeLists.txt # CMake build configuration
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Run tests with `make test`
4. Submit a pull request

## License

MIT License - feel free to use, modify, and distribute!

---

*Named after the Elvish language from Tolkien's legendarium*
