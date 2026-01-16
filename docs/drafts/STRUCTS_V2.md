# Structs V2: Methods on Structs

**Status:** Draft
**Created:** 2026-01-16
**Motivation:** Enable SDK extensibility without compiler changes; provide clean method-call syntax for user-defined types

---

## Summary

This proposal adds **methods to structs**, transforming them into lightweight classes (without inheritance). Combined with existing C interop features, this enables SDK authors to create new types entirely outside the compiler core.

| Feature | Description |
|---------|-------------|
| Instance methods | Functions with implicit `self` parameter |
| Static methods | Functions called on the type, not an instance |
| Native methods | Methods backed by C functions |
| Mixed support | Both Sindarin and native methods on same struct |

---

## Motivation

### The Problem

Currently, adding a new SDK type like `Map` requires changes across the entire compiler:

1. **Lexer** - Add keywords
2. **Parser** - Add syntax rules
3. **Type Checker** - Hardcode method signatures
4. **Code Generator** - Hardcode C function mappings
5. **Runtime** - Implement C functions

This tight coupling means:
- Only compiler maintainers can add new types
- Each type requires full test coverage across all compiler phases
- SDK cannot evolve independently of compiler releases

### The Solution

Allow methods to be defined on structs. Combined with existing interop features (`native struct`, `native fn`, `#pragma`), SDK authors can:

1. Define a struct with an opaque handle
2. Declare methods that map to C runtime functions
3. Ship the `.sn` declaration + C implementation
4. Users import and use with clean `obj.method()` syntax

No compiler changes required after this feature is implemented.

### Use Cases

1. **SDK Types** - Map, Set, Json, Regex, HttpClient
2. **Domain Models** - User-defined types with behavior
3. **Builder Patterns** - Fluent APIs with method chaining
4. **Wrappers** - Clean interfaces over C libraries

---

## Proposed Syntax

### Instance Methods

Instance methods receive an implicit `self` parameter referring to the struct instance.

```sindarin
struct Point =>
    x: double
    y: double

    fn magnitude(): double =>
        return sqrt(self.x * self.x + self.y * self.y)

    fn distanceTo(other: Point): double =>
        var dx: double = self.x - other.x
        var dy: double = self.y - other.y
        return sqrt(dx * dx + dy * dy)

    fn translate(dx: double, dy: double): Point =>
        return Point { x: self.x + dx, y: self.y + dy }
```

**Usage:**

```sindarin
var p: Point = Point { x: 3.0, y: 4.0 }
var mag: double = p.magnitude()           # 5.0
var dist: double = p.distanceTo(origin)   # distance to origin
var moved: Point = p.translate(1.0, 1.0)  # Point { x: 4.0, y: 5.0 }
```

### Static Methods

Static methods belong to the type, not an instance. They do not have access to `self`.

```sindarin
struct Point =>
    x: double
    y: double

    # Static factory methods
    static fn origin(): Point =>
        return Point { x: 0.0, y: 0.0 }

    static fn fromPolar(r: double, theta: double): Point =>
        return Point { x: r * cos(theta), y: r * sin(theta) }

    # Instance method for comparison
    fn magnitude(): double =>
        return sqrt(self.x * self.x + self.y * self.y)
```

**Usage:**

```sindarin
var o: Point = Point.origin()              # Static call
var p: Point = Point.fromPolar(5.0, 0.0)   # Static call
var m: double = p.magnitude()              # Instance call
```

### Native Methods

Native methods map to C functions. The compiler passes `self` as the first argument.

```sindarin
#pragma include "point_lib.h"
#pragma link "pointlib"

native struct Point =>
    _handle: *void

    # Static native method - no self
    static native fn create(x: double, y: double): Point

    # Instance native methods - self passed as first arg
    native fn magnitude(): double
    native fn distanceTo(other: Point): double
    native fn translate(dx: double, dy: double): Point
    native fn destroy(): void
```

**Generated C calls:**

```c
// Point.create(1.0, 2.0) generates:
Point p = rt_point_create(1.0, 2.0);

// p.magnitude() generates:
double m = rt_point_magnitude(p);

// p.distanceTo(other) generates:
double d = rt_point_distanceTo(p, other);

// p.translate(1.0, 1.0) generates:
Point moved = rt_point_translate(p, 1.0, 1.0);
```

Note: The C runtime accesses the current arena implicitly (e.g., via thread-local storage). Arena is never passed explicitly in generated code.

### Mixed Methods

Structs can have both native and Sindarin methods:

```sindarin
#pragma include "runtime/runtime_map.h"

native struct Map =>
    _ptr: *void

    # Native methods (C implementation)
    static native fn create(): Map
    native fn set(key: str, value: int): void
    native fn get(key: str): int
    native fn has(key: str): bool
    native fn size(): int

    # Pure Sindarin method (uses native methods internally)
    fn getOrDefault(key: str, default: int): int =>
        if self.has(key) =>
            return self.get(key)
        return default

    fn increment(key: str): void =>
        var current: int = self.getOrDefault(key, 0)
        self.set(key, current + 1)
```

---

## Detailed Examples

### Example 1: Pure Sindarin Struct

A 2D vector with methods:

```sindarin
import "sdk/math" as math

struct Vec2 =>
    x: double
    y: double

    # === Static Constructors ===

    static fn zero(): Vec2 =>
        return Vec2 { x: 0.0, y: 0.0 }

    static fn one(): Vec2 =>
        return Vec2 { x: 1.0, y: 1.0 }

    static fn fromAngle(radians: double): Vec2 =>
        return Vec2 { x: math.cos(radians), y: math.sin(radians) }

    # === Instance Methods ===

    fn length(): double =>
        return math.sqrt(self.x * self.x + self.y * self.y)

    fn lengthSquared(): double =>
        return self.x * self.x + self.y * self.y

    fn normalize(): Vec2 =>
        var len: double = self.length()
        if len == 0.0 =>
            return Vec2.zero()
        return Vec2 { x: self.x / len, y: self.y / len }

    fn add(other: Vec2): Vec2 =>
        return Vec2 { x: self.x + other.x, y: self.y + other.y }

    fn sub(other: Vec2): Vec2 =>
        return Vec2 { x: self.x - other.x, y: self.y - other.y }

    fn scale(factor: double): Vec2 =>
        return Vec2 { x: self.x * factor, y: self.y * factor }

    fn dot(other: Vec2): double =>
        return self.x * other.x + self.y * other.y

    fn cross(other: Vec2): double =>
        return self.x * other.y - self.y * other.x

    fn angle(): double =>
        return math.atan2(self.y, self.x)

    fn rotate(radians: double): Vec2 =>
        var c: double = math.cos(radians)
        var s: double = math.sin(radians)
        return Vec2 {
            x: self.x * c - self.y * s,
            y: self.x * s + self.y * c
        }

    fn lerp(other: Vec2, t: double): Vec2 =>
        return Vec2 {
            x: self.x + (other.x - self.x) * t,
            y: self.y + (other.y - self.y) * t
        }

    fn toString(): str =>
        return $"Vec2({self.x}, {self.y})"

# Usage
fn main(): void =>
    var v1: Vec2 = Vec2 { x: 3.0, y: 4.0 }
    var v2: Vec2 = Vec2.fromAngle(0.0)

    print($"Length: {v1.length()}\n")           # 5.0
    print($"Normalized: {v1.normalize()}\n")    # Vec2(0.6, 0.8)
    print($"Sum: {v1.add(v2).toString()}\n")    # Vec2(4.0, 4.0)
```

### Example 2: SDK Type with Native Backend (Map)

```sindarin
# sdk/collections/map_str_int.sn

#pragma include "runtime/runtime_map.h"
#pragma link "snruntime"

native struct MapStrInt =>
    _ptr: *void

    # === Static Constructors ===

    static native fn create(): MapStrInt
    static native fn withCapacity(capacity: int): MapStrInt

    # === Core Operations (Native) ===

    native fn set(key: str, value: int): void
    native fn get(key: str): int
    native fn has(key: str): bool
    native fn delete(key: str): bool
    native fn clear(): void

    # === Accessors (Native) ===

    native fn size(): int
    native fn isEmpty(): bool
    native fn keys(): str[]
    native fn values(): int[]

    # === Convenience Methods (Sindarin) ===

    fn getOr(key: str, default: int): int =>
        if self.has(key) =>
            return self.get(key)
        return default

    fn setDefault(key: str, default: int): int =>
        if !self.has(key) =>
            self.set(key, default)
        return self.get(key)

    fn increment(key: str): int =>
        var newVal: int = self.getOr(key, 0) + 1
        self.set(key, newVal)
        return newVal

    fn decrement(key: str): int =>
        var newVal: int = self.getOr(key, 0) - 1
        self.set(key, newVal)
        return newVal

    fn update(key: str, fn: fn(int): int): void =>
        var current: int = self.getOr(key, 0)
        self.set(key, fn(current))

    fn contains(key: str, value: int): bool =>
        return self.has(key) && self.get(key) == value
```

**Usage:**

```sindarin
import "sdk/collections/map_str_int" as collections

fn main(): void =>
    var wordCounts: MapStrInt = MapStrInt.create()

    # Count words
    var words: str[] = {"apple", "banana", "apple", "cherry", "banana", "apple"}
    for word in words =>
        wordCounts.increment(word)

    # Print counts
    for key in wordCounts.keys() =>
        print($"{key}: {wordCounts.get(key)}\n")

    # Use convenience methods
    var defaulted: int = wordCounts.getOr("mango", 0)  # 0
    wordCounts.update("apple", (n: int) => n * 2)       # double apple count
```

### Example 3: Builder Pattern

```sindarin
struct HttpRequestBuilder =>
    _method: str = "GET"
    _url: str = ""
    _headers: str[] = {}
    _body: str = ""

    static fn new(): HttpRequestBuilder =>
        return HttpRequestBuilder {}

    fn method(m: str): HttpRequestBuilder =>
        self._method = m
        return self

    fn url(u: str): HttpRequestBuilder =>
        self._url = u
        return self

    fn header(name: str, value: str): HttpRequestBuilder =>
        self._headers = self._headers.push($"{name}: {value}")
        return self

    fn body(b: str): HttpRequestBuilder =>
        self._body = b
        return self

    fn build(): HttpRequest =>
        return HttpRequest {
            method: self._method,
            url: self._url,
            headers: self._headers,
            body: self._body
        }

# Fluent usage
var request: HttpRequest = HttpRequestBuilder.new()
    .method("POST")
    .url("https://api.example.com/users")
    .header("Content-Type", "application/json")
    .header("Authorization", "Bearer token123")
    .body("{\"name\": \"Alice\"}")
    .build()
```

### Example 4: Wrapping C Library (SQLite-style)

```sindarin
#pragma include "sqlite3.h"
#pragma link "sqlite3"

type sqlite3 = opaque
type sqlite3_stmt = opaque

native struct Database =>
    _db: *sqlite3

    static native fn open(path: str): Database

    native fn close(): void
    native fn execute(sql: str): void
    native fn prepare(sql: str): Statement
    native fn lastInsertId(): int
    native fn changes(): int

    # Sindarin convenience method
    fn transaction(work: fn(): void): void =>
        self.execute("BEGIN")
        work()
        self.execute("COMMIT")

native struct Statement =>
    _stmt: *sqlite3_stmt

    native fn bind(index: int, value: int): void
    native fn bindStr(index: int, value: str): void
    native fn step(): bool
    native fn getInt(col: int): int
    native fn getStr(col: int): str
    native fn reset(): void
    native fn finalize(): void

# Usage
fn main(): void =>
    var db: Database = Database.open("test.db")

    db.execute("CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY, name TEXT)")

    var insert: Statement = db.prepare("INSERT INTO users (name) VALUES (?)")
    insert.bindStr(1, "Alice")
    insert.step()
    insert.finalize()

    db.close()
```

---

## Grammar Changes

### Struct Declaration

```
struct_decl := 'struct' IDENT '=>' NEWLINE INDENT struct_body DEDENT
             | 'native' 'struct' IDENT '=>' NEWLINE INDENT struct_body DEDENT

struct_body := (field_decl | method_decl)*

field_decl := IDENT ':' type ('=' expr)? NEWLINE

method_decl := 'static'? 'fn' IDENT '(' params ')' memory_modifier? ':' type '=>' (expr | block)
             | 'static'? 'native' 'fn' IDENT '(' params ')' memory_modifier? ':' type

memory_modifier := 'shared' | 'private'
```

Notes:
- `static` appears before `fn` (determines instance vs static method)
- `shared`/`private` appears after params, before return type (determines arena behavior)
- Both modifiers are optional and independent

### Method Call

No grammar change needed. Existing member access (`expr.IDENT`) already parses. The type checker determines if it's a field access or method call.

---

## Semantic Rules

### Self Parameter

1. Instance methods have an implicit `self` parameter of the struct type
2. `self` is immutable by default (cannot reassign `self = ...`)
3. `self` fields can be read: `self.x`
4. `self` fields can be written: `self.x = value` (if struct is mutable)
5. Static methods cannot reference `self`

### Method Resolution

1. When `expr.name(args)` is encountered:
   - First, check if `name` is a field of the struct type
   - If not a field, check if `name` is a method
   - If method found, transform to method call
2. Static calls `Type.name(args)`:
   - Look up `name` in static methods of `Type`
   - Error if not found or if it's an instance method

### Name Conflicts

1. A method cannot have the same name as a field
2. Multiple methods can have the same name if parameter counts differ (basic overloading)
3. Instance and static methods cannot share a name

### Native Method Mapping

For native methods, the C function name **must match exactly** using the convention:

```
rt_{struct_lowercase}_{method_name}
```

Examples:
- `MapStrInt.create()` → `rt_mapstrint_create()`
- `map.set(k, v)` → `rt_mapstrint_set(map, k, v)`
- `map.get(k)` → `rt_mapstrint_get(map, k)`

The C runtime accesses the current arena implicitly. For instance methods, `self` is passed as the first argument.

**No aliasing is supported.** The C function names must follow this convention exactly. If you need to wrap a C library with different naming conventions, use non-native wrapper methods:

```sindarin
#pragma include "some_lib.h"

# Declare the external C functions with their actual names
native fn somelib_map_new(): *void
native fn somelib_map_put(m: *void, key: str, val: int): void

native struct Map =>
    _handle: *void

    # Non-native wrapper provides the clean interface
    static fn create(): Map =>
        return Map { _handle: somelib_map_new() }

    # Non-native wrapper calls the actual C function
    fn set(key: str, value: int): void =>
        somelib_map_put(self._handle, key, value)
```

This approach keeps the native function declarations honest (matching C exactly) while allowing SDK authors to provide ergonomic method names.

### Memory Management

Methods follow the same arena semantics as regular functions (see [MEMORY.md](../language/MEMORY.md)):

**Default behavior:**
- Each method has its own arena
- Returned values are promoted to the caller's arena
- Method's arena is freed when method returns

**With `shared` modifier:**
- Method uses the caller's arena directly
- No promotion overhead for returned values
- Useful for builder patterns and methods that construct return values

```sindarin
struct StringBuilder =>
    _parts: str[]

    static fn new(): StringBuilder =>
        return StringBuilder { _parts: {} }

    # shared - builds result directly in caller's arena
    fn append(s: str) shared: StringBuilder =>
        self._parts.push(s)
        return self

    # shared - avoids promotion of the joined string
    fn build() shared: str =>
        return self._parts.join("")

# Usage - all allocations in main's arena
fn main(): void =>
    var result: str = StringBuilder.new()
        .append("Hello, ")
        .append("World!")
        .build()
```

**With `private` modifier:**
- Method has an isolated arena
- Only primitives (`int`, `double`, `bool`, `char`) can be returned
- All heap allocations (arrays, strings, structs) are freed when method returns
- Compile error if method tries to return non-primitive types
- Useful for methods that process large temporary data

```sindarin
struct DataProcessor =>
    _source: str

    # private - processes large data, only returns primitive result
    fn countLines() private: int =>
        var contents: str = TextFile.readAll(self._source)  # Maybe 100MB
        var lines: str[] = contents.splitLines()             # Thousands of strings
        return lines.length                                  # Only int escapes
        # All temporary data freed here - guaranteed

    # private - analyze without leaking memory
    fn averageLineLength() private: double =>
        var contents: str = TextFile.readAll(self._source)
        var lines: str[] = contents.splitLines()
        var total: int = 0
        for line in lines =>
            total = total + line.length
        return total as double / lines.length as double

    # COMPILE ERROR - cannot return str from private method
    # fn firstLine() private: str =>
    #     var contents: str = TextFile.readAll(self._source)
    #     return contents.splitLines()[0]  # ERROR: str cannot escape private
```

| Modifier | Arena | Can Return | Use Case |
|----------|-------|------------|----------|
| (default) | Own arena, promoted | Any type | General methods |
| `shared` | Caller's arena | Any type | Builders, chained methods |
| `private` | Isolated arena | Primitives only | Large temporary processing |

**Native methods:**
- Native methods follow the same rules
- The runtime arena is accessed implicitly (not passed explicitly)
- C implementations use the thread-local arena via `rt_arena_*` functions

```sindarin
native struct Map =>
    _ptr: *void

    # Default - method has own arena, result promoted
    native fn keys(): str[]

    # shared - allocates directly in caller's arena
    native fn values() shared: int[]
```

For the C implementation, native methods access the current arena via the runtime:

```c
// C runtime accesses arena implicitly
char **rt_map_keys(RtMap *map) {
    RtArena *arena = rt_current_arena();  // Thread-local
    // ... allocate in arena ...
}
```

---

## Implementation Notes

### Parser Changes

In `parser_stmt.c`, modify struct body parsing:

```c
// After parsing struct name and '=>'
while (!check(TOKEN_DEDENT)) {
    if (check(TOKEN_STATIC) || check(TOKEN_FN) || check(TOKEN_NATIVE)) {
        // Parse method
        bool is_static = match(TOKEN_STATIC);
        bool is_native = check(TOKEN_NATIVE) && check_next(TOKEN_FN);
        if (is_native) advance(); // consume 'native'

        consume(TOKEN_FN, "Expected 'fn'");
        Method *method = parse_method_declaration(is_static, is_native);
        add_method_to_struct(struct_decl, method);
    } else {
        // Parse field (existing code)
        StructField *field = parse_field();
        add_field_to_struct(struct_decl, field);
    }
}
```

### AST Changes

Extend `StructField` or add new `Method` struct:

```c
typedef struct Method {
    const char *name;
    Parameter *params;      // Does NOT include 'self'
    int param_count;
    Type *return_type;
    bool is_static;
    bool is_native;
    Stmt *body;             // NULL for native declarations
} Method;

// In struct_type:
struct {
    const char *name;
    StructField *fields;
    int field_count;
    Method *methods;        // NEW
    int method_count;       // NEW
    // ...
} struct_type;
```

### Type Checker Changes

1. **Register methods** when processing struct declarations
2. **Resolve method calls** in member access expressions:

```c
// In type_check_member_access():
Type *object_type = type_check_expr(expr->object);

if (object_type->kind == TYPE_STRUCT) {
    // First try field lookup
    StructField *field = find_field(object_type, member_name);
    if (field) {
        return field->type;
    }

    // Then try method lookup
    Method *method = find_method(object_type, member_name);
    if (method) {
        if (method->is_static) {
            error("Cannot call static method on instance");
        }
        // Mark this as a method call for code gen
        expr->is_method_call = true;
        expr->method = method;
        return method->return_type;
    }

    error("No field or method named '%s'", member_name);
}
```

3. **Resolve static calls**:

```c
// In type_check_static_call():
// Already handles Type.method() syntax
// Extend to look up methods in struct type
```

### Code Generator Changes

1. **Generate method as function** with self parameter:

```c
// For: fn magnitude(): double => ...
// Generate:
double Point_magnitude(Point self) {
    return sqrt(self.x * self.x + self.y * self.y);
}

// For: fn translate(dx: double, dy: double) shared: Point => ...
// Generate (shared uses caller's arena context):
Point Point_translate_shared(Point self, double dx, double dy) {
    // Allocations use caller's arena via rt_current_arena()
    return (Point){ .x = self.x + dx, .y = self.y + dy };
}
```

2. **Transform method calls**:

```c
// For: p.magnitude()
// Generate:
Point_magnitude(p)

// For: p.translate(1.0, 2.0)
// Generate:
Point_translate(p, 1.0, 2.0)
```

3. **Native method calls**:

```c
// For native: map.set("foo", 42)
// Generate:
rt_mapstrint_set(map, "foo", 42)
```

The arena is accessed implicitly by the runtime via thread-local storage (`rt_current_arena()`). The `shared` modifier affects which arena context is active, but the arena is never passed as an explicit parameter.

---

## Migration Path

### Phase 1: Core Implementation
- Parser recognizes methods in struct body
- Type checker resolves method calls
- Code generator emits functions with self parameter

### Phase 2: Native Methods
- Support `native fn` in struct body
- Convention-based C function naming (must match exactly)
- Non-native wrapper pattern for libraries with different naming conventions

### Phase 3: SDK Migration
- Convert built-in types (Date, Time, UUID, etc.) to use this pattern
- Create new SDK types (Map, Set, etc.) using pure struct+methods

---

## Open Questions

1. **Mutating methods**: Should there be a `mutating` keyword for methods that modify `self`?
   ```sindarin
   mutating fn reset(): void =>
       self.x = 0.0
       self.y = 0.0
   ```

2. **Property syntax**: Should getters have special syntax?
   ```sindarin
   # Option A: Method that looks like property
   fn size(): int => self._size

   # Option B: Explicit property syntax
   property size: int =>
       get => self._size
   ```

3. **Visibility**: Should methods support `private`?
   ```sindarin
   private fn internalHelper(): void => ...
   ```

4. **Self type in return**: Should `Self` be a keyword for the enclosing struct type?
   ```sindarin
   fn clone(): Self => Point { x: self.x, y: self.y }
   ```

---

## Rejected Alternatives

### 1. External Method Syntax

```sindarin
struct Point =>
    x: double
    y: double

# Methods declared outside struct
fn Point.magnitude(): double =>
    return sqrt(self.x * self.x + self.y * self.y)
```

**Rejected because:** Harder to see all methods of a type at once; complicates tooling.

### 2. Trait/Interface System First

Add traits and implement methods via traits.

**Rejected because:** Much larger scope; methods on structs is simpler and provides immediate value.

### 3. Implicit Receiver (Go-style)

```sindarin
fn (p: Point) magnitude(): double =>
    return sqrt(p.x * p.x + p.y * p.y)
```

**Rejected because:** Unfamiliar syntax; inconsistent with Sindarin's style.

---

## References

- [STRUCTS.md](./STRUCTS.md) - Original struct specification
- [INTEROP-V2.md](./INTEROP-V2.md) - Interop improvements
- Rust impl blocks: https://doc.rust-lang.org/book/ch05-03-method-syntax.html
- Kotlin data classes: https://kotlinlang.org/docs/data-classes.html
