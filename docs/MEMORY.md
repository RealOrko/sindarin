# Sindarin Memory Management

Sindarin uses arena-based memory management for efficient allocation and automatic cleanup. This document describes the memory control features available.

## Overview

By default, memory in Sindarin is allocated from the current function's arena and automatically freed when the function returns. The memory system provides control over allocation and ownership through:

- **Function modifiers**: `shared` and `private`
- **Block modifiers**: `shared` and `private`
- **Loop modifiers**: `shared for`, `shared while`, `shared for-each`
- **Variable modifiers**: `as val` and `as ref`

## Function Modifiers

### Shared Functions

Shared functions use the caller's arena context. This is efficient for helper functions that return complex types.

```sn
fn add_numbers(a: int, b: int) shared: int =>
  return a + b

fn create_greeting(name: str) shared: str =>
  return $"Hello, {name}!"
```

**Use shared functions when:**
- The function is a simple helper
- You want to minimize allocation overhead
- The function needs to return arrays or strings that the caller will use

### Private Functions

Private functions have their own isolated arena that is destroyed when the function returns. They can only return primitive types (`int`, `double`, `bool`, `char`).

```sn
fn compute_sum() private: int =>
  // All allocations here are cleaned up automatically
  var sum: int = 0
  for var i: int = 1; i <= 100; i++ =>
    sum = sum + i
  return sum  // Only primitive can escape
```

**Use private functions when:**
- The function does heavy temporary work
- You want guaranteed cleanup of intermediate allocations
- The function only needs to return a primitive result

## Block Modifiers

### Shared Blocks

Shared blocks use the parent scope's arena. Variables declared inside remain accessible after the block.

```sn
var x: int = 10
shared =>
  var y: int = 20
  x = x + y
// y is out of scope, but x retains its value
print($"x = {x}\n")  // x = 30
```

### Private Blocks

Private blocks create an isolated arena that is destroyed when the block ends. Only primitive values can escape through existing variables.

```sn
var result: int = 0
private =>
  // Heavy temporary work
  var arr: int[] = {1, 2, 3, 4, 5}
  for n in arr =>
    result = result + n
  // arr is freed here
print($"result = {result}\n")  // result = 15
```

**Use private blocks when:**
- You need temporary allocations within a function
- You want deterministic cleanup at a specific point
- You're working with large temporary data structures

## Shared Loops

By default, loops create per-iteration arenas for automatic cleanup. Use `shared` to avoid this overhead when not needed.

### Shared For Loop

```sn
var total: int = 0
shared for var i: int = 0; i < 100; i++ =>
  total = total + i
```

### Shared While Loop

```sn
var count: int = 0
shared while count < 10 =>
  count = count + 1
```

### Shared For-Each Loop

```sn
var arr: int[] = {1, 2, 3, 4, 5}
var sum: int = 0
shared for n in arr =>
  sum = sum + n
```

**Use shared loops when:**
- Loop iterations don't create temporary allocations
- You need maximum performance
- The loop body only works with primitives or pre-existing references

## Variable Modifiers

### as val (Copy Semantics)

The `as val` modifier creates an independent copy of arrays and strings. Changes to the original don't affect the copy and vice versa.

```sn
var original: int[] = {10, 20, 30}
var copy: int[] as val = original

original.push(40)
print(original)  // [10, 20, 30, 40]
print(copy)      // [10, 20, 30] - unchanged
```

**Use as val when:**
- You need a snapshot of data at a point in time
- You want to modify a copy without affecting the original
- You're passing data to a function that might modify it

### as ref (Reference Semantics for Primitives)

The `as ref` modifier allocates primitives on the heap instead of the stack. This is useful for values that need to escape their declaring scope or be shared.

```sn
var x: int as ref = 42
var y: double as ref = 3.14
var flag: bool as ref = true

// Can be modified normally
x = 100
print($"x = {x}\n")  // x = 100
```

**Use as ref when:**
- A primitive needs to outlive its declaring scope
- You need to share a primitive value between closures (future feature)
- You're implementing certain callback patterns

## Best Practices

1. **Default to shared functions** for simple helpers that don't do heavy allocation work.

2. **Use private functions** for compute-heavy operations that create many temporaries.

3. **Use private blocks** for localized cleanup within a function.

4. **Use shared loops** when loop bodies only manipulate primitives or existing references.

5. **Use as val** when you need defensive copies of arrays or strings.

6. **Use as ref sparingly** - most code doesn't need heap-allocated primitives.

## Memory Safety

Sindarin's arena system provides several safety guarantees:

- **No manual free**: Memory is automatically freed when arenas are destroyed
- **No use-after-free**: Private scopes prevent dangling references
- **No memory leaks**: All allocations are tracked and cleaned up
- **Escape analysis**: The type checker prevents non-primitives from escaping private scopes

## Example: Efficient Data Processing

```sn
fn process_data(input: int[]) shared: int =>
  var result: int = 0

  private =>
    // All these temporaries are cleaned up after the block
    var doubled: int[] = {}
    for n in input =>
      doubled.push(n * 2)

    var filtered: int[] = {}
    for n in doubled =>
      if n > 10 =>
        filtered.push(n)

    shared for n in filtered =>
      result = result + n
    // doubled and filtered are freed here

  return result  // Only the primitive escapes

fn main(): void =>
  var data: int[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
  var sum: int = process_data(data)
  print($"Sum of doubled values > 10: {sum}\n")
```
