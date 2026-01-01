# Sn Compiler Project Restructuring

This document tracks the restructuring of the Sn compiler project layout and test suite organization.

## Status

| Phase | Description | Status |
|-------|-------------|--------|
| Phase 0 | Root directory restructuring (`compiler/` → `src/`, tests to root) | :black_square_button: PENDING |
| Phase 1 | Reorganize unit tests into module subfolders | :black_square_button: PENDING |
| Phase 2 | Reorganize integration tests by feature area | :black_square_button: PENDING |
| Phase 3 | Reorganize exploratory tests by feature area | :black_square_button: PENDING |

---

## Phase 0: Root Directory Restructuring

Adopt conventional project layout by renaming `compiler/` to `src/` and moving tests to the project root.

### Current Structure

```
sn/
├── compiler/                      # Source code
│   ├── main.c
│   ├── ast.c/h
│   ├── lexer.c/h
│   ├── ...
│   ├── runtime/
│   └── tests/                     # Tests nested inside source
│       ├── _all_.c
│       ├── *_tests.c
│       ├── integration/
│       └── exploratory/
├── samples/
├── bin/
├── log/
├── Makefile
└── *.md
```

### Target Structure

```
sn/
├── src/                           # Was: compiler/
│   ├── main.c
│   ├── ast.c/h
│   ├── ast/
│   ├── lexer.c/h
│   ├── lexer/
│   ├── parser.c/h
│   ├── parser/
│   ├── type_checker.c/h
│   ├── type_checker/
│   ├── optimizer.c/h
│   ├── optimizer/
│   ├── code_gen.c/h
│   ├── code_gen/
│   ├── runtime.c/h
│   └── runtime/
│
├── tests/                         # Was: compiler/tests/ (now at root)
│   ├── unit/
│   │   ├── _all_.c
│   │   ├── test_utils.h
│   │   ├── ast/
│   │   ├── lexer/
│   │   ├── parser/
│   │   ├── type_checker/
│   │   ├── optimizer/
│   │   ├── code_gen/
│   │   ├── runtime/
│   │   └── standalone/
│   ├── integration/
│   └── exploratory/
│
├── samples/
├── bin/
├── log/
├── Makefile
└── *.md
```

### Moves

| Current Location | New Location |
|------------------|--------------|
| `compiler/` | `src/` |
| `compiler/tests/` | `tests/` |
| `compiler/tests/*.c` | `tests/unit/*.c` (then Phase 1 subfolders) |
| `compiler/tests/integration/` | `tests/integration/` |
| `compiler/tests/exploratory/` | `tests/exploratory/` |

### Implementation Steps

1. **Move tests out first** (to avoid nested move issues)
   ```bash
   git mv compiler/tests tests
   ```

2. **Rename compiler to src**
   ```bash
   git mv compiler src
   ```

3. **Create unit test directory and move files**
   ```bash
   mkdir tests/unit
   git mv tests/_all_.c tests/test_utils.h tests/*_tests*.c tests/unit/
   ```

4. **Update root Makefile**
   - Change `compiler/` references to `src/`
   - Change `compiler/tests/` references to `tests/`
   - Update test target paths

5. **Update src/Makefile** (was compiler/Makefile)
   - Update paths for test binary compilation

6. **Update include paths in source files**
   - Any `#include` paths referencing `../` may need adjustment

7. **Update documentation**
   - `CLAUDE.md` architecture diagram
   - `REFACTOR.md` paths
   - `README.md` if present

8. **Verify**
   ```bash
   make clean && make build && make test
   ```

### Benefits

- **Convention**: `src/` and `tests/` at root is standard C project layout
- **Separation**: Clear boundary between production code and test code
- **Tooling**: Many tools expect this layout (coverage tools, IDEs, CI templates)
- **Scalability**: Room for future directories (`docs/`, `lib/`, `include/`, etc.)

---

## Phase 1: Unit Test Reorganization

Unit tests in `tests/unit/` should be organized into subfolders that mirror the compiler module structure. This follows the same pattern as the runtime module in `src/runtime/`.

### Current Structure (after Phase 0)

```
tests/unit/
├── _all_.c                        # Master orchestrator
├── test_utils.h                   # Assertion framework
├── arena_tests.c
├── ast_tests.c
├── ast_tests_expr.c
├── ast_tests_stmt.c
├── ast_tests_type.c
├── ast_tests_util.c
├── code_gen_tests.c
├── code_gen_tests_array.c
├── code_gen_tests_constfold.c
├── code_gen_tests_expr.c
├── code_gen_tests_memory.c
├── code_gen_tests_optimization.c
├── code_gen_tests_stmt.c
├── code_gen_tests_util.c
├── file_tests.c
├── lexer_tests.c
├── lexer_tests_array.c
├── lexer_tests_indent.c
├── lexer_tests_literal.c
├── lexer_tests_memory.c
├── lexer_tests_operator.c
├── optimizer_tests.c
├── parser_tests.c
├── parser_tests_array.c
├── parser_tests_basic.c
├── parser_tests_control.c
├── parser_tests_lambda.c
├── parser_tests_memory.c
├── parser_tests_program.c
├── parser_tests_static.c
├── runtime_arena_tests.c
├── symbol_table_tests.c
├── token_tests.c
└── type_checker_tests.c
```

### Target Structure

```
tests/unit/
├── _all_.c                        # Master orchestrator (updated includes)
├── test_utils.h                   # Assertion framework (stays)
│
├── ast/                           # AST unit tests
│   ├── ast_tests.c
│   ├── ast_tests_expr.c
│   ├── ast_tests_stmt.c
│   ├── ast_tests_type.c
│   └── ast_tests_util.c
│
├── lexer/                         # Lexer unit tests
│   ├── lexer_tests.c
│   ├── lexer_tests_array.c
│   ├── lexer_tests_indent.c
│   ├── lexer_tests_literal.c
│   ├── lexer_tests_memory.c
│   └── lexer_tests_operator.c
│
├── parser/                        # Parser unit tests
│   ├── parser_tests.c
│   ├── parser_tests_array.c
│   ├── parser_tests_basic.c
│   ├── parser_tests_control.c
│   ├── parser_tests_lambda.c
│   ├── parser_tests_memory.c
│   ├── parser_tests_program.c
│   └── parser_tests_static.c
│
├── type_checker/                  # Type checker unit tests
│   └── type_checker_tests.c
│
├── optimizer/                     # Optimizer unit tests
│   └── optimizer_tests.c
│
├── code_gen/                      # Code generation unit tests
│   ├── code_gen_tests.c
│   ├── code_gen_tests_array.c
│   ├── code_gen_tests_constfold.c
│   ├── code_gen_tests_expr.c
│   ├── code_gen_tests_memory.c
│   ├── code_gen_tests_optimization.c
│   ├── code_gen_tests_stmt.c
│   └── code_gen_tests_util.c
│
├── runtime/                       # Runtime unit tests
│   └── runtime_arena_tests.c
│
└── standalone/                    # Standalone module tests
    ├── arena_tests.c
    ├── file_tests.c
    ├── symbol_table_tests.c
    └── token_tests.c
```

### Modules to Reorganize

#### 1. AST Tests → `tests/unit/ast/`

| Current Location | New Location |
|------------------|--------------|
| `tests/unit/ast_tests.c` | `tests/unit/ast/ast_tests.c` |
| `tests/unit/ast_tests_expr.c` | `tests/unit/ast/ast_tests_expr.c` |
| `tests/unit/ast_tests_stmt.c` | `tests/unit/ast/ast_tests_stmt.c` |
| `tests/unit/ast_tests_type.c` | `tests/unit/ast/ast_tests_type.c` |
| `tests/unit/ast_tests_util.c` | `tests/unit/ast/ast_tests_util.c` |

#### 2. Lexer Tests → `tests/unit/lexer/`

| Current Location | New Location |
|------------------|--------------|
| `tests/unit/lexer_tests.c` | `tests/unit/lexer/lexer_tests.c` |
| `tests/unit/lexer_tests_array.c` | `tests/unit/lexer/lexer_tests_array.c` |
| `tests/unit/lexer_tests_indent.c` | `tests/unit/lexer/lexer_tests_indent.c` |
| `tests/unit/lexer_tests_literal.c` | `tests/unit/lexer/lexer_tests_literal.c` |
| `tests/unit/lexer_tests_memory.c` | `tests/unit/lexer/lexer_tests_memory.c` |
| `tests/unit/lexer_tests_operator.c` | `tests/unit/lexer/lexer_tests_operator.c` |

#### 3. Parser Tests → `tests/unit/parser/`

| Current Location | New Location |
|------------------|--------------|
| `tests/unit/parser_tests.c` | `tests/unit/parser/parser_tests.c` |
| `tests/unit/parser_tests_array.c` | `tests/unit/parser/parser_tests_array.c` |
| `tests/unit/parser_tests_basic.c` | `tests/unit/parser/parser_tests_basic.c` |
| `tests/unit/parser_tests_control.c` | `tests/unit/parser/parser_tests_control.c` |
| `tests/unit/parser_tests_lambda.c` | `tests/unit/parser/parser_tests_lambda.c` |
| `tests/unit/parser_tests_memory.c` | `tests/unit/parser/parser_tests_memory.c` |
| `tests/unit/parser_tests_program.c` | `tests/unit/parser/parser_tests_program.c` |
| `tests/unit/parser_tests_static.c` | `tests/unit/parser/parser_tests_static.c` |

#### 4. Type Checker Tests → `tests/unit/type_checker/`

| Current Location | New Location |
|------------------|--------------|
| `tests/unit/type_checker_tests.c` | `tests/unit/type_checker/type_checker_tests.c` |

#### 5. Optimizer Tests → `tests/unit/optimizer/`

| Current Location | New Location |
|------------------|--------------|
| `tests/unit/optimizer_tests.c` | `tests/unit/optimizer/optimizer_tests.c` |

#### 6. Code Generation Tests → `tests/unit/code_gen/`

| Current Location | New Location |
|------------------|--------------|
| `tests/unit/code_gen_tests.c` | `tests/unit/code_gen/code_gen_tests.c` |
| `tests/unit/code_gen_tests_array.c` | `tests/unit/code_gen/code_gen_tests_array.c` |
| `tests/unit/code_gen_tests_constfold.c` | `tests/unit/code_gen/code_gen_tests_constfold.c` |
| `tests/unit/code_gen_tests_expr.c` | `tests/unit/code_gen/code_gen_tests_expr.c` |
| `tests/unit/code_gen_tests_memory.c` | `tests/unit/code_gen/code_gen_tests_memory.c` |
| `tests/unit/code_gen_tests_optimization.c` | `tests/unit/code_gen/code_gen_tests_optimization.c` |
| `tests/unit/code_gen_tests_stmt.c` | `tests/unit/code_gen/code_gen_tests_stmt.c` |
| `tests/unit/code_gen_tests_util.c` | `tests/unit/code_gen/code_gen_tests_util.c` |

#### 7. Runtime Tests → `tests/unit/runtime/`

| Current Location | New Location |
|------------------|--------------|
| `tests/unit/runtime_arena_tests.c` | `tests/unit/runtime/runtime_arena_tests.c` |

#### 8. Standalone Module Tests → `tests/unit/standalone/`

| Current Location | New Location |
|------------------|--------------|
| `tests/unit/arena_tests.c` | `tests/unit/standalone/arena_tests.c` |
| `tests/unit/file_tests.c` | `tests/unit/standalone/file_tests.c` |
| `tests/unit/symbol_table_tests.c` | `tests/unit/standalone/symbol_table_tests.c` |
| `tests/unit/token_tests.c` | `tests/unit/standalone/token_tests.c` |

### Implementation Steps

1. **Create subfolder structure**
   ```bash
   mkdir -p tests/unit/{ast,lexer,parser,type_checker,optimizer,code_gen,runtime,standalone}
   ```

2. **Move files** - Use `git mv` to preserve history
   ```bash
   git mv tests/unit/ast_tests*.c tests/unit/ast/
   git mv tests/unit/lexer_tests*.c tests/unit/lexer/
   git mv tests/unit/parser_tests*.c tests/unit/parser/
   git mv tests/unit/type_checker_tests*.c tests/unit/type_checker/
   git mv tests/unit/optimizer_tests*.c tests/unit/optimizer/
   git mv tests/unit/code_gen_tests*.c tests/unit/code_gen/
   git mv tests/unit/runtime_arena_tests.c tests/unit/runtime/
   git mv tests/unit/arena_tests.c tests/unit/file_tests.c \
          tests/unit/symbol_table_tests.c tests/unit/token_tests.c tests/unit/standalone/
   ```

3. **Update `_all_.c`** - Change include paths
   ```c
   // Before
   #include "ast_tests.c"

   // After
   #include "ast/ast_tests.c"
   ```

4. **Update Makefile** - Adjust test source paths

5. **Verify** - `make clean && make build && make test-unit`

---

## Phase 2: Integration Test Reorganization

Integration tests in `tests/integration/` could be organized by feature area for better discoverability.

### Current Structure

53 test pairs in a flat structure:
```
integration/
├── hello_world.sn / .expected
├── factorial.sn / .expected
├── string_interp.sn / .expected
├── test_lambda_capture.sn / .expected
└── ... (49 more pairs)
```

### Proposed Structure

```
integration/
├── core/                          # Basic language features
│   ├── hello_world.sn / .expected
│   ├── conditionals.sn / .expected
│   └── loops.sn / .expected
│
├── functions/                     # Function features
│   ├── factorial.sn / .expected
│   ├── test_forward_decl.sn / .expected
│   └── test_private_function.sn / .expected
│
├── lambdas/                       # Lambda and closure features
│   ├── test_lambda_basic.sn / .expected
│   ├── test_lambda_capture.sn / .expected
│   ├── test_lambda_infer.sn / .expected
│   └── test_lambda_shared.sn / .expected
│
├── strings/                       # String features
│   ├── string_interp.sn / .expected
│   ├── string_interp_escaped.sn / .expected
│   ├── nested_interp.sn / .expected
│   └── format_specifiers.sn / .expected
│
├── arrays/                        # Array features
│   └── sized_array_syntax.sn / .expected
│
├── memory/                        # Memory management
│   ├── test_as_ref.sn / .expected
│   ├── test_as_val.sn / .expected
│   └── test_escape_analysis.sn / .expected
│
├── scoping/                       # Scope and visibility
│   ├── test_scope.sn / .expected
│   ├── test_nested_blocks.sn / .expected
│   ├── test_private_block.sn / .expected
│   └── test_shared_block.sn / .expected
│
└── optimization/                  # Optimization verification
    ├── dead_code.sn / .expected
    ├── tail_recursion.sn / .expected
    └── loop_counter_opt.sn / .expected
```

### Implementation Notes

- Update `Makefile` test-integration target to recurse into subdirectories
- Consider using `find` instead of globbing for test discovery

---

## Phase 3: Exploratory Test Reorganization

Exploratory tests (213 files) could follow a similar categorization.

### Current Structure

Numbered tests in a flat structure:
```
exploratory/
├── test_01_int_literals.sn
├── test_02_double_literals.sn
├── test_06_arithmetic_ops.sn
├── test_21_lambda_basic.sn
└── ... (209 more files)
```

### Proposed Structure

```
exploratory/
├── literals/
│   ├── test_01_int_literals.sn
│   ├── test_02_double_literals.sn
│   ├── test_03_str_literals.sn
│   ├── test_04_char_literals.sn
│   └── test_05_bool_literals.sn
│
├── operators/
│   ├── test_06_arithmetic_ops.sn
│   ├── test_07_comparison_ops.sn
│   ├── test_08_logical_ops.sn
│   └── test_10_operator_precedence.sn
│
├── control_flow/
│   ├── test_11_if_else.sn
│   ├── test_12_while_loops.sn
│   ├── test_13_for_loops.sn
│   └── test_14_foreach_loops.sn
│
├── functions/
├── lambdas/
├── arrays/
├── strings/
└── memory/
```

### Implementation Notes

- Remove numeric prefixes after reorganization (numbers were for ordering in flat structure)
- Update `Makefile` test-explore target to recurse into subdirectories

---

## Benefits

1. **Consistency**: Test structure mirrors code structure from REFACTOR.md
2. **Discoverability**: Easy to find tests for a specific module
3. **Maintainability**: Related tests grouped together
4. **Scalability**: Clear location for new tests as modules grow
5. **IDE Navigation**: Better file tree organization

---

## Dependencies

- **Prerequisite**: REFACTOR.md Phase 2 must be complete before starting this work
- **Phase 0** should be done first (root restructuring)
- **Phase 1** depends on Phase 0 completing
- **Phases 2 and 3** are optional improvements with lower priority

This is a standalone activity, separate from REFACTOR.md.

---

## Alternatives Considered

### Ceedling (Unity + CMock)

Evaluated adopting [Ceedling](https://github.com/ThrowTheSwitch/Ceedling) testing framework:

**Pros:**
- Industry-standard Unity assertion framework
- CMock for mocking/stubbing dependencies
- Automatic test discovery
- Built-in code coverage plugins

**Cons:**
- Requires Ruby 3+ as build dependency
- Migration effort to convert existing tests
- Additional build system complexity

**Decision:** Keep existing custom framework (`test_utils.h`). The structural reorganization provides the main benefits without introducing new dependencies.
