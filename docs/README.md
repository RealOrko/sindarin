# Sindarin Documentation

Welcome to the Sindarin language documentation. This index provides navigation to all available documentation.

## Getting Started

| Document | Description |
|----------|-------------|
| [BUILDING.md](language/BUILDING.md) | How to build Sindarin on Linux, macOS, and Windows |
| [OVERVIEW.md](language/OVERVIEW.md) | Language philosophy, syntax overview, and examples |

## Language Reference

Core documentation for the Sindarin programming language:

| Document | Description |
|----------|-------------|
| [TYPES.md](language/TYPES.md) | Primitive and built-in types |
| [STRINGS.md](language/STRINGS.md) | String methods, interpolation, and escape sequences |
| [ARRAYS.md](language/ARRAYS.md) | Array operations, slicing, and methods |
| [STRUCTS.md](language/STRUCTS.md) | Struct declarations and C interop |
| [LAMBDAS.md](language/LAMBDAS.md) | Lambda expressions and closures |
| [FILE_IO.md](language/FILE_IO.md) | TextFile and BinaryFile operations |
| [DATE.md](language/DATE.md) | Calendar date operations |
| [TIME.md](language/TIME.md) | Date and time operations |
| [RANDOM.md](language/RANDOM.md) | Random number generation |
| [UUID.md](language/UUID.md) | Universally unique identifiers |
| [ENVIRONMENT.md](language/ENVIRONMENT.md) | Environment variables |
| [MEMORY.md](language/MEMORY.md) | Arena memory management |
| [THREADING.md](language/THREADING.md) | Threading with `&` spawn and `!` sync operators |
| [PROCESSES.md](language/PROCESSES.md) | Process execution and output capture |
| [NAMESPACES.md](language/NAMESPACES.md) | Namespaced imports for collision resolution |
| [NETWORK_IO.md](language/NETWORK_IO.md) | TCP and UDP network operations |
| [INTEROP.md](language/INTEROP.md) | C interoperability and native functions |
| [INTERCEPTORS.md](language/INTERCEPTORS.md) | Function interception for debugging and mocking |

## Draft Specifications

These features are planned or partially implemented:

| Document | Status | Description |
|----------|--------|-------------|
| [FORMATTING.md](drafts/FORMATTING.md) | Partial | String formatting and format specifiers |

## Internal Documentation

For contributors and compiler developers:

| Document | Description |
|----------|-------------|
| [TESTING.md](internal/TESTING.md) | Test coverage analysis and testing guide |
| [OPTIMIZATIONS.md](internal/OPTIMIZATIONS.md) | Compiler optimization passes |
| [REFACTOR.md](internal/REFACTOR.md) | Compiler refactoring notes |
| [RUNTIME.md](internal/RUNTIME.md) | Runtime library documentation |
| [ISSUES.md](internal/ISSUES.md) | Known issues and tracking |
| [ARRAY_PLAN.md](internal/ARRAY_PLAN.md) | Array feature implementation plan |
| [REFACTOR_TESTS.md](internal/REFACTOR_TESTS.md) | Project restructuring notes |
| [DOCS.md](internal/DOCS.md) | Documentation consolidation plan |

### Analysis Reports

| Document | Description |
|----------|-------------|
| [TYPE_CHECKER.md](internal/TYPE_CHECKER.md) | Type checker error message audit |
| [OPTIMIZATIONS_REPORT.md](internal/OPTIMIZATIONS_REPORT.md) | Optimization impact report |
| [STRING_INTERPOLATION.md](internal/STRING_INTERPOLATION.md) | String interpolation technical analysis |

## Quick Links

- [Main README](../README.md) - Project overview and quick start
- [CLAUDE.md](../CLAUDE.md) - Project instructions for AI assistants
- [Benchmark Results](../benchmark/RESULTS.md) - Performance benchmarks

## New to Sindarin?

Start with these documents:

1. **[BUILDING.md](language/BUILDING.md)** - Build the compiler from source
2. **[OVERVIEW.md](language/OVERVIEW.md)** - Language philosophy and syntax
3. **[TYPES.md](language/TYPES.md)** - Data types and type system
4. **[STRINGS.md](language/STRINGS.md)** - String manipulation
5. **[ARRAYS.md](language/ARRAYS.md)** - Working with arrays
6. **[STRUCTS.md](language/STRUCTS.md)** - Structured data and C interop

## Building

See [BUILDING.md](language/BUILDING.md) for complete build instructions. Quick start with CMake:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
bin/sn --version
```

## Testing

```bash
bin/tests                           # Unit tests
./scripts/run_tests.sh integration  # Integration tests
./scripts/run_tests.sh explore      # Exploratory tests
```

See [TESTING.md](internal/TESTING.md) for detailed test coverage information.
