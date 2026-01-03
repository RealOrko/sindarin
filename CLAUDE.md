# 🐍 Sn Compiler

A statically-typed procedural language that compiles `.sn` → C → executable.

## 🔨 Build & Run

```bash
make build            # Build compiler and test binary
make run              # Compile and run samples/main.sn
make test             # All tests (unit + integration + exploratory)
make test-unit        # Unit tests only
make test-integration # Integration tests only
make test-explore     # Exploratory tests only
make clean            # Remove build artifacts
make help             # Show all targets
```

Binaries: `bin/sn` (compiler), `bin/tests` (unit test runner)

## 🏗️ Architecture

```
Source (.sn)
    ↓
┌─────────────────────────────────────────────────────┐
│  Lexer (lexer.c, lexer_scan.c, lexer_util.c)        │
│    → tokens                                          │
├─────────────────────────────────────────────────────┤
│  Parser (parser_stmt.c, parser_expr.c, parser_util.c)│
│    → AST                                             │
├─────────────────────────────────────────────────────┤
│  Type Checker (type_checker_stmt.c, _expr.c, _util.c)│
│    → typed AST                                       │
├─────────────────────────────────────────────────────┤
│  Optimizer (optimizer.c)                             │
│    → optimized AST                                   │
├─────────────────────────────────────────────────────┤
│  Code Gen (code_gen.c, _stmt.c, _expr.c, _util.c)   │
│    → C code                                          │
├─────────────────────────────────────────────────────┤
│  GCC Backend (gcc_backend.c)                         │
│    → executable                                      │
└─────────────────────────────────────────────────────┘
```

Key modules:
- `main.c` → `compiler.c`: Entry point and orchestration
- `symbol_table.c`: Scope and symbol management
- `runtime.c/h`: Built-in functions and types
- `diagnostic.c`: Error reporting and phase tracking
- `arena.c`: Memory management

## ⚙️ Usage

```bash
bin/sn <source.sn> [-o <executable>] [options]

Output options:
  -o <file>          Output executable (default: source without extension)
  --emit-c           Only output C code, don't compile
  --keep-c           Keep intermediate C file after compilation

Debug options:
  -v                 Verbose mode
  -g                 Debug build (symbols + address sanitizer)
  -l <level>         Log level (0=none, 1=error, 2=warning, 3=info, 4=verbose)

Optimization:
  -O0                No Sn optimization
  -O1                Basic (dead code elimination, string merging)
  -O2                Full (default: + tail call optimization)
  --unchecked        Unchecked arithmetic (no overflow checking)
```

## 🧪 Tests

- **Unit:** `tests/unit/*_tests.c` → `bin/tests`
- **Integration:** `tests/integration/*.sn`
- **Exploratory:** `tests/exploratory/*.sn`

## 📚 Syntax

```
fn add(a: int, b: int): int => a + b
var x: int = 42
var b: byte = 255
if cond => ... else => ...
$"Hello {name}"
```

Types: `int`, `long`, `double`, `str`, `char`, `bool`, `byte`, `void`

Built-in types: `TextFile`, `BinaryFile`, `Date`, `Time`

## 🚨 IMPORTANT: 
 
 - This project uses ASAN not valgrind. All memory are detected using `-fsanitize=address`. Do not introduce valgrind into this project!

 - Always execute compiled artifacts (both the compiler and sindarin code files) using a `timeout` prefix to avoid crashing the host machine if there is an accidental/unintended infinite loop.

 - Always debug with output written to the /tmp/ directory, this avoids accidentally comitting generated artifacts which do not form part of the solution. 

## 📖 Documentation

See [docs/README.md](docs/README.md) for the full documentation index.

**Language Reference:**
- [docs/language/OVERVIEW.md](docs/language/OVERVIEW.md) - Language philosophy and syntax
- [docs/language/TYPES.md](docs/language/TYPES.md) - Primitive and built-in types
- [docs/language/STRINGS.md](docs/language/STRINGS.md) - String methods and interpolation
- [docs/language/ARRAYS.md](docs/language/ARRAYS.md) - Array operations
- [docs/language/LAMBDAS.md](docs/language/LAMBDAS.md) - Lambda expressions and closures
- [docs/language/FILE_IO.md](docs/language/FILE_IO.md) - File I/O (TextFile, BinaryFile)
- [docs/language/DATE.md](docs/language/DATE.md) - Date operations
- [docs/language/TIME.md](docs/language/TIME.md) - Time operations
- [docs/language/MEMORY.md](docs/language/MEMORY.md) - Arena memory management
- [docs/language/THREADING.md](docs/language/THREADING.md) - Threading

**Draft Specifications:**
- [docs/drafts/FORMATTING.md](docs/drafts/FORMATTING.md) - Format specifiers (draft)
- [docs/drafts/INTEROP.md](docs/drafts/INTEROP.md) - C interoperability (draft)
- [docs/drafts/NAMESPACES.md](docs/drafts/NAMESPACES.md) - Namespaces (draft)
- [docs/drafts/NETWORK_IO.md](docs/drafts/NETWORK_IO.md) - Network I/O (draft)
- [docs/drafts/PROCESSES.md](docs/drafts/PROCESSES.md) - Process management (draft)
