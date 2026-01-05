# Large Files Analysis

Analysis of source files over 300 lines with strategies for splitting into smaller, more maintainable modules.

## Summary

| Priority | Files | Total Lines |
|----------|-------|-------------|
| Critical (1000+) | 9 | ~15,500 |
| Medium (500-1000) | 11 | ~9,500 |
| Low (300-500) | 10 | ~3,800 |

## Critical Priority (1000+ lines)

| File | Lines | Suggested Splits |
|------|-------|------------------|
| `type_checker_expr_call.c` | 3544 | Split by type |
| `code_gen_expr.c` | 3094 | Split by expression type |
| `runtime_random.c` | 1966 | Split by operation type |
| `code_gen_stmt.c` | 1489 | Split by statement type |
| `parser_stmt.c` | 1428 | Split by statement type |
| `symbol_table.c` | 1327 | Split by concern |
| `runtime_array.c` | 1312 | Split by element type |
| `code_gen_util.c` | 1232 | Split by utility category |
| `type_checker_expr.c` | 1106 | Split by expression type |

---

## Detailed Splitting Strategies

### 1. `type_checker_expr_call.c` (3544 lines) → 7 files

This file has clear separation by type method. Current structure:

```
Line 21:   is_builtin_name, token_equals (utilities)
Line 40:   type_check_call_expression (main dispatcher)
Line 341:  type_check_array_method
Line 516:  type_check_string_method
Line 712:  type_check_text_file_method
Line 957:  type_check_binary_file_method
Line 1139: type_check_time_method
Line 1381: type_check_date_method
Line 1660: type_check_process_method
Line 1709: type_check_tcp_listener_method
Line 1760: type_check_tcp_stream_method
Line 1853: type_check_udp_socket_method
Line 1927: type_check_random_method
Line 2071: type_check_static_method_call
```

**Proposed split:**

| New File | Functions | Est. Lines |
|----------|-----------|------------|
| `type_checker_expr_call_array.c` | `type_check_array_method` | ~175 |
| `type_checker_expr_call_string.c` | `type_check_string_method` | ~200 |
| `type_checker_expr_call_file.c` | `type_check_text_file_method`, `type_check_binary_file_method` | ~380 |
| `type_checker_expr_call_time.c` | `type_check_time_method`, `type_check_date_method` | ~520 |
| `type_checker_expr_call_net.c` | `type_check_tcp_*`, `type_check_udp_*` methods | ~250 |
| `type_checker_expr_call_random.c` | `type_check_random_method` | ~150 |
| `type_checker_expr_call.c` | Core dispatch, `type_check_call_expression`, utilities | ~400 |

---

### 2. `code_gen_expr.c` (3094 lines) → 4 files

Current structure:

```
Line 46:   expression_produces_temp
Line 99:   code_gen_binary_expression
Line 207:  code_gen_unary_expression
Line 249:  code_gen_literal_expression
Line 286:  code_gen_variable_expression
Line 312:  code_gen_assign_expression
Line 346:  code_gen_index_assign_expression
Line 380:  code_gen_call_expression (~1650 lines!)
Line 2031: code_gen_increment_expression
Line 2048: code_gen_decrement_expression
Line 2065: code_gen_member_expression
Line 2172: code_gen_range_expression
Line 2183: code_gen_spread_expression
Line 2269: code_gen_thread_spawn_expression
Line 2788: code_gen_thread_sync_expression
Line 2952: code_gen_sized_array_alloc_expression
Line 3006: code_gen_as_val_expression
Line 3033: code_gen_expression (main dispatcher)
```

**Proposed split:**

| New File | Functions | Est. Lines |
|----------|-----------|------------|
| `code_gen_expr_thread.c` | `code_gen_thread_spawn_expression`, `code_gen_thread_sync_expression` | ~700 |
| `code_gen_expr_binary.c` | Binary, unary, literal, variable, assign expressions | ~300 |
| `code_gen_expr_call.c` | `code_gen_call_expression` | ~1650 |
| `code_gen_expr.c` | Dispatch + remaining expressions | ~450 |

**Note:** The `code_gen_call_expression` function itself is ~1650 lines and should be further split by called type (following existing pattern of `code_gen_expr_call_array.c`, `code_gen_expr_call_file.c`, etc.).

---

### 3. `runtime_random.c` (1966 lines) → 5 files

Current structure:

```
Line 27:   rt_random_fill_entropy
Line 130:  rotl, xoshiro256_next, splitmix64_next (core RNG)
Line 201:  rt_random_create, rt_random_create_with_seed
Line 258:  rt_random_int, rt_random_long, rt_random_double, etc.
Line 409:  rt_random_static_* functions
Line 643:  rt_random_*_many functions
Line 735:  rt_random_*_choice functions
Line 883:  weighted choice helpers
Line 1017: rt_random_*_weighted_choice functions
Line 1333: rt_random_*_shuffle functions
Line 1579: rt_random_*_sample functions
```

**Proposed split:**

| New File | Functions | Est. Lines |
|----------|-----------|------------|
| `runtime_random_core.c` | RNG core, seeding, xoshiro256, entropy | ~250 |
| `runtime_random_basic.c` | `rt_random_int`, `_double`, `_bool`, `_byte`, `_gaussian` | ~350 |
| `runtime_random_static.c` | All `rt_random_static_*` functions | ~400 |
| `runtime_random_choice.c` | `choice`, `weighted_choice` functions | ~450 |
| `runtime_random_collection.c` | `shuffle`, `sample`, `many` functions | ~500 |

---

### 4. `symbol_table.c` (1327 lines) → 3 files

Current structure:

```
Line 7:    get_type_size
Line 45:   symbol_table_print
Line 131:  symbol_table_init
Line 196:  symbol_table_push_scope, pop_scope
Line 314:  symbol_table_add_symbol_*
Line 445:  symbol_table_lookup_*
Line 631:  symbol_table_add_namespace
Line 706:  symbol_table_add_symbol_to_namespace
Line 901:  symbol_table_lookup_in_namespace
Line 993:  symbol_table_enter_arena, exit_arena
Line 1013: symbol_table_mark_pending, mark_synchronized
Line 1067: symbol_table_freeze_symbol, unfreeze_symbol
Line 1132: symbol_table_get_thread_state
Line 1223: symbol_table_add_type, lookup_type
```

**Proposed split:**

| New File | Functions | Est. Lines |
|----------|-----------|------------|
| `symbol_table_core.c` | `init`, `push_scope`, `pop_scope`, `add_symbol`, `lookup`, `print` | ~500 |
| `symbol_table_namespace.c` | All namespace operations | ~300 |
| `symbol_table_thread.c` | Thread state, freeze/unfreeze, pending/sync, arena tracking | ~250 |

---

### 5. `code_gen_stmt.c` (1489 lines) → 3 files

Current structure:

```
Line 11:   push_loop_arena, pop_loop_arena
Line 60:   code_gen_thread_sync_statement
Line 187:  code_gen_expression_statement
Line 226:  code_gen_var_declaration
Line 311:  code_gen_free_locals
Line 375:  code_gen_block
Line 439:  code_gen_function
Line 681:  code_gen_return_statement
Line 757:  code_gen_if_statement
Line 772:  code_gen_while_statement
Line 833:  code_gen_for_statement
Line 943:  code_gen_for_each_statement
Line 1049: code_gen_statement (main dispatcher)
Line 1139: is_primitive_for_capture
Line 1158: scan_expr_for_captures, scan_stmt_for_captures
Line 1379: code_gen_scan_captured_primitives
Line 1414: push_arena_to_stack, pop_arena_from_stack
Line 1446: push_loop_counter, pop_loop_counter
```

**Proposed split:**

| New File | Functions | Est. Lines |
|----------|-----------|------------|
| `code_gen_stmt_loop.c` | `for`, `for_each`, `while`, loop arena/counter management | ~400 |
| `code_gen_stmt_capture.c` | `scan_*_for_captures`, captured primitives tracking | ~300 |
| `code_gen_stmt.c` | Core: `if`, `return`, `block`, `function`, var decl, dispatch | ~800 |

---

### 6. `parser_stmt.c` (1428 lines) → 3 files

Current structure:

```
Line 10:   parser_memory_qualifier, parser_function_modifier
Line 61:   parser_indented_block
Line 134:  parser_statement (main dispatcher)
Line 239:  parser_declaration
Line 285:  parser_var_declaration
Line 380:  parser_function_declaration
Line 517:  parser_native_function_declaration
Line 698:  parser_native_function_type
Line 790:  parser_type_declaration
Line 867:  parser_return_statement
Line 894:  parser_if_statement
Line 966:  parser_while_statement
Line 1004: parser_for_statement
Line 1228: parser_block_statement
Line 1271: parser_expression_statement
Line 1323: parser_pragma_statement
Line 1348: parser_import_statement
```

**Proposed split:**

| New File | Functions | Est. Lines |
|----------|-----------|------------|
| `parser_stmt_decl.c` | `var_declaration`, `function_declaration`, `native_function_declaration`, `type_declaration` | ~500 |
| `parser_stmt_control.c` | `if`, `while`, `for`, `return` statements | ~400 |
| `parser_stmt.c` | Core dispatch, block, expression statements, pragma, import | ~500 |

---

## Medium Priority (500-1000 lines)

| File | Lines | Strategy |
|------|-------|----------|
| `parser_expr.c` | 990 | Split by precedence levels or expression types |
| `code_gen_expr_lambda.c` | 976 | Keep as-is (cohesive topic) |
| `runtime_date.c` | 966 | Split creation vs arithmetic vs formatting |
| `runtime_text_file.c` | 945 | Split read vs write operations |
| `type_checker_stmt.c` | 944 | Split by statement type |
| `code_gen.c` | 914 | Keep as-is (orchestration) |
| `runtime_net.c` | 908 | Split TCP vs UDP |
| `runtime_string.c` | 873 | Split manipulation vs search vs formatting |
| `runtime_thread.c` | 838 | Split spawn vs sync vs utilities |
| `runtime_binary_file.c` | 777 | Split read vs write operations |
| `type_checker_util.c` | 755 | Split type operations vs error helpers |

---

## Low Priority (300-500 lines)

These files are near the threshold and may not need splitting:

| File | Lines | Notes |
|------|-------|-------|
| `optimizer_util.c` | 661 | Consider splitting if grows |
| `code_gen_expr_static.c` | 619 | Keep as-is |
| `ast_expr.c` | 596 | Keep as-is |
| `code_gen_expr_call_array.c` | 554 | Keep as-is |
| `runtime_time.c` | 544 | Keep as-is |
| `runtime_path.c` | 531 | Keep as-is |
| `ast.h` | 527 | Header, keep as-is |
| `code_gen_expr_call_file.c` | 519 | Keep as-is |
| `lexer_scan.c` | 518 | Keep as-is |
| `ast_print.c` | 490 | Keep as-is |

---

## Implementation Order

Recommended order based on impact and ease of splitting:

1. **`type_checker_expr_call.c`** - Largest file, clearest boundaries by type
2. **`runtime_random.c`** - Clear functional groupings, self-contained
3. **`code_gen_expr.c`** - Follow existing `code_gen_expr_call_*.c` patterns
4. **`symbol_table.c`** - Core infrastructure, careful refactoring needed
5. **`parser_stmt.c`** - Follow existing parser module patterns
6. **`code_gen_stmt.c`** - Follow existing code_gen patterns

---

## Conventions

The codebase already establishes good patterns for module splitting:

- `code_gen_expr_call_array.c`, `code_gen_expr_call_file.c` - split by domain
- `type_checker_expr.c`, `type_checker_stmt.c` - split by AST node type
- `parser_expr.c`, `parser_stmt.c` - split by parsing concern

New splits should follow these naming conventions:
- `{module}_{concern}_{subconcern}.c`
- Each split file gets a corresponding `.h` header
- Main file retains dispatch logic and common utilities

---
---

# Test Files Analysis

Analysis of test files over 300 lines with strategies for splitting into smaller, more focused test suites.

## Test Summary

| Priority | Files | Total Lines |
|----------|-------|-------------|
| Critical (1000+) | 12 | ~30,000 |
| Medium (500-1000) | 12 | ~8,000 |
| Low (300-500) | 8 | ~3,000 |

## Critical Priority (1000+ lines)

| File | Lines | Suggested Splits |
|------|-------|------------------|
| `runtime_random_tests.c` | 7360 | Split by operation category |
| `runtime_date_tests.c` | 4830 | Split by operation type |
| `type_checker_tests_native.c` | 3551 | Split by feature |
| `type_checker_tests_random.c` | 3159 | Split by method category |
| `type_checker_tests_thread.c` | 2159 | Split by spawn vs sync |
| `runtime_net_tests.c` | 1750 | Split by protocol |
| `code_gen_tests_array.c` | 1710 | Split by operation |
| `symbol_table_tests.c` | 1589 | Split by concern |
| `parser_tests_basic.c` | 1444 | Split by statement type |
| `code_gen_tests_optimization.c` | 1370 | Split by optimization type |
| `type_checker_tests_array.c` | 1031 | Split by method |
| `parser_tests_control.c` | 1006 | Split by control structure |

---

## Detailed Splitting Strategies

### 1. `runtime_random_tests.c` (7360 lines) → 6 files

Current test groupings:

```
Line 22:    test_rt_random_fill_entropy_* (entropy tests)
Line 182:   test_rt_random_create_* (creation tests)
Line 393:   test_rt_random_*_int/long/double/bool/byte (basic generation)
Line 715:   test_rt_random_static_* (static function tests)
Line 1256:  test_rt_random_*_many (batch generation tests)
Line 1472+: test_rt_random_*_choice (choice tests)
Line 2000+: test_rt_random_*_weighted_choice (weighted tests)
Line 3000+: test_rt_random_*_shuffle (shuffle tests)
Line 4000+: test_rt_random_*_sample (sample tests)
```

**Proposed split:**

| New File | Tests | Est. Lines |
|----------|-------|------------|
| `runtime_random_tests_core.c` | Entropy, creation, seeding tests | ~400 |
| `runtime_random_tests_basic.c` | int, long, double, bool, byte, gaussian | ~700 |
| `runtime_random_tests_static.c` | All static function tests | ~800 |
| `runtime_random_tests_many.c` | Batch generation (*_many) tests | ~600 |
| `runtime_random_tests_choice.c` | choice, weighted_choice tests | ~1500 |
| `runtime_random_tests_collection.c` | shuffle, sample tests | ~1500 |

---

### 2. `runtime_date_tests.c` (4830 lines) → 4 files

Current test groupings:

```
Line 14:    test_rt_date_format_* (formatting tests)
Line 520:   test_rt_date_add_days_* (day arithmetic)
Line 683:   test_rt_date_add_weeks_* (week arithmetic)
Line 776:   test_rt_date_diff_days_* (difference calculations)
Line 897:   test_rt_date_start_of_*/end_of_* (boundary operations)
Line 1184:  test_rt_date_calculate_* (helper functions)
Line 1290:  test_rt_date_add_months_* (month arithmetic - very large section)
Line 1971:  test_rt_date_add_years_* (year arithmetic)
Line 2149:  test_rt_date_*_boundaries/transitions (edge cases)
```

**Proposed split:**

| New File | Tests | Est. Lines |
|----------|-------|------------|
| `runtime_date_tests_format.c` | All formatting tests | ~500 |
| `runtime_date_tests_arithmetic.c` | add_days, add_weeks, diff_days | ~600 |
| `runtime_date_tests_months.c` | add_months tests (largest section) | ~1500 |
| `runtime_date_tests_boundaries.c` | start_of, end_of, years, edge cases | ~1200 |

---

### 3. `type_checker_tests_native.c` (3551 lines) → 5 files

Current test groupings:

```
Line 29:    test_native_context_* (context management)
Line 147:   test_pointer_* (pointer handling)
Line 562:   test_inline_* (inline passing)
Line 632:   test_as_val_* (as_val unwrapping)
Line 1152:  test_pointer_slice_* (slice operations)
Line 1644:  test_*_buffer_* (buffer operations)
Line 2067:  test_as_ref_* (reference parameters)
Line 2249:  test_variadic_* (variadic functions)
Line 2389:  test_native_callback_* (callbacks)
Line 2609:  test_native_lambda_* (lambdas)
Line 2911:  test_opaque_* (opaque types)
Line 3144:  test_*_type_* (interop types)
```

**Proposed split:**

| New File | Tests | Est. Lines |
|----------|-------|------------|
| `type_checker_tests_native_context.c` | Context management tests | ~150 |
| `type_checker_tests_native_pointer.c` | Pointer handling, as_val, as_ref | ~1000 |
| `type_checker_tests_native_slice.c` | Slice and buffer operations | ~700 |
| `type_checker_tests_native_callback.c` | Callbacks, lambdas, variadic | ~800 |
| `type_checker_tests_native_types.c` | Opaque types, interop types | ~500 |

---

### 4. `type_checker_tests_random.c` (3159 lines) → 4 files

Current test groupings:

```
Line 12:    test_random_create_* (creation tests)
Line 254:   test_random_*_returns_* (basic method return types)
Line 895:   test_random_*Many_* (batch method tests)
Line 1411:  test_random_choice_* (choice method tests)
Line 1734:  test_random_shuffle_* (shuffle tests)
Line 1925:  test_random_sample_* (sample tests)
Line 2200:  test_random_weighted_* (weighted choice tests)
```

**Proposed split:**

| New File | Tests | Est. Lines |
|----------|-------|------------|
| `type_checker_tests_random_basic.c` | create, int, long, double, bool, byte, gaussian | ~900 |
| `type_checker_tests_random_many.c` | All *Many method tests | ~500 |
| `type_checker_tests_random_choice.c` | choice, weighted_choice tests | ~800 |
| `type_checker_tests_random_collection.c` | shuffle, sample tests | ~700 |

---

### 5. `type_checker_tests_thread.c` (2159 lines) → 3 files

Current test groupings:

```
Line 11:    test_thread_spawn_* (spawn validation)
Line 357:   test_*_frozen_* (freeze mechanics)
Line 473:   test_sync_* (sync validation)
Line 910:   test_array_sync_* (array sync)
Line 1354:  test_*_variable_access_* (variable access rules)
Line 1645:  test_frozen_*_method_* (frozen object methods)
Line 1858:  test_private_function_* (function constraints)
```

**Proposed split:**

| New File | Tests | Est. Lines |
|----------|-------|------------|
| `type_checker_tests_thread_spawn.c` | Spawn validation, freeze mechanics | ~700 |
| `type_checker_tests_thread_sync.c` | Sync validation, array sync | ~700 |
| `type_checker_tests_thread_access.c` | Variable access, frozen methods, constraints | ~700 |

---

### 6. `symbol_table_tests.c` (1589 lines) → 3 files

Current test groupings:

```
Line 28:    test_symbol_table_init_* (initialization)
Line 84:    test_symbol_table_*_scope_* (scope management)
Line 252:   test_symbol_table_add_symbol_* (symbol operations)
Line 362:   test_symbol_table_lookup_* (lookup operations)
Line 544:   test_symbol_table_*_offset* (offset tracking)
Line 786:   test_symbol_table_print (utility)
Line 812:   test_thread_state_* (thread state)
Line 1039:  test_frozen_state_* (freeze state)
Line 1277:  test_namespace_* (namespace operations)
```

**Proposed split:**

| New File | Tests | Est. Lines |
|----------|-------|------------|
| `symbol_table_tests_core.c` | Init, scope, add, lookup, offset, print | ~750 |
| `symbol_table_tests_thread.c` | Thread state, frozen state tests | ~450 |
| `symbol_table_tests_namespace.c` | Namespace operations | ~300 |

---

### 7. `parser_tests_basic.c` (1444 lines) → 3 files

**Proposed split:**

| New File | Tests | Est. Lines |
|----------|-------|------------|
| `parser_tests_literal.c` | Literal parsing tests | ~400 |
| `parser_tests_declaration.c` | Variable, function declaration tests | ~500 |
| `parser_tests_expression.c` | Expression parsing tests | ~500 |

---

### 8. `code_gen_tests_array.c` (1710 lines) → 3 files

**Proposed split:**

| New File | Tests | Est. Lines |
|----------|-------|------------|
| `code_gen_tests_array_creation.c` | Array creation, initialization | ~500 |
| `code_gen_tests_array_methods.c` | Array method code gen | ~600 |
| `code_gen_tests_array_operations.c` | Indexing, slicing, iteration | ~600 |

---

## Medium Priority (500-1000 lines)

| File | Lines | Strategy |
|------|-------|----------|
| `runtime_array_tests.c` | 916 | Split by element type or operation |
| `optimizer_tests.c` | 892 | Split by optimization type |
| `runtime_arena_tests.c` | 819 | Keep as-is (cohesive) |
| `type_checker_tests_promotion.c` | 817 | Keep as-is (single concern) |
| `code_gen_tests_stmt.c` | 809 | Split by statement type |
| `runtime_string_tests.c` | 752 | Split manipulation vs search |
| `lexer_tests_literal.c` | 682 | Split by literal type |
| `lexer_tests_array.c` | 669 | Keep as-is |
| `parser_tests_array.c` | 659 | Keep as-is |
| `token_tests.c` | 647 | Keep as-is |
| `code_gen_tests_expr.c` | 608 | Keep as-is |
| `parser_tests_memory.c` | 588 | Keep as-is |

---

## Low Priority (300-500 lines)

These test files are near the threshold and may not need splitting:

| File | Lines | Notes |
|------|-------|-------|
| `ast_tests_expr.c` | 576 | Keep as-is |
| `runtime_thread_tests.c` | 555 | Keep as-is |
| `runtime_arithmetic_tests.c` | 512 | Keep as-is |
| `code_gen_tests_constfold.c` | 508 | Keep as-is |
| `runtime_time_tests.c` | 503 | Keep as-is |
| `runtime_byte_tests.c` | 476 | Keep as-is |
| `type_checker_tests_errors.c` | 472 | Keep as-is |
| `lexer_tests_memory.c` | 470 | Keep as-is |

---

## Test Implementation Order

Recommended order based on test file size and maintainability impact:

1. **`runtime_random_tests.c`** - Largest test file by far, clear functional boundaries
2. **`runtime_date_tests.c`** - Second largest, well-structured sections
3. **`type_checker_tests_native.c`** - Large with distinct feature areas
4. **`type_checker_tests_random.c`** - Mirrors runtime_random structure
5. **`type_checker_tests_thread.c`** - Clear spawn/sync separation
6. **`symbol_table_tests.c`** - Core infrastructure tests

---

## Test Naming Conventions

The test codebase uses consistent naming patterns:

- `{module}_tests_{concern}.c` - e.g., `runtime_random_tests.c`
- Test functions: `test_{module}_{operation}_{scenario}()` - e.g., `test_rt_random_int_range()`

New split files should follow:
- `{module}_tests_{concern}_{subconcern}.c`
- Example: `runtime_random_tests_choice.c`, `type_checker_tests_native_pointer.c`

Each split file needs:
1. Include of `test_utils.h`
2. A main function that runs all tests in that file
3. Registration in `tests/unit/_all_.c`