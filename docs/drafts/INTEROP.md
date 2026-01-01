# C Interoperability in Sindarin

> **DRAFT - NOT IMPLEMENTED** - This document specifies how Sindarin will interface with native C libraries. It contains both decided features and open questions.

Since Sindarin compiles to C, interoperability is natural but requires explicit declarations for external functions, headers, and linking.

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

### Gap Analysis Required

The following C types may need Sindarin equivalents for full interop:

| C Type | Status | Notes |
|--------|--------|-------|
| `size_t` | **TBD** | Common in C APIs |
| `int32_t` | **TBD** | Explicit 32-bit |
| `uint64_t` | **TBD** | Unsigned 64-bit |
| `float` | **TBD** | 32-bit float |
| `void` | **TBD** | For return types / pointers |
| `void*` | **TBD** | Generic pointer |
| `unsigned int` | **TBD** | Common in legacy APIs |

### Pass-by Semantics

Sindarin's existing `as val` and `as ref` semantics can help bridge pointer behavior:

```sindarin
// as val - copy semantics (default for primitives)
fn process(x as val int): int => ...

// as ref - reference semantics (pointer in generated C)
fn modify(x as ref int): void => ...
```

**Open Question:** How do these semantics extend to explicit pointer types?

---

## Open Questions

### 1. Pointer Types (`T` vs `*T`)

How should Sindarin handle explicit pointer types?

**Options under consideration:**

```sindarin
// Option A: C-style pointer syntax
var p: *int = ...
var pp: **int = ...  // pointer to pointer

// Option B: Pointer as generic type
var p: Ptr<int> = ...

// Option C: Only expose through as ref
fn takes_ptr(x as ref int): void => ...
```

**Considerations:**
- Pointer arithmetic - allowed or forbidden?
- Null pointers - how to represent?
- Dereferencing syntax

---

### 2. Memory Ownership

Sindarin uses arena-based memory management. C libraries may expect different ownership models.

**Scenarios to address:**

1. **Sindarin allocates, C uses:**
   ```sindarin
   var buffer: byte[] = ...  // arena allocated
   native_function(buffer)   // C reads/writes to it
   ```

2. **C allocates, Sindarin uses:**
   ```sindarin
   var ptr: *byte = malloc(1024)  // C heap
   // Who frees? When?
   ```

3. **C owns with static lifetime:**
   ```sindarin
   var s: str = getenv("PATH")  // C static storage
   // Must not free, must copy if storing
   ```

**Options under consideration:**
- Automatic copy into arena (safe but potentially wasteful)
- Explicit ownership markers (`@owned`, `@borrowed`)
- Wrapper types for C-managed memory
- Documentation-only (programmer responsibility)

---

### 3. Structs and Opaque Types

Sindarin currently has no struct support. C interop often requires struct handling.

**Minimum viable support:**

```sindarin
// Opaque handle (can't see inside, just pass around)
type FILE = opaque

native fn fopen(path: str, mode: str): *FILE
native fn fclose(f: *FILE): int
```

**Full struct support (future):**

```sindarin
// C-compatible layout
@repr(C)
struct Point {
    x: double
    y: double
}

native fn distance(a: *Point, b: *Point): double
```

**Decision deferred** until struct implementation is planned.

---

### 4. Variadic Functions

C variadic functions like `printf` are common. Sindarin should support calling them.

**Proposed syntax:**

```sindarin
native fn printf(format: str, ...): int
```

**Implementation considerations:**
- Type checking of variadic arguments
- Format string validation (optional)
- Code generation to C varargs

**Status:** Desired feature, implementation TBD.

---

### 5. Safety Boundary

Should there be an `unsafe` marker for native calls?

**Arguments for:**
- Makes dangerous code explicit
- Encourages safe wrappers
- Familiar to Rust users

**Arguments against:**
- Everything transpiles to C anyway
- Adds ceremony without enforcement
- Value unclear without memory ownership model

**Current position:** Deferred until memory ownership is resolved. May not be needed.

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

### Decided (Ready to Implement)
- [ ] `native` keyword for function declarations
- [ ] `#pragma include` for headers
- [ ] `#pragma link` for library linking
- [ ] Basic type mapping (existing types)

### Needs Design
- [ ] Type gap analysis (size_t, float, etc.)
- [ ] Pointer types syntax and semantics
- [ ] Variadic function support

### Deferred
- [ ] Memory ownership model
- [ ] Struct/opaque type support
- [ ] Safety boundaries (if any)

---

## Revision History

| Date | Changes |
|------|---------|
| 2024-12-30 | Initial draft with decided features and open questions |

---

## See Also

- [MEMORY.md](../language/MEMORY.md) - Arena memory management
- [ARRAYS.md](../language/ARRAYS.md) - Array types and byte arrays
- [FILE_IO.md](../language/FILE_IO.md) - Built-in file I/O (example of runtime integration)
