# Code Generation for `as val` Expression

This document describes the implementation of the `as val` operator in the code generation phase, including the three existing cases and what's missing for the `ptr[0..len] as val` pattern.

---

## Overview

The `as val` operator provides safe pointer unwrapping for C interoperability. It converts pointer types to their underlying value types, copying data into the arena for safe use in Sindarin code.

**Key Files:**
- `src/code_gen/code_gen_expr.c:2987-3017` - `code_gen_as_val_expression()` function
- `src/type_checker/type_checker_expr.c:1023-1076` - Type checking for EXPR_AS_VAL
- `src/type_checker/type_checker_expr_array.c:122-207` - Type checking for pointer slices
- `src/ast.h:245-250` - AsValExpr structure definition

---

## AST Structure

```c
typedef struct
{
    Expr *operand;       // The expression to copy/pass by value
    bool is_cstr_to_str; // True if this is *char => str (null-terminated string conversion)
    bool is_noop;        // True if operand is already array type (ptr[0..len] produces array)
} AsValExpr;
```

The type checker sets these boolean flags during type checking to guide code generation.

---

## Three Existing Cases

### Case 1: `is_noop` - Array Pass-Through

**When it applies:** When the operand is already an array type (e.g., from `ptr[0..len]` slice).

**Type Checker Logic** (`type_checker_expr.c:1040-1048`):
```c
else if (operand_type->kind == TYPE_ARRAY)
{
    /* Array type: 'as val' is a no-op, returns same type.
     * This supports ptr[0..len] as val where the slice already
     * produces an array type. */
    t = operand_type;
    expr->as.as_val.is_cstr_to_str = false;
    expr->as.as_val.is_noop = true;
}
```

**Code Generation** (`code_gen_expr.c:2999-3004`):
```c
if (as_val->is_noop)
{
    /* Operand is already an array type (e.g., from ptr[0..len] slice).
     * Just pass through without any transformation. */
    return operand_code;
}
```

**Generated C Code:** Simply passes through the operand unchanged.

**Example:**
```sindarin
var data: byte[] = get_buffer()[0..len] as val
```
The slice `get_buffer()[0..len]` already produces a `byte[]` via `code_gen_array_slice_expression()`, so `as val` is a semantic marker with no additional code generation.

---

### Case 2: `is_cstr_to_str` - C String to Sindarin String

**When it applies:** When the operand is a `*char` pointer (C-style null-terminated string).

**Type Checker Logic** (`type_checker_expr.c:1057-1064`):
```c
Type *base_type = operand_type->as.pointer.base_type;
/* Special case: *char => str (null-terminated string conversion) */
if (base_type->kind == TYPE_CHAR)
{
    t = ast_create_primitive_type(table->arena, TYPE_STRING);
    expr->as.as_val.is_cstr_to_str = true;
    expr->as.as_val.is_noop = false;
}
```

**Code Generation** (`code_gen_expr.c:3005-3011`):
```c
else if (as_val->is_cstr_to_str)
{
    /* *char => str: use rt_arena_strdup to copy null-terminated C string to arena.
     * Handle NULL pointer by returning empty string. */
    return arena_sprintf(gen->arena, "((%s) ? rt_arena_strdup(%s, %s) : rt_arena_strdup(%s, \"\"))",
                        operand_code, ARENA_VAR(gen), operand_code, ARENA_VAR(gen));
}
```

**Generated C Code:**
```c
((ptr) ? rt_arena_strdup(arena, ptr) : rt_arena_strdup(arena, ""))
```

**Example:**
```sindarin
native fn getenv(name: str): *char
var home: str = getenv("HOME") as val
```
Converts the C string returned by `getenv()` to a Sindarin `str` by copying to the arena. NULL pointers become empty strings.

---

### Case 3: Primitive Dereference

**When it applies:** When the operand is a pointer to a primitive type (`*int`, `*double`, `*float`, `*bool`, `*byte`, `*long`).

**Type Checker Logic** (`type_checker_expr.c:1066-1073`):
```c
else
{
    /* Result type is the base type of the pointer */
    t = base_type;
    expr->as.as_val.is_cstr_to_str = false;
    expr->as.as_val.is_noop = false;
}
```

**Code Generation** (`code_gen_expr.c:3012-3016`):
```c
else
{
    /* Primitive pointer dereference: *int, *double, *float, etc. */
    return arena_sprintf(gen->arena, "(*(%s))", operand_code);
}
```

**Generated C Code:**
```c
(*(ptr))
```

**Example:**
```sindarin
native fn get_value(): *int
var val: int = get_value() as val
```
Dereferences the `*int` pointer to copy the integer value into the local variable.

---

## What's Missing for `ptr[0..len] as val` Pattern

The `ptr[0..len] as val` pattern is **already fully implemented**. Here's how the complete flow works:

### Current Implementation Flow

1. **Parsing:** `ptr[0..len]` is parsed as an `EXPR_ARRAY_SLICE` expression with a pointer operand.

2. **Type Checking (pointer slice):** `type_check_array_slice()` in `type_checker_expr_array.c:122-207`:
   - Detects `operand_t->kind == TYPE_POINTER`
   - Sets `expr->as.array_slice.is_from_pointer = true`
   - Validates `as_val` context is active (required for pointer slices in non-native functions)
   - Returns an array type with element type from the pointer base type

3. **Type Checking (as val wrapper):** `type_check_expr()` handles `EXPR_AS_VAL`:
   - Enters `as_val_context` before type-checking operand
   - Receives array type from the slice expression
   - Sets `is_noop = true` since operand is already array type

4. **Code Generation (slice):** `code_gen_array_slice_expression()` in `code_gen_expr_array.c:180-237`:
   - Detects pointer slicing via `operand_type->kind == TYPE_POINTER`
   - Generates: `rt_array_create_<type>(arena, (size_t)(end - start), ptr + start)`
   - This creates a proper Sindarin array by copying data from the pointer

5. **Code Generation (as val):** `code_gen_as_val_expression()`:
   - With `is_noop = true`, simply returns the operand code unchanged

### Complete Generated C Code Example

For `get_buffer()[0..len] as val`:
```c
rt_array_create_byte(arena, (size_t)((len) - (0)), (get_buffer()) + (0))
```

### No Missing Functionality

The pattern works correctly because:

1. **The slice does the actual work:** `code_gen_array_slice_expression()` handles pointer-to-array conversion by calling runtime array creation functions.

2. **`as val` is semantic:** For pointer slices, `as val` is a compile-time requirement enforced by the type checker, not a runtime operation. The actual memory copy happens in the slice expression.

3. **Type safety is enforced:** In non-native functions, `as_val_context_is_active()` check in `type_check_array_slice()` ensures pointer slices must be wrapped in `as val`.

---

## Summary Table

| Pattern | `is_noop` | `is_cstr_to_str` | Generated Code |
|---------|-----------|------------------|----------------|
| `arr[0..n] as val` | true | false | (pass-through) |
| `ptr[0..n] as val` | true | false | (pass-through, slice does the work) |
| `*char as val` | false | true | `rt_arena_strdup(arena, ptr)` |
| `*int as val` | false | false | `(*(ptr))` |
| `*double as val` | false | false | `(*(ptr))` |

---

## Related Context Functions

The type checker uses context tracking to validate the `as val` pattern:

```c
// From type_checker_util.c
void as_val_context_enter(void);
void as_val_context_exit(void);
bool as_val_context_is_active(void);
```

These ensure that pointer slices in non-native functions are properly wrapped with `as val`, providing compile-time safety while allowing the slice expression to handle the actual data copy.

---

## Pointer Slice Code Generation Details

### ArraySliceExpr Structure

```c
typedef struct
{
    Expr *array;
    Expr *start;  // NULL means from beginning
    Expr *end;    // NULL means to end
    Expr *step;   // NULL means step of 1
    bool is_from_pointer;  // True if slicing a pointer type (set by type checker)
} ArraySliceExpr;
```

### How `is_from_pointer` Flag Works

1. **Set by Type Checker** (`type_checker_expr_array.c:136-155`):
   - Type checker examines the operand type
   - If `operand_t->kind == TYPE_POINTER`, sets `is_from_pointer = true`
   - Stored in `expr->as.array_slice.is_from_pointer`

2. **NOT Used by Code Generator**:
   - Code generator directly checks `operand_type->kind == TYPE_POINTER`
   - The `is_from_pointer` flag is redundant for code gen purposes
   - It's primarily used by type checker for validation

### Where `rt_array_create_byte` is Called

In `code_gen_array_slice_expression()` at `code_gen_expr_array.c:213-237`:

```c
if (operand_type->kind == TYPE_POINTER)
{
    const char *create_func = NULL;
    switch (elem_type->kind) {
        case TYPE_LONG:
        case TYPE_INT:
            create_func = "rt_array_create_long";
            break;
        case TYPE_DOUBLE:
            create_func = "rt_array_create_double";
            break;
        case TYPE_CHAR:
            create_func = "rt_array_create_char";
            break;
        case TYPE_BYTE:
            create_func = "rt_array_create_byte";  // <-- HERE
            break;
        default:
            fprintf(stderr, "Error: Unsupported pointer element type for slice\n");
            exit(1);
    }
    /* Generate: rt_array_create_<type>(arena, (size_t)(end - start), ptr + start) */
    return arena_sprintf(gen->arena, "%s(%s, (size_t)((%s) - (%s)), (%s) + (%s))",
                         create_func, ARENA_VAR(gen), end_str, start_str, array_str, start_str);
}
```

### Interaction Between Slice Codegen and `as val` Wrapper

```
                    Sindarin Code
                         │
                         ▼
            ┌─────────────────────────┐
            │  get_buffer()[0..len]   │
            │       as val            │
            └─────────────────────────┘
                         │
          ┌──────────────┴──────────────┐
          │                              │
          ▼                              ▼
    ┌───────────────┐          ┌────────────────┐
    │ EXPR_AS_VAL   │──────────│EXPR_ARRAY_SLICE│
    │               │ operand  │ (pointer type) │
    │ is_noop=true  │          │is_from_ptr=true│
    └───────────────┘          └────────────────┘
          │                              │
          │                              │
          ▼                              ▼
    ┌───────────────┐          ┌────────────────────────────────────────────┐
    │code_gen_as_val│          │   code_gen_array_slice_expression          │
    │               │          │                                            │
    │ is_noop=true  │          │ operand_type->kind == TYPE_POINTER         │
    │ → pass-thru   │          │ → rt_array_create_byte(arena, len, ptr)    │
    └───────────────┘          └────────────────────────────────────────────┘
          │                              │
          │                              │
          ▼                              ▼
    ┌─────────────────────────────────────────────────────────────────────────┐
    │               Final Generated C Code:                                   │
    │   rt_array_create_byte(__arena_1__, (size_t)((len) - (0)),             │
    │                        (get_buffer()) + (0))                            │
    └─────────────────────────────────────────────────────────────────────────┘
```

### Key Observation

The `as val` wrapper is a **semantic requirement**, not a code generation requirement for pointer slices. The actual array creation happens in `code_gen_array_slice_expression()`. The `as val` wrapper:

1. Enforces compile-time safety (type checker rejects `ptr[0..len]` without `as val` in non-native functions)
2. At code generation time, just passes through the already-generated array code (`is_noop = true`)

---

## Revision History

| Date | Changes |
|------|---------|
| 2025-01-05 | Added detailed analysis of is_from_pointer flag and slice/as_val interaction |
| 2025-01-05 | Initial documentation of code_gen_as_val_expression implementation |
