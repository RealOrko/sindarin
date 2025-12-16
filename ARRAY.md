# Array Feature Implementation Plan

This document outlines the current state of array support in the Sn compiler and provides a comprehensive implementation plan for a full-featured array system.

---

## Implementation Progress

### Phase 1: Core Slicing - ✅ COMPLETE

Array slicing has been fully implemented! The following features are now working:

| Feature | Status | Test Verified |
|---------|--------|---------------|
| `TOKEN_RANGE` (`..`) in lexer | ✅ Complete | `lexer_tests_array.c` |
| `EXPR_ARRAY_SLICE` AST node | ✅ Complete | `ast_tests_expr.c` |
| Parser slice syntax | ✅ Complete | `parser_tests_array.c` |
| Type checker for slices | ✅ Complete | `type_checker_tests_array.c` |
| Runtime slice functions | ✅ Complete | `runtime.c/h` |
| Code generation for slices | ✅ Complete | `code_gen_expr.c` |

**Supported Syntax:**
```sn
var arr: int[] = {10, 20, 30, 40, 50}
var slice1: int[] = arr[1..4]   // [20, 30, 40]
var slice2: int[] = arr[..3]    // [10, 20, 30]
var slice3: int[] = arr[2..]    // [30, 40, 50]
var slice4: int[] = arr[..]     // [10, 20, 30, 40, 50] (full copy)
```

**Files Modified:**
- `token.h/c` - Added `TOKEN_RANGE`
- `lexer.c` - Handle `..` tokenization
- `ast.h` - Added `EXPR_ARRAY_SLICE` and `ArraySliceExpr`
- `ast_expr.c/h` - Added `ast_create_array_slice_expr()`
- `ast_print.c` - Print slice expressions
- `parser_expr.c` - Parse slice syntax in `parser_array_access()`
- `type_checker_expr.c` - Type check slice expressions
- `runtime.c/h` - Added `rt_array_slice_*()` functions for all types
- `code_gen.c` - Added extern declarations for slice functions
- `code_gen_expr.c` - Generate slice function calls

---

### Phase 2: Standalone Built-in Functions - ✅ COMPLETE

Standalone functions for array manipulation are now available:

| Function | Syntax | Description |
|----------|--------|-------------|
| `len(arr)` | `var n: int = len(arr)` | Returns array length |
| `rev(arr)` | `var r: int[] = rev(arr)` | Returns a new reversed array |
| `push(elem, arr)` | `var a: int[] = push(5, arr)` | Returns new array with element appended |
| `pop(arr)` | `var elem: int = pop(arr)` | Removes and returns last element |
| `rem(index, arr)` | `var a: int[] = rem(2, arr)` | Returns new array with element at index removed |
| `ins(elem, index, arr)` | `var a: int[] = ins(99, 0, arr)` | Returns new array with element inserted at index |

**Example Usage:**
```sn
var nums: int[] = {10, 20, 30, 40, 50}
var length: int = len(nums)           // 5
var reversed: int[] = rev(nums)       // [50, 40, 30, 20, 10]
var extended: int[] = push(60, nums)  // [10, 20, 30, 40, 50, 60]
var removed: int[] = rem(2, nums)     // [10, 20, 40, 50]
var inserted: int[] = ins(99, 0, nums) // [99, 10, 20, 30, 40, 50]
```

**Files Modified:**
- `parser.c` - Register built-in functions in symbol table
- `type_checker_expr.c` - Type checking for built-in function calls
- `runtime.c/h` - Added `rt_array_rev_*()`, `rt_array_rem_*()`, `rt_array_ins_*()`, `rt_array_push_copy_*()` functions
- `code_gen.c` - Added extern declarations for new runtime functions
- `code_gen_expr.c` - Generate runtime calls for built-in functions
- `test_utils.h` - Updated expected header for code gen tests

---

### Phase 3: Additional Array Methods - ✅ COMPLETE

Extended method syntax for arrays is now available:

| Method | Syntax | Description |
|--------|--------|-------------|
| `.indexOf(elem)` | `var idx: int = arr.indexOf(30)` | Find first index of element (-1 if not found) |
| `.contains(elem)` | `var has: bool = arr.contains(30)` | Check if element exists |
| `.clone()` | `var copy: int[] = arr.clone()` | Create a deep copy of the array |
| `.join(sep)` | `var s: str = arr.join(", ")` | Join elements into a string |
| `.reverse()` | `arr.reverse()` | Reverse the array in-place |
| `.insert(elem, idx)` | `arr.insert(99, 2)` | Insert element at index (in-place) |
| `.remove(idx)` | `arr.remove(2)` | Remove element at index (in-place) |

**Example Usage:**
```sn
var nums: int[] = {10, 20, 30, 40, 50}

// Query methods
var idx: int = nums.indexOf(30)     // 2
var has: bool = nums.contains(30)   // true
var copy: int[] = nums.clone()      // [10, 20, 30, 40, 50]
var joined: str = nums.join(", ")   // "10, 20, 30, 40, 50"

// Mutating methods
var arr: int[] = {1, 2, 3, 4, 5}
arr.reverse()                       // arr = [5, 4, 3, 2, 1]
arr.insert(99, 2)                   // arr = [5, 4, 99, 3, 2, 1]
arr.remove(2)                       // arr = [5, 4, 3, 2, 1]
```

**Files Modified:**
- `type_checker_expr.c` - Added type checking for `.indexOf()`, `.contains()`, `.clone()`, `.join()`, `.reverse()`, `.insert()`, `.remove()`
- `runtime.c/h` - Added `rt_array_indexOf_*()`, `rt_array_contains_*()`, `rt_array_clone_*()`, `rt_array_join_*()` functions
- `code_gen.c` - Added extern declarations for new runtime functions
- `code_gen_expr.c` - Generate runtime calls for new methods
- `test_utils.h` - Updated expected header for code gen tests

---

## Current State Analysis

### What's Working

| Feature | Status | Location |
|---------|--------|----------|
| Array type declarations (`int[]`, `str[]`) | ✅ Complete | `parser_util.c`, `ast.h` |
| Array literal syntax (`{1, 2, 3}`) | ✅ Complete | `parser_expr.c` |
| Array access (`arr[i]`) | ✅ Complete | `parser_expr.c`, `code_gen_expr.c` |
| Dynamic array runtime | ✅ Complete | `runtime.c/h` |
| `.length` property | ✅ Complete | `type_checker_expr.c` |
| `.push()` method | ✅ Complete | `type_checker_expr.c`, `runtime.c` |
| `.pop()` method | ✅ Complete | `type_checker_expr.c`, `runtime.c` |
| `.clear()` method | ✅ Complete | `type_checker_expr.c`, `runtime.c` |
| `.concat()` method | ✅ Complete | `type_checker_expr.c`, `runtime.c` |
| **Array slicing (`arr[1..3]`)** | ✅ Complete | Phase 1 |
| **Half-open slices (`arr[2..]`, `arr[..3]`)** | ✅ Complete | Phase 1 |
| **Full copy slice (`arr[..]`)** | ✅ Complete | Phase 1 |
| **Standalone functions (`len`, `rev`, `push`, `pop`, `rem`, `ins`)** | ✅ Complete | Phase 2 |
| **`.indexOf()` method** | ✅ Complete | Phase 3 |
| **`.contains()` method** | ✅ Complete | Phase 3 |
| **`.clone()` method** | ✅ Complete | Phase 3 |
| **`.join()` method** | ✅ Complete | Phase 3 |
| **`.reverse()` method** | ✅ Complete | Phase 3 |
| **`.insert()` method** | ✅ Complete | Phase 3 |
| **`.remove()` method** | ✅ Complete | Phase 3 |
| **For-each iteration (`for x in arr`)** | ✅ Complete | Phase 4 |
| **Negative indexing (`arr[-1]`)** | ✅ Complete | Phase 5 |
| **Array equality (`==`, `!=`)** | ✅ Complete | Phase 5 |
| **Step slicing (`arr[0..10:2]`)** | ✅ Complete | Phase 5 |

### Phase 4: For-Each Iteration - ✅ COMPLETE

The for-each iteration syntax is now available:

```sn
var items: int[] = {10, 20, 30, 40, 50}

// Iterate over elements
for x in items =>
    print($"x = {x}\n")

// Use for summing
var total: int = 0
for n in items =>
    total = total + n

// Works with string arrays too
var names: str[] = {"Alice", "Bob", "Charlie"}
for name in names =>
    print($"{name} ")
```

**Implementation Details:**
- Added `TOKEN_IN` keyword to the lexer
- Added `STMT_FOR_EACH` statement type to AST
- Parser detects `for <var> in <array>` syntax vs traditional `for` loop
- Type checker infers loop variable type from array element type
- Code generator desugars to indexed for loop with temporary variables

**Generated C Code:**
```c
{
    long *__arr_0__ = items;
    long __len_0__ = rt_array_length(__arr_0__);
    for (long __idx_0__ = 0; __idx_0__ < __len_0__; __idx_0__++) {
        long x = __arr_0__[__idx_0__];
        // body
    }
}
```

**Files Modified:**
- `token.h/c` - Added `TOKEN_IN`
- `lexer_scan.c` - Recognize `in` keyword
- `ast.h` - Added `STMT_FOR_EACH` and `ForEachStmt`
- `ast_stmt.c` - Added `ast_create_for_each_stmt()`
- `ast_print.c` - Print for-each statements
- `parser_stmt.c` - Parse for-each syntax
- `type_checker_stmt.c` - Type check for-each loops
- `code_gen_stmt.c` - Generate desugared loop code
- `runtime.c/h` - Added `rt_array_create_*()` functions for array literals

---

### Phase 5: Advanced Features - ✅ COMPLETE

Advanced array features are now available:

| Feature | Syntax | Description |
|---------|--------|-------------|
| Negative indexing | `arr[-1]` | Access elements from the end |
| Array equality | `arr1 == arr2` | Element-wise comparison |
| Step slicing | `arr[..:2]` | Slice with step/stride |

**Example Usage:**
```sn
var arr: int[] = {10, 20, 30, 40, 50}

// Negative indexing
var last: int = arr[-1]              // 50
var second_last: int = arr[-2]       // 40
var tail: int[] = arr[-2..]          // [40, 50]
var without_last: int[] = arr[..-1]  // [10, 20, 30, 40]

// Array equality
var a1: int[] = {1, 2, 3}
var a2: int[] = {1, 2, 3}
var eq: bool = (a1 == a2)            // true
var ne: bool = (a1 != a2)            // false

// Step slicing
var nums: int[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}
var evens: int[] = nums[..:2]        // [0, 2, 4, 6, 8]
var odds: int[] = nums[1..:2]        // [1, 3, 5, 7, 9]
var thirds: int[] = nums[..:3]       // [0, 3, 6, 9]
var sub_step: int[] = nums[1..8:2]   // [1, 3, 5, 7]
var neg_step: int[] = nums[-6..:2]   // [4, 6, 8]
```

**Files Modified:**
- `parser_expr.c` - Parse negative indices and step syntax
- `runtime.c/h` - Handle negative indices in all access/slice functions, added step parameter
- `code_gen_expr.c` - Generate runtime calls with step parameter
- `type_checker_expr.c` - Added array equality type checking
- `code_gen.c` - Added extern declarations for array equality functions

---

### What's Missing (Future Phases)

| Feature | Priority | Complexity |
|---------|----------|------------|
| Higher-order functions (map, filter) | Low | High |
| Array comprehensions | Low | High |
| Spread operator | Low | Medium |

---

## Full Feature Set Specification

### 1. Array Declaration & Initialization

```sn
// Empty array
var nums: int[] = {}

// Array literal
var nums: int[] = {1, 2, 3, 4, 5}

// Multi-dimensional arrays
var matrix: int[][] = {{1, 2}, {3, 4}}

// Type inference (future consideration)
var nums = {1, 2, 3}  // inferred as int[]
```

### 2. Array Access Operations

```sn
// Single element access (0-indexed)
var first: int = nums[0]
var last: int = nums[len(nums) - 1]

// Bounds checking (runtime error on out-of-bounds)
var x: int = nums[100]  // Runtime error

// Negative indexing (convenience feature)
var last: int = nums[-1]    // Last element
var second_last: int = nums[-2]
```

### 3. Slicing Operations

Slicing creates a **new array** (copy semantics, not a view).

```sn
var arr: int[] = {1, 2, 3, 4, 5}

// Full slice with start and end (exclusive end)
var slice: int[] = arr[1..4]     // {2, 3, 4}

// Half-open slices
var tail: int[] = arr[2..]       // {3, 4, 5} - from index to end
var head: int[] = arr[..3]       // {1, 2, 3} - from start to index

// Full copy
var copy: int[] = arr[..]        // {1, 2, 3, 4, 5}

// With negative indices
var last_two: int[] = arr[-2..]  // {4, 5}
var without_last: int[] = arr[..-1]  // {1, 2, 3, 4}

// Step slicing (optional, lower priority)
var evens: int[] = arr[0..5:2]   // {1, 3, 5} - every 2nd element
var reversed: int[] = arr[4..0:-1]  // {5, 4, 3, 2, 1}
```

### 4. Built-in Functions (Standalone)

These work as function calls with the array as an argument:

```sn
// Length
var size: int = len(arr)

// Push (returns new array with element appended)
var extended: int[] = push(6, arr)

// Pop (returns new array with last element removed)
var shorter: int[] = pop(arr)

// Reverse (returns new reversed array)
var backwards: int[] = rev(arr)

// Remove at index (returns new array)
var without: int[] = rem(2, arr)

// Insert at index (returns new array)
var with_insert: int[] = ins(99, 0, arr)

// Concatenation
var combined: int[] = concat(arr1, arr2)

// Find index of element (-1 if not found)
var idx: int = find(3, arr)

// Contains check
var has_it: bool = contains(3, arr)

// Sort (returns new sorted array)
var sorted: int[] = sort(arr)
```

### 5. Method Syntax (Object-Oriented Style)

In-place mutations where applicable:

```sn
var arr: int[] = {1, 2, 3}

// Properties
var size: int = arr.length

// Mutating methods (modify in place)
arr.push(4)           // arr is now {1, 2, 3, 4}
var last: int = arr.pop()  // Returns 4, arr is now {1, 2, 3}
arr.clear()           // arr is now {}
arr.insert(99, 0)     // Insert 99 at index 0
arr.remove(1)         // Remove element at index 1
arr.reverse()         // Reverse in place

// Non-mutating methods (return new values)
var idx: int = arr.indexOf(2)     // Returns index or -1
var has: bool = arr.contains(2)   // Returns true/false
var copy: int[] = arr.clone()     // Deep copy
var str: str = arr.join(", ")     // "1, 2, 3"
```

### 6. Iteration Support

```sn
// Traditional for loop
for var i: int = 0; i < len(arr); i++ =>
    print($"{arr[i]}\n")

// For-each loop (syntactic sugar)
for x in arr =>
    print($"{x}\n")

// For-each with index
for i, x in arr =>
    print($"arr[{i}] = {x}\n")

// Range-based iteration
for i in 0..10 =>
    print($"{i}\n")
```

### 7. String as Character Array

Strings should support array operations seamlessly:

```sn
var text: str = "hello"

// Length
var size: int = len(text)  // 5

// Character access
var first: char = text[0]  // 'h'

// Slicing
var sub: str = text[1..4]  // "ell"

// Push/pop for strings
text = push('!', text)     // "hello!"
text = pop(text)           // "hello"

// Reverse
var rev: str = rev(text)   // "olleh"
```

---

## Implementation Plan

### Phase 1: Core Slicing (High Priority)

**Goal:** Implement the `..` range operator and slice syntax.

#### 1.1 Lexer Updates
- Add `TOKEN_RANGE` for `..` operator
- Ensure proper tokenization in array context

**Files:** `lexer.c`, `token.h`

#### 1.2 Parser Updates
- Extend `parser_array_access()` to handle slice syntax
- Add `EXPR_ARRAY_SLICE` AST node type
- Parse forms: `arr[a..b]`, `arr[a..]`, `arr[..b]`, `arr[..]`

**Files:** `parser_expr.c`, `ast.h`, `ast.c`

```c
typedef struct {
    Expr *array;
    Expr *start;  // NULL for arr[..b]
    Expr *end;    // NULL for arr[a..]
} ArraySliceExpr;
```

#### 1.3 Type Checker Updates
- Type check slice expressions
- Ensure start/end are numeric
- Result type is same as input array type

**Files:** `type_checker_expr.c`

#### 1.4 Runtime Support
- Add slice functions for each type:
  - `rt_array_slice_long(arr, start, end)`
  - `rt_array_slice_double(arr, start, end)`
  - etc.

**Files:** `runtime.c`, `runtime.h`

#### 1.5 Code Generation
- Generate calls to runtime slice functions

**Files:** `code_gen_expr.c`

---

### Phase 2: Standalone Built-in Functions (High Priority)

**Goal:** Implement `len()`, `push()`, `pop()`, `rev()`, `rem()`, `ins()` as callable functions.

#### 2.1 Built-in Function Registry
- Create a registry of built-in functions
- Define signatures for array functions

**Files:** `builtins.c`, `builtins.h` (new files)

#### 2.2 Type Checker Updates
- Recognize built-in function calls
- Type check arguments appropriately
- Handle polymorphic nature (works with any array type)

**Files:** `type_checker_expr.c`

#### 2.3 Runtime Functions
- Implement standalone versions:
  - `rt_len(arr)` - already exists as `rt_array_length`
  - `rt_push_*()` - wrap existing push with correct return
  - `rt_pop_*()` - wrap existing pop
  - `rt_rev_*()` - new reverse implementation
  - `rt_rem_*()` - new remove at index
  - `rt_ins_*()` - new insert at index

**Files:** `runtime.c`, `runtime.h`

#### 2.4 Code Generation
- Emit calls to appropriate runtime functions based on element type

**Files:** `code_gen_expr.c`

---

### Phase 3: Additional Methods (Medium Priority)

**Goal:** Extend method syntax with more operations.

#### 3.1 New Methods to Add
- `.reverse()` - in-place reverse
- `.insert(elem, idx)` - insert at position
- `.remove(idx)` - remove at position
- `.indexOf(elem)` - find first index
- `.contains(elem)` - check membership
- `.clone()` - deep copy
- `.join(separator)` - string representation

#### 3.2 Implementation Approach
- Extend type checker method resolution
- Add runtime implementations for each
- Generate appropriate code

**Files:** `type_checker_expr.c`, `runtime.c`, `code_gen_expr.c`

---

### Phase 4: For-Each Iteration (Medium Priority)

**Goal:** Add `for x in arr` syntax.

#### 4.1 Parser Updates
- Add `STMT_FOR_EACH` statement type
- Parse `for <var> in <expr>` syntax
- Optional: `for <idx>, <var> in <expr>`

**Files:** `parser_stmt.c`, `ast.h`

#### 4.2 Type Checker
- Infer loop variable type from array element type
- Validate iterable is actually an array or string

**Files:** `type_checker_stmt.c`

#### 4.3 Code Generation
- Desugar to standard for loop:
```c
for (int __i = 0; __i < rt_array_length(arr); __i++) {
    int x = arr[__i];
    // body
}
```

**Files:** `code_gen_stmt.c`

---

### Phase 5: Advanced Features (Lower Priority)

#### 5.1 Negative Indexing
- Runtime bounds adjustment
- `arr[-1]` → `arr[len(arr) - 1]`

#### 5.2 Array Equality
- Element-wise comparison
- Add `==` and `!=` for arrays

#### 5.3 Multi-dimensional Array Literals
- Nested brace parsing
- Type validation for rectangular arrays

#### 5.4 Higher-Order Functions
- `map(fn, arr)` - transform elements
- `filter(fn, arr)` - select elements
- `reduce(fn, init, arr)` - fold operation

---

## Convenience Features (Quality of Life)

### Spread Operator
```sn
var arr1: int[] = {1, 2, 3}
var arr2: int[] = {0, ...arr1, 4}  // {0, 1, 2, 3, 4}
```

### Array Destructuring
```sn
var arr: int[] = {1, 2, 3}
var [first, second, ...rest]: int = arr
// first = 1, second = 2, rest = {3}
```

### Array Comprehensions
```sn
var squares: int[] = [x * x for x in 1..10]
var evens: int[] = [x for x in arr if x % 2 == 0]
```

### Range Literals
```sn
var range: int[] = 1..10        // {1, 2, 3, ..., 9}
var inclusive: int[] = 1...10   // {1, 2, 3, ..., 10}
```

---

## Testing Strategy

### Unit Tests

1. **Lexer Tests**
   - Range token recognition
   - Slice syntax tokenization

2. **Parser Tests**
   - Slice expression parsing
   - For-each statement parsing
   - Built-in function call parsing

3. **Type Checker Tests**
   - Slice type inference
   - Built-in function type checking
   - For-each variable typing

4. **Code Generation Tests**
   - Slice code output
   - Built-in function calls
   - For-each loop desugaring

### Integration Tests

Create `compiler/tests/integration/` test files:

```sn
// arrays_slice.sn - Test all slicing operations
// arrays_builtins.sn - Test all built-in functions
// arrays_methods.sn - Test all method calls
// arrays_foreach.sn - Test for-each iteration
// arrays_strings.sn - Test strings as char arrays
```

---

## Implementation Order Summary

| Phase | Features | Effort | Impact | Status |
|-------|----------|--------|--------|--------|
| **Phase 1** | Slicing (`arr[a..b]`) | High | High | ✅ Complete |
| **Phase 2** | Built-in functions (`len`, `push`, etc.) | Medium | High | ✅ Complete |
| **Phase 3** | Additional methods (`.reverse()`, etc.) | Medium | Medium | ✅ Complete |
| **Phase 4** | For-each iteration | Medium | High | ✅ Complete |
| **Phase 5** | Advanced features (negative indexing, equality, step slicing) | High | Medium | ✅ Complete |

---

## Technical Notes

### Memory Management

Arrays use a metadata header approach:
```c
typedef struct {
    size_t size;
    size_t capacity;
} ArrayMetadata;

// Metadata is stored before the data pointer
// arr points to the first element, metadata is at arr[-1]
```

All slice operations create **new arrays** (copy semantics). This simplifies reasoning but has performance implications for large arrays.

### Type Safety

The runtime uses type-specific functions generated via macros:
```c
#define DEFINE_ARRAY_PUSH(type, suffix) \
    type* rt_array_push_##suffix(type* arr, type elem) { ... }
```

This ensures type safety at the C level while the Sn type checker ensures correctness at compile time.

### String/Array Unification

Strings are represented as `char*` with the same metadata header as arrays. This allows unified treatment:
- `len("hello")` works like `len(arr)`
- `"hello"[1..3]` works like `arr[1..3]`
- `push('!', text)` appends to strings

---

## Design Decisions (Resolved)

1. **Copy vs View Semantics for Slices**
   - **Decision:** Copy semantics (simpler, safer)

2. **Negative Index Behavior**
   - **Decision:** Python-style wrap-around at runtime
   - `arr[-1]` → `arr[len - 1]`
   - Out-of-bounds after normalization returns NULL

3. **Empty Slice Behavior**
   - **Decision:** Returns empty array
   - `arr[5..3]` → `[]` (empty)
   - Bounds are clamped to valid range

4. **Slice Step Syntax**
   - **Decision:** Implemented `arr[start..end:step]` syntax
   - Step must be positive (negative step not supported)
   - Default step is 1

5. **Mutability**
   - **Decision:** Mixed approach
   - Methods (`.push()`, `.reverse()`) mutate in place
   - Functions (`push()`, `rev()`) return new arrays

---

## Conclusion

This plan provides a roadmap for implementing a comprehensive array system in the Sn language. The phased approach prioritizes the most impactful features (slicing and built-in functions) while leaving room for advanced features in later phases.

The key principles are:
1. **Consistency**: Functions and methods behave predictably
2. **Safety**: Bounds checking and type safety throughout
3. **Ergonomics**: Multiple syntaxes for common operations
4. **Simplicity**: Copy semantics over complex views
5. **Interoperability**: Strings work like character arrays

Starting with Phase 1 (slicing) will provide immediate value and establish patterns for subsequent phases.
