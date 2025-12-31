# Sn Compiler Refactoring Proposal

This document outlines a comprehensive restructuring plan for the Sn compiler codebase to improve maintainability, readability, and modularity.

## Current State Analysis

### File Size Overview

| File | Lines | Status |
|------|-------|--------|
| `runtime.c` | 5,557 | **Critical - needs splitting** |
| `code_gen_expr.c` | 3,419 | **Critical - needs splitting** |
| `type_checker_expr.c` | 2,481 | **High priority** |
| `code_gen_tests_array.c` | 1,708 | Test file - acceptable |
| `optimizer.c` | 1,282 | Medium priority |
| `code_gen_stmt.c` | 1,276 | Medium priority |
| `code_gen_util.c` | 1,154 | Medium priority |
| `code_gen_tests_optimization.c` | 1,142 | Test file - acceptable |
| `type_checker_tests_array.c` | 1,031 | Test file - acceptable |
| `parser_tests_control.c` | 1,006 | Test file - acceptable |
| `parser_stmt.c` | 951 | Acceptable |
| `optimizer_tests.c` | 892 | Test file - acceptable |
| `parser_expr.c` | 870 | Acceptable |
| `runtime.h` | 850 | Medium priority |

**Total codebase:** ~44,840 lines

---

## 1. runtime.c Restructuring (5,557 lines)

### Current Structure

The file contains well-marked sections with `/* ============ */` comment blocks:

| Section | Line Range | Approx Lines | Description |
|---------|------------|--------------|-------------|
| Arena Memory Management | 15-264 | 250 | `rt_arena_*` functions |
| File Handle Tracking | 265-378 | 114 | Arena file tracking |
| String Functions | 379-788 | 410 | `rt_str_concat`, `rt_to_string_*`, `rt_format_*`, `rt_print_*` |
| Array Operations | 789-2161 | 1,373 | All `rt_array_*` functions |
| String Manipulation | 2162-2409 | 248 | `rt_str_substring`, `rt_str_split`, `rt_str_trim`, etc. |
| TextFile Static Methods | 2410-2655 | 246 | `rt_text_file_exists`, `rt_text_file_read_all`, etc. |
| TextFile Instance Reading | 2656-2922 | 267 | `rt_text_file_read_word`, `rt_text_file_read_line`, etc. |
| TextFile Instance Writing | 2923-3015 | 93 | `rt_text_file_write`, `rt_text_file_write_line`, etc. |
| TextFile State Methods | 3016-3185 | 170 | `rt_text_file_has_chars`, `rt_text_file_is_eof`, etc. |
| TextFile Properties | 3186-3294 | 109 | `rt_text_file_get_path`, `rt_text_file_get_name`, etc. |
| BinaryFile Static Methods | 3295-3569 | 275 | `rt_binary_file_exists`, `rt_binary_file_read_all`, etc. |
| BinaryFile Instance Reading | 3570-3735 | 166 | Binary read operations |
| BinaryFile Instance Writing | 3736-3784 | 49 | Binary write operations |
| BinaryFile State Methods | 3785-3906 | 122 | EOF, position methods |
| BinaryFile Properties | 3907-4015 | 109 | Path, name, size properties |
| Standard Streams | 4016-4145 | 130 | Stdin, Stdout, Stderr operations |
| Byte Array Extensions | 4146-4289 | 144 | Byte array methods |
| String to Byte Conversions | 4290-4449 | 160 | Encoding conversions |
| Path Utilities | 4450-4673 | 224 | Path manipulation |
| Directory Operations | 4674-4967 | 294 | Directory listing, creation |
| String Splitting Methods | 4968-5077 | 110 | Additional split methods |
| Time Methods | 5078-5459 | 382 | `rt_time_*` functions |
| Time Arithmetic | 5460-5512 | 53 | Time add/subtract |
| Time Comparison | 5513-5557 | 45 | Time comparison functions |

### Proposed Split

```
compiler/runtime/
├── runtime.h              # Main header (includes all sub-headers)
├── runtime_internal.h     # Shared internal declarations
│
├── runtime_arena.h        # Arena type definitions
├── runtime_arena.c        # Arena memory management (~250 lines)
│
├── runtime_string.h       # String function declarations
├── runtime_string.c       # String operations (~650 lines)
│                          # - rt_str_concat, rt_str_substring, rt_str_split
│                          # - rt_str_trim, rt_str_toUpper, rt_str_toLower
│                          # - rt_str_startsWith, rt_str_endsWith, rt_str_contains
│                          # - rt_str_replace, rt_to_string_*, rt_format_*, rt_print_*
│
├── runtime_array.h        # Array type and function declarations
├── runtime_array.c        # Array operations (~1,400 lines)
│                          # - rt_array_push_*, rt_array_pop_*
│                          # - rt_array_concat_*, rt_array_slice_*
│                          # - rt_array_rev_*, rt_array_rem_*, rt_array_ins_*
│                          # - rt_array_contains_*, rt_array_clone_*
│                          # - rt_array_join_*, rt_array_create_*
│                          # - rt_array_alloc_*, rt_array_eq_*
│                          # - rt_print_array_*, rt_array_clear
│
├── runtime_file.h         # TextFile and BinaryFile type definitions
│
├── runtime_text_file.c    # TextFile operations (~900 lines)
│                          # - Static: exists, read_all, write_all, delete, copy, move
│                          # - Instance reading: read_char, read_word, read_line, read_all
│                          # - Instance writing: write, write_line, write_char
│                          # - State: has_chars, has_words, has_lines, is_eof
│                          # - Properties: get_path, get_name, get_size, get_position
│                          # - Control: open, close, seek, rewind, flush
│
├── runtime_binary_file.c  # BinaryFile operations (~700 lines)
│                          # - Static: exists, read_all, write_all, delete, copy, move
│                          # - Instance reading: read_byte, read_bytes, read_*_le/be
│                          # - Instance writing: write_byte, write_bytes, write_*_le/be
│                          # - State: is_eof, get_position
│                          # - Properties: get_path, get_name, get_size
│                          # - Control: open, close, seek, rewind, flush
│
├── runtime_io.h           # Standard stream declarations
├── runtime_io.c           # Stdin/Stdout/Stderr operations (~200 lines)
│                          # - rt_stdin_*, rt_stdout_*, rt_stderr_*
│
├── runtime_path.h         # Path utility declarations
├── runtime_path.c         # Path and directory operations (~550 lines)
│                          # - Path: combine, get_directory, get_filename, get_extension
│                          # - Directory: list, create, delete, exists
│
├── runtime_time.h         # Time type and function declarations
├── runtime_time.c         # Time operations (~480 lines)
│                          # - Creation: now, from_*, create
│                          # - Properties: year, month, day, hour, minute, second, etc.
│                          # - Formatting: format, to_string
│                          # - Arithmetic: add_*, subtract_*
│                          # - Comparison: equals, compare, before, after
│
└── runtime_byte.c         # Byte array and encoding (~300 lines)
                           # - Byte array extensions
                           # - String to byte conversions (UTF-8, ASCII, etc.)
```

### Header Dependency Graph

```
runtime.h (main include)
    ├── runtime_arena.h
    ├── runtime_string.h (depends on arena)
    ├── runtime_array.h (depends on arena)
    ├── runtime_file.h (depends on arena)
    ├── runtime_io.h
    ├── runtime_path.h (depends on arena)
    └── runtime_time.h (depends on arena)
```

---

## 2. code_gen_expr.c Restructuring (3,419 lines)

### Current Structure

| Function | Line Range | Lines | Description |
|----------|------------|-------|-------------|
| Helper structs/functions | 1-508 | 508 | CapturedVars, LocalVars, utility functions |
| `code_gen_binary_expression` | 509-606 | 98 | Binary operators |
| `code_gen_unary_expression` | 607-648 | 42 | Unary operators |
| `code_gen_literal_expression` | 649-685 | 37 | Literals |
| `code_gen_variable_expression` | 686-711 | 26 | Variable access |
| `code_gen_assign_expression` | 712-745 | 34 | Assignment |
| `code_gen_index_assign_expression` | 746-804 | 59 | Index assignment |
| `code_gen_interpolated_expression` | 805-956 | 152 | String interpolation |
| **`code_gen_call_expression`** | 957-2203 | **1,247** | **Method calls - CRITICAL** |
| `code_gen_array_expression` | 2204-2304 | 101 | Array literals |
| `code_gen_array_access_expression` | 2305-2334 | 30 | Array indexing |
| `code_gen_increment_expression` | 2335-2351 | 17 | Increment |
| `code_gen_decrement_expression` | 2352-2368 | 17 | Decrement |
| `code_gen_member_expression` | 2369-2425 | 57 | Member access |
| `code_gen_range_expression` | 2426-2436 | 11 | Range expressions |
| `code_gen_spread_expression` | 2437-2444 | 8 | Spread operator |
| `code_gen_array_slice_expression` | 2445-2492 | 48 | Array slicing |
| `code_gen_lambda_stmt_body` | 2493-2529 | 37 | Lambda body |
| `code_gen_lambda_expression` | 2530-3000 | 471 | Lambda expressions |
| `code_gen_static_call_expression` | 3001-3318 | 318 | Static method calls |
| `code_gen_sized_array_alloc_expression` | 3319-3367 | 49 | Sized array allocation |
| `code_gen_expression` | 3368-3419 | 52 | Main dispatch |

### Critical Issue: `code_gen_call_expression`

This 1,247-line function handles all method calls on objects:
- Array methods: `push`, `pop`, `clear`, `contains`, `join`, `clone`, `rev`, `rem`, `ins`, etc.
- String methods: Various string operations
- TextFile methods: `read`, `write`, `readLine`, `writeLine`, etc.
- BinaryFile methods: Binary I/O operations
- Time methods: Time operations

### Proposed Split

```
compiler/codegen/
├── code_gen_expr.h            # Expression codegen declarations
├── code_gen_expr.c            # Core expressions (~500 lines)
│                              # - code_gen_expression (main dispatch)
│                              # - code_gen_binary_expression
│                              # - code_gen_unary_expression
│                              # - code_gen_literal_expression
│                              # - code_gen_variable_expression
│                              # - code_gen_assign_expression
│                              # - code_gen_index_assign_expression
│                              # - code_gen_increment_expression
│                              # - code_gen_decrement_expression
│                              # - code_gen_member_expression
│
├── code_gen_expr_call.h       # Call expression declarations
├── code_gen_expr_call.c       # Method call dispatch (~400 lines)
│                              # - code_gen_call_expression (refactored)
│                              # - Dispatch to type-specific handlers
│
├── code_gen_expr_call_array.c # Array method codegen (~400 lines)
│                              # - Array push, pop, clear, contains
│                              # - Array join, clone, rev, rem, ins
│                              # - Array concat, slice
│
├── code_gen_expr_call_file.c  # File method codegen (~350 lines)
│                              # - TextFile method calls
│                              # - BinaryFile method calls
│
├── code_gen_expr_call_string.c # String method codegen (~150 lines)
│                              # - String method calls
│
├── code_gen_expr_static.c     # Static calls (~350 lines)
│                              # - code_gen_static_call_expression
│                              # - TextFile.*, BinaryFile.*, Time.* static methods
│
├── code_gen_expr_lambda.c     # Lambda expressions (~550 lines)
│                              # - CapturedVars helpers
│                              # - LocalVars helpers
│                              # - code_gen_lambda_stmt_body
│                              # - code_gen_lambda_expression
│
├── code_gen_expr_array.c      # Array expressions (~200 lines)
│                              # - code_gen_array_expression
│                              # - code_gen_array_access_expression
│                              # - code_gen_array_slice_expression
│                              # - code_gen_range_expression
│                              # - code_gen_spread_expression
│                              # - code_gen_sized_array_alloc_expression
│
└── code_gen_expr_string.c     # String expressions (~150 lines)
                               # - code_gen_interpolated_expression
```

---

## 3. type_checker_expr.c Restructuring (2,481 lines)

### Proposed Split

```
compiler/typecheck/
├── type_checker_expr.h        # Expression type checking declarations
├── type_checker_expr.c        # Core expression type checking (~600 lines)
│                              # - Basic expressions (binary, unary, literal, variable)
│                              # - Assignment expressions
│                              # - Main dispatch function
│
├── type_checker_expr_call.c   # Call expression type checking (~800 lines)
│                              # - Function call type checking
│                              # - Method call type checking (arrays, files, strings)
│                              # - Built-in function validation
│
├── type_checker_expr_array.c  # Array type checking (~500 lines)
│                              # - Array literal type checking
│                              # - Array access type checking
│                              # - Array slice type checking
│                              # - Array method return types
│
└── type_checker_expr_lambda.c # Lambda type checking (~400 lines)
                               # - Lambda expression type checking
                               # - Captured variable type resolution
                               # - Function type construction
```

---

## 4. optimizer.c Restructuring (1,282 lines)

### Current Structure

| Function Group | Line Range | Lines | Description |
|----------------|------------|-------|-------------|
| Core utilities | 1-78 | 78 | Init, stats, terminator check |
| Noop detection | 79-232 | 154 | `is_literal_zero`, `is_literal_one`, `expr_is_noop` |
| Variable tracking | 233-475 | 243 | `add_used_variable`, `collect_used_variables*`, `is_variable_used` |
| Dead code elimination | 476-851 | 376 | `remove_unreachable_statements`, `remove_unused_variables`, `simplify_noop_*` |
| Tail call optimization | 852-1021 | 170 | `is_tail_recursive_return`, `function_has_tail_recursion`, etc. |
| String merging | 1022-1282 | 261 | `merge_interpolated_parts`, `optimize_string_*` |

### Proposed Split

```
compiler/optimizer/
├── optimizer.h                # Main optimizer declarations
├── optimizer.c                # Core optimizer (~400 lines)
│                              # - optimizer_init, optimizer_get_stats
│                              # - stmt_is_terminator
│                              # - Main optimization entry points
│                              # - remove_unreachable_statements
│
├── optimizer_util.h           # Utility declarations
├── optimizer_util.c           # Utility functions (~400 lines)
│                              # - is_literal_zero, is_literal_one
│                              # - expr_is_noop, simplify_noop_expr/stmt
│                              # - Variable tracking (add_used_variable, collect_*, is_variable_used)
│                              # - remove_unused_variables
│
├── optimizer_tail_call.c      # Tail call optimization (~200 lines)
│                              # - tokens_equal
│                              # - get_tail_call_expr
│                              # - is_tail_recursive_return
│                              # - check_stmt_for_tail_recursion
│                              # - function_has_tail_recursion
│                              # - mark_tail_calls_in_stmt
│                              # - optimizer_mark_tail_calls
│                              # - optimizer_tail_call_optimization
│
└── optimizer_string.c         # String literal optimization (~280 lines)
                               # - is_string_literal
                               # - get_string_literal_value
                               # - create_string_literal
                               # - merge_interpolated_parts
                               # - optimize_string_stmt
                               # - optimize_string_function
                               # - optimizer_merge_string_literals
```

---

## 5. Proposed Final Directory Structure

```
compiler/
├── main.c                     # Entry point
├── compiler.c/h               # Orchestration
│
├── lexer/                     # Lexical analysis (optional grouping)
│   ├── lexer.c/h
│   ├── lexer_scan.c
│   └── lexer_util.c
│
├── parser/                    # Parsing (optional grouping)
│   ├── parser.c/h
│   ├── parser_stmt.c
│   ├── parser_expr.c
│   └── parser_util.c/h
│
├── ast/                       # AST definitions (optional grouping)
│   ├── ast.h
│   ├── ast_stmt.c
│   ├── ast_expr.c/h
│   ├── ast_type.c
│   └── ast_print.c
│
├── typecheck/                 # Type checking
│   ├── type_checker.c/h
│   ├── type_checker_stmt.c/h
│   ├── type_checker_expr.c/h
│   ├── type_checker_expr_call.c
│   ├── type_checker_expr_array.c
│   ├── type_checker_expr_lambda.c
│   └── type_checker_util.c/h
│
├── optimizer/                 # Optimization passes
│   ├── optimizer.c/h
│   ├── optimizer_util.c/h
│   ├── optimizer_tail_call.c
│   └── optimizer_string.c
│
├── codegen/                   # Code generation
│   ├── code_gen.c/h
│   ├── code_gen_stmt.c/h
│   ├── code_gen_expr.c/h
│   ├── code_gen_expr_call.c/h
│   ├── code_gen_expr_call_array.c
│   ├── code_gen_expr_call_file.c
│   ├── code_gen_expr_call_string.c
│   ├── code_gen_expr_static.c
│   ├── code_gen_expr_lambda.c
│   ├── code_gen_expr_array.c
│   ├── code_gen_expr_string.c
│   └── code_gen_util.c/h
│
├── runtime/                   # Runtime library
│   ├── runtime.h
│   ├── runtime_internal.h
│   ├── runtime_arena.c/h
│   ├── runtime_string.c/h
│   ├── runtime_array.c/h
│   ├── runtime_file.h
│   ├── runtime_text_file.c
│   ├── runtime_binary_file.c
│   ├── runtime_io.c/h
│   ├── runtime_path.c/h
│   ├── runtime_time.c/h
│   └── runtime_byte.c
│
├── backend/                   # Backend (optional grouping)
│   └── gcc_backend.c/h
│
├── util/                      # Utilities (optional grouping)
│   ├── arena.c/h
│   ├── symbol_table.c/h
│   ├── diagnostic.c/h
│   └── debug.h
│
└── tests/                     # Tests (unchanged)
    ├── *_tests.c
    └── integration/
```

---

## 6. Implementation Priority

### Phase 1: Critical (Highest Impact)
1. **Split `runtime.c`** - 12% of codebase in one file
   - Start with clear section boundaries already present
   - Each section maps to a logical module

2. **Split `code_gen_expr.c`** - 1,247-line function is unmaintainable
   - Extract `code_gen_call_expression` first
   - Split by object type (array, file, string methods)

### Phase 2: High Priority
3. **Split `type_checker_expr.c`**
   - Similar structure to code_gen_expr.c
   - Benefits from parallel refactoring

4. **Update `runtime.h`** to match runtime.c split
   - Create sub-headers for each module

### Phase 3: Medium Priority
5. **Split `optimizer.c`**
   - Clean separation by optimization pass

6. **Consider directory grouping**
   - Optional: group related files into subdirectories

---

## 7. Migration Strategy

### Step 1: Create New Files (No Breaking Changes)
- Extract code into new files
- Keep original files with `#include` of new files temporarily
- Verify compilation works

### Step 2: Update Build System
- Modify `Makefile` to compile new source files
- Add new object files to link step

### Step 3: Update Headers
- Create new header files for split modules
- Update `#include` statements throughout codebase

### Step 4: Remove Temporary Includes
- Delete forwarding includes from original files
- Original files become the "core" modules

### Step 5: Verify
- Run full test suite: `make test`
- Verify all integration tests pass
- Check for any missing symbols or link errors

---

## 8. Makefile Changes Required

Current object file list will need expansion:

```makefile
# Current (partial)
OBJS = $(BUILD_DIR)/main.o \
       $(BUILD_DIR)/runtime.o \
       $(BUILD_DIR)/code_gen_expr.o \
       ...

# After refactoring (partial)
OBJS = $(BUILD_DIR)/main.o \
       $(BUILD_DIR)/runtime/runtime_arena.o \
       $(BUILD_DIR)/runtime/runtime_string.o \
       $(BUILD_DIR)/runtime/runtime_array.o \
       $(BUILD_DIR)/runtime/runtime_text_file.o \
       $(BUILD_DIR)/runtime/runtime_binary_file.o \
       $(BUILD_DIR)/runtime/runtime_io.o \
       $(BUILD_DIR)/runtime/runtime_path.o \
       $(BUILD_DIR)/runtime/runtime_time.o \
       $(BUILD_DIR)/runtime/runtime_byte.o \
       $(BUILD_DIR)/codegen/code_gen_expr.o \
       $(BUILD_DIR)/codegen/code_gen_expr_call.o \
       $(BUILD_DIR)/codegen/code_gen_expr_call_array.o \
       $(BUILD_DIR)/codegen/code_gen_expr_call_file.o \
       $(BUILD_DIR)/codegen/code_gen_expr_call_string.o \
       $(BUILD_DIR)/codegen/code_gen_expr_static.o \
       $(BUILD_DIR)/codegen/code_gen_expr_lambda.o \
       $(BUILD_DIR)/codegen/code_gen_expr_array.o \
       $(BUILD_DIR)/codegen/code_gen_expr_string.o \
       ...
```

---

## 9. Estimated Effort

| Task | Files Created | Estimated Changes |
|------|---------------|-------------------|
| Split runtime.c | 10 new .c files, 8 new .h files | ~200 include updates |
| Split code_gen_expr.c | 8 new .c files, 2 new .h files | ~50 include updates |
| Split type_checker_expr.c | 4 new .c files, 1 new .h file | ~30 include updates |
| Split optimizer.c | 4 new .c files, 2 new .h files | ~20 include updates |
| Makefile updates | 1 file | ~50 lines |
| **Total** | **~26 new .c files, ~13 new .h files** | **~350 changes** |

---

## 10. Benefits

1. **Improved Maintainability**: Smaller, focused files are easier to understand and modify
2. **Better Compilation**: Changes to one module don't require recompiling others
3. **Clearer Dependencies**: Explicit headers show what each module needs
4. **Easier Testing**: Can unit test individual modules more easily
5. **Better Code Navigation**: IDE/editor navigation works better with smaller files
6. **Reduced Merge Conflicts**: Multiple developers less likely to conflict on same file
7. **Documentation**: File names self-document what code does

---

## Appendix A: Line Count by Proposed Module

### Runtime Modules
| Module | Est. Lines |
|--------|------------|
| runtime_arena.c | 250 |
| runtime_string.c | 650 |
| runtime_array.c | 1,400 |
| runtime_text_file.c | 900 |
| runtime_binary_file.c | 700 |
| runtime_io.c | 200 |
| runtime_path.c | 550 |
| runtime_time.c | 480 |
| runtime_byte.c | 300 |
| **Total** | **5,430** |

### Code Generation Expression Modules
| Module | Est. Lines |
|--------|------------|
| code_gen_expr.c | 500 |
| code_gen_expr_call.c | 400 |
| code_gen_expr_call_array.c | 400 |
| code_gen_expr_call_file.c | 350 |
| code_gen_expr_call_string.c | 150 |
| code_gen_expr_static.c | 350 |
| code_gen_expr_lambda.c | 550 |
| code_gen_expr_array.c | 200 |
| code_gen_expr_string.c | 150 |
| **Total** | **3,050** |

### Type Checker Expression Modules
| Module | Est. Lines |
|--------|------------|
| type_checker_expr.c | 600 |
| type_checker_expr_call.c | 800 |
| type_checker_expr_array.c | 500 |
| type_checker_expr_lambda.c | 400 |
| **Total** | **2,300** |

### Optimizer Modules
| Module | Est. Lines |
|--------|------------|
| optimizer.c | 400 |
| optimizer_util.c | 400 |
| optimizer_tail_call.c | 200 |
| optimizer_string.c | 280 |
| **Total** | **1,280** |
