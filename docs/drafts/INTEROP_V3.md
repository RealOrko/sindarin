# Interop V3: Issues and Enhancement Proposals

**Status:** Draft - Documenting Issues
**Created:** 2026-01-17
**Extends:** [../language/INTEROP.md](../language/INTEROP.md)

---

## Overview

This document tracks issues discovered during SDK type implementation that relate to C interop functionality. These issues prevent the full realization of the struct-based SDK pattern described in STRUCTS_V2.md.

---

## Current Interop Capabilities (Working)

The following interop features work correctly:

1. **`#pragma include`** - Include C headers
2. **`#pragma source`** - Inline C code (adapters, shims)
3. **`#pragma link`** - Link external libraries
4. **`native fn` declarations** - Declare C functions
5. **Pointer types in native context** - `*void`, `*int`, etc.
6. **`as ref` parameters** - Pass-by-pointer for primitives and structs
7. **Opaque types** - `type FILE = opaque`
8. **The `arena` built-in** - Access caller's arena for allocation
9. **Native struct methods** - Methods can be declared in native structs

---

## Issues Discovered

### RESOLVED: Module Import Does Not Register Exported Types

**Severity:** Critical - Blocks SDK module pattern

**Status:** RESOLVED - Implemented import-first processing

**Problem:** When importing a Sindarin module that exports struct types, those types are not available for use in the importing module.

**Example:**
```sindarin
# sdk/date.sn - exports SdkDate struct
struct SdkDate =>
    _days: int32
    static fn today(): SdkDate => ...

# test.sn - imports sdk/date.sn
import "sdk/date.sn"

fn main(): void =>
    var d: SdkDate = SdkDate.today()
    # ERROR: Type 'unknown' has no member 'today'
```

**Analysis:**

The import mechanism processes modules in the wrong order relative to type resolution:

| Step | What Happens | Problem |
|------|--------------|---------|
| 1 | Parser begins parsing `test.sn` | - |
| 2 | Parser encounters `import "sdk/date.sn"` | Import noted but not processed |
| 3 | Parser parses `fn main()` body | - |
| 4 | Parser encounters `SdkDate.today()` | Type lookup for `SdkDate` |
| 5 | Type `SdkDate` not found | Creates forward-reference "unknown" type |
| 6 | Parser finishes main module | - |
| 7 | Import processing begins | Too late! |
| 8 | `sdk/date.sn` is parsed | `SdkDate` struct registered |
| 9 | Statements merged | Type exists but references are already broken |

**Evidence from debug trace:**
```
[Line 257] Type lookup: 'SdkDate' -> not found, creating forward ref
...
[Line 7598] Registering struct type 'SdkDate' from import
```

**Impact:**
- Cannot create reusable SDK modules
- Cannot import struct types between modules
- Forces code duplication (embedding SDK code in each file)

**Workaround:** Embed full SDK implementation in each file that needs it (not scalable).

**Proposed Fix Options:**

1. **Two-pass parsing:**
   - First pass: Parse all imports recursively, register all types
   - Second pass: Parse function bodies with full type information

2. **Lazy type resolution:**
   - During parsing, create forward-reference types
   - During type checking, resolve all forward references
   - Requires type checker to have access to imported type tables

3. **Import-first processing:**
   - When `import` is encountered, immediately process that module
   - Register all exported types before continuing to parse main module
   - May require handling circular imports

**Solution Implemented:**

The fix uses "Import-first processing" (option 3 from the proposed fixes):

1. Added `ImportContext` struct to track imported modules during parsing
2. When `parser_import_statement` encounters an import, it immediately:
   - Recursively parses the imported module
   - Registers all struct types and functions in the symbol table
3. Post-parsing merges imported statements into the main module's AST

Key changes:
- `src/parser.h`: Added `ImportContext` struct and `import_ctx` field to Parser
- `src/parser.c`: Added `parser_process_import()` and `process_import_callback()` functions
- `src/parser/parser_stmt.c`: Modified `parser_import_statement()` to call `parser_process_import()`

Test: `tests/integration/test_import_struct.sn` demonstrates importing a struct type from another module.

**Status:** RESOLVED - Import-first processing implemented

---

### RESOLVED: Forward Reference Types Not Resolved for Imported Structs

**Severity:** High - Related to above

**Status:** RESOLVED - Fixed by import-first processing

**Problem:** When the parser encounters an unknown type name, it creates a "forward reference" placeholder. These placeholders should be resolved during type checking, but they remain unresolved for types that originate from imported modules.

**Technical Details:**

The type system creates forward references for efficiency:
```c
// In type lookup when type not found:
Type *forward_ref = create_forward_reference(type_name);
// Later should be resolved to actual type
```

For locally-defined types, this works because:
1. The type is registered later in the same parse
2. Type checking resolves the forward reference

For imported types, this previously failed because:
1. The type is registered in a different symbol table (the imported module's)
2. The resolution phase doesn't check imported symbol tables

**Solution:** With import-first processing, imported types are registered in the symbol table BEFORE the importing file's function bodies are parsed. This means forward references are no longer needed for imported types - they are found immediately during type lookup.

**Status:** RESOLVED - Fixed by import-first processing

---

### DOCUMENTED: Native Struct Boundary Restriction

**Severity:** Informational - By Design

**Behavior:** Native structs (structs with pointer fields) can only be used in native function contexts.

```sindarin
native struct Buffer =>
    data: *byte
    length: int

# Works
native fn use_buffer(): void =>
    var buf: Buffer = Buffer {}

# Compile Error
fn regular(): void =>
    var buf: Buffer = Buffer {}
```

**Rationale:** This maintains the safety boundary that separates pointer-safe code from regular Sindarin code.

**Workaround:** Use the adapter pattern with regular structs:
1. Define a regular struct with primitive fields matching the C layout
2. Use `#pragma source` to provide C adapters for type conversion
3. Declare `native fn` adapters callable from regular functions

**Status:** DOCUMENTED - Working as designed

---

### RESOLVED: C Adapter Function Signatures

**Severity:** Medium - Implementation detail

**Problem:** When bridging between Sindarin structs and C runtime types, adapter functions need careful signature design.

**Key discoveries:**

1. **Instance methods pass `self` as pointer:**
   ```c
   // Sindarin: fn year(): int
   // C adapter receives pointer:
   static inline long long sdk_date_year(SdkDate *d) { ... }
   ```

2. **Static methods don't have `self`:**
   ```c
   // Sindarin: static fn today(): SdkDate
   // C adapter only receives arena:
   static inline SdkDate sdk_date_today(RtArena *arena) { ... }
   ```

3. **Return types must match exactly:**
   - Sindarin `int` = C `long long` (64-bit)
   - Sindarin `int32` = C `int32_t`
   - Sindarin `bool` = C `int`

4. **Struct parameters by value vs pointer:**
   ```c
   // By value (for small structs):
   static inline int sdk_date_equals(SdkDate *d, SdkDate other) { ... }

   // self is always pointer, 'other' is value (copied in)
   ```

**Status:** RESOLVED - Pattern documented in sdk/date.sn

---

## Working Patterns

### Pattern 1: Regular Struct with C Adapters

This is the recommended pattern for SDK types that need to be used in regular functions:

```sindarin
#pragma include "runtime/runtime_sdk.h"

# 1. Regular struct with matching memory layout
struct SdkDate =>
    _days: int32

    # 2. Methods delegate to native adapters
    static fn today(): SdkDate =>
        return sdk_date_today(arena)

    fn year(): int =>
        return sdk_date_year(self)

# 3. C adapter layer
#pragma source """
static inline SdkDate sdk_date_today(RtArena *arena) {
    RtDate *d = rt_date_today(arena);
    SdkDate result;
    result._days = d->days;
    return result;
}

static inline long long sdk_date_year(SdkDate *d) {
    RtDate rd = { .days = d->_days };
    return rt_date_get_year(&rd);
}
"""

# 4. Native fn declarations
native fn sdk_date_today(a: *void): SdkDate
native fn sdk_date_year(d: *SdkDate): int
```

**Why this works:**
- Regular structs can be used anywhere
- `#pragma source` bridges type systems at C level
- Native fn declarations make adapters callable
- `arena` built-in provides allocation context

**Note:** Can now be imported thanks to import-first processing (see resolved issue above).

---

### Pattern 2: Direct Native Struct (Native Context Only)

For code that will only run in native function context:

```sindarin
native struct ZStream =>
    next_in: *byte
    avail_in: uint
    # ...

native fn deflateInit(strm: ZStream as ref, level: int): int
native fn deflate(strm: ZStream as ref, flush: int): int

native fn compress(input: byte[]): byte[] =>
    var strm: ZStream = ZStream {}
    deflateInit(strm, -1)
    # ...
```

**Limitation:** Cannot use the struct in regular functions.

---

## Recommendations

### Short Term

1. **Document workarounds** - Embedding SDK code directly works today
2. **Create test files with embedded SDK** - Validates the pattern works
3. **Complete sdk/date.sn and sdk/time.sn** - Ready for when imports work

### Medium Term

1. **Fix import type registration** - Critical for SDK modularity
2. **Add import tests** - Ensure structs can be imported
3. **Consider export declarations** - Explicit `export struct SdkDate` syntax

### Long Term

1. **Module system redesign** - First-class module support
2. **Type resolution improvements** - Better forward reference handling
3. **Cross-module optimization** - Inline adapter functions across modules

---

## References

- [../language/INTEROP.md](../language/INTEROP.md) - C interop documentation
- [STRUCTS_V2.md](STRUCTS_V2.md) - Struct implementation progress
- [STRUCTS.md](STRUCTS.md) - Original struct specification
