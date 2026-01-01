# Sn Compiler Refactoring

This document tracks the comprehensive restructuring of the Sn compiler codebase to improve maintainability, readability, and modularity.

## Refactoring Status: ✅ COMPLETE

All planned refactoring tasks have been successfully completed. The codebase has been modularized into smaller, focused files with clear responsibilities.

---

## Completed Changes Summary

### 1. Runtime Module Split (5,557 → 9 modules)

The monolithic `runtime.c` has been split into focused modules:

| Module | Lines | Description |
|--------|-------|-------------|
| `runtime/runtime_arena.c` | 306 | Arena memory management |
| `runtime/runtime_string.c` | 873 | String operations and formatting |
| `runtime/runtime_array.c` | 1,258 | Array operations (push, pop, slice, etc.) |
| `runtime/runtime_text_file.c` | 945 | TextFile I/O operations |
| `runtime/runtime_binary_file.c` | 777 | BinaryFile I/O operations |
| `runtime/runtime_io.c` | 152 | Stdin/Stdout/Stderr operations |
| `runtime/runtime_path.c` | 533 | Path utilities and directory operations |
| `runtime/runtime_time.c` | 507 | Time functions and formatting |
| `runtime/runtime_byte.c` | 315 | Byte array conversions (hex, base64) |

**Headers created:**
- `runtime_arena.h` (101 lines) - Arena type definitions and functions
- `runtime_string.h` (160 lines) - String function declarations
- `runtime_array.h` (247 lines) - Array type and function declarations
- `runtime_file.h` (261 lines) - TextFile and BinaryFile types
- `runtime_io.h` (79 lines) - Standard stream declarations
- `runtime_path.h` (72 lines) - Path utility declarations
- `runtime_time.h` (116 lines) - Time type and functions
- `runtime_byte.h` (42 lines) - Byte conversion declarations
- `runtime_internal.h` (61 lines) - Shared internal declarations

### 2. Code Generation Expression Split (3,419 → 9 modules)

The massive `code_gen_expr.c` has been modularized:

| Module | Lines | Description |
|--------|-------|-------------|
| `code_gen_expr.c` | 1,941 | Core expression code generation |
| `code_gen_expr_lambda.c` | 883 | Lambda expressions and captures |
| `code_gen_expr_call_array.c` | 554 | Array method code generation |
| `code_gen_expr_call_file.c` | 519 | TextFile/BinaryFile methods |
| `code_gen_expr_static.c` | 331 | Static method calls |
| `code_gen_expr_array.c` | 223 | Array literals and access |
| `code_gen_expr_string.c` | 209 | String interpolation |
| `code_gen_expr_call_string.c` | 207 | String method calls |
| `code_gen_expr_call_time.c` | 121 | Time method calls |

**Headers created:**
- `code_gen_expr_call.h` (116 lines) - Call expression dispatching
- `code_gen_expr_lambda.h` (80 lines) - Lambda helper structures
- `code_gen_expr_array.h` (14 lines) - Array expression declarations
- `code_gen_expr_static.h` (19 lines) - Static call declarations
- `code_gen_expr_string.h` (24 lines) - String expression declarations

### 3. Type Checker Expression Split (2,481 → 4 modules)

The `type_checker_expr.c` has been split for better organization:

| Module | Lines | Description |
|--------|-------|-------------|
| `type_checker_expr.c` | 487 | Core expression type checking |
| `type_checker_expr_call.c` | 2,062 | Method call type checking |
| `type_checker_expr_array.c` | 283 | Array type checking |
| `type_checker_expr_lambda.c` | 133 | Lambda type checking |

**Headers created:**
- `type_checker_expr_call.h` (45 lines) - Call type checking declarations
- `type_checker_expr_array.h` (53 lines) - Array type checking declarations
- `type_checker_expr_lambda.h` (21 lines) - Lambda type checking declarations

### 4. Optimizer Split (1,282 → 4 modules)

The `optimizer.c` has been split by optimization pass:

| Module | Lines | Description |
|--------|-------|-------------|
| `optimizer.c` | 200 | Core optimizer and dead code elimination |
| `optimizer_util.c` | 659 | Utility functions and variable tracking |
| `optimizer_string.c` | 281 | String literal merging |
| `optimizer_tail_call.c` | 171 | Tail call optimization |

**Headers created:**
- `optimizer_util.h` (75 lines) - Utility function declarations
- `optimizer_string.h` (29 lines) - String optimization declarations
- `optimizer_tail_call.h` (43 lines) - Tail call declarations

---

## New Module Structure

```
compiler/
├── main.c                     # Entry point
├── compiler.c/h               # Orchestration
│
├── runtime/                   # Runtime library (9 modules)
│   ├── runtime.h              # Main header (includes all sub-headers)
│   ├── runtime_internal.h     # Shared internal declarations
│   ├── runtime_arena.c/h      # Arena memory management
│   ├── runtime_string.c/h     # String operations
│   ├── runtime_array.c/h      # Array operations
│   ├── runtime_file.h         # File type definitions
│   ├── runtime_text_file.c    # TextFile operations
│   ├── runtime_binary_file.c  # BinaryFile operations
│   ├── runtime_io.c/h         # Standard streams
│   ├── runtime_path.c/h       # Path and directory ops
│   ├── runtime_time.c/h       # Time operations
│   └── runtime_byte.c/h       # Byte conversions
│
├── # Code Generation Expression (9 modules)
├── code_gen_expr.c/h              # Core expressions
├── code_gen_expr_lambda.c/h       # Lambda expressions
├── code_gen_expr_call.h           # Call dispatch header
├── code_gen_expr_call_array.c     # Array method calls
├── code_gen_expr_call_file.c      # File method calls
├── code_gen_expr_call_string.c    # String method calls
├── code_gen_expr_call_time.c      # Time method calls
├── code_gen_expr_static.c/h       # Static method calls
├── code_gen_expr_array.c/h        # Array expressions
├── code_gen_expr_string.c/h       # String interpolation
│
├── # Type Checker Expression (4 modules)
├── type_checker_expr.c/h          # Core type checking
├── type_checker_expr_call.c/h     # Method call types
├── type_checker_expr_array.c/h    # Array types
├── type_checker_expr_lambda.c/h   # Lambda types
│
├── # Optimizer (4 modules)
├── optimizer.c/h                  # Core optimizer
├── optimizer_util.c/h             # Utilities
├── optimizer_string.c/h           # String merging
└── optimizer_tail_call.c/h        # Tail call optimization
```

---

## Benefits Achieved

1. **Improved Maintainability**: Files now average 200-900 lines instead of 1,200-5,500
2. **Better Compilation**: Incremental builds are faster with modular code
3. **Clearer Dependencies**: Explicit headers show module relationships
4. **Easier Testing**: Individual modules can be tested in isolation
5. **Better Navigation**: IDE/editor tools work better with smaller files
6. **Reduced Conflicts**: Less chance of merge conflicts on large files
7. **Self-Documenting**: File names clearly indicate purpose

---

## Line Count Summary

### Before Refactoring
| File | Lines |
|------|-------|
| `runtime.c` | 5,557 |
| `code_gen_expr.c` | 3,419 |
| `type_checker_expr.c` | 2,481 |
| `optimizer.c` | 1,282 |
| **Total (critical files)** | **12,739** |

### After Refactoring
| Category | Modules | Total Lines |
|----------|---------|-------------|
| Runtime | 9 .c files, 9 .h files | 5,666 + 1,139 headers |
| Code Gen Expression | 9 .c files, 6 .h files | 4,988 + 284 headers |
| Type Checker Expression | 4 .c files, 4 .h files | 2,965 + 129 headers |
| Optimizer | 4 .c files, 4 .h files | 1,311 + 241 headers |

**Result:** Large monolithic files split into 26 focused modules with appropriate headers.

---

## Original Planning Reference

The sections below document the original planning that guided this refactoring effort.

### Original File Size Overview

| File | Lines | Status |
|------|-------|--------|
| `runtime.c` | 5,557 | ✅ Split into 9 modules |
| `code_gen_expr.c` | 3,419 | ✅ Split into 9 modules |
| `type_checker_expr.c` | 2,481 | ✅ Split into 4 modules |
| `optimizer.c` | 1,282 | ✅ Split into 4 modules |
| `code_gen_tests_array.c` | 1,708 | Test file - acceptable |
| `code_gen_stmt.c` | 1,276 | Acceptable |
| `code_gen_util.c` | 1,154 | Acceptable |
| `code_gen_tests_optimization.c` | 1,142 | Test file - acceptable |
| `type_checker_tests_array.c` | 1,031 | Test file - acceptable |
| `parser_tests_control.c` | 1,006 | Test file - acceptable |
| `parser_stmt.c` | 951 | Acceptable |
| `optimizer_tests.c` | 892 | Test file - acceptable |
| `parser_expr.c` | 870 | Acceptable |
| `runtime.h` | 850 | ✅ Now includes sub-headers |

---

## 1. runtime.c Restructuring (5,557 lines) - ✅ COMPLETE

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

### Implemented Split

```
compiler/runtime/
├── runtime.h              # Main header (includes all sub-headers)
├── runtime_internal.h     # Shared internal declarations
│
├── runtime_arena.h        # Arena type definitions
├── runtime_arena.c        # Arena memory management (~306 lines)
│
├── runtime_string.h       # String function declarations
├── runtime_string.c       # String operations (~873 lines)
│
├── runtime_array.h        # Array type and function declarations
├── runtime_array.c        # Array operations (~1,258 lines)
│
├── runtime_file.h         # TextFile and BinaryFile type definitions
├── runtime_text_file.c    # TextFile operations (~945 lines)
├── runtime_binary_file.c  # BinaryFile operations (~777 lines)
│
├── runtime_io.h           # Standard stream declarations
├── runtime_io.c           # Stdin/Stdout/Stderr operations (~152 lines)
│
├── runtime_path.h         # Path utility declarations
├── runtime_path.c         # Path and directory operations (~533 lines)
│
├── runtime_time.h         # Time type and function declarations
├── runtime_time.c         # Time operations (~507 lines)
│
└── runtime_byte.h/c       # Byte array and encoding (~315 lines)
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
    ├── runtime_time.h (depends on arena)
    └── runtime_byte.h
```

---

## 2. code_gen_expr.c Restructuring (3,419 lines) - ✅ COMPLETE

### Original Structure

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

### Implemented Split

```
compiler/
├── code_gen_expr.h            # Expression codegen declarations
├── code_gen_expr.c            # Core expressions (~1,941 lines)
│
├── code_gen_expr_call.h       # Call expression declarations
├── code_gen_expr_call_array.c # Array method codegen (~554 lines)
├── code_gen_expr_call_file.c  # File method codegen (~519 lines)
├── code_gen_expr_call_string.c # String method codegen (~207 lines)
├── code_gen_expr_call_time.c  # Time method codegen (~121 lines)
│
├── code_gen_expr_static.c/h   # Static calls (~331 lines)
├── code_gen_expr_lambda.c/h   # Lambda expressions (~883 lines)
├── code_gen_expr_array.c/h    # Array expressions (~223 lines)
└── code_gen_expr_string.c/h   # String interpolation (~209 lines)
```

---

## 3. type_checker_expr.c Restructuring (2,481 lines) - ✅ COMPLETE

### Implemented Split

```
compiler/
├── type_checker_expr.h        # Expression type checking declarations
├── type_checker_expr.c        # Core expression type checking (~487 lines)
│
├── type_checker_expr_call.c/h # Call expression type checking (~2,062 lines)
├── type_checker_expr_array.c/h # Array type checking (~283 lines)
└── type_checker_expr_lambda.c/h # Lambda type checking (~133 lines)
```

---

## 4. optimizer.c Restructuring (1,282 lines) - ✅ COMPLETE

### Original Structure

| Function Group | Line Range | Lines | Description |
|----------------|------------|-------|-------------|
| Core utilities | 1-78 | 78 | Init, stats, terminator check |
| Noop detection | 79-232 | 154 | `is_literal_zero`, `is_literal_one`, `expr_is_noop` |
| Variable tracking | 233-475 | 243 | `add_used_variable`, `collect_used_variables*`, `is_variable_used` |
| Dead code elimination | 476-851 | 376 | `remove_unreachable_statements`, `remove_unused_variables`, `simplify_noop_*` |
| Tail call optimization | 852-1021 | 170 | `is_tail_recursive_return`, `function_has_tail_recursion`, etc. |
| String merging | 1022-1282 | 261 | `merge_interpolated_parts`, `optimize_string_*` |

### Implemented Split

```
compiler/
├── optimizer.h                # Main optimizer declarations
├── optimizer.c                # Core optimizer (~200 lines)
│
├── optimizer_util.h           # Utility declarations
├── optimizer_util.c           # Utility functions (~659 lines)
│
├── optimizer_tail_call.h      # Tail call declarations
├── optimizer_tail_call.c      # Tail call optimization (~171 lines)
│
├── optimizer_string.h         # String optimization declarations
└── optimizer_string.c         # String literal optimization (~281 lines)
```

---

## 5. Makefile Updates - ✅ COMPLETE

The `compiler/Makefile` has been updated to include all new source files:

**SRCS additions:**
- `optimizer_util.c`, `optimizer_tail_call.c`, `optimizer_string.c`
- `code_gen_expr_lambda.c`, `code_gen_expr_call_array.c`, `code_gen_expr_call_file.c`
- `code_gen_expr_call_string.c`, `code_gen_expr_call_time.c`
- `code_gen_expr_array.c`, `code_gen_expr_static.c`, `code_gen_expr_string.c`
- `type_checker_expr_call.c`, `type_checker_expr_array.c`, `type_checker_expr_lambda.c`
- Runtime modules in `runtime/` directory

**HEADERS additions:**
- All corresponding `.h` header files

**TEST_TARGET updates:**
- All new `.o` object files added to test linking

---

## Verification

All changes have been verified with:
- `make clean && make build` - Successful compilation with no warnings
- `make test-unit` - All unit tests pass (100%)
- `make test-integration` - 52 passed, 0 failed, 1 skipped (100% pass rate)
- `make test-explore` - All 208 exploratory tests pass (100%)

---

## Final Completion Notes

**Refactoring completed:** January 2026

**Summary of achievements:**
1. Split 4 monolithic files totaling 12,739 lines into 26 focused modules
2. Created 23 new header files with proper declarations
3. Maintained 100% backward compatibility - all tests pass
4. No warnings during compilation with `-Wall -Wextra`
5. Incremental build times improved due to modular structure

**Module count by category:**
- Runtime: 9 implementation files + 9 headers
- Code Generation Expression: 9 implementation files + 6 headers
- Type Checker Expression: 4 implementation files + 4 headers
- Optimizer: 4 implementation files + 4 headers

**Total new files created:** 49 files (26 .c files + 23 .h files)

All verification criteria met. The refactoring is complete and production-ready.
