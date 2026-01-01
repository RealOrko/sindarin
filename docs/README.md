# Sindarin Documentation

Welcome to the Sindarin language documentation. This index provides navigation to all available documentation.

## Language Reference

Core documentation for the Sindarin programming language:

| Document | Description |
|----------|-------------|
| [OVERVIEW.md](language/OVERVIEW.md) | Language philosophy, syntax overview, and examples |
| [TYPES.md](language/TYPES.md) | Primitive and built-in types |
| [STRINGS.md](language/STRINGS.md) | String methods, interpolation, and escape sequences |
| [ARRAYS.md](language/ARRAYS.md) | Array operations, slicing, and methods |
| [FILE_IO.md](language/FILE_IO.md) | TextFile and BinaryFile operations |
| [TIME.md](language/TIME.md) | Date and time operations |
| [MEMORY.md](language/MEMORY.md) | Arena memory management |

## Draft Specifications

These features are planned but **not yet implemented**:

| Document | Status | Description |
|----------|--------|-------------|
| [DATE.md](drafts/DATE.md) | Not Implemented | Calendar date type (separate from Time) |
| [INTEROP.md](drafts/INTEROP.md) | Draft | C interoperability and native functions |
| [FORMAT_SPECIFIERS.md](drafts/FORMAT_SPECIFIERS.md) | Draft | Format specifiers for string interpolation |

## Internal Documentation

For contributors and compiler developers:

| Document | Description |
|----------|-------------|
| [TESTING.md](internal/TESTING.md) | Test coverage analysis and testing guide |
| [OPTIMIZATIONS.md](internal/OPTIMIZATIONS.md) | Compiler optimization passes |
| [REFACTOR.md](internal/REFACTOR.md) | Compiler refactoring notes |
| [RUNTIME.md](internal/RUNTIME.md) | Runtime library documentation |
| [ISSUES.md](internal/ISSUES.md) | Known issues and tracking |
| [ARRAY_IMPLEMENTATION.md](internal/ARRAY_IMPLEMENTATION.md) | Array implementation notes |

### Audit Reports

| Document | Description |
|----------|-------------|
| [type_checker_error_audit.md](internal/type_checker_error_audit.md) | Type checker error audit |
| [optimization_report.md](internal/optimization_report.md) | Optimization analysis report |
| [string_interpolation_analysis.md](internal/string_interpolation_analysis.md) | String interpolation analysis |

## Quick Links

- [Main README](../README.md) - Project overview and quick start
- [CLAUDE.md](../CLAUDE.md) - Project instructions for AI assistants
- [Benchmark Results](../benchmark/RESULTS.md) - Performance benchmarks

## Getting Started

New to Sindarin? Start with these documents:

1. **[OVERVIEW.md](language/OVERVIEW.md)** - Language philosophy and syntax
2. **[TYPES.md](language/TYPES.md)** - Data types and type system
3. **[STRINGS.md](language/STRINGS.md)** - String manipulation
4. **[ARRAYS.md](language/ARRAYS.md)** - Working with arrays

## Building and Testing

```bash
make build            # Build compiler
make test             # Run all tests
make run              # Compile and run samples/main.sn
```

See [TESTING.md](internal/TESTING.md) for detailed test coverage information.
