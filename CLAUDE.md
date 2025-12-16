# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is **Sn**, a statically-typed procedural programming language compiler written in C. The compiler translates `.sn` source files to C code, which is then compiled with GCC.

## Build Commands

All commands are run from the repository root.

```bash
# Full build (clean, compile, run tests)
./scripts/build.sh

# Build compiler only
cd compiler && make

# Build and run tests only
cd compiler && make tests
./scripts/test.sh

# Run a single .sn program
./scripts/run.sh  # compiles samples/main.sn and runs it
```

The compiled binary is output to `bin/sn`. Test binary is `bin/tests`.

## Compiler Architecture

The compiler follows a traditional pipeline in `compiler/`:

1. **Lexer** (`lexer.c`) - Tokenizes source into tokens defined in `token.c`
2. **Parser** (`parser.c`) - Builds AST from tokens, structures in `ast.c`
3. **Type Checker** (`type_checker.c`) - Validates types using `symbol_table.c`
4. **Code Gen** (`code_gen.c`) - Emits C code from the typed AST

Entry point: `main.c` → `compiler.c` orchestrates the pipeline.

Memory management uses a custom arena allocator (`arena.c`).

## Compiler Usage

```bash
bin/sn <source.sn> -o <output.c> [-v] [-l <level>]
# -o: output file (default: source.s)
# -v: verbose mode
# -l: log level (0=none, 1=error, 2=warning, 3=info, 4=verbose)
```

Generated C code requires linking with `bin/arena.o`, `bin/debug.o`, and `bin/runtime.o`.

## Testing

**Unit Tests:** Tests are in `compiler/tests/` with one file per module (e.g., `lexer_tests.c`, `parser_tests.c`). All tests are aggregated in `_all_.c` and run via `bin/tests`.

**Integration Tests:** End-to-end tests in `compiler/tests/integration/` compile `.sn` files and verify output.

```bash
# Run unit tests
./scripts/test.sh

# Run integration tests
./scripts/integration_test.sh
```

**Test Utilities:** `compiler/tests/test_utils.h` provides assertion macros and shared helpers.

## Runtime

The runtime library (`runtime.c`, `runtime.h`) provides:
- Overflow-checked arithmetic operations (`rt_add_long`, etc.)
- String operations (`rt_str_concat`, `rt_free_string`)
- Type-to-string conversion for string interpolation
- Dynamic array operations (`rt_array_push_*`)

## Sn Language Syntax

- Functions: `fn name(param: type): return_type => body`
- Variables: `var name: type = value`
- Control flow uses `=>` blocks: `if cond => ... else => ...`
- String interpolation: `$"text {expr}"`
- Types: `int`, `double`, `str`, `char`, `bool`
