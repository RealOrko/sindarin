# Structs in Sindarin (DRAFT)

**Status:** Draft
**Authors:** TBD
**Created:** 2026-01-12
**Motivation:** C interop for libraries requiring struct field access (e.g., zlib, compression)

---

## Motivation

### The Problem

Sindarin's current interop system supports:
- Native function declarations (`native fn`)
- Opaque types (`type FILE = opaque`)
- Pointer types in native functions (`*void`, `*int`)
- Out parameters (`as ref`)

However, many C libraries require **struct field access**. For example, zlib's streaming API:

```c
z_stream strm;
strm.next_in = input_buffer;
strm.avail_in = input_length;
strm.next_out = output_buffer;
strm.avail_out = output_capacity;

deflate(&strm, Z_FINISH);

int bytes_written = output_capacity - strm.avail_out;
```

Currently, Sindarin cannot:
1. Allocate C-compatible structs
2. Read or write struct fields
3. Pass structs by value to/from C functions

This forces workarounds like writing C wrapper functions that hide struct access behind opaque handles.

### Use Cases

1. **Compression libraries** (zlib, lz4, zstd) - streaming state management
2. **Graphics/multimedia** (stb_image, SDL) - image/buffer descriptors
3. **Networking** (sockets) - address structures like `sockaddr_in`
4. **Databases** (SQLite) - result row structures
5. **File formats** - parsing headers with known layouts

---

## Proposed Syntax

### Struct Declaration

```sindarin
struct Point =>
    x: double
    y: double

struct Rectangle =>
    origin: Point
    width: double
    height: double

# With default field values
struct Config =>
    timeout: int = 30
    retries: int = 3
    verbose: bool = false

struct ServerConfig =>
    host: str = "localhost"
    port: int = 8080
    maxConnections: int = 100

# For C interop - native struct required due to pointer fields
native struct ZStream =>
    next_in: *byte
    avail_in: uint
    total_in: uint
    next_out: *byte
    avail_out: uint
    total_out: uint
    msg: *char
    # ... remaining fields
```

### Struct Instantiation

```sindarin
# Full initialization with field initializers
var p: Point = Point { x: 10.0, y: 20.0 }

# Zero initialization (fields without defaults get zero)
var origin: Point = Point {}

# Partial initialization - unspecified fields get defaults (or zero if no default)
var rect: Rectangle = Rectangle { width: 100.0, height: 50.0 }

# Using default field values
var cfg: Config = Config {}                     # All defaults: timeout=30, retries=3, verbose=false
var cfg2: Config = Config { timeout: 60 }       # Override timeout, others use defaults
var srv: ServerConfig = ServerConfig { port: 443, maxConnections: 1000 }  # Override some
```

### Field Access

```sindarin
# Read
var x_val: double = p.x
print($"Point: ({p.x}, {p.y})\n")

# Write
p.x = 30.0
p.y = 40.0

# Nested access
rect.origin.x = 5.0
```

### Pointers to Structs

```sindarin
# For C functions that return struct pointers
native fn get_config(): *Config

# Access fields through pointer (automatic dereference)
var cfg: *Config = get_config()
var value: int = cfg.some_field    # Equivalent to cfg->some_field in C

# Or explicit dereference
var local: Config = cfg as val     # Copy struct to local
```

---

## Memory Layout

### C Compatibility

Structs must have **C-compatible memory layout** for interop:

```sindarin
struct Example =>
    a: int      # 8 bytes (int64_t)
    b: int32    # 4 bytes
    c: byte     # 1 byte
    # Compiler adds padding for alignment
```

Maps to:
```c
typedef struct {
    int64_t a;    // offset 0
    int32_t b;    // offset 8
    uint8_t c;    // offset 12
    // 3 bytes padding to align to 8
} Example;        // total: 16 bytes
```

### Alignment Rules

- Fields are naturally aligned based on their size
- Struct alignment is the maximum alignment of its fields
- Padding is inserted as needed
- Total size is rounded up to alignment

### Packed Structs (Optional)

For binary formats requiring exact layouts:

```sindarin
#pragma pack(1)
struct FileHeader =>
    magic: int32       # offset 0
    version: byte      # offset 4
    flags: byte        # offset 5
    size: int32        # offset 6
#pragma pack()
```

---

## Arena Integration and Memory Model

Structs must integrate seamlessly with Sindarin's arena-based memory management. This section defines how struct allocation, lifetime, and escape behavior align with existing memory semantics.

### First Principles (Consistency with Existing Model)

Sindarin's memory model establishes clear patterns:

| Type | Assignment | Location | Escape Behavior |
|------|------------|----------|-----------------|
| Primitives (`int`, `double`, etc.) | Copy value | Stack | N/A (copied) |
| Primitives `as ref` | Reference | Heap/Arena | Auto-promoted |
| Fixed arrays (`int[N]`) | Reference | Stack (small) or Heap | Auto-promoted |
| Dynamic arrays (`int[]`) | Reference | Heap/Arena | Auto-promoted |
| Strings (`str`) | Reference | Heap/Arena | Auto-promoted |
| **Structs** | **Copy value** | **Stack (small) or Heap** | **Auto-promoted** |

**Key decision:** Structs behave like **fixed arrays** - value semantics by default, stack allocation for small structs, with automatic promotion to arena when escaping scope.

### Value Semantics (Default)

Struct assignment **copies the entire struct**, matching C semantics:

```sindarin
var p1: Point = Point { x: 1.0, y: 2.0 }
var p2: Point = p1       # p2 is a COPY of p1
p2.x = 99.0              # p1.x is still 1.0
```

This differs from arrays (which are references) but matches:
- C struct assignment behavior (required for interop)
- Primitive behavior (predictable, no aliasing)
- User expectations for "plain old data"

### Reference Semantics (Explicit)

Use `as ref` parameters for functions that need to modify structs:

```sindarin
fn modify_point(p: Point as ref): void =>
    p.x = 99.0

var p1: Point = Point { x: 1.0, y: 2.0 }
modify_point(p1)         # p1.x is now 99.0 (compiler passes pointer)
```

Within `native fn` bodies, explicit pointer work is allowed (see Integration with Native Functions).

### Stack vs Arena Allocation

#### Small Structs: Stack by Default

```sindarin
fn process(): void =>
    var p: Point = Point { x: 1.0, y: 2.0 }
    # p is stack-allocated (16 bytes - small)
    # Freed automatically when function returns
```

#### Large Structs: Auto-Promoted to Arena

```sindarin
struct LargeBuffer =>
    data: byte[8192]
    length: int

fn process(): void =>
    var buf: LargeBuffer = LargeBuffer {}
    # buf is arena-allocated (>8KB threshold)
    # Freed when function's arena is destroyed
```

**Threshold:** Same as fixed arrays (~8KB). Larger structs are automatically heap-allocated in the current arena. This is transparent to the programmer.

### Escape Behavior and Auto-Promotion

When a struct escapes its scope, it is **copied to the outer arena**, exactly like fixed arrays:

```sindarin
var outer: Point

if condition =>
    var inner: Point = Point { x: 1.0, y: 2.0 }  # Stack or inner arena
    outer = inner                                  # COPIED to outer scope
    # inner's memory reclaimed, but outer has a copy

print($"x = {outer.x}\n")  # Safe - outer owns its copy
```

#### Returning Structs from Functions

```sindarin
fn make_point(x: double, y: double): Point =>
    var p: Point = Point { x: x, y: y }  # Function's stack/arena
    return p                              # COPIED to caller's arena

var result: Point = make_point(1.0, 2.0)  # result owns the copy
```

### Integration with `shared`

#### `shared` Functions

A `shared` function allocates directly in the caller's arena, avoiding copy overhead:

```sindarin
fn make_point(x: double, y: double) shared: Point =>
    var p: Point = Point { x: x, y: y }  # Allocated in CALLER's arena
    return p                              # No copy needed

fn main(): void =>
    var p: Point = make_point(1.0, 2.0)  # Efficient - no promotion
```

#### `shared` Loops

```sindarin
fn process_points(count: int) shared: Point[] =>
    var points: Point[] = {}

    for var i: int = 0; i < count; i++ shared =>
        var p: Point = Point { x: i as double, y: 0.0 }
        points.push(p)  # No per-iteration arena overhead

    return points
```

#### `shared` Blocks

```sindarin
fn build_geometry(): Rectangle[] =>
    var rects: Rectangle[] = {}

    shared =>
        # All struct allocations use function's arena
        for var i: int = 0; i < 100; i++ =>
            var r: Rectangle = Rectangle {
                origin: Point { x: i as double, y: 0.0 },
                width: 10.0,
                height: 10.0
            }
            rects.push(r)

    return rects
```

### Integration with `private`

#### Structs with Only Primitive Fields: Can Escape

```sindarin
struct Point =>
    x: double
    y: double

fn get_origin() private: Point =>
    var p: Point = Point { x: 0.0, y: 0.0 }
    return p  # OK: struct contains only primitives
```

#### Structs with Heap Fields: Cannot Escape

```sindarin
struct NamedPoint =>
    name: str      # Heap-allocated string
    x: double
    y: double

fn bad() private: NamedPoint =>
    var p: NamedPoint = NamedPoint { name: "origin", x: 0.0, y: 0.0 }
    return p  # COMPILE ERROR: struct contains heap data (str)
```

#### Structs with Pointer Fields: Cannot Escape

```sindarin
native struct BufferView =>
    data: *byte    # Pointer to external memory
    length: int

native fn bad() private: BufferView =>
    var view: BufferView = BufferView { data: get_buffer(), length: 100 }
    return view  # COMPILE ERROR: struct contains pointer
```

#### Private Block Rules

| Struct Contains | Can Escape `private`? |
|-----------------|----------------------|
| Only primitives (`int`, `double`, `bool`, etc.) | Yes |
| Fixed arrays of primitives | No |
| Dynamic arrays | No |
| Strings | No |
| Pointers | No |
| Nested structs with heap data | No |

```sindarin
fn analyze(data: byte[]): int =>
    var count: int = 0

    private =>
        var strm: ZStream = ZStream {}  # Contains pointers
        # ... process ...
        count = strm.total_out as int   # Primitive escapes

        # strm CANNOT escape - contains pointer fields
        # All struct memory freed here

    return count  # Only primitive escapes
```

### C-Allocated vs Sindarin-Allocated Structs

#### Sindarin-Allocated (Owned)

Structs created in Sindarin are managed by the arena:

```sindarin
var p: Point = Point { x: 1.0, y: 2.0 }  # Sindarin owns this
# Lifetime managed by current arena
```

#### C-Allocated (Borrowed)

Structs returned by C functions are **not** arena-managed:

```sindarin
native fn get_static_config(): *Config  # Returns pointer to C-managed memory

fn use_config(): void =>
    var cfg: *Config = get_static_config()
    var timeout: int = cfg.timeout  # Read from C memory
    # cfg points to C-managed memory - NOT freed by Sindarin arena
```

#### Copying C Data into Arena

Use `as val` to copy C-allocated struct into Sindarin's arena:

```sindarin
native fn get_default_point(): *Point

fn safe_copy(): Point =>
    var c_point: *Point = get_default_point()  # C-managed
    var local: Point = c_point as val          # COPY into arena
    return local                                # Safe - arena-managed copy
```

### Pointer Lifetime Safety

#### Dangling Pointer Prevention

Regular functions cannot return pointers to local structs (pointer types not allowed in regular functions). Within `native fn`, the compiler should prevent returning pointers to stack-allocated structs:

```sindarin
native fn bad(): *Point =>
    var p: Point = Point { x: 1.0, y: 2.0 }  # Stack allocated
    return p as ptr  # COMPILE ERROR: returning pointer to local variable
```

Note: The exact syntax for obtaining a pointer within native functions needs design consideration (see Open Questions).

#### Returning Struct Pointers

Returning struct pointers from regular functions is not supported - return by value instead (auto-promoted to caller's arena):

```sindarin
fn create_point(x: double, y: double): Point =>
    var p: Point = Point { x: x, y: y }
    return p  # Copied to caller's arena

# For C interop requiring pointers, use native functions with C allocation
native fn create_point_c(x: double, y: double): *Point =>
    var p: *Point = malloc(sizeof Point) as *Point
    p.x = x
    p.y = y
    return p  # Caller responsible for free()
```

### Summary: Struct Lifetime Rules

| Scenario | Behavior |
|----------|----------|
| Local struct variable | Stack (small <8KB) or arena (large) |
| Struct assigned to outer scope | Copied to outer arena |
| Struct returned from function | Copied to caller's arena |
| Struct in `shared` function | Allocated in caller's arena directly |
| Struct in `private` block | Freed when block ends; only primitive-only structs can escape |
| Pointer to local struct | Cannot escape function (compile error) |
| C-allocated struct pointer | Not arena-managed; lifetime is C's responsibility |
| `as val` on C struct pointer | Copied into Sindarin arena |

### Arena Hierarchy with Structs

```
Caller's Arena
  └── Function Arena
        ├── local structs (stack or arena)
        ├── returned structs (promoted on return)
        └── Loop Arena (per iteration)
              └── temporary structs (freed each iteration unless shared)
```

Example:

```sindarin
# Assuming update_summary is declared with 'as ref' parameters:
# fn update_summary(summary: Summary as ref, item: ProcessedItem as ref): void

fn process_items(items: Item[]): Summary =>
    var summary: Summary = Summary {}  # Function arena

    for item in items =>
        # Loop arena (per iteration)
        var temp: ProcessedItem = process(item)  # Loop arena
        update_summary(summary, temp)  # Compiler passes pointers via 'as ref'
        # temp freed here (unless loop is shared)

    return summary  # Copied to caller's arena
```

---

## Integration with Native Functions

### Passing Structs

```sindarin
struct TimeVal =>
    tv_sec: int
    tv_usec: int

# By value (copy)
native fn process_time(t: TimeVal): void

# By pointer using 'as ref' (matches existing interop pattern)
native fn gettimeofday(tv: TimeVal as ref, tz: *void): int

# Usage - call site is transparent, compiler handles pointer
var tv: TimeVal = TimeVal {}
gettimeofday(tv, nil)           # Compiler passes &tv based on 'as ref'
print($"Seconds: {tv.tv_sec}\n")
```

### Pass-by-Reference with `as ref`

The `as ref` modifier in parameter declarations tells the compiler to pass a pointer:

```sindarin
# C function signature: void modify_point(Point* p)
native fn modify_point(p: Point as ref): void

fn example(): void =>
    var pt: Point = Point { x: 1.0, y: 2.0 }
    modify_point(pt)           # Compiler passes &pt
    print($"Modified: {pt.x}\n")  # pt was modified by C
```

### Explicit Pointers in Native Functions

Within `native fn` bodies, explicit pointer variables are allowed (matching existing interop rules):

```sindarin
native fn create_and_use(): void =>
    var p: *Point = allocate_point()  # OK in native fn
    p.x = 10.0                         # Modify through pointer
    free_point(p)
```

Regular functions cannot store pointers - use `as ref` for out-parameters instead.

### Returning Structs

```sindarin
# Small structs can be returned by value
native fn make_point(x: double, y: double): Point

# Large structs typically returned by pointer
native fn create_buffer(): *Buffer
```

### Using `sizeof`

The `sizeof` operator returns the size in bytes, essential for C interop:

```sindarin
struct Packet =>
    header: int32
    flags: byte
    payload: byte[256]

# Get size for buffer allocation
var packet_size: int = sizeof(Packet)
var buffer: byte[sizeof Packet] = {}

# Use with native functions
native fn memset(ptr: *void, value: int, size: int): *void
native fn memcpy(dest: *void, src: *void, size: int): *void

native fn clear_packet(p: Packet as ref): void =>
    memset(p as ptr, 0, sizeof Packet)

native fn copy_packet(dest: Packet as ref, src: Packet as ref): void =>
    memcpy(dest as ptr, src as ptr, sizeof(Packet))
```

---

## Practical Example: zlib Streaming

With struct support, zlib streaming becomes possible in pure Sindarin:

```sindarin
#pragma include "<zlib.h>"
#pragma link "z"

# Native struct required - contains pointer fields
native struct ZStream =>
    next_in: *byte
    avail_in: uint
    total_in: uint
    next_out: *byte
    avail_out: uint
    total_out: uint
    msg: *char
    state: *void
    zalloc: *void
    zfree: *void
    opaque: *void
    data_type: int
    adler: uint
    reserved: uint

native fn deflateInit(strm: ZStream as ref, level: int): int
native fn deflate(strm: ZStream as ref, flush: int): int
native fn deflateEnd(strm: ZStream as ref): int

# Must be native fn to use native struct
native fn compress_stream(input: byte[], output: byte[]): int =>
    var strm: ZStream = ZStream {}

    if deflateInit(strm, -1) != 0 =>  # Z_DEFAULT_COMPRESSION (compiler passes &strm)
        return -1

    strm.next_in = input.ptr()
    strm.avail_in = input.length as uint
    strm.next_out = output.ptr()
    strm.avail_out = output.length as uint

    var result: int = deflate(strm, 4)  # Z_FINISH
    var bytes_written: int = output.length - (strm.avail_out as int)

    deflateEnd(strm)

    if result != 1 =>  # Z_STREAM_END
        return -1

    return bytes_written
```

---

## Design Decisions

### 1. Value Semantics vs Reference Semantics

**Chosen:** Value semantics (copy on assignment)

```sindarin
var p1: Point = Point { x: 1.0, y: 2.0 }
var p2: Point = p1   # p2 is a COPY
p2.x = 99.0          # p1.x unchanged
```

**Alternative:** Reference semantics (like arrays)

```sindarin
var p2: Point = p1   # p2 aliases p1 (NOT chosen)
```

**Rationale:**
- Matches C struct behavior (essential for interop)
- Predictable - no hidden aliasing
- Safe - modifications don't affect other variables unexpectedly
- Consistent with primitives
- Use explicit pointers (`*Point`) when reference semantics needed

### 2. Syntax: `struct Name =>` vs Alternatives

**Chosen:** `struct Name =>` with indented fields

```sindarin
struct Point =>
    x: double
    y: double
```

**Alternatives considered:**

```sindarin
# C-style braces
struct Point {
    x: double
    y: double
}

# Type alias style
type Point = struct { x: double, y: double }

# Record style
record Point(x: double, y: double)
```

**Rationale:** Consistent with Sindarin's `=>` block syntax used elsewhere (functions, if/while, etc.)

### 2. Field Initialization: Named vs Positional

**Chosen:** Named initialization only

```sindarin
var p: Point = Point { x: 10.0, y: 20.0 }
```

**Alternative:** Positional

```sindarin
var p: Point = Point(10.0, 20.0)
```

**Rationale:** Named initialization is explicit and safe for structs with many fields. Order independence prevents bugs when struct definition changes.

### 3. Mutability

**Chosen:** Structs are mutable by default (like arrays)

```sindarin
var p: Point = Point { x: 1.0, y: 2.0 }
p.x = 3.0  # Allowed
```

**Rationale:** Matches C semantics required for interop. Immutability could be added later with a `const` modifier.

### 4. No Methods

**Chosen:** Structs are plain data containers without methods

```sindarin
# NOT supported
struct Point =>
    x: double
    y: double

    fn distance(other: Point): double =>  # NO
        # ...
```

**Rationale:**
- Keeps structs simple and C-compatible
- Methods can be standalone functions: `fn point_distance(a: Point, b: Point): double`
- Avoids object-oriented complexity
- Consistent with Sindarin's procedural philosophy

### 5. No Inheritance

**Chosen:** No struct inheritance or embedding

**Rationale:** Keep initial implementation simple. Composition via nested structs provides similar functionality.

### 6. Pointer Field Access Syntax

**Chosen:** Automatic dereference with `.` (like C's `->`)

```sindarin
var ptr: *Point = get_point()  # In native fn context
var x: double = ptr.x          # Automatic dereference
```

**Alternative:** Explicit dereference required

```sindarin
var x: double = (ptr as val).x
```

**Rationale:** Ergonomic for common interop patterns. C programmers expect `->` equivalent.

### 7. Default Field Values

**Chosen:** Structs support default field values

```sindarin
struct Config =>
    timeout: int = 30
    retries: int = 3
    verbose: bool = false

struct Connection =>
    host: str = "localhost"
    port: int = 8080

# Usage - unspecified fields get defaults
var cfg: Config = Config {}                    # All defaults
var cfg2: Config = Config { timeout: 60 }      # Override timeout only
var conn: Connection = Connection { port: 443 } # Override port only
```

**Rationale:**
- Reduces boilerplate for common configurations
- Matches user expectations from other languages
- Partial initialization becomes safe and explicit
- Zero-initialization still works: `Config {}` uses all defaults

**Code generation:** Default values are applied at struct instantiation, not stored in the type.

### 8. `sizeof` Operator

**Chosen:** Support `sizeof` for structs (with or without parentheses)

```sindarin
struct Point =>
    x: double
    y: double

var size1: int = sizeof(Point)   # Returns 16 (with parentheses)
var size2: int = sizeof Point    # Returns 16 (without parentheses)

# Also works with expressions
var p: Point = Point { x: 1.0, y: 2.0 }
var size3: int = sizeof(p)       # Returns 16
var size4: int = sizeof p        # Returns 16
```

**Rationale:**
- Essential for C interop (buffer allocation, memory operations)
- Matching C syntax maximizes familiarity
- Both forms (with/without parentheses) supported for flexibility
- Works with both type names and expressions

**Code generation:** Compiles to C's `sizeof` directly.

### 9. Struct Equality with `==`

**Chosen:** Structs are comparable using `==` and `!=` (byte-wise comparison)

```sindarin
struct Point =>
    x: double
    y: double

var p1: Point = Point { x: 1.0, y: 2.0 }
var p2: Point = Point { x: 1.0, y: 2.0 }
var p3: Point = Point { x: 3.0, y: 4.0 }

if p1 == p2 =>
    print("Points are equal\n")      # This prints

if p1 != p3 =>
    print("Points are different\n")  # This prints
```

**Semantics:** Byte-wise comparison of the entire struct (equivalent to `memcmp`).

**Rationale:**
- Consistent with primitive equality behavior
- Simple and predictable
- Efficient (single memcmp operation)
- Useful for caching, deduplication, change detection

**Note:** For native structs containing pointers, equality compares pointer values (addresses), not pointed-to data. This matches C semantics.

```sindarin
native struct Buffer =>
    data: *byte
    length: int

# Inside native fn context:
var b1: Buffer = Buffer { data: get_data(), length: 10 }
var b2: Buffer = b1  # Copy - same pointer value
var b3: Buffer = Buffer { data: get_data(), length: 10 }  # Different pointer

b1 == b2  # true (same pointer address)
b1 == b3  # false (different pointer address, even if data is same)
```

### 10. Pass-by-Pointer: `as ref` (Already Implemented)

**Chosen:** Use `as ref` in parameter declaration (matches existing interop)

```sindarin
native fn deflateInit(strm: ZStream as ref, level: int): int

deflateInit(strm, -1)  # Compiler passes pointer based on declaration
```

**Implementation status:** Already implemented for primitives. The code generator automatically:
1. Creates a local variable to hold the unboxed value
2. Passes `&variable` to the C function
3. Copies modified value back after the call

See `tests/integration/test_as_ref_out_params.sn` and `tests/integration/test_interop_pointers.sn` for working examples.

**Rationale:**
- Consistent with existing `as ref` pattern for primitives
- Call site is cleaner and more readable
- Compiler enforces pointer passing based on function signature
- No new operator needed
- Struct support extends naturally from existing primitive implementation

### 11. Arrays in Structs

**Chosen:** Arrays preserve reference semantics; `as val` provides deep copy

```sindarin
struct Container =>
    items: int[]       # Dynamic array field (reference)
    count: int

struct Buffer =>
    data: byte[1024]   # Fixed-size array field (inline storage)
    length: int
```

**Default behavior (reference semantics preserved):**

```sindarin
var c1: Container = Container { items: {1, 2, 3}, count: 3 }
var c2: Container = c1           # Shallow copy: c2.items aliases c1.items
c2.items.push(4)                 # Both c1.items and c2.items see the change
```

**`as val` for deep copy:**

```sindarin
var c3: Container = c1 as val    # Deep copy: c3.items is independent
c3.items.push(5)                 # Only c3.items affected

# Also works on field access
var myArr: int[] = c1.items as val  # Deep copy of just the array field
myArr.push(6)                        # c1.items unaffected
```

**Parameter semantics:**

```sindarin
# Default - reference (no copy)
fn process(c: Container): void =>
    c.items.push(4)              # Modifies caller's array

# as ref - same as default, but explicit (visual enforcement)
fn process_ref(c: Container as ref): void =>
    c.items.push(4)              # Modifies caller's array

# as val - deep copy on entry
fn process_copy(c: Container as val): void =>
    c.items.push(4)              # Only modifies local copy
```

**Field access with `as ref`:**

```sindarin
# as ref on field returns pointer
var ptr: *int[] = c1.items as ref   # Pointer to array field (in native fn context)
```

**Rationale:**
- Consistent with existing array reference semantics
- `as val` provides explicit deep copy when needed
- `as ref` remains visual enforcement of reference passing
- Matches user expectations: arrays are always references unless explicitly copied
- Fixed-size arrays in structs are inline (copied with struct by default)

### 12. No Anonymous Structs

**Chosen:** Anonymous structs are not supported

```sindarin
# NOT supported
var point: struct { x: double, y: double } = { x: 1.0, y: 2.0 }
```

**Rationale:**
- All structs must be named with explicit `struct Name =>` declaration
- Keeps the language simple and consistent
- Named types improve code readability and error messages
- C interop requires known struct layouts with predictable names

### 13. All Fields Public

**Chosen:** No access protection - all struct fields are publicly accessible

```sindarin
struct Point =>
    x: double    # Public (only option)
    y: double    # Public (only option)

var p: Point = Point { x: 1.0, y: 2.0 }
p.x = 99.0       # Always allowed
var val: double = p.y  # Always allowed
```

**Rationale:**
- Sindarin has no concept of access protection anywhere in the language
- Adding visibility modifiers to structs would be inconsistent
- Keeps structs simple as plain data containers
- Matches C struct semantics (required for interop)

### 14. Automatic Stack/Heap Allocation (Same as Arrays)

**Chosen:** No explicit allocation keyword - compiler handles automatically

```sindarin
# Small struct - stack allocated
var p: Point = Point { x: 1.0, y: 2.0 }

# Large struct - auto-promoted to heap
struct LargeBuffer =>
    data: byte[8192]
    length: int

var buf: LargeBuffer = LargeBuffer {}  # Heap (>8KB)

# Escaping struct - auto-promoted to outer arena
var outer: Point
if condition =>
    var inner: Point = Point { x: 1.0, y: 2.0 }
    outer = inner  # Copied to outer arena
```

**Allocation rules (same as fixed arrays):**
- Small structs (<8KB): stack by default
- Large structs (≥8KB): auto-promoted to heap
- Escaping structs: copied to outer arena
- Returned structs: copied to caller's arena

**Rationale:**
- Consistent with existing array allocation behavior
- No new keywords needed (`new`, `alloc`, etc.)
- Transparent to programmer - compiler handles it
- Matches the "automatic promotion" mental model

**For C interop pointers:** Use C allocation functions (`malloc`, etc.) within `native fn` context when explicit heap pointers are needed.

### 15. Pointer Fields Require `native struct`

**Chosen:** Structs containing pointer fields must be declared with `native struct`

```sindarin
# Regular struct - no pointer fields allowed
struct Point =>
    x: double
    y: double

# Native struct - pointer fields allowed (for C interop)
native struct ZStream =>
    next_in: *byte
    avail_in: uint
    next_out: *byte
    avail_out: uint
    # ...

native struct Buffer =>
    data: *byte
    length: int
    capacity: int
```

**Usage restrictions:**

```sindarin
# Regular struct - can be used anywhere
var p: Point = Point { x: 1.0, y: 2.0 }

# Native struct - can only be instantiated/used in native fn context
native fn compress(input: byte[]): byte[] =>
    var strm: ZStream = ZStream {}  # OK: inside native fn
    # ...

fn regular(): void =>
    var strm: ZStream = ZStream {}  # COMPILE ERROR: native struct in regular fn
```

**Rationale:**
- Maintains the safety boundary: pointer types only in `native` context
- Consistent with existing rule: pointer types only allowed in `native fn`
- Regular Sindarin code remains pointer-free and safe
- Clear visual distinction between safe structs and interop structs
- Compiler can enforce safety at declaration site

**Compile-time checks:**
1. `struct` with pointer field → compile error, suggest `native struct`
2. `native struct` used outside `native fn` → compile error
3. `native struct` can be passed to/from `native fn` freely

---

## Implementation Considerations

### Parser Changes

1. New keyword: `struct`
2. Parse struct declarations as new AST node type
3. Parse struct literals: `TypeName { field: value, ... }`
4. Parse field access: `expr.field`

### Type System Changes

1. New type kind: `TYPE_STRUCT`
2. Store struct definition (name, fields with types and offsets)
3. Field lookup by name
4. Size and alignment calculation

### Code Generation

1. Generate C struct typedefs
2. Generate field access as direct memory operations
3. Handle alignment and padding correctly
4. Generate address-of operations

### Memory Model

1. Stack allocation for small struct variables (<8KB)
2. Auto-promotion to arena for large structs or escaping structs
3. Proper lifetime tracking

---

## Open Questions

None - all design decisions have been made.

---

## Migration Path

### Phase 1: Basic Structs
- Declaration and instantiation
- Field read/write
- Default field values
- `sizeof` operator
- Equality comparison (`==`, `!=`)
- Stack allocation only
- No pointer types in structs

### Phase 2: Interop Enhancement
- `native struct` for structs with pointer fields
- Pass to native functions with `as ref`
- Auto-promotion to arena (large structs, escaping structs)
- Nested struct initialization

### Phase 3: Advanced Features
- Packed structs
- Fixed arrays inside structs
- Structs containing dynamic arrays (deep copy semantics)

---

## References

### Language Documentation
- [MEMORY.md](../language/MEMORY.md) - **Critical:** Arena memory model, escape behavior, `shared`, `private`
- [INTEROP.md](../language/INTEROP.md) - Current C interop capabilities, `as val`, `as ref`, opaque types
- [TYPES.md](../language/TYPES.md) - Existing type system and primitives
- [ARRAYS.md](../language/ARRAYS.md) - Array semantics (reference behavior, escape rules)

### Related Drafts
- [STREAMING.md](STREAMING.md) - Callback-based streaming patterns (works today, motivates struct need)

### External References
- zlib manual: https://zlib.net/manual.html (motivating use case)
- C struct memory layout: https://en.cppreference.com/w/c/language/struct
