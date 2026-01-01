# Sindarin Compiler Performance Guide

A practical roadmap for dramatically improving Sindarin's runtime performance. Based on benchmark analysis showing Sindarin 50-100x slower than C in array/string operations.

## Executive Summary

| Bottleneck | Current Impact | Solution | Expected Gain |
|------------|----------------|----------|---------------|
| No sized array allocation | Prime sieve 8000ms vs C's 8ms | Add `type[size]` syntax | 100x |
| O(n²) string concatenation | String ops 1100ms vs C's 2ms | Add `str.append()` | 100x |
| Negative index check on every access | ~15% overhead on loops | Elide when provably non-negative | 10-15% |
| Runtime function call overhead | ~10% overhead | Inline hot functions as macros | 5-10% |
| Checked arithmetic default | 2-4x slower arithmetic | Default unchecked in -O2 | 2-4x on math |

---

## Part 1: Critical Language Features

These require parser, type-checker, and code-gen changes but deliver the biggest wins.

### 1.1 Sized Array Allocation

**The Problem**

Building a 1M-element array requires 1M individual `.push()` calls, each potentially reallocating:

```sindarin
// Current: O(n) function calls + O(n) reallocations = catastrophic
var sieve: bool[] = {}
for var i: int = 0; i <= 1000000; i++ =>
    sieve.push(true)
```

**Proposed Syntax**

```sindarin
var sieve: bool[1000001] = true      // All elements initialized to true
var arr: int[n]                       // Zero-initialized by default
var arr: int[n] = 42                  // All elements set to 42
var matrix: int[rows][cols] = 0       // 2D array (future)
```

**Semantics**

| Declaration | Meaning |
|-------------|---------|
| `var arr: int[n]` | Allocate n elements, zero-initialized |
| `var arr: int[n] = x` | Allocate n elements, all set to x |
| `var arr: int[] = {1,2,3}` | Existing literal syntax (unchanged) |
| `var arr: int[] = {}` | Existing empty array (unchanged) |

The size expression `n` can be a literal or any integer expression (runtime-determined).

**Implementation Steps**

1. **Lexer** (`lexer_scan.c`): Already handles `[` `]`, no changes needed

2. **Parser** (`parser_expr.c`, `parser_util.c`):
   - In type parsing, detect `TYPE[expr]` pattern
   - Create new AST node `EXPR_SIZED_ARRAY_ALLOC` with size expression and optional default value

   ```c
   // parser_util.c - extend parse_type()
   if (match(parser, TOKEN_LEFT_BRACKET)) {
       AstExpr *size_expr = parse_expression(parser);
       expect(parser, TOKEN_RIGHT_BRACKET, "Expected ']' after array size");
       // Return sized array type info
   }
   ```

3. **Type Checker** (`type_checker_expr.c`):
   - Validate size expression is integer type
   - Validate default value matches element type (if provided)
   - Size can be variable (runtime-determined)

4. **Code Generation** (`code_gen_expr.c`):
   ```c
   // Generate for: var arr: int[n] = 0
   case EXPR_SIZED_ARRAY_ALLOC: {
       const char *type = get_c_type(elem_type);
       const char *size = generate_expr(size_expr);
       const char *default_val = default_expr ? generate_expr(default_expr) : "0";

       return arena_sprintf(gen->arena,
           "rt_array_alloc_%s(__arena__, %s, %s)",
           type, size, default_val);
   }
   ```

5. **Runtime** (`runtime.c`):
   ```c
   // Single allocation + memset, returns array with metadata
   long *rt_array_alloc_long(RtArena *arena, size_t count, long default_value) {
       size_t data_size = count * sizeof(long);
       size_t total = sizeof(ArrayMetadata) + data_size;

       ArrayMetadata *meta = rt_arena_alloc(arena, total);
       meta->size = count;
       meta->capacity = count;

       long *data = (long *)(meta + 1);
       if (default_value == 0) {
           memset(data, 0, data_size);
       } else {
           for (size_t i = 0; i < count; i++) data[i] = default_value;
       }
       return data;
   }
   ```

**Files to Modify**
- `src/parser/parser_util.c` - Type parsing
- `src/parser/parser_expr.c` - Expression parsing
- `src/ast.h` - New AST node type
- `src/type_checker/type_checker_expr.c` - Type validation
- `src/code_gen/code_gen_expr.c` - C code generation
- `src/runtime.c` - Allocation functions
- `src/runtime.h` - Function declarations

**Expected Impact**: Prime sieve 8263ms → ~50ms (165x improvement)

---

### 1.2 Mutable Strings with `.append()`

**The Problem**

String concatenation creates a new string each time, copying all previous content:

```sindarin
// Current: O(n²) - each + copies entire string
var result: str = ""
for var i: int = 0; i < 100000; i++ =>
    result = result + "Hello"
// 100,000 iterations × average 250,000 chars copied = 25 billion char copies
```

**Proposed API**

Add `.append()` method directly to `str` type - no separate StringBuilder needed:

```sindarin
var result: str = ""
for var i: int = 0; i < 100000; i++ =>
    result.append("Hello")           // Mutates in place, O(1) amortized
    result.append($" world {n}")     // Works with interpolation too

// result is already a normal str, no conversion needed
```

**Design Decisions**

1. **Mutable by default**: Strings become growable buffers (like Rust's `String`)
2. **No separate type**: `.append()` is just a method on `str`
3. **Capacity tracking**: Strings internally track `length` and `capacity`
4. **Seamless interop**: Appended strings work everywhere normal strings do

**Implementation Steps**

1. **Extend String Representation** (`runtime.h`):
   ```c
   // String metadata (stored before the char* pointer, like arrays)
   typedef struct RtStringMeta {
       size_t length;      // Current length (excluding null terminator)
       size_t capacity;    // Allocated capacity
       RtArena *arena;     // For reallocation
   } RtStringMeta;

   // Helper to get metadata from string pointer
   #define RT_STR_META(s) ((RtStringMeta*)((char*)(s) - sizeof(RtStringMeta)))
   ```

2. **Runtime Functions** (`runtime.c`):
   ```c
   // Create string with capacity (for append operations)
   char *rt_string_with_capacity(RtArena *arena, size_t capacity) {
       size_t total = sizeof(RtStringMeta) + capacity + 1;
       RtStringMeta *meta = rt_arena_alloc(arena, total);
       meta->length = 0;
       meta->capacity = capacity;
       meta->arena = arena;
       char *str = (char*)(meta + 1);
       str[0] = '\0';
       return str;
   }

   // Append string in place (grows if needed)
   char *rt_string_append(char *dest, const char *src) {
       RtStringMeta *meta = RT_STR_META(dest);
       size_t src_len = strlen(src);
       size_t new_len = meta->length + src_len;

       if (new_len + 1 > meta->capacity) {
           // Grow by 2x
           size_t new_cap = (new_len + 1) * 2;
           char *new_str = rt_string_with_capacity(meta->arena, new_cap);
           memcpy(new_str, dest, meta->length);
           dest = new_str;
           meta = RT_STR_META(dest);
       }

       memcpy(dest + meta->length, src, src_len + 1);
       meta->length = new_len;
       return dest;  // May return new pointer if reallocated
   }
   ```

3. **Handle Pointer Reassignment**

   Since `append` may reallocate, we need to reassign:
   ```c
   // result.append("Hello") generates:
   result = rt_string_append(result, "Hello");
   ```

   This is similar to how array `.push()` works.

4. **Type Checker** (`type_checker_expr.c`):
   - Recognize `.append()` as valid method on `str` type
   - Validate argument is string-compatible
   - Mark expression as requiring reassignment

5. **Code Generation** (`code_gen_expr.c`):
   ```c
   // For method call: str_var.append(arg)
   if (is_string_type(receiver) && strcmp(method, "append") == 0) {
       return arena_sprintf(gen->arena,
           "(%s = rt_string_append(%s, %s))",
           receiver_str, receiver_str, arg_str);
   }
   ```

6. **Literal Strings**: Literal strings remain immutable (no metadata). Calling `.append()` on a literal would first copy it to a mutable string.

**Compatibility with Existing Strings**

- Existing code using `+` continues to work (creates new string)
- `.append()` is opt-in for performance-critical code
- Literal strings passed to functions work unchanged

**Files to Modify**
- `src/runtime.h` - String metadata structure
- `src/runtime.c` - `rt_string_append()`, `rt_string_with_capacity()`
- `src/type_checker/type_checker_expr.c` - Validate `.append()` method
- `src/code_gen/code_gen_expr.c` - Generate append code

**Expected Impact**: String ops 1166ms → ~10ms (100x improvement)

---

## Part 2: Code Generation Optimizations

These don't require language changes, just smarter C code output.

### 2.1 Elide Negative Index Check

**Current Code** (`code_gen_expr.c:2285-2288`):
```c
// Every array access generates:
arr[(idx) < 0 ? rt_array_length(arr) + (idx) : (idx)]
```

This ternary is evaluated even when `idx` is obviously non-negative.

**Optimization Strategy**

Track "provably non-negative" expressions:
- Literal integers >= 0
- Loop counter variables (`for var i: int = 0; ...`)
- Result of `len(arr)` (always >= 0)
- Result of `len(arr) - constant` when constant <= 0
- Variables assigned from non-negative sources

**Implementation**

```c
// code_gen_expr.c - new helper function
static bool is_provably_non_negative(CodeGenerator *gen, AstExpr *expr) {
    switch (expr->type) {
        case EXPR_LITERAL:
            if (expr->literal.type == LITERAL_INT)
                return expr->literal.int_value >= 0;
            break;

        case EXPR_VARIABLE:
            // Check if variable is a loop counter
            // (would need to track this in symbol table or gen context)
            return is_loop_counter(gen, expr->variable.name);

        case EXPR_CALL:
            // len() always returns non-negative
            if (strcmp(expr->call.name, "len") == 0)
                return true;
            break;

        case EXPR_BINARY:
            // len(x) - n is non-negative analysis (complex)
            break;
    }
    return false;
}

// In generate_array_access_expression:
if (is_provably_non_negative(gen, index_expr)) {
    // Direct access - no bounds check
    return arena_sprintf(gen->arena, "%s[%s]", array_str, index_str);
} else {
    // Full negative-index handling
    return arena_sprintf(gen->arena,
        "%s[(%s) < 0 ? rt_array_length(%s) + (%s) : (%s)]",
        array_str, index_str, array_str, index_str, index_str);
}
```

**Quick Win**: Start by just checking for literal non-negative indices:
```c
if (index_expr->type == EXPR_LITERAL &&
    index_expr->literal.type == LITERAL_INT &&
    index_expr->literal.int_value >= 0) {
    return arena_sprintf(gen->arena, "%s[%lld]", array_str, index_expr->literal.int_value);
}
```

**Files to Modify**
- `src/code_gen/code_gen_expr.c:2265-2289`

**Expected Impact**: 10-15% improvement on array-heavy code

---

### 2.2 Hoist Loop-Invariant Length

**Current Code** (`code_gen_stmt.c`):
```c
// for num in arr generates:
for (size_t __idx__ = 0; __idx__ < rt_array_length(arr); __idx__++)
```

The `rt_array_length(arr)` call happens every iteration.

**Optimization**

```c
// Generate instead:
size_t __len__ = rt_array_length(arr);
for (size_t __idx__ = 0; __idx__ < __len__; __idx__++)
```

**Implementation** (`code_gen_stmt.c:763-800`):

```c
static void generate_for_each_statement(CodeGenerator *gen, AstStmt *stmt) {
    const char *arr_expr = generate_expression(gen, stmt->for_each.iterable);

    // Hoist length calculation
    indented_fprintf(gen, gen->indent_level,
        "size_t __len_%d__ = rt_array_length(%s);\n",
        gen->temp_counter, arr_expr);

    indented_fprintf(gen, gen->indent_level,
        "for (size_t __idx_%d__ = 0; __idx_%d__ < __len_%d__; __idx_%d__++) {\n",
        gen->temp_counter, gen->temp_counter, gen->temp_counter, gen->temp_counter);

    // ... rest of loop body
}
```

**Files to Modify**
- `src/code_gen/code_gen_stmt.c` - For-each loop generation

**Expected Impact**: 5-10% improvement on for-each loops

---

### 2.3 Inline Hot Runtime Functions

**Candidates**

| Function | Current | Inlined |
|----------|---------|---------|
| `rt_array_length()` | Function call | Single memory read |
| `rt_bool_to_string()` | Function call | Ternary expression |

**Implementation** (`code_gen.c` - header generation):

Instead of declaring as `extern`, emit as macros:

```c
// In generate_runtime_declarations():
indented_fprintf(gen, 0,
    "#define rt_array_length(arr) "
    "((arr) ? ((ArrayMetadata*)((char*)(arr) - sizeof(ArrayMetadata)))->size : 0)\n");

indented_fprintf(gen, 0,
    "#define rt_bool_to_string(b) ((b) ? \"true\" : \"false\")\n");
```

**Note**: Need to expose `ArrayMetadata` structure in generated code header.

**Files to Modify**
- `src/code_gen.c:192-250` - Runtime declarations

**Expected Impact**: 3-5% improvement overall

---

## Part 3: Arithmetic Optimizations

### 3.1 Default Unchecked in Release Mode

**Current Behavior**
- Default: Checked arithmetic (runtime overflow detection)
- `--unchecked`: Native C operators

**Proposed Behavior**
```bash
bin/sn source.sn -o prog           # Debug: checked (default)
bin/sn source.sn -o prog -O2       # Release: unchecked
bin/sn source.sn -o prog --checked # Force checked even with -O2
```

**Implementation** (`main.c`, `compiler.c`):

```c
// In parse_arguments():
if (strcmp(argv[i], "-O2") == 0) {
    options->optimization_level = 2;
    options->unchecked_arithmetic = true;  // New default for -O2
}
if (strcmp(argv[i], "--checked") == 0) {
    options->unchecked_arithmetic = false;
}
```

**Files to Modify**
- `src/main.c` - Argument parsing
- `src/compiler.c` - Option handling
- `src/code_gen.c:59` - Check flag

**Expected Impact**: 2-4x improvement on arithmetic-heavy code

---

### 3.2 Elide Division Check for Non-Zero Divisors

**Current Code** (`code_gen_util.c:706-709`):
```c
// x / y generates:
rt_div_long(x, y)  // Checks for division by zero
```

**Optimization**: When divisor is provably non-zero, use native `/`:
- Literal != 0
- Loop counter (for i = 1; ...)
- Result of len() on non-empty array

```c
// In generate_binary_expression():
if (expr->binary.op == OP_DIV) {
    if (is_provably_non_zero(gen, expr->binary.right)) {
        return arena_sprintf(gen->arena, "(%s / %s)", left_str, right_str);
    }
    return arena_sprintf(gen->arena, "rt_div_long(%s, %s)", left_str, right_str);
}
```

**Files to Modify**
- `src/code_gen/code_gen_util.c:706-709`
- `src/code_gen/code_gen_expr.c` - Division generation

**Expected Impact**: Minor, but cleaner generated code

---

## Part 4: Memory Optimizations

### 4.1 Boolean Array Bit Packing

**Current**: `bool[]` uses 4 bytes per element (int)

**Optimization**: For large boolean arrays, pack 8 booleans per byte:

```c
// Runtime: bit-packed boolean array
typedef struct RtBitArray {
    size_t size;
    unsigned char data[];
} RtBitArray;

bool rt_bitarray_get(RtBitArray *arr, size_t idx) {
    return (arr->data[idx / 8] >> (idx % 8)) & 1;
}

void rt_bitarray_set(RtBitArray *arr, size_t idx, bool val) {
    if (val)
        arr->data[idx / 8] |= (1 << (idx % 8));
    else
        arr->data[idx / 8] &= ~(1 << (idx % 8));
}
```

**Trigger**: Use bit-packing for `bool[n]` when `n > 64`.

**Trade-off**: 8x less memory, but slightly slower access due to bit manipulation. Net win for cache-bound workloads like prime sieve.

**Expected Impact**: Prime sieve memory 4MB → 500KB, potential 2x speedup from cache effects

---

### 4.2 Stack Allocation for Small Arrays

**Current**: All arrays use arena (heap) allocation.

**Optimization**: For small, non-escaping arrays, use stack:

```c
// var temp: int[4] = {1, 2, 3, 4}
// Generate:
long _temp_data[4] = {1, 2, 3, 4};
long *temp = _temp_data;
```

**Requires**: Escape analysis to ensure array doesn't outlive stack frame.

**Simpler Alternative**: Stack allocate arrays with literal size <= 64 that are:
- Local variables
- Not passed to functions
- Not returned

---

## Part 5: Implementation Roadmap

### Phase 1: Quick Wins (1-2 days each)
High impact, low risk changes:

1. **Inline `rt_array_length()` as macro**
   - Single file change
   - No semantic changes
   - Immediate 3-5% gain

2. **Hoist length in for-each loops**
   - Localized change in `code_gen_stmt.c`
   - No semantic changes
   - 5-10% gain on loops

3. **Elide negative check for literal indices**
   - Simple pattern match
   - `arr[0]`, `arr[1]` etc. become direct access
   - 5% gain on common patterns

### Phase 2: Medium Effort (1 week each)
Requires more testing:

4. **Default unchecked in -O2 mode**
   - CLI flag changes
   - Update documentation
   - 2-4x gain on math

5. **Non-negative index tracking for loop counters**
   - Track loop variables in codegen context
   - Elide checks for `arr[i]` in counted loops
   - 10-15% gain on array loops

### Phase 3: Language Features (2 weeks each)
Major features requiring full pipeline changes:

6. **Sized array allocation**
   - Parser, type-checker, codegen, runtime changes
   - Extensive testing needed
   - 100x gain on allocation-heavy code

7. **String `.append()` method**
   - Mutable string support
   - Runtime + codegen changes
   - 100x gain on string building

### Phase 4: Advanced (ongoing)
Research and exploration:

8. **Boolean bit-packing**
   - Runtime changes
   - Codegen changes for bool array access
   - Memory/cache improvements

9. **Escape analysis for stack allocation**
   - Compiler infrastructure
   - Conservative analysis acceptable

---

## Benchmarking Protocol

After each optimization, measure impact:

```bash
# Build compiler
make build

# Run full benchmark suite
cd benchmark && ./run_all.sh

# Compare results
diff results_before.json results_after.json
```

Track metrics:
- **Execution time** (primary)
- **Generated code size** (secondary)
- **Compilation time** (should not regress significantly)

---

## Files Reference

| Optimization | Primary Files |
|--------------|---------------|
| Sized arrays | `parser_util.c`, `parser_expr.c`, `ast.h`, `type_checker_expr.c`, `code_gen_expr.c`, `runtime.c`, `runtime.h` |
| String `.append()` | `type_checker_expr.c`, `code_gen_expr.c`, `runtime.c`, `runtime.h` |
| Index elision | `code_gen_expr.c:2265-2289` |
| Length hoisting | `code_gen_stmt.c:763-800` |
| Inline runtime | `code_gen.c:192-250` |
| Build modes | `main.c`, `compiler.c`, `code_gen.c:59` |
| Division elision | `code_gen_util.c:706-709` |
