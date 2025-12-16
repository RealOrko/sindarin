# 🐍 Sn Compiler

A statically-typed procedural language that compiles `.sn` → C → executable.

## 🔨 Build & Run

```bash
./scripts/build.sh          # Full build + tests
./scripts/run.sh            # Run samples/main.sn
./scripts/test.sh           # Unit tests
./scripts/integration_test.sh  # Integration tests
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

## 📚 Syntax

```
fn add(a: int, b: int): int => a + b
var x: int = 42
if cond => ... else => ...
$"Hello {name}"
```

Types: `int`, `double`, `str`, `char`, `bool`
