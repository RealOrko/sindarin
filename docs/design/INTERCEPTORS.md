# Interceptors Design Document

## Overview

This document specifies the design for method interception in Sindarin, enabling debugging, profiling, mocking, and AOP-style programming patterns.

## Goals

1. **Simple API** - Easy to register and use interceptors
2. **Full control** - Access to function name, arguments, and return values
3. **Composable** - Multiple interceptors can be chained
4. **Zero overhead when disabled** - No performance impact when no interceptors registered
5. **Type-safe foundation** - Built on a proper `any` type with runtime type checking

## Non-Goals

- Intercepting native functions (external C functions)
- Intercepting built-in type methods (`TextFile`, `TcpListener`, `Random`, `Date`, etc.)
- Compile-time AOP weaving
- Modifying function bytecode at runtime

## Scope

**Intercepted:** User-defined functions declared with `fn`

**Not intercepted:**
- Built-in type methods (`TextFile.open()`, `TcpStream.write()`, etc.)
- Native functions (`native fn`)
- Built-in functions (`print`, `len`, `exit`, `assert`)

---

## Part 1: The `any` Type

### 1.1 Syntax

```sindarin
var x: any = 42
var y: any = "hello"
var z: any = true
var arr: any[] = {1, "two", false}
```

### 1.2 Runtime Representation

The `any` type is represented as a tagged union in C:

```c
typedef enum {
    RT_ANY_INT,
    RT_ANY_LONG,
    RT_ANY_DOUBLE,
    RT_ANY_FLOAT,
    RT_ANY_STRING,
    RT_ANY_CHAR,
    RT_ANY_BOOL,
    RT_ANY_BYTE,
    RT_ANY_ARRAY,
    RT_ANY_FUNCTION,
    RT_ANY_NIL,
    // Built-in object types
    RT_ANY_TEXT_FILE,
    RT_ANY_BINARY_FILE,
    RT_ANY_DATE,
    RT_ANY_TIME,
    RT_ANY_PROCESS,
    RT_ANY_TCP_LISTENER,
    RT_ANY_TCP_STREAM,
    RT_ANY_UDP_SOCKET,
    RT_ANY_RANDOM,
    RT_ANY_UUID
} RtAnyTag;

typedef struct {
    RtAnyTag tag;
    union {
        int64_t i64;        // int, long
        int32_t i32;        // int32
        uint64_t u64;       // uint
        uint32_t u32;       // uint32
        double d;           // double
        float f;            // float
        char *s;            // str
        char c;             // char
        bool b;             // bool
        uint8_t byte;       // byte
        RtArray *arr;       // arrays
        void *fn;           // function pointers
        void *obj;          // object types (files, etc.)
    } value;
    // For arrays: track element type for nested any[] support
    RtAnyTag element_tag;  // Only used when tag == RT_ANY_ARRAY
} RtAny;
```

### 1.3 Boxing (Converting to `any`)

When assigning a concrete type to `any`, the compiler generates boxing code:

```sindarin
var x: int = 42
var a: any = x  // Boxing
```

Generated C:
```c
int64_t x = 42;
RtAny a = rt_box_int(x);
```

Boxing functions:
```c
RtAny rt_box_int(int64_t value);
RtAny rt_box_long(int64_t value);
RtAny rt_box_double(double value);
RtAny rt_box_string(RtArena *arena, const char *value);
RtAny rt_box_char(char value);
RtAny rt_box_bool(bool value);
RtAny rt_box_byte(uint8_t value);
RtAny rt_box_array(RtArray *value, RtAnyTag element_tag);
RtAny rt_box_function(void *fn);
RtAny rt_box_nil(void);
// Object types
RtAny rt_box_text_file(RtTextFile *value);
// ... etc for other object types
```

### 1.4 Unboxing (Converting from `any`)

Explicit cast required with `as` operator:

```sindarin
var a: any = 42
var x: int = a as int  // Unboxing - panics if a is not int
```

Generated C:
```c
RtAny a = rt_box_int(42);
int64_t x = rt_unbox_int(a);  // Panics if a.tag != RT_ANY_INT
```

Unboxing functions:
```c
int64_t rt_unbox_int(RtAny value);     // Panics if not RT_ANY_INT
double rt_unbox_double(RtAny value);   // Panics if not RT_ANY_DOUBLE
const char *rt_unbox_string(RtAny value);
// ... etc
```

### 1.5 Comparison Operators

The `any` type supports equality and comparison operators:

```sindarin
var a: any = 42
var b: any = 42
var c: any = "hello"

if a == b =>
    print("Equal\n")      // true - same type and value

if a != c =>
    print("Not equal\n")  // true - different types

if a == 42 =>
    print("Mixed\n")      // true - concrete value boxed for comparison
```

Comparison semantics:
- `==` / `!=`: Compare type tag first, then value if types match
- Different types are never equal (e.g., `42 != "42"`)
- Arrays compare element-by-element
- Comparing `any` with a concrete type boxes the concrete value first

Generated C:
```c
// a == b
rt_any_equals(a, b)  // Returns bool
```

---

## Part 2: Type Operators

### 2.1 `typeof` Operator

Returns a type value. Can be used with or without brackets:

```sindarin
var a: any = 42

// With brackets
if typeof(a) == typeof(int) =>
    print("It's an int\n")

// Without brackets
if typeof a == typeof int =>
    print("It's an int\n")

// Compare two values
var b: any = 100
if typeof a == typeof b =>
    print("Same type\n")
```

The `typeof` operator:
- Applied to a value: returns the runtime type
- Applied to a type name: returns that type (for comparison)
- Returns a `type` value (new primitive type for type comparisons)

Generated C:
```c
// typeof(a) where a is any
RtTypeTag __t1 = a.tag;

// typeof(int) - compile-time constant
RtTypeTag __t2 = RT_ANY_INT;

// Comparison
if (__t1 == __t2) { ... }
```

### 2.2 `is` Type Check

Syntactic sugar for type comparison. These are equivalent:

```sindarin
var a: any = 42

// Using 'is' (preferred for readability)
if a is int =>
    print("It's an int\n")

// Using typeof comparison (equivalent)
if typeof a == typeof int =>
    print("It's an int\n")
```

The `is` operator:
- Left operand must be `any` type
- Right operand must be a type name
- Returns `bool`
- Equivalent to `typeof left == typeof right`

Generated C:
```c
// a is int
if (a.tag == RT_ANY_INT) {
    // ...
}
```

### 2.3 `as` Cast Operator

Casts an `any` value to a concrete type:

```sindarin
var a: any = 42
var x: int = a as int
```

Semantics:
- If the type matches, returns the unboxed value
- If the type doesn't match, **panics** with a clear error message
- Can only cast `any` to concrete types (not concrete to concrete)

Generated C:
```c
int64_t x = rt_unbox_int(a);  // Panics: "Type error: expected int, got str"
```

---

## Part 3: Interceptor API

### 3.1 Handler Signature

```sindarin
fn my_interceptor(name: str, args: any[], continue: fn(): any): any =>
    // Pre-call logic
    var result: any = continue()
    // Post-call logic
    return result
```

Parameters:
- `name: str` - Function name (e.g., `"add"`, `"getUserName"`)
- `args: any[]` - Array of boxed arguments (mutable - can modify before continue)
- `continue: fn(): any` - Thunk that calls the original function with current args

Return:
- `any` - The value to return to the caller (can be original result or substituted)

### 3.2 Registration API

```sindarin
// Register an interceptor for ALL user-defined functions
Interceptor.register(handler: fn(str, any[], fn(): any): any): void

// Register an interceptor with pattern matching
// Wildcard (*) can appear at start, middle, or end
Interceptor.registerWhere(handler: fn(str, any[], fn(): any): any, pattern: str): void

// Pattern examples:
Interceptor.registerWhere(handler, "get*")      // Matches: getUser, getName, get
Interceptor.registerWhere(handler, "*User")     // Matches: getUser, deleteUser, User
Interceptor.registerWhere(handler, "get*Name")  // Matches: getUserName, getFileName
Interceptor.registerWhere(handler, "*")         // Matches all (same as register)

// Get all registered interceptors
Interceptor.interceptors: [fn(str, any[], fn(): any): any]

// Clear all interceptors
Interceptor.clearAll(): void

// Check if currently inside an interceptor call (for recursion detection)
Interceptor.isActive(): bool
```

### 3.3 Interceptor Type

`Interceptor` is a built-in namespace (like `Path`, `Directory`, `Stdin`) with static methods only. It cannot be instantiated.

---

## Part 4: Code Generation

### 4.1 Call Site Transformation

Every non-native function call is wrapped with intercept logic:

**Original Sindarin:**
```sindarin
var result: int = add(1, 2)
```

**Generated C (simplified):**
```c
int64_t result;
if (__rt_interceptor_count > 0) {
    // Box arguments
    RtAny __args[2];
    __args[0] = rt_box_int(1);
    __args[1] = rt_box_int(2);

    // Create continue thunk (captures args by reference)
    RtAny __continue_result;
    void __continue_thunk(void) {
        int64_t __unboxed_result = add(
            rt_unbox_int(__args[0]),
            rt_unbox_int(__args[1])
        );
        __continue_result = rt_box_int(__unboxed_result);
    }

    // Call through interceptor chain
    RtAny __intercepted_result = __rt_call_intercepted(
        "add",
        __args, 2,
        __continue_thunk
    );

    // Unbox result
    result = rt_unbox_int(__intercepted_result);
} else {
    // Direct call when no interceptors
    result = add(1, 2);
}
```

### 4.2 Interceptor Chain Execution

The runtime chains multiple interceptors:

```c
RtAny __rt_call_intercepted(
    const char *name,
    RtAny *args,
    int arg_count,
    void (*original_thunk)(void)
) {
    // Build the chain: interceptor[n] -> interceptor[n-1] -> ... -> original
    // Each interceptor's continue calls the next in chain
}
```

### 4.3 Optimization: Skip When No Interceptors

The `if (__rt_interceptor_count > 0)` check ensures zero overhead when no interceptors are registered. The counter is updated atomically on register/clearAll.

---

## Part 5: Runtime Library Additions

### 5.1 New Files

```
src/runtime/runtime_any.h      - RtAny type definition and boxing/unboxing
src/runtime/runtime_any.c      - Implementation
src/runtime/runtime_intercept.h - Interceptor registry
src/runtime/runtime_intercept.c - Implementation
```

### 5.2 `runtime_any.h`

```c
#ifndef RUNTIME_ANY_H
#define RUNTIME_ANY_H

#include <stdint.h>
#include <stdbool.h>
#include "runtime_arena.h"
#include "runtime_array.h"

typedef enum { /* RtAnyTag values */ } RtAnyTag;
typedef struct { /* RtAny definition */ } RtAny;

// Boxing
RtAny rt_box_int(int64_t value);
RtAny rt_box_double(double value);
RtAny rt_box_string(RtArena *arena, const char *value);
RtAny rt_box_bool(bool value);
// ... etc

// Unboxing (panic on type mismatch)
int64_t rt_unbox_int(RtAny value);
double rt_unbox_double(RtAny value);
const char *rt_unbox_string(RtAny value);
// ... etc

// Type checking
bool rt_any_is_int(RtAny value);
bool rt_any_is_string(RtAny value);
// ... etc

// Type name for error messages and typeof()
const char *rt_any_type_name(RtAny value);

// Comparison
bool rt_any_equals(RtAny a, RtAny b);

// String conversion for debugging
char *rt_any_to_string(RtArena *arena, RtAny value);

#endif
```

### 5.3 `runtime_intercept.h`

```c
#ifndef RUNTIME_INTERCEPT_H
#define RUNTIME_INTERCEPT_H

#include "runtime_any.h"

// Handler function type
typedef RtAny (*RtInterceptHandler)(
    const char *name,
    RtAny *args,
    int arg_count,
    RtAny (*continue_fn)(void)
);

// Global interceptor count (for fast check at call sites)
extern volatile int __rt_interceptor_count;

// Per-thread interception depth (for isActive check)
extern __thread int __rt_intercept_depth;

// Registration
void rt_interceptor_register(RtInterceptHandler handler);
void rt_interceptor_clear_all(void);
int rt_interceptor_count(void);
RtInterceptHandler *rt_interceptor_list(int *out_count);

// Recursion detection
bool rt_interceptor_is_active(void);  // Returns __rt_intercept_depth > 0

// Call interception (called by generated code)
RtAny rt_call_intercepted(
    const char *name,
    RtAny *args,
    int arg_count,
    RtAny (*original_thunk)(void)
);

#endif
```

---

## Part 6: Lexer and Parser Changes

### 6.1 New Tokens

```c
TOKEN_ANY,      // 'any' keyword
TOKEN_TYPEOF,   // 'typeof' keyword
TOKEN_IS,       // 'is' keyword
TOKEN_AS,       // 'as' keyword
```

### 6.2 New AST Nodes

```c
// In ExprType enum:
EXPR_TYPEOF,    // typeof(expr)
EXPR_IS,        // expr is Type
EXPR_AS,        // expr as Type

// TypeofExpr
typedef struct {
    Expr *operand;
} TypeofExpr;

// IsExpr
typedef struct {
    Expr *operand;
    Type *type;
} IsExpr;

// AsExpr
typedef struct {
    Expr *operand;
    Type *type;
} AsExpr;
```

### 6.3 Parsing Rules

```
primary        → ... | 'typeof' '(' expression ')' ;
postfix        → primary ( ... | 'is' type | 'as' type )* ;
type           → ... | 'any' ;
```

---

## Part 7: Type Checker Changes

### 7.1 `any` Type Compatibility

- Any value can be assigned to `any`
- `any` can only be assigned to `any` or cast with `as`
- `any[]` elements are all `any` typed

### 7.2 Operator Type Rules

| Expression | Operand Types | Result Type |
|------------|---------------|-------------|
| `typeof(x)` | `x: any` | `str` |
| `x is T` | `x: any` | `bool` |
| `x as T` | `x: any` | `T` |

### 7.3 Interceptor Handler Validation

When registering an interceptor, validate the handler signature matches:
```
fn(str, any[], fn(): any): any
```

---

## Part 8: Edge Cases and Considerations

### 8.1 Void Functions

Functions returning `void` box their result as `nil`:

```sindarin
fn greet(name: str): void => print($"Hello {name}\n")

// In interceptor:
var result: any = continue()  // result is nil for void functions
```

### 8.2 Recursive Interception

Interceptors are allowed to call intercepted functions, which will trigger interception recursively. This is intentional - it provides maximum flexibility.

To prevent infinite recursion, users can check `Interceptor.isActive()`:

```sindarin
fn safe_interceptor(name: str, args: any[], continue: fn(): any): any =>
    // Avoid recursion: don't intercept calls made from within interceptors
    if Interceptor.isActive() =>
        return continue()

    // Safe to call other functions here - they will be intercepted
    log($"Calling {name}")  // This call to log() will also be intercepted

    return continue()
```

**Implementation:** The runtime tracks interception depth per-thread. `Interceptor.isActive()` returns `true` when depth > 0. The depth is incremented before calling handlers and decremented after.

### 8.3 Thread Safety

The interceptor registry must be thread-safe:
- `__rt_interceptor_count` uses atomic operations
- Registration/clearing uses a mutex
- Handler list is copy-on-write for safe iteration during calls

### 8.4 Memory Management

- Boxed strings in `any` values are arena-allocated
- The args array is allocated on the caller's arena
- Continue thunk result is arena-allocated
- Interceptor handlers should not store references to args beyond their scope

### 8.5 Argument Mutation

When an interceptor modifies `args[i]`:
- The type can change (e.g., set an int arg to a string)
- The continue callback will unbox with the new type
- Type mismatch causes a panic

```sindarin
fn my_interceptor(name: str, args: any[], continue: fn(): any): any =>
    args[0] = "wrong type"  // Was int, now str
    continue()              // PANIC: expected int, got str
```

### 8.6 Shared Functions and Arena

For `shared` functions that receive an arena parameter:
- The arena is NOT included in the `args` array
- The continue callback automatically passes the correct arena
- Interceptors cannot modify the arena parameter

### 8.7 Nested Any Values

Nested `any` values (e.g., `any[]` containing `any[]`) are supported up to 3-5 levels deep:

```sindarin
var nested: any = {{1, 2}, {3, 4}}  // any containing int[][]
var deep: any = {{{1}}}             // 3 levels - supported
```

Unboxing nested values requires casting at each level:

```sindarin
var outer: any = {{1, 2}, {3, 4}}
var inner: any[] = outer as any[]
var first: any = inner[0]
var arr: int[] = first as int[]
```

---

## Part 9: Example Usage

### 9.1 Logging Interceptor

```sindarin
fn log_interceptor(name: str, args: any[], continue: fn(): any): any =>
    var arg_strs: str[] = args.map(fn(a: any): str =>
        if a is str => return $"\"{a as str}\""
        if a is int => return $"{a as int}"
        if a is bool => return $"{a as bool}"
        return "<?>"
    )
    print($"CALL {name}({arg_strs.join(\", \")})\n")

    var result: any = continue()

    print($"RETURN {name} => {result}\n")
    return result

fn main =>
    Interceptor.register(log_interceptor)

    var x: int = add(1, 2)
    // Output:
    // CALL add(1, 2)
    // RETURN add => 3
```

### 9.2 Mocking Interceptor

```sindarin
fn mock_interceptor(name: str, args: any[], continue: fn(): any): any =>
    // Mock the database lookup to return a fixed value
    if name == "getUserName" =>
        return "Mock User"

    // Mock the API call to simulate failure
    if name == "fetchBalance" =>
        return -1

    // All other functions proceed normally
    return continue()

fn main =>
    Interceptor.register(mock_interceptor)

    var name: str = getUserName(123)  // Returns "Mock User", no DB call
    print($"Got: {name}\n")

    var balance: int = fetchBalance(123)  // Returns -1, no API call
    print($"Balance: {balance}\n")
```

### 9.3 Timing Interceptor

```sindarin
fn timing_interceptor(name: str, args: any[], continue: fn(): any): any =>
    var start: Time = Time.now()
    var result: any = continue()
    var elapsed: int = Time.now().diffMillis(start)
    print($"{name} took {elapsed}ms\n")
    return result
```

---

## Part 10: Implementation Order

### Phase 1: `any` Type Foundation
1. Add `any` keyword to lexer
2. Parse `any` type in parser
3. Implement `RtAny` in runtime
4. Add boxing/unboxing code generation
5. Basic type checker support for `any`

### Phase 2: Type Operators
1. Add `typeof`, `is`, `as` tokens
2. Parse new expression types
3. Type check the operators
4. Generate code for operators

### Phase 3: Interceptor Infrastructure
1. Implement `runtime_intercept.c`
2. Add `Interceptor` namespace to parser/type checker
3. Transform call sites in code generator
4. Generate continue thunks

### Phase 4: Polish
1. Thread safety
2. Error messages
3. Edge case handling
4. Documentation
5. Tests

---

## Appendix A: Alternative Designs Considered

### A.1 String-based Arguments

Instead of `any[]`, pass arguments as `str[]`:

**Pros:** No new type system, simpler
**Cons:** Lossy (can't reconstruct values), can't modify arguments

### A.2 Typed Per-Function Interceptors

Register interceptors for specific function signatures:

**Pros:** Type-safe, no `any` needed
**Cons:** Verbose, hard to write generic interceptors

### A.3 Compile-time Weaving

Insert interceptor calls at compile time based on annotations:

**Pros:** Zero runtime registration overhead
**Cons:** Less flexible, can't enable/disable at runtime

---

## Appendix B: Comparison with Other Languages

| Language | Mechanism | Runtime Type System |
|----------|-----------|---------------------|
| Java | Dynamic proxies, AspectJ | Reflection, `Object` |
| Python | Decorators, `__call__` | Duck typing, `Any` |
| JavaScript | Proxy objects | Dynamic typing |
| Go | No built-in, middleware pattern | `interface{}` |
| Rust | No built-in, proc macros | `dyn Any` |
| **Sindarin** | `Interceptor.register` | `any` type |

---

## Appendix C: Open Questions

(None remaining)
