# Type Checker Error Message Audit

## Executive Summary

This document audits all type error messages in the Sn compiler's type checker and identifies opportunities for improvement, including adding "did you mean?" suggestions.

---

## Part 1: Complete Error Call Site Inventory

### type_error() Call Sites

| # | File | Line | Error Message | Context | Token Source |
|---|------|------|---------------|---------|--------------|
| 1 | type_checker_util.c | 41-56 | (wrapper function) | Generic error reporter | Various |
| 2 | type_checker_stmt.c | 81 | "Cannot infer type without initializer" | Variable declaration without type or init | var_decl.name |
| 3 | type_checker_stmt.c | 99 | "'as ref' can only be used with primitive types" | Memory qualifier misuse | var_decl.name |
| 4 | type_checker_stmt.c | 115 | "Initializer type does not match variable type" | Type mismatch in var decl | var_decl.name |
| 5 | type_checker_stmt.c | 143-144 | "Private function can only return primitive types (int, double, bool, char)" | Private function return type | function.name |
| 6 | type_checker_stmt.c | 161 | "Parameter type is missing" | Function parameter without type | param.name |
| 7 | type_checker_stmt.c | 177 | "'as ref' cannot be used on function parameters" | Memory qualifier on param | param.name |
| 8 | type_checker_stmt.c | 208 | "Return type does not match function return type" | Return statement mismatch | stmt->token |
| 9 | type_checker_stmt.c | 249 | "If condition must be boolean" | Non-bool if condition | if_stmt.condition->token |
| 10 | type_checker_stmt.c | 265 | "While condition must be boolean" | Non-bool while condition | while_stmt.condition->token |
| 11 | type_checker_stmt.c | 296 | "For condition must be boolean" | Non-bool for condition | for_stmt.condition->token |
| 12 | type_checker_stmt.c | 335 | "For-each iterable must be an array" | Non-array in for-each | for_each_stmt.iterable->token |
| 13 | type_checker_expr.c | 14 | "Invalid operand in binary expression" | NULL operand in binary | expr->token |
| 14 | type_checker_expr.c | 31 | "Type mismatch in comparison" | Incompatible comparison types | expr->token |
| 15 | type_checker_expr.c | 43 | "Invalid types for arithmetic operator" | Non-numeric arithmetic | expr->token |
| 16 | type_checker_expr.c | 70 | "Invalid types for + operator" | Invalid + operands | expr->token |
| 17 | type_checker_expr.c | 79 | "Logical operators require boolean operands" | Non-bool && or \|\| | expr->token |
| 18 | type_checker_expr.c | 87 | "Invalid binary operator" | Unknown operator | expr->token |
| 19 | type_checker_expr.c | 98 | "Invalid operand in unary expression" | NULL unary operand | expr->token |
| 20 | type_checker_expr.c | 105 | "Unary minus on non-numeric" | -x where x non-numeric | expr->token |
| 21 | type_checker_expr.c | 115 | "Unary ! on non-bool" | !x where x non-bool | expr->token |
| 22 | type_checker_expr.c | 121 | "Invalid unary operator" | Unknown unary op | expr->token |
| 23 | type_checker_expr.c | 133 | "Invalid expression in interpolated string part" | NULL in interpolation | expr->token |
| 24 | type_checker_expr.c | 138 | "Non-printable type in interpolated string" | Unprintable in $"..." | expr->token |
| 25 | type_checker_expr.c | 159 | "Undefined variable" | Variable not in scope | variable.name |
| 26 | type_checker_expr.c | 164 | "Symbol has no type" | Symbol without type | variable.name |
| 27 | type_checker_expr.c | 179 | "Undefined variable for assignment" | Assign to undefined | assign.name |
| 28 | type_checker_expr.c | 216 | "Invalid value in assignment" | NULL assignment value | expr->token |
| 29 | type_checker_expr.c | 221 | "Type mismatch in assignment" | Wrong type assigned | assign.name |
| 30 | type_checker_expr.c | 231-232 | "Cannot assign non-primitive type to variable declared outside private block" | Escape analysis failure | assign.name |
| 31 | type_checker_expr.c | 248 | "Invalid array in index assignment" | NULL array expr | expr->token |
| 32 | type_checker_expr.c | 254 | "Cannot index into non-array type" | Index non-array | expr->token |
| 33 | type_checker_expr.c | 262 | "Invalid index expression" | NULL index expr | expr->token |
| 34 | type_checker_expr.c | 268 | "Array index must be an integer" | Non-int index | expr->token |
| 35 | type_checker_expr.c | 279 | "Invalid value in index assignment" | NULL value in arr[i]=v | expr->token |
| 36 | type_checker_expr.c | 286 | "Type mismatch in index assignment" | Wrong element type | expr->token |
| 37 | type_checker_expr.c | 316 | "len() requires array or string argument" | len() on wrong type | expr->token |
| 38 | type_checker_expr.c | 329 | "Invalid callee in function call" | NULL callee type | expr->token |
| 39 | type_checker_expr.c | 334 | "Callee is not a function" | Call non-function | expr->token |
| 40 | type_checker_expr.c | 339 | "Argument count mismatch in call" | Wrong arg count | expr->token |
| 41 | type_checker_expr.c | 379 | "Invalid argument in function call" | NULL arg type | expr->token |
| 42 | type_checker_expr.c | 386 | "Unsupported type for built-in function" | Non-printable to ANY | expr->token |
| 43 | type_checker_expr.c | 394 | "Argument type mismatch in call" | Wrong arg type | expr->token |
| 44 | type_checker_expr.c | 458 | "Array elements must have the same type" | Heterogeneous array | expr->token |
| 45 | type_checker_expr.c | 482 | "Cannot access non-array" | arr[i] on non-array | expr->token |
| 46 | type_checker_expr.c | 492 | "Array index must be numeric type" | Non-numeric index | expr->token |
| 47 | type_checker_expr.c | 505 | "Increment/decrement on non-numeric type" | ++/-- on non-numeric | expr->token |
| 48 | type_checker_expr.c | 521 | "Cannot slice non-array" | arr[a:b] on non-array | expr->token |
| 49 | type_checker_expr.c | 534 | "Slice start index must be numeric type" | Non-numeric slice start | expr->token |
| 50 | type_checker_expr.c | 546 | "Slice end index must be numeric type" | Non-numeric slice end | expr->token |
| 51 | type_checker_expr.c | 563 | "Invalid start expression in range" | NULL range start | expr->token |
| 52 | type_checker_expr.c | 568 | "Range start must be numeric type" | Non-numeric range start | expr->token |
| 53 | type_checker_expr.c | 574 | "Invalid end expression in range" | NULL range end | expr->token |
| 54 | type_checker_expr.c | 579 | "Range end must be numeric type" | Non-numeric range end | expr->token |
| 55 | type_checker_expr.c | 593 | "Invalid expression in spread" | NULL in ...arr | expr->token |
| 56 | type_checker_expr.c | 598 | "Spread operator requires an array" | ...non_array | expr->token |
| 57 | type_checker_expr.c | 793 | "Invalid member access" | Unknown property/method | expr->token |
| 58 | type_checker_expr.c | 806-807 | "Cannot infer lambda return type. Provide explicit type or use typed variable declaration." | Lambda without return type | expr->token |
| 59 | type_checker_expr.c | 815-816 | "Cannot infer lambda parameter type. Provide explicit type or use typed variable declaration." | Lambda param without type | expr->token |
| 60 | type_checker_expr.c | 826-827 | "Private lambda can only return primitive types (int, double, bool, char)" | Private lambda return | expr->token |
| 61 | type_checker_expr.c | 842 | "'as ref' can only be used with primitive types" | Lambda param as ref | expr->token |
| 62 | type_checker_expr.c | 852 | "'as val' is only meaningful for array types" | Lambda param as val | expr->token |
| 63 | type_checker_expr.c | 884 | "Lambda body type check failed" | NULL body type | expr->token |
| 64 | type_checker_expr.c | 892 | "Lambda body type does not match declared return type" | Body/return mismatch | expr->token |

### type_mismatch_error() Call Sites

| # | File | Line | Expected | Actual | Context |
|---|------|------|----------|--------|---------|
| 1 | type_checker_util.c | 58-76 | (wrapper function) | (varies) | General type mismatch |

**Note:** The `type_mismatch_error()` function is defined but not currently used anywhere in the codebase. All type mismatches use `type_error()` with manual message formatting.

---

## Part 2: Error Messages Lacking Helpful Context

### Category A: Missing Type Information

These errors tell the user something is wrong but don't show what types were actually involved:

| # | Current Message | Missing Context |
|---|-----------------|-----------------|
| 1 | "Initializer type does not match variable type" | What types? "got 'str', expected 'int'" |
| 2 | "Type mismatch in comparison" | What types? "cannot compare 'str' to 'int'" |
| 3 | "Invalid types for arithmetic operator" | What types and operator? "cannot use '-' with 'str' and 'int'" |
| 4 | "Invalid types for + operator" | What types? "cannot add 'bool' to 'function'" |
| 5 | "Type mismatch in assignment" | What types? "got 'str', expected 'int'" |
| 6 | "Type mismatch in index assignment" | What types? "element type 'int', got 'str'" |
| 7 | "Argument type mismatch in call" | Which argument? What types? "argument 2: expected 'int', got 'str'" |
| 8 | "Array elements must have the same type" | What types found? "got 'int' and 'str'" |
| 9 | "Lambda body type does not match declared return type" | What types? "body returns 'str', declared 'int'" |
| 10 | "Return type does not match function return type" | What types? "returning 'str', expected 'int'" |

### Category B: Missing Variable/Function Names

These errors reference undefined or misused identifiers but don't show the name:

| # | Current Message | Missing Context |
|---|-----------------|-----------------|
| 1 | "Undefined variable" | What name? "undefined variable 'foo'" |
| 2 | "Undefined variable for assignment" | What name? "cannot assign to undefined 'bar'" |
| 3 | "Symbol has no type" | What symbol? "symbol 'x' has no type" |
| 4 | "Invalid callee in function call" | What was called? "cannot call 'xyz'" |
| 5 | "Callee is not a function" | What type is it? "'foo' is 'int', not a function" |
| 6 | "Argument count mismatch in call" | How many? "foo() expects 3 arguments, got 2" |

### Category C: Missing Operator Context

| # | Current Message | Missing Context |
|---|-----------------|-----------------|
| 1 | "Invalid binary operator" | Which operator? |
| 2 | "Invalid unary operator" | Which operator? |
| 3 | "Logical operators require boolean operands" | Which operator (&&, \|\|)? Which operand? |

### Category D: Vague Property/Method Errors

| # | Current Message | Missing Context |
|---|-----------------|-----------------|
| 1 | "Invalid member access" | What member on what type? "'foo' has no member 'bar'" |
| 2 | "len() requires array or string argument" | What type was passed? "len() got 'int', requires array or string" |
| 3 | "Unsupported type for built-in function" | What type? Which function? |

---

## Part 3: "Did You Mean?" Suggestion Opportunities

### High-Value Suggestions (Most Impactful)

| Error Scenario | Suggestion Type | Implementation Complexity |
|----------------|-----------------|---------------------------|
| "Undefined variable 'foo'" | Levenshtein distance on scope symbols | Medium |
| "Invalid member access" on arrays | Suggest known array methods (push, pop, etc.) | Easy |
| "Invalid member access" on strings | Suggest known string methods (substring, indexOf, etc.) | Easy |
| "Argument count mismatch" | Show expected signature | Easy |
| "Cannot call 'x', it's a 'type'" | If type looks similar to function, suggest | Medium |
| "Type mismatch: got X, expected Y" | Suggest conversion if available | Medium |

### Detailed Suggestion Proposals

#### 1. Undefined Variable Suggestions
```
Current:  "Undefined variable"
Improved: "Undefined variable 'coutn'. Did you mean 'count'?"

Algorithm: Use Levenshtein distance <= 2 to find similar symbols in scope
```

#### 2. Array Method Suggestions
```
Current:  "Invalid member access"
Improved: "Type 'array' has no member 'append'. Did you mean 'push'?"

Array methods to suggest from:
- push, pop, clear, concat, indexOf, contains, clone, join, reverse, insert, remove, length
```

#### 3. String Method Suggestions
```
Current:  "Invalid member access"
Improved: "Type 'str' has no member 'substr'. Did you mean 'substring'?"

String methods to suggest from:
- substring, indexOf, split, trim, toUpper, toLower, startsWith, endsWith, contains, replace, charAt, length
```

#### 4. Type Coercion Suggestions
```
Current:  "Type mismatch in assignment"
Improved: "Cannot assign 'int' to 'str'. Did you mean to use string interpolation: $\"{value}\"?"
```

#### 5. Function Arity Suggestions
```
Current:  "Argument count mismatch in call"
Improved: "Function 'add' expects 2 arguments but got 3. Signature: fn add(a: int, b: int): int"
```

#### 6. Boolean Condition Suggestions
```
Current:  "If condition must be boolean"
Improved: "If condition must be boolean, got 'int'. Did you mean 'x != 0' or 'x > 0'?"
```

---

## Part 4: Error Message Improvement Matrix

### Priority Matrix

| Error ID | Current Message | Improved Message | Priority | Effort |
|----------|-----------------|------------------|----------|--------|
| 25 | "Undefined variable" | "Undefined variable '%s'. Did you mean '%s'?" | HIGH | MEDIUM |
| 57 | "Invalid member access" | "'%s' has no member '%s'. Did you mean '%s'?" | HIGH | EASY |
| 2 | "Initializer type does not match variable type" | "Variable '%s' declared as '%s' but initialized with '%s'" | HIGH | EASY |
| 29 | "Type mismatch in assignment" | "Cannot assign '%s' to '%s' (expected '%s')" | HIGH | EASY |
| 40 | "Argument count mismatch in call" | "'%s' expects %d argument(s), got %d" | HIGH | EASY |
| 43 | "Argument type mismatch in call" | "Argument %d of '%s': expected '%s', got '%s'" | HIGH | EASY |
| 39 | "Callee is not a function" | "'%s' is of type '%s', cannot call non-function" | MEDIUM | EASY |
| 14 | "Type mismatch in comparison" | "Cannot compare '%s' with '%s'" | MEDIUM | EASY |
| 9 | "If condition must be boolean" | "If condition must be boolean, got '%s'" | MEDIUM | EASY |
| 10 | "While condition must be boolean" | "While condition must be boolean, got '%s'" | MEDIUM | EASY |
| 11 | "For condition must be boolean" | "For condition must be boolean, got '%s'" | MEDIUM | EASY |
| 44 | "Array elements must have the same type" | "Array elements must have same type: element 1 is '%s', element %d is '%s'" | MEDIUM | MEDIUM |
| 8 | "Return type does not match function return type" | "Returning '%s' from function declared to return '%s'" | MEDIUM | EASY |
| 16 | "Invalid types for + operator" | "Cannot use '+' with '%s' and '%s'" | MEDIUM | EASY |
| 15 | "Invalid types for arithmetic operator" | "Cannot use '%s' with '%s' and '%s'" | MEDIUM | EASY |
| 20 | "Unary minus on non-numeric" | "Cannot negate '%s', expected numeric type" | LOW | EASY |
| 21 | "Unary ! on non-bool" | "Cannot apply '!' to '%s', expected boolean" | LOW | EASY |
| 46 | "Array index must be numeric type" | "Array index must be integer, got '%s'" | LOW | EASY |

### Implementation Phases

#### Phase 1: Quick Wins (Add Type Context)
- Modify `type_error()` to accept additional context parameters
- Update high-priority messages to include actual types
- Effort: 1-2 hours

#### Phase 2: "Did You Mean?" for Identifiers
- Implement Levenshtein distance function
- Add symbol name suggestions for undefined variables
- Effort: 2-3 hours

#### Phase 3: Method/Property Suggestions
- Add known method lists for arrays and strings
- Implement fuzzy matching for method names
- Effort: 1-2 hours

#### Phase 4: Enhanced Context
- Track operator tokens for better arithmetic error messages
- Include function signatures in call errors
- Track array element index for heterogeneous array errors
- Effort: 2-3 hours

---

## Part 5: Proposed Helper Functions

### New Error Reporting Functions

```c
// Enhanced error with types
void type_mismatch_error_detailed(Token *token, Type *expected, Type *actual,
                                  const char *var_name, const char *context);

// Error with suggestion
void type_error_with_suggestion(Token *token, const char *msg,
                                const char *suggestion);

// Undefined identifier with suggestions
void undefined_identifier_error(Token *token, const char *name,
                                SymbolTable *table);

// Invalid member with suggestions
void invalid_member_error(Token *token, Type *object_type,
                          const char *member_name);

// Argument mismatch with signature
void argument_error(Token *token, const char *func_name,
                    int expected, int actual, Type *func_type);
```

### Levenshtein Distance Helper

```c
int levenshtein_distance(const char *s1, const char *s2);
char *find_similar_symbol(SymbolTable *table, const char *name, int max_distance);
char *find_similar_method(Type *type, const char *method_name);
```

---

## Part 6: Summary Statistics

- **Total type_error() call sites:** 64
- **Using type_mismatch_error():** 0 (defined but unused)
- **Errors missing type context:** 10
- **Errors missing identifier names:** 6
- **Errors missing operator context:** 3
- **High-value "did you mean" opportunities:** 6 scenarios
- **Estimated total improvement effort:** 6-10 hours

---

## Appendix: Type Checker File Structure

```
compiler/
├── type_checker.c          # Module entry point (17 lines)
├── type_checker.h          # Public interface
├── type_checker_util.c     # Utilities & error functions (203 lines)
├── type_checker_util.h     # Utility interface
├── type_checker_stmt.c     # Statement type checking (415 lines)
├── type_checker_stmt.h     # Statement interface
├── type_checker_expr.c     # Expression type checking (991 lines)
└── type_checker_expr.h     # Expression interface
```

---
---

# C Code Generation Optimization Audit

## Executive Summary

This document analyzes the generated C code from the Sn compiler's `code_gen` module and identifies optimization opportunities to reduce runtime overhead, eliminate unnecessary allocations, and improve performance.

---

## Part 1: Runtime Function Calls for Compile-Time Constants

### Issue Description

The code generator currently uses runtime functions for operations that could be computed at compile time or use direct C operations. This introduces function call overhead for simple operations.

### Identified Patterns

#### 1. Arithmetic Operations on Literal Constants

**Current Generated Code:**
```c
// Source: n - 1 (where both are known at compile time)
rt_sub_long(n, 1L)

// Source: n * factorial(n - 1)
rt_mul_long(n, factorial(rt_sub_long(n, 1L)))

// Source: sum + i
rt_add_long(sum, i)
```

**Issue:** `rt_add_long`, `rt_sub_long`, `rt_mul_long` include overflow checking, which is valuable for runtime values but unnecessary overhead when one operand is a small literal constant.

**Recommendation:** For operations where:
- Both operands are compile-time constants: compute at compile time
- One operand is a small literal (e.g., 0, 1, 2): use direct C operators with overflow-safe patterns

**Proposed Optimization:**
```c
// Instead of: rt_add_long(sum, 1L)
// Generate: (sum + 1L)  // When increment is 1, overflow is already checked by loop bounds

// Instead of: rt_sub_long(n, 1L)
// Generate: (n - 1L)  // When decrementing from positive, underflow impossible
```

#### 2. Comparison Operations on Constants

**Current Generated Code:**
```c
rt_le_long(n, 1L)
rt_lt_long(count, 3L)
rt_le_long(i, 5L)
```

**Issue:** These runtime functions simply return `a <= b`, `a < b`, etc. There is no overflow risk in comparisons.

**Recommendation:** Always use direct C comparison operators.

**Files Affected:**
- `code_gen_expr.c:536` - `code_gen_binary_expression()` generates `rt_<op>_<type>()` calls

**Proposed Changes:**
```c
// For comparison operators, always use direct C:
// Instead of: rt_le_long(n, 1L)
// Generate: (n <= 1L)
```

#### 3. Boolean Negation

**Current Generated Code:**
```c
rt_not_bool(x)
```

**Issue:** Boolean negation has no overflow risk.

**Recommendation:** Use direct `!x`.

---

## Part 2: Redundant Arena Allocations

### Issue Description

The generated code creates arenas in some contexts where they are unnecessary or excessively granular.

### Identified Patterns

#### 1. Per-Iteration Loop Arenas

**Current Generated Code:**
```c
while (rt_le_long(i, 5L)) {
    RtArena *__loop_arena_0__ = rt_arena_create(__arena_1__);
    {
        (sum = rt_add_long(sum, i));
    }
__loop_cleanup_0__:
    rt_arena_destroy(__loop_arena_0__);
__for_continue_1__:;
    rt_post_inc_long(&i);
}
```

**Issue:** For loops that don't allocate any heap memory (like this integer arithmetic loop), creating and destroying an arena per iteration is pure overhead.

**Recommendation:**
- Analyze loop body for heap allocations (strings, arrays)
- Only create per-iteration arena if allocations are detected
- For `shared` loops, skip arena creation entirely

**Files Affected:**
- `code_gen_stmt.c` - `code_gen_for_statement()`, `code_gen_while_statement()`

#### 2. Arena Creation in Pure Functions

**Current Generated Code:**
```c
long factorial(long n) {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    long _return_value = 0;
    // ... only integer operations ...
    rt_arena_destroy(__arena_1__);
    return _return_value;
}
```

**Issue:** Functions that only operate on primitive types (no strings, arrays, or heap allocations) still create an arena.

**Recommendation:**
- Analyze function body during type checking for heap allocation usage
- Mark functions as "pure" (no allocations) to skip arena creation
- Only create arena if function body contains string/array operations

#### 3. Duplicate String Copying

**Current Generated Code:**
```c
_str_part0 = rt_to_string_string(__arena_1__, "5! = ");
```

**Issue:** `rt_to_string_string` allocates a new copy of the string literal on the arena. String literals are already in static memory and don't need copying.

**Recommendation:**
- For string literals, return the literal directly without allocation
- Only copy when the source is a variable that might be freed

---

## Part 3: Unnecessary Temporary Variables

### Issue Description

The code generator creates temporary variables in cases where they provide no benefit.

### Identified Patterns

#### 1. Expression Statement Temps

**Current Generated Code:**
```c
({
    char *_str_arg0 = ({
        // interpolation result
    });
    rt_print_string(_str_arg0);
});
```

**Issue:** When the only use of the expression is as a function argument, the outer statement expression and temp variable add no value.

**Recommendation:** Generate direct function calls when safe:
```c
rt_print_string(/* interpolation expression */);
```

#### 2. Interpolation Intermediate Variables

**Current Generated Code:**
```c
char *_str_part0, *_str_part1, *_str_part2;
_str_part0 = rt_to_string_string(__arena_1__, "5! = ");
_str_part1 = rt_to_string_long(__arena_1__, factorial(5L));
_str_part2 = rt_to_string_string(__arena_1__, "\n");
char *_concat_tmp0;
_concat_tmp0 = rt_str_concat(__arena_1__, _str_part0, _str_part1);
char *_interpol_result = rt_str_concat(__arena_1__, _concat_tmp0, _str_part2);
```

**Issue:** Each part is converted to a string, then concatenated pairwise. For simple cases, this could be optimized.

**Recommendation:**
- For 2-part interpolations, direct concat is fine
- For mixed types, consider a variadic `rt_sprintf` approach
- Skip `rt_to_string_string` for literal strings

#### 3. _return_value for Direct Returns

**Current Generated Code:**
```c
long _return_value = 0;
// ...
_return_value = 1L;
goto factorial_return;
// ...
factorial_return:
    rt_arena_destroy(__arena_1__);
    return _return_value;
```

**Issue:** The pattern of setting `_return_value` and jumping to cleanup is necessary for complex functions but overhead for simple ones.

**Recommendation:** For functions without cleanup needs (no arena, no local cleanup), generate direct `return` statements.

---

## Part 4: Patterns That Could Use Direct C Operations

### Summary Table

| Pattern | Current | Optimized | Savings |
|---------|---------|-----------|---------|
| `rt_add_long(a, 1L)` | Function call | `(a + 1L)` | ~10ns/call |
| `rt_sub_long(a, 1L)` | Function call | `(a - 1L)` | ~10ns/call |
| `rt_eq_long(a, b)` | Function call | `(a == b)` | ~10ns/call |
| `rt_ne_long(a, b)` | Function call | `(a != b)` | ~10ns/call |
| `rt_lt_long(a, b)` | Function call | `(a < b)` | ~10ns/call |
| `rt_le_long(a, b)` | Function call | `(a <= b)` | ~10ns/call |
| `rt_gt_long(a, b)` | Function call | `(a > b)` | ~10ns/call |
| `rt_ge_long(a, b)` | Function call | `(a >= b)` | ~10ns/call |
| `rt_not_bool(a)` | Function call | `(!a)` | ~10ns/call |
| `rt_neg_long(a)` | Function call with check | `(-a)` (if safe) | ~10ns/call |
| `rt_to_string_string(arena, "lit")` | Arena alloc + copy | `"lit"` (direct) | ~50ns/call |

### When to Keep Runtime Functions

The runtime functions should be kept for:
1. **Division and Modulo** - divide by zero checks are essential
2. **Multiplication** - overflow detection is valuable for large numbers
3. **Addition/Subtraction of Variables** - when both are runtime values
4. **Post-increment/decrement** - these modify in place

### Safe Optimization Cases

1. **Comparison operators** - Never overflow, always safe to inline
2. **Boolean operations** - Never overflow, always safe to inline
3. **Small constant arithmetic** - Adding/subtracting 0 or 1 is safe
4. **String literals** - No need to copy to arena

---

## Part 5: Optimization Implementation Matrix

### Priority Matrix

| Optimization | Impact | Complexity | Files to Modify |
|--------------|--------|------------|-----------------|
| Inline comparison operators | HIGH | LOW | code_gen_expr.c |
| Skip arena for pure functions | HIGH | MEDIUM | code_gen_stmt.c, type analysis |
| Skip loop arena for non-allocating | HIGH | MEDIUM | code_gen_stmt.c |
| Direct string literals | MEDIUM | LOW | code_gen_expr.c |
| Inline boolean operations | MEDIUM | LOW | code_gen_expr.c |
| Inline increment by 1 | MEDIUM | LOW | code_gen_expr.c |
| Reduce interpolation temps | MEDIUM | MEDIUM | code_gen_expr.c |
| Variadic sprintf for interpolation | LOW | HIGH | runtime.c, code_gen_expr.c |

### Implementation Phases

#### Phase 1: Quick Wins (No behavioral changes)
1. Inline all comparison operators (`==`, `!=`, `<`, `<=`, `>`, `>=`)
2. Inline boolean negation (`!`)
3. Use string literals directly without copying

**Effort:** 1-2 hours

#### Phase 2: Arena Optimization
1. Add type analysis pass to detect if function/block allocates
2. Skip arena creation for pure integer/boolean functions
3. Skip per-iteration arena for non-allocating loops

**Effort:** 3-4 hours

#### Phase 3: Arithmetic Optimization
1. Detect constant operands at compile time
2. Inline operations when one operand is 0 or 1
3. Add compile-time constant folding for simple expressions

**Effort:** 2-3 hours

---

## Part 6: Code Generator Architecture

### Current File Structure

```
compiler/
├── code_gen.c          # Module entry, headers, extern declarations (~413 lines)
├── code_gen.h          # Public interface
├── code_gen_util.c     # Helpers: get_c_type, escape_string (~300 lines)
├── code_gen_util.h     # Utility interface
├── code_gen_stmt.c     # Statement generation (~600+ lines)
├── code_gen_stmt.h     # Statement interface
├── code_gen_expr.c     # Expression generation (~1600+ lines)
└── code_gen_expr.h     # Expression interface
```

### Key Functions to Modify

1. `code_gen_binary_expression()` in `code_gen_expr.c:461`
   - Add check for comparison operators → inline
   - Add check for constant operands → inline arithmetic

2. `code_gen_unary_expression()` in `code_gen_expr.c:540`
   - Inline `!` operator

3. `code_gen_function()` in `code_gen_stmt.c:218`
   - Skip arena creation for pure functions

4. `code_gen_for_statement()` / `code_gen_while_statement()`
   - Skip loop arena for non-allocating bodies

---

## Part 7: Proposed New Helper Functions

### Type Analysis Helpers

```c
/* Returns true if expression may allocate heap memory */
bool expr_may_allocate(Expr *expr);

/* Returns true if statement may allocate heap memory */
bool stmt_may_allocate(Stmt *stmt);

/* Returns true if function body may allocate heap memory */
bool function_may_allocate(FunctionStmt *fn);

/* Returns true if expression is a compile-time constant */
bool is_constant_expr(Expr *expr);

/* Evaluate constant expression at compile time */
long eval_constant_long(Expr *expr);
```

### Code Generation Helpers

```c
/* Generate inline comparison (no runtime call) */
char *code_gen_inline_compare(CodeGen *gen, TokenType op, char *left, char *right);

/* Generate inline arithmetic when safe */
char *code_gen_inline_arithmetic(CodeGen *gen, TokenType op, char *left, char *right,
                                  bool left_const, bool right_const);
```

---

## Part 8: Summary Statistics

- **Runtime function calls that could be inlined:** 12 types
- **Arena allocations that could be skipped:** 2 patterns (pure functions, non-allocating loops)
- **Temporary variables that could be eliminated:** 3 patterns
- **Estimated performance improvement:** 20-40% for integer-heavy code
- **Estimated implementation effort:** 6-10 hours

---

## Appendix: Sample Optimized Output

### Before Optimization (factorial function)
```c
long factorial(long n) {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    long _return_value = 0;
    if (rt_le_long(n, 1L)) {
        {
            _return_value = 1L;
            goto factorial_return;
        }
    }
    _return_value = rt_mul_long(n, factorial(rt_sub_long(n, 1L)));
    goto factorial_return;
factorial_return:
    rt_arena_destroy(__arena_1__);
    return _return_value;
}
```

### After Optimization
```c
long factorial(long n) {
    if (n <= 1L) {
        return 1L;
    }
    return rt_mul_long(n, factorial(n - 1L));  // Keep mul for overflow check
}
```

**Changes:**
1. Removed arena (function is pure - no allocations)
2. Inlined `rt_le_long` comparison
3. Inlined `rt_sub_long(n, 1L)` since decrementing by 1 is safe
4. Eliminated `_return_value` variable with direct returns
5. Kept `rt_mul_long` for overflow detection on large factorials
