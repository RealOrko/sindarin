# Sn Compiler Refactoring

This document tracks the restructuring of the Sn compiler codebase to improve maintainability, readability, and modularity.

## Status

| Phase | Description | Status |
|-------|-------------|--------|
| Phase 1 | Split monolithic files into focused modules | ✅ COMPLETE |
| Phase 2 | Organize modules into subfolders | 🔲 PENDING |

---

## Phase 2: Directory Structure Reorganization

Files with common prefixes in `./compiler` should be moved into subfolders following the same modularization style as `./compiler/runtime/`. File names remain unchanged.

### Reference: Runtime Module Pattern

The `runtime/` folder demonstrates the target structure:

```
compiler/
├── runtime.h                      # Main header (includes sub-headers)
├── runtime.c                      # Main implementation (shared code)
└── runtime/
    ├── runtime_internal.h         # Shared internal declarations
    ├── runtime_arena.c/h          # Arena module
    ├── runtime_string.c/h         # String module
    ├── runtime_array.c/h          # Array module
    ├── runtime_file.h             # File type definitions
    ├── runtime_text_file.c        # TextFile implementation
    ├── runtime_binary_file.c      # BinaryFile implementation
    ├── runtime_io.c/h             # I/O module
    ├── runtime_path.c/h           # Path module
    ├── runtime_time.c/h           # Time module
    └── runtime_byte.c/h           # Byte module
```

**Pattern:**
1. Main header (`prefix.h`) stays in compiler root, includes all sub-headers
2. Main implementation (`prefix.c`) stays in compiler root if needed for shared code
3. All sub-modules move to `prefix/` subfolder with unchanged file names

### Modules to Reorganize

#### 1. Code Generation → `code_gen/`

| Current Location | New Location |
|------------------|--------------|
| `code_gen.c` | `compiler/code_gen.c` (stays) |
| `code_gen.h` | `compiler/code_gen.h` (stays - main header) |
| `code_gen_expr.c/h` | `compiler/code_gen/code_gen_expr.c/h` |
| `code_gen_expr_array.c/h` | `compiler/code_gen/code_gen_expr_array.c/h` |
| `code_gen_expr_call.h` | `compiler/code_gen/code_gen_expr_call.h` |
| `code_gen_expr_call_array.c` | `compiler/code_gen/code_gen_expr_call_array.c` |
| `code_gen_expr_call_file.c` | `compiler/code_gen/code_gen_expr_call_file.c` |
| `code_gen_expr_call_string.c` | `compiler/code_gen/code_gen_expr_call_string.c` |
| `code_gen_expr_call_time.c` | `compiler/code_gen/code_gen_expr_call_time.c` |
| `code_gen_expr_lambda.c/h` | `compiler/code_gen/code_gen_expr_lambda.c/h` |
| `code_gen_expr_static.c/h` | `compiler/code_gen/code_gen_expr_static.c/h` |
| `code_gen_expr_string.c/h` | `compiler/code_gen/code_gen_expr_string.c/h` |
| `code_gen_stmt.c/h` | `compiler/code_gen/code_gen_stmt.c/h` |
| `code_gen_util.c/h` | `compiler/code_gen/code_gen_util.c/h` |

#### 2. Type Checker → `type_checker/`

| Current Location | New Location |
|------------------|--------------|
| `type_checker.c` | `compiler/type_checker.c` (stays) |
| `type_checker.h` | `compiler/type_checker.h` (stays - main header) |
| `type_checker_expr.c/h` | `compiler/type_checker/type_checker_expr.c/h` |
| `type_checker_expr_array.c/h` | `compiler/type_checker/type_checker_expr_array.c/h` |
| `type_checker_expr_call.c/h` | `compiler/type_checker/type_checker_expr_call.c/h` |
| `type_checker_expr_lambda.c/h` | `compiler/type_checker/type_checker_expr_lambda.c/h` |
| `type_checker_stmt.c/h` | `compiler/type_checker/type_checker_stmt.c/h` |
| `type_checker_util.c/h` | `compiler/type_checker/type_checker_util.c/h` |

#### 3. Optimizer → `optimizer/`

| Current Location | New Location |
|------------------|--------------|
| `optimizer.c` | `compiler/optimizer.c` (stays) |
| `optimizer.h` | `compiler/optimizer.h` (stays - main header) |
| `optimizer_string.c/h` | `compiler/optimizer/optimizer_string.c/h` |
| `optimizer_tail_call.c/h` | `compiler/optimizer/optimizer_tail_call.c/h` |
| `optimizer_util.c/h` | `compiler/optimizer/optimizer_util.c/h` |

#### 4. Parser → `parser/`

| Current Location | New Location |
|------------------|--------------|
| `parser.c` | `compiler/parser.c` (stays) |
| `parser.h` | `compiler/parser.h` (stays - main header) |
| `parser_expr.c/h` | `compiler/parser/parser_expr.c/h` |
| `parser_stmt.c/h` | `compiler/parser/parser_stmt.c/h` |
| `parser_util.c/h` | `compiler/parser/parser_util.c/h` |

#### 5. Lexer → `lexer/`

| Current Location | New Location |
|------------------|--------------|
| `lexer.c` | `compiler/lexer.c` (stays) |
| `lexer.h` | `compiler/lexer.h` (stays - main header) |
| `lexer_scan.c/h` | `compiler/lexer/lexer_scan.c/h` |
| `lexer_util.c/h` | `compiler/lexer/lexer_util.c/h` |

#### 6. AST → `ast/`

| Current Location | New Location |
|------------------|--------------|
| `ast.c` | `compiler/ast.c` (stays) |
| `ast.h` | `compiler/ast.h` (stays - main header) |
| `ast_expr.c/h` | `compiler/ast/ast_expr.c/h` |
| `ast_print.c/h` | `compiler/ast/ast_print.c/h` |
| `ast_stmt.c/h` | `compiler/ast/ast_stmt.c/h` |
| `ast_type.c/h` | `compiler/ast/ast_type.c/h` |

### Implementation Steps

1. **Create subfolder structure** - `mkdir -p compiler/{ast,lexer,parser,type_checker,optimizer,code_gen}`
2. **Remove empty placeholder directories** - `rmdir compiler/{codegen,typecheck}` (if empty)
3. **Move files** - Use `git mv` to preserve history
4. **Update include paths** - Change `#include "file.h"` to `#include "prefix/file.h"`
5. **Update Makefile** - Adjust SRCS paths
6. **Verify** - `make clean && make build && make test`

### Target Directory Structure

```
compiler/
├── main.c                         # Entry point
├── compiler.c/h                   # Orchestration
│
├── ast.c/h                        # AST main
├── ast/                           # AST modules (8 files)
│
├── lexer.c/h                      # Lexer main
├── lexer/                         # Lexer modules (4 files)
│
├── parser.c/h                     # Parser main
├── parser/                        # Parser modules (6 files)
│
├── type_checker.c/h               # Type checker main
├── type_checker/                  # Type checker modules (12 files)
│
├── optimizer.c/h                  # Optimizer main
├── optimizer/                     # Optimizer modules (6 files)
│
├── code_gen.c/h                   # Code gen main
├── code_gen/                      # Code gen modules (18 files)
│
├── runtime.c/h                    # Runtime main
├── runtime/                       # Runtime modules (18 files) ✅ DONE
│
├── symbol_table.c/h               # Standalone
├── token.c/h                      # Standalone
├── arena.c/h                      # Standalone
├── diagnostic.c/h                 # Standalone
├── gcc_backend.c/h                # Standalone
├── file.c/h                       # Standalone
├── debug.c/h                      # Standalone
├── error.h                        # Standalone
│
└── tests/                         # Test files
```

---

## Phase 1: Module Split (COMPLETED)

Phase 1 split large monolithic files into focused modules. All modules currently reside in the compiler root directory.

### Summary

| Original File | Original Lines | Split Into | New Modules |
|---------------|----------------|------------|-------------|
| `runtime.c` | 5,557 | 9 modules | `runtime/runtime_*.c` |
| `code_gen_expr.c` | 3,419 | 9 modules | `code_gen_expr_*.c` |
| `type_checker_expr.c` | 2,481 | 4 modules | `type_checker_expr_*.c` |
| `optimizer.c` | 1,282 | 4 modules | `optimizer_*.c` |

### Runtime Modules (in `runtime/`)

| Module | Description |
|--------|-------------|
| `runtime_arena.c/h` | Arena memory management |
| `runtime_string.c/h` | String operations |
| `runtime_array.c/h` | Array operations |
| `runtime_file.h` | File type definitions |
| `runtime_text_file.c` | TextFile I/O |
| `runtime_binary_file.c` | BinaryFile I/O |
| `runtime_io.c/h` | Stdin/Stdout/Stderr |
| `runtime_path.c/h` | Path and directory ops |
| `runtime_time.c/h` | Time operations |
| `runtime_byte.c/h` | Byte conversions |

### Code Generation Modules (in compiler root)

| Module | Description |
|--------|-------------|
| `code_gen_expr.c/h` | Core expression codegen |
| `code_gen_expr_lambda.c/h` | Lambda expressions |
| `code_gen_expr_call.h` | Call dispatch header |
| `code_gen_expr_call_array.c` | Array method calls |
| `code_gen_expr_call_file.c` | File method calls |
| `code_gen_expr_call_string.c` | String method calls |
| `code_gen_expr_call_time.c` | Time method calls |
| `code_gen_expr_static.c/h` | Static calls |
| `code_gen_expr_array.c/h` | Array expressions |
| `code_gen_expr_string.c/h` | String interpolation |
| `code_gen_stmt.c/h` | Statement codegen |
| `code_gen_util.c/h` | Utilities |

### Type Checker Modules (in compiler root)

| Module | Description |
|--------|-------------|
| `type_checker_expr.c/h` | Core expression type checking |
| `type_checker_expr_call.c/h` | Call type checking |
| `type_checker_expr_array.c/h` | Array type checking |
| `type_checker_expr_lambda.c/h` | Lambda type checking |
| `type_checker_stmt.c/h` | Statement type checking |
| `type_checker_util.c/h` | Utilities |

### Optimizer Modules (in compiler root)

| Module | Description |
|--------|-------------|
| `optimizer.c/h` | Core optimizer |
| `optimizer_util.c/h` | Utility functions |
| `optimizer_string.c/h` | String literal merging |
| `optimizer_tail_call.c/h` | Tail call optimization |

---

## Benefits

1. **Improved Maintainability**: Files average 200-900 lines instead of 1,200-5,500
2. **Faster Incremental Builds**: Modular code compiles faster
3. **Clearer Dependencies**: Explicit headers show module relationships
4. **Better Navigation**: IDE/editor tools work better with smaller files
5. **Reduced Merge Conflicts**: Less chance of conflicts on large files
6. **Self-Documenting**: File names clearly indicate purpose
