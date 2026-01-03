# C Interoperability in Sindarin

> **DRAFT - NOT IMPLEMENTED** - This document specifies how Sindarin will interface with native C libraries. Design decisions have been finalized; implementation is pending.

Since Sindarin compiles to C, interoperability is natural but requires explicit declarations for external functions, headers, and linking.

---

## Overview

```sindarin
#pragma include "<math.h>"
#pragma link "m"

native fn sin(x: double): double
native fn cos(x: double): double

fn main(): int =>
    var angle: double = 3.14159 / 4.0
    print($"sin(45°) = {sin(angle)}\n")
    print($"cos(45°) = {cos(angle)}\n")
    return 0
```

---

## Native Function Declarations

External C functions are declared using the `native` keyword. These declarations tell the compiler the function exists externally and specify its signature.

### Syntax

```sindarin
native fn <name>(<params>): <return_type>
```

### Examples

```sindarin
// Math library
native fn sin(x: double): double
native fn cos(x: double): double
native fn sqrt(x: double): double

// Standard I/O
native fn puts(s: str): int

// Memory (if exposed)
native fn malloc(size: int): *void
native fn free(ptr: *void): void
```

### Code Generation

Native declarations generate C function prototypes:

```c
// Sindarin: native fn sin(x: double): double
// Generated: (prototype only, no implementation)
extern double sin(double x);
```

---

## Pragma Directives

Pragma statements control compilation behavior for C interop.

### Header Inclusion

```sindarin
#pragma include "<math.h>"
#pragma include "<stdlib.h>"
#pragma include "mylib.h"
```

**Generated C:**
```c
#include <math.h>
#include <stdlib.h>
#include "mylib.h"
```

### Library Linking

```sindarin
#pragma link "m"
#pragma link "pthread"
```

**Compiler behavior:** These directives instruct the compiler to pass `-lm`, `-lpthread` etc. to the C compiler/linker.

---

## Type Mapping

Sindarin types naturally map to C types. This section documents the mappings and identifies gaps.

### Established Mappings

| Sindarin | C | Notes |
|----------|---|-------|
| `int` | `int64_t` | 64-bit signed integer |
| `double` | `double` | 64-bit floating point |
| `bool` | `bool` | Via stdbool.h |
| `byte` | `uint8_t` | Unsigned 8-bit |
| `char` | `char` | Single character |
| `str` | `char*` | Null-terminated string |
| `T[]` | Arena array struct | Length + data pointer |

### Extended Type Mappings

See [Design Decisions > Type Gap Analysis](#6-type-gap-analysis-new-primitive-types) for the complete type mapping.

| C Type | Sindarin | Notes |
|--------|----------|-------|
| `size_t` | `uint` | 64-bit unsigned |
| `int32_t` | `int32` | New type |
| `uint32_t` | `uint32` | New type |
| `uint64_t` | `uint` | New type |
| `float` | `float` | New type |
| `void` | `void` | Already exists |
| `void*` | `*void` | Pointer syntax |
| `unsigned int` | `uint32` | 32-bit unsigned |

### Pass-by Semantics

Sindarin's existing `as val` and `as ref` semantics extend naturally to native interop. See [Design Decisions > Memory Ownership](#2-memory-ownership-using-as-val-and-as-ref) for details.

```sindarin
// Parameters: default = reference, as val = copy
native fn process(data: byte[]): void        // C sees arena pointer
native fn safe_sort(data: int[] as val): void // Copy first, then pass

// Return types: default = copy to arena, as ref = borrowed
native fn strdup(s: str): str                 // Copied into arena
native fn getenv(name: str): str as ref       // Borrowed from C
```

---

## Design Decisions

### 1. Pointer Types: C-Style Syntax with Restrictions

Sindarin uses C-style pointer syntax for native interop, with safety restrictions.

#### Syntax

```sindarin
var p: *int = ...
var pp: **char = ...    // pointer to pointer
var vp: *void = ...     // void pointer
```

#### Null Pointers

The `nil` constant represents a null pointer:

```sindarin
var p: *int = nil

if p != nil =>
    process(p)
```

`nil` is only valid for pointer types. Attempting to use `nil` with non-pointer types is a compile error.

#### Restrictions (Safety First)

1. **No pointer arithmetic** - `p + 1` is a compile error
2. **Pointer variables only in `native` functions** - regular functions cannot store pointers
3. **Immediate unwrapping required** - regular functions must use `as val` when calling pointer-returning natives

#### Unwrapping Pointers with `as val`

The `as val` operator copies the data a pointer points to into the arena:

```sindarin
native fn get_number(): *int
native fn get_name(): *char

// Native function - can work with pointers directly
native fn process_ptr(): int =>
    var p: *int = get_number()      // OK: store pointer
    var val: int = p as val         // Read value from pointer
    return val

// Regular function - must unwrap immediately with 'as val'
fn process(): int =>
    var val: int = get_number() as val    // OK: unwrapped to int
    var name: str = get_name() as val     // OK: unwrapped to str
    return val

// ERROR: cannot store pointer in non-native function
fn bad(): void =>
    var p: *int = get_number()            // Compile error: pointer type not allowed
```

#### `as val` Semantics for Pointers

| Pointer Type | `as val` Result | Behavior |
|--------------|-----------------|----------|
| `*int`, `*double`, etc. | `int`, `double`, etc. | Copies single value |
| `*char` | `str` | Copies null-terminated string to arena |
| `*byte` with length | `byte[]` | Use slice syntax: `ptr[0..len] as val` |

```sindarin
native fn getenv(name: str): *char
native fn get_data(): *byte
native fn get_len(): int

fn example(): void =>
    // C string - null terminated, length implicit
    var home: str = getenv("HOME") as val

    // Buffer - need explicit length via slice syntax
    var len: int = get_len()
    var data: byte[] = get_data()[0..len] as val
```

#### Out Parameters with `as ref`

When C functions need to write results back, use `as ref` parameters:

```sindarin
// C function: void get_dimensions(int* width, int* height)
native fn get_dimensions(width: int as ref, height: int as ref): void

fn example(): void =>
    var w: int = 0
    var h: int = 0
    get_dimensions(w, h)    // C writes to w and h
    print($"Size: {w}x{h}\n")
```

This avoids the need for explicit pointer write syntax - `as ref` parameters let C write directly to Sindarin variables.

#### The `native` Function Boundary

Regular functions can call any native function, but:
- Must immediately unwrap pointer returns with `as val`
- Cannot declare variables of pointer types
- Cannot pass pointers as arguments (unless unwrapped from a call in the same expression)

```sindarin
native fn malloc(size: int): *void
native fn free(ptr: *void): void
native fn get_handle(): *int
native fn use_handle(h: *int): void

// Regular function
fn example(): void =>
    var val: int = get_handle() as val   // OK: unwrapped

    use_handle(get_handle())             // OK: pointer passed directly

    var p: *int = get_handle()           // ERROR: can't store pointer
    use_handle(p)                        // ERROR: can't use stored pointer

    free(malloc(100))                    // OK: pointer passed directly
```

#### Safe Wrappers (Optional)

For complex pointer operations, `native` wrapper functions are still useful:

```sindarin
type FILE = opaque

native fn fopen(path: str, mode: str): *FILE
native fn fclose(f: *FILE): int
native fn fread(buf: byte[], size: int, count: int, f: *FILE): int

// Wrapper handles the pointer lifecycle
native fn read_file(path: str): str =>
    var f: *FILE = fopen(path, "rb")
    if f == nil =>
        panic($"Cannot open: {path}")
    var buf: byte[4096] = {}
    var n: int = fread(buf, 1, 4096, f)
    fclose(f)
    return buf[0..n].toString()

// Regular function uses the safe wrapper
fn process(path: str): void =>
    var data: str = read_file(path)
    print(data)
```

#### Summary

| Context | Can store `*T`? | Can call native returning `*T`? | Can pass `*T` args? |
|---------|-----------------|--------------------------------|---------------------|
| `native fn` | Yes | Yes | Yes |
| Regular `fn` | No | Yes, with `as val` | Only inline from native call |

#### Rationale

- `as val` provides safe unwrapping at call sites
- No need for wrappers just to hide pointers
- Complex lifecycles still benefit from `native` wrappers
- Regular Sindarin code stays pointer-free
- Aligns with "Safety First" principle

---

### 2. Memory Ownership: Using `as val` and `as ref`

Memory ownership for native functions uses the existing `as val` and `as ref` semantics, maintaining consistency with the rest of the language.

#### Pointer Returns: Use Explicit Pointer Types

For C functions returning pointers, use explicit pointer types (`*char`, `*byte`, etc.) and unwrap with `as val`:

```sindarin
native fn getenv(name: str): *char
native fn get_buffer(): *byte
native fn get_buffer_len(): int

fn example(): void =>
    // C string - check for nil, then unwrap
    var p: *char = getenv("HOME") as val  // In regular fn, unwraps immediately

    // Or in native fn, can check nil first
    // var p: *char = getenv("HOME")
    // if p != nil =>
    //     var s: str = p as val

    // Buffer with length
    var len: int = get_buffer_len()
    var data: byte[] = get_buffer()[0..len] as val
```

#### String/Array Returns: Copy into Arena (Default)

For native functions declared with `str` or array return types, the default is to copy into the arena:

```sindarin
// C's malloc'd result is copied into Sindarin's arena, C memory is freed
native fn strdup(s: str): str

fn example(): void =>
    var dup: str = strdup("hello")  // Automatically copied to arena
```

#### Parameters

```sindarin
// Default: pass by reference (C sees pointer to arena memory)
native fn strlen(s: str): int
native fn process(data: byte[]): void

// as val: copy first, then pass the copy
native fn sort_inplace(data: int[] as val): void
// Useful if C mutates the buffer and you want to preserve original

// as ref: C can write back through this parameter
native fn get_size(width: int as ref, height: int as ref): void
```

| Annotation | Meaning | Use When |
|------------|---------|----------|
| (default) | Pass arena pointer | C needs to read (or safely mutate) your data |
| `as val` | Copy, pass the copy | C mutates data and you want to preserve original |
| `as ref` | Pass pointer for out-param | C writes results back |

#### Examples

```sindarin
#pragma include "<stdlib.h>"
#pragma include "<string.h>"

native fn strdup(s: str): str
native fn getenv(name: str): *char
native fn memcpy(dest: byte[], src: byte[], n: int): void
native fn get_dimensions(w: int as ref, h: int as ref): void

fn main(): int =>
    // strdup: returns str, auto-copied to arena
    var dup: str = strdup("hello")

    // getenv: returns *char, unwrap with as val
    var home: str = getenv("HOME") as val

    // out parameters: C writes to our variables
    var w: int = 0
    var h: int = 0
    get_dimensions(w, h)

    return 0
```

#### Rationale

- Pointer types (`*char`, `*byte`) for nullable C pointers - explicit unwrap with `as val`
- Safe types (`str`, `byte[]`) for non-null returns - auto-copied to arena
- `as ref` parameters for C out-parameters
- Simple, consistent mental model

---

### 3. Structs and Opaque Types: Opaque Handles First

Sindarin supports opaque type declarations for C interop. Full struct support is deferred until the language has native struct types.

#### Opaque Types

Opaque types are handles to C structures that cannot be inspected or allocated from Sindarin.

```sindarin
type FILE = opaque
type SQLite = opaque
type regex_t = opaque
```

#### Usage

```sindarin
type FILE = opaque

native fn fopen(path: str, mode: str): *FILE
native fn fclose(f: *FILE): int
native fn fread(buf: byte[], size: int, count: int, f: *FILE): int

fn read_file(path: str): byte[] =>
    var f: *FILE = fopen(path, "rb")
    if f == nil =>
        panic("Failed to open file")

    var buffer: byte[1024] = {}
    var n: int = fread(buffer, 1, 1024, f)
    fclose(f)

    return buffer[0..n]
```

#### Opaque Type Rules

1. Can declare variables of pointer-to-opaque type (`*FILE`)
2. Can pass opaque pointers to native functions
3. Can compare to `nil`
4. **Cannot** dereference or inspect contents
5. **Cannot** allocate directly in Sindarin

#### Future: Full Struct Support

When Sindarin adds native struct types, C-compatible structs will be supported:

```sindarin
// Future syntax (not yet implemented)
@repr(C)
struct Point {
    x: double
    y: double
}

native fn distance(a: *Point, b: *Point): double
```

This is deferred until the language has struct support.

---

### 4. Variadic Functions: Pass-Through Support

Sindarin supports calling C variadic functions with pass-through semantics.

#### Syntax

```sindarin
native fn printf(format: str, ...): int
native fn sprintf(buf: byte[], format: str, ...): int
native fn fprintf(f: *FILE, format: str, ...): int
```

#### Semantics

- Type checker allows any arguments after `...`
- Variadic arguments must be primitive types or `str`
- Arguments are passed directly to C's varargs mechanism
- No format string validation (matches C behavior)

#### Example

```sindarin
#pragma include "<stdio.h>"

native fn printf(format: str, ...): int

fn main(): int =>
    var name: str = "World"
    var count: int = 42
    var pi: double = 3.14159

    printf("Hello, %s!\n", name)
    printf("Count: %d, Pi: %.2f\n", count, pi)

    return 0
```

#### Allowed Variadic Argument Types

| Type | C Format |
|------|----------|
| `int`, `long` | `%d`, `%ld`, `%lld` |
| `double` | `%f`, `%e`, `%g` |
| `str` | `%s` |
| `char` | `%c` |
| `bool` | `%d` (as 0/1) |
| `*T` (pointers) | `%p` |

Arrays cannot be passed as variadic arguments (pass `.length` or element pointers instead).

---

### 5. Safety Boundary: No `unsafe` Keyword

Sindarin does not introduce an `unsafe` keyword for native interop.

#### Rationale

1. **The `native` keyword already signals different rules** - any function marked `native` is understood to interface with external C code

2. **Everything compiles to C anyway** - the entire program becomes C code, so "safety" is advisory rather than enforced

3. **Ceremony without enforcement adds friction** - requiring `unsafe` blocks around native calls would be noise without meaningful guarantees

4. **Simplicity over complexity** - aligns with Sindarin's design philosophy

#### The `native` Keyword Is Sufficient

```sindarin
// The 'native' keyword already communicates:
// - This is external C code
// - Normal Sindarin guarantees may not apply
// - Programmer takes responsibility for correct usage

native fn dangerous_c_function(ptr: *void): int
```

#### Safe Wrappers Are Encouraged (Not Enforced)

```sindarin
// Low-level native declaration
native fn c_read(fd: int, buf: byte[], count: int): int

// Safe Sindarin wrapper (recommended pattern)
fn read_file(fd: int, count: int): byte[] =>
    var buffer: byte[count] = {}
    var n: int = c_read(fd, buffer, count)
    if n < 0 =>
        panic("Read error")
    return buffer[0..n]
```

This pattern is encouraged through documentation and examples, not language enforcement.

---

### 6. Type Gap Analysis: New Primitive Types

To support common C APIs, Sindarin adds minimal new primitive types.

#### New Types

| Sindarin | C Equivalent | Description |
|----------|--------------|-------------|
| `int32` | `int32_t` | 32-bit signed integer |
| `uint` | `uint64_t` | 64-bit unsigned integer |
| `uint32` | `uint32_t` | 32-bit unsigned integer |
| `float` | `float` | 32-bit floating point |

#### Naming Pattern

Base name = 64-bit, `32` suffix = 32-bit:

| Signed | Unsigned |
|--------|----------|
| `int` (64-bit) | `uint` (64-bit) |
| `int32` (32-bit) | `uint32` (32-bit) |

#### Mapping C Types

| C Type | Sindarin | Notes |
|--------|----------|-------|
| `size_t` | `uint` | Typically 64-bit unsigned |
| `ssize_t` | `int` | Signed size type |
| `void*` | `*void` | With pointer syntax |
| `unsigned int` | `uint32` | C's unsigned int is typically 32-bit |
| `int` (C) | `int32` | C's int is typically 32-bit |

#### Examples

```sindarin
#pragma include "<stdint.h>"

// Using new types for precise C interop
native fn read32(ptr: *int32): int32
native fn hash64(data: byte[], len: uint): uint
native fn get_flags(): uint32
native fn compute(x: float, y: float): float

fn main(): int =>
    var small: int32 = 42
    var flags: uint32 = 0xFF
    var size: uint = 18446744073709551615  // Max uint64
    var precise: float = 3.14

    return 0
```

#### Not Adding (Use Existing Types)

| C Type | Use Instead | Rationale |
|--------|-------------|-----------|
| `int64_t` | `int` | Already 64-bit signed |
| `long long` | `int` | Already 64-bit signed |
| `double` | `double` | Already exists |
| `uint8_t` | `byte` | Already exists |

---

### 7. Native Callbacks: Function Pointers for C

Sindarin's lambda syntax extends to C-compatible function pointers using the `native` modifier.

#### Defining Native Callback Types

```sindarin
// Native callback type - C-compatible function pointer
type EventCallback = native fn(event: int, userdata: *void): void
type Comparator = native fn(a: *void, b: *void): int
type SignalHandler = native fn(signal: int): void
```

#### Declaring C Functions That Accept Callbacks

```sindarin
#pragma include "<stdlib.h>"
#pragma include "<signal.h>"

type SignalHandler = native fn(sig: int): void
type QsortComparator = native fn(a: *void, b: *void): int

native fn signal(sig: int, handler: SignalHandler): void
native fn qsort(base: *void, count: int, size: int, cmp: QsortComparator): void
```

#### Creating Native Callbacks

Native callbacks are created using lambda syntax, but must be declared in `native` functions:

```sindarin
native fn setup_signal_handler(): void =>
    var handler: SignalHandler = fn(sig: int): void =>
        print($"Received signal: {sig}\n")

    signal(2, handler)  // SIGINT
```

#### Passing State with `as ref`

C callbacks typically use `void* userdata` for state. Use `as ref` to pass Sindarin values:

```sindarin
type Callback = native fn(event: int, userdata: *void): void
native fn register_callback(cb: Callback, userdata: *void): void

native fn setup(): void =>
    // Heap allocated with reference semantics
    var state: int as ref = 42

    var handler: Callback = fn(event: int, data: *void): void =>
        var s: int = data as val  // Unwrap pointer to value
        print($"Event: {event}, state: {s}\n")

    // Pass state - compiler passes pointer since it's 'as ref'
    register_callback(handler, state)
```

#### Restrictions: No Closures

Native callbacks **cannot capture variables** from their enclosing scope. C function pointers have no mechanism for closures.

```sindarin
native fn setup(): void =>
    var counter: int = 0

    // ERROR: Native lambda cannot capture 'counter'
    var handler: Callback = fn(event: int, data: *void): void =>
        counter = counter + 1  // Compile error!
        print($"Event: {event}\n")
```

Use the `void* userdata` pattern instead:

```sindarin
native fn setup(): void =>
    var counter: int as ref = 0

    var handler: Callback = fn(event: int, data: *void): void =>
        // Access counter through userdata pointer
        var c: int = data as val
        print($"Count: {c}\n")

    register_callback(handler, counter)
```

#### Regular vs Native Lambdas

| Aspect | Regular `fn(...)` | `native fn(...)` |
|--------|-------------------|------------------|
| Captures variables | Yes (closures) | No - compile error |
| Pointer types in signature | No | Yes |
| Opaque types in signature | No | Yes |
| Declared in | Any function | `native fn` only |
| Passed to | Sindarin functions | C functions |

#### Complete Example: qsort

```sindarin
#pragma include "<stdlib.h>"

type Comparator = native fn(a: *void, b: *void): int

native fn qsort(base: *void, count: int, size: int, cmp: Comparator): void

native fn sort_integers(arr: int[]): void =>
    var cmp: Comparator = fn(a: *void, b: *void): int =>
        var x: int = a as val
        var y: int = b as val
        if x < y =>
            return -1
        if x > y =>
            return 1
        return 0

    qsort(arr.ptr(), arr.length, 8, cmp)  // 8 = sizeof(int64_t)

fn main(): void =>
    var numbers: int[] = {5, 2, 8, 1, 9}
    sort_integers(numbers)
    print($"Sorted: {numbers}\n")  // {1, 2, 5, 8, 9}
```

#### Rationale

- Reuses existing lambda syntax with `native` modifier
- `as ref` provides pointer-to-state without new operators
- No closures enforced at compile time (C compatibility)
- Consistent with overall native function design

---

## Example: Using the Math Library

Complete example showing intended usage:

```sindarin
#pragma include "<math.h>"
#pragma link "m"

native fn sin(x: double): double
native fn cos(x: double): double
native fn sqrt(x: double): double
native fn pow(base: double, exp: double): double

fn main(): int =>
    var angle: double = 3.14159 / 4.0
    var s: double = sin(angle)
    var c: double = cos(angle)

    print($"sin(45deg) = {s}\n")
    print($"cos(45deg) = {c}\n")
    print($"sqrt(2) = {sqrt(2.0)}\n")

    return 0
```

**Generated C (conceptual):**

```c
#include <math.h>
#include "runtime.h"

int main() {
    double angle = 3.14159 / 4.0;
    double s = sin(angle);
    double c = cos(angle);

    printf("sin(45deg) = %f\n", s);
    printf("cos(45deg) = %f\n", c);
    printf("sqrt(2) = %f\n", sqrt(2.0));

    return 0;
}
```

---

## Implementation Checklist

### Phase 1: Core Interop
- [ ] `native` keyword for function declarations
- [ ] `#pragma include` for headers
- [ ] `#pragma link` for library linking
- [ ] Basic type mapping (existing types)

### Phase 2: Pointer Support
- [ ] Pointer type syntax (`*T`, `**T`, `*void`)
- [ ] `nil` constant for null pointers
- [ ] Pointer equality comparison (`==`, `!=`)
- [ ] Compile error for pointer arithmetic
- [ ] Compile error for pointer variable declarations in non-native functions
- [ ] Require `as val` when assigning pointer-returning native call in non-native fn
- [ ] Allow inline pointer passing (e.g., `free(malloc(100))`) in non-native fn
- [ ] Allow `native fn` with body (Sindarin implementation)
- [ ] `ptr as val` for reading through pointer (primitives)
- [ ] `*char as val` → `str` conversion (null-terminated)
- [ ] `ptr[0..len] as val` → `byte[]` conversion (sized buffer)
- [ ] `as ref` parameters generate C pointers for out-params

### Phase 3: Memory Ownership
- [ ] `as val` on native parameters (copy before pass)
- [ ] `as ref` on native parameters (out-params, C writes back)
- [ ] Default copy-to-arena for native `str`/array returns
- [ ] Runtime support for copying C strings to arena

### Phase 4: Extended Types
- [ ] `int32` primitive type
- [ ] `uint` primitive type
- [ ] `uint32` primitive type
- [ ] `float` primitive type
- [ ] Type literals and conversions

### Phase 5: Advanced Features
- [ ] Opaque type declarations (`type X = opaque`)
- [ ] Variadic function support (`...`)
- [ ] Variadic argument type checking

### Phase 6: Native Callbacks
- [ ] `native fn(...)` function pointer types
- [ ] Type aliases for native function types (`type X = native fn(...)`)
- [ ] Native lambda creation in `native fn` context
- [ ] Compile error for captures in native lambdas
- [ ] Pass `as ref` values to `*void` parameters
- [ ] `arr.ptr()` method to get pointer to array data

### Deferred (Until Structs Exist)
- [ ] C-compatible struct declarations (`@repr(C)`)
- [ ] Struct field access for native types

---

## Revision History

| Date | Changes |
|------|---------|
| 2025-01-03 | Added native callbacks (function pointers) using `native fn(...)` syntax |
| 2025-01-03 | Simplified pointer unwrapping: `as val` only, no pointer write syntax, `as ref` for out-params |
| 2025-01-03 | Added `uint32` for type consistency |
| 2025-01-03 | Resolved all open questions: pointer syntax, memory ownership via `as val`/`as ref`, opaque types, variadic functions, no `unsafe` keyword, type gap analysis |
| 2024-12-30 | Initial draft with decided features and open questions |

---

## See Also

- [MEMORY.md](../language/MEMORY.md) - Arena memory management
- [ARRAYS.md](../language/ARRAYS.md) - Array types and byte arrays
- [FILE_IO.md](../language/FILE_IO.md) - Built-in file I/O (example of runtime integration)
