# Sindarin UX Issues & Improvements

This document tracks user experience issues discovered during development and testing.

## Fixed Issues

### 1. Type Checker Crash (SEGV) - FIXED
**Location:** `compiler/parser_stmt.c`

**Fix:** Added NULL guards in `parser_stmt.c` for all calls to `parser_indented_block()` that store results in block statement arrays. When `parser_indented_block()` returns NULL, we now create an empty block statement instead of storing NULL.

**Files modified:**
- `compiler/parser_stmt.c` - Lines 381, 409, 443, 495, 565, 652

---

### 2. Missing Forward Declarations in Generated C Code - FIXED
**Location:** `compiler/code_gen.c`

**Fix:** Added a first-pass in `code_gen_module()` that emits forward declarations for all user-defined functions (except `main`) before generating the function definitions.

**Files modified:**
- `compiler/code_gen.c` - Added `code_gen_forward_declaration()` helper and modified `code_gen_module()`

---

### 3. Boolean Operators `&&` and `||` Limited Support - FIXED
**Location:** `compiler/type_checker_expr.c`

**Fix:** Added handling for `TOKEN_AND` and `TOKEN_OR` in `type_check_binary()`. The operators now require boolean operands and return boolean type.

**Files modified:**
- `compiler/type_checker_expr.c` - Added case for logical operators in `type_check_binary()`

---

### 4. Variable Scoping in Nested Loops - FIXED
**Location:** `compiler/lexer.c`

**Root Cause:** When dedenting through multiple levels (e.g., from indent 12 to indent 4), the lexer would recalculate indentation after the first DEDENT but would get 0 instead of the actual value because `lexer->current` had already advanced past the whitespace. This caused extra DEDENTs to be emitted, which popped the function scope prematurely.

**Fix:** Added `pending_indent` and `pending_current` fields to the Lexer struct to save the indentation level and position when emitting DEDENTs with more pending. On subsequent calls, the saved values are used instead of recalculating.

**Files modified:**
- `compiler/lexer.h` - Added `pending_indent` and `pending_current` fields to Lexer struct
- `compiler/lexer_util.c` - Initialize new fields in `lexer_init()`
- `compiler/lexer.c` - Modified `lexer_scan_token()` to use saved values during multi-dedent

**Example that now works:**
```sindarin
var max: int = arr[0]
for n in arr =>
  if n > max =>
    max = n
print($"Max = {max}\n")  // Now works correctly
```

---

## Remaining Issues

### 5. Reserved/Conflicting Variable Names
**Status:** May have been resolved by Issue 4 fix - needs confirmation

**Symptoms:**
- Some variable names cause unexpected errors
- `max` appeared to conflict with something internal

**Workaround:** Use more specific variable names like `maxVal`, `maxValue`

---

## Low Priority / Polish

### 6. C Compiler Warnings
**Symptoms:**
- `warning: implicit declaration of function` - Now mostly resolved with forward declarations
- `warning: passing argument from incompatible pointer type` (string arrays)

**Impact:** Cosmetic - programs still run correctly

---

### 7. Import System Namespace Pollution
**Symptoms:**
- All functions from imported modules share the same namespace
- Function name conflicts between modules cause redefinition errors

**Workaround:** Use unique prefixes for internal helper functions (`show_*` instead of `demo_*`)

**Future improvement:** Consider module namespacing or private functions

---

## Testing Recommendations

1. **Run integration tests regularly** - `./scripts/integration_test.sh`
2. **Test imports incrementally** - Add one import at a time to isolate issues
3. **Run with ASan** - Helps catch memory issues early

---

## Integration Tests Added

| Test File | Tests |
|-----------|-------|
| `test_forward_decl.sn` | Functions calling each other with return values |
| `test_boolean_ops.sn` | `&&` and `||` in various contexts |
| `test_nested_blocks.sn` | Loop structures and variable scoping |
| `test_scope.sn` | Variable scoping after nested for-each/if/while |

---

## Files Modified

| Issue | File(s) Modified |
|-------|-----------------|
| Type checker crash | `compiler/parser_stmt.c` |
| Forward declarations | `compiler/code_gen.c` |
| Boolean operators | `compiler/type_checker_expr.c` |
| Variable scoping in nested loops | `compiler/lexer.h`, `compiler/lexer_util.c`, `compiler/lexer.c` |
