# 🐍 Sn Compiler

A statically-typed procedural language that compiles `.sn` → C → executable.

## 🔨 Build & Run

```bash
make                  # Full build + tests
make run              # Run samples/main.sn
make test             # Unit tests
make test-integration # Integration tests
make help             # Show all targets
```

Binaries: `bin/sn` (compiler), `bin/tests`

## 🏗️ Architecture

```
lexer.c → parser.c → type_checker.c → code_gen.c
   ↓         ↓            ↓              ↓
 tokens     AST      typed AST        C code
```

Entry: `main.c` → `compiler.c` • Memory: `arena.c`

## ⚙️ Usage

```bash
bin/sn <source.sn> -o <output.c> [-v] [-l 0-4]
```

Link output with: `bin/arena.o`, `bin/debug.o`, `bin/runtime.o`

## 🧪 Tests

- **Unit:** `compiler/tests/*_tests.c` → `bin/tests`
- **Integration:** `compiler/tests/integration/*.sn`
- **Exploratory:** `make test-explore` runs `testing/*.sn`

## 📚 Syntax

```
fn add(a: int, b: int): int => a + b
var x: int = 42
var b: byte = 255
if cond => ... else => ...
$"Hello {name}"
```

Types: `int`, `double`, `str`, `char`, `bool`, `byte`

File I/O: `TextFile`, `BinaryFile` (see `docs/FILE_IO.md`)
