# Threading in Sindarin

> **DRAFT - NOT IMPLEMENTED** - This document specifies how Sindarin handles threading. It contains both decided features and open questions.

Sindarin provides OS-level threading with a minimal syntax. The `&` operator spawns threads, the `!` operator synchronizes them.

---

## Overview

```sindarin
fn add(a: int, b: int): int => a + b

fn main(): int =>
    var r: int = &add(1, 2)   // spawn thread
    // ... do other work ...
    r!                         // synchronize
    print(r)                   // 3
    return 0
```

---

## Spawning Threads (`&`)

The `&` operator spawns a new OS thread to execute a function call. The result variable enters a "pending" state until synchronized.

### Basic Spawn

```sindarin
fn compute(n: int): int =>
    // expensive computation
    return n * n

var result: int = &compute(42)   // thread starts, result is pending
```

### Inline Declaration

```sindarin
var result: int = &compute(42)   // declare and spawn
```

### Void Functions

```sindarin
fn doWork(): void =>
    print("Working in background\n")

&doWork()   // fire and forget - thread runs independently
```

---

## Synchronizing Threads (`!`)

The `!` operator blocks until a pending variable is populated by its thread.

### Basic Synchronization

```sindarin
var r: int = &add(1, 2)
// ... do other work while thread runs ...
r!                          // block until complete
print(r)                    // safe to use
```

### Immediate Synchronization

Combine spawn and sync for blocking calls:

```sindarin
var r: int = &add(1, 2)!    // spawn and wait immediately
print(r)                     // already synchronized

&doWork()!                   // spawn void and wait
```

### Sync in Expressions

The `!` operator syncs and returns the value, allowing inline use:

```sindarin
var x: int = &add(1, 2)
var y: int = &add(3, 4)

// Sync inline and use values
var z: int = x! + y!        // z = 3 + 7 = 10

// Or directly in function calls
print(x! + y!)
```

After `!` is used, the variable is synchronized and can be accessed normally:

```sindarin
var x: int = &add(1, 2)
var sum: int = x! + x + x   // first x! syncs, subsequent x reads value
                            // sum = 3 + 3 + 3 = 9
```

### Multiple Thread Synchronization

Use array syntax to wait on multiple threads:

```sindarin
var r1: int = &add(1, 2)
var r2: int = &add(3, 4)
var r3: int = &multiply(5, 6)

// Wait for all to complete
[r1, r2, r3]!

// Now all are synchronized
print(r1 + r2 + r3)
```

Individual synchronization is also valid:

```sindarin
r1!
r2!
r3!
```

Or inline:

```sindarin
print(r1! + r2! + r3!)
```

---

## Compiler Enforcement

The compiler tracks pending state and enforces synchronization before use.

### Access Before Sync

```sindarin
var r: int = &add(1, 2)
print(r)                    // COMPILE ERROR: r is unsynchronized
r!
print(r)                    // OK
```

### Reassignment Before Sync

```sindarin
var r: int = &add(1, 2)
r = &add(3, 4)              // COMPILE ERROR: r is unsynchronized
r!
r = &add(3, 4)              // OK - can reassign after sync
```

### Rationale

Preventing reassignment before sync avoids accidental thread orphaning and race conditions. Use separate variables for concurrent operations:

```sindarin
// Correct: separate variables
var r1: int = &add(1, 2)
var r2: int = &add(3, 4)
[r1, r2]!
```

---

## Fire and Forget

Void function calls with `&` run independently:

```sindarin
fn cleanup(): void =>
    // slow background work

&cleanup()   // main continues immediately
             // thread runs in background
```

Fire-and-forget threads are killed when the process exits. If `main` returns or panics, all threads terminate immediately. There are no detached threads that survive the process.

---

## Summary

| Syntax | Behavior |
|--------|----------|
| `var r: T = &fn()` | Spawn thread, r is pending |
| `r!` | Block until synced, returns value |
| `x! + y!` | Sync in expressions |
| `[r1, r2, ...]!` | Block until all are synchronized |
| `var r: T = &fn()!` | Spawn and wait immediately |
| `&fn()` | Fire and forget (void only) |
| `&fn()!` | Spawn and wait (void) |

| Compiler Rule | |
|---------------|---|
| Access unsynchronized variable | Compile error |
| Reassign unsynchronized variable | Compile error |
| After `!` | Variable is normal, can access/reassign |

---

## Memory Semantics

Thread arguments follow the same `as val` and `as ref` semantics as regular function calls, with one addition: references become **frozen** to the parent thread until synchronization.

### Default Behavior

| Type | Default | Thread Behavior |
|------|---------|-----------------|
| Primitives | Copy (value) | Thread gets copy, no restrictions |
| Arrays | Reference | Parent frozen until sync |
| Strings | Reference | Safe (immutable anyway) |

### Arrays: Frozen Reference

By default, arrays are passed by reference. The parent thread is frozen from writes until sync:

```sindarin
fn sum(data: int[]): int =>
    var total: int = 0
    for n in data => total = total + n
    return total

var numbers: int[] = {1, 2, 3}
var r: int = &sum(numbers)     // reference passed, numbers frozen

// Parent thread restrictions while pending:
numbers[0] = 99                 // COMPILE ERROR: numbers frozen
numbers.push(4)                 // COMPILE ERROR: numbers frozen
print(numbers[0])               // OK - reads allowed
print(numbers.length)           // OK - reads allowed

r!                              // sync releases the freeze

numbers[0] = 99                 // OK - unfrozen
```

### Explicit Copy with `as val`

Use `as val` to pass an independent copy. No freezing occurs:

```sindarin
fn destructive(data: int[] as val): int =>
    var total: int = 0
    while data.length > 0 =>
        total = total + data.pop()  // modifies local copy
    return total

var numbers: int[] = {1, 2, 3}
var r: int = &destructive(numbers)  // thread gets copy

numbers[0] = 99                      // OK - not frozen, thread has own copy
numbers.push(4)                      // OK

r!
print(numbers)                       // {99, 2, 3, 4}
```

### Shared Mutable with `as ref` (Primitives)

Primitives with `as ref` are shared between threads. Parent is frozen until sync:

```sindarin
fn increment(counter: int as ref): void =>
    counter = counter + 1

var count: int as ref = 0
var r1: void = &increment(count)
var r2: void = &increment(count)

count = 5                       // COMPILE ERROR: count frozen by r1 and r2

[r1, r2]!                       // sync both

count = 5                       // OK - unfrozen
print(count)                    // 2 (or 5 after assignment)
```

### Multiple References to Same Array

Multiple threads can share read access to the same frozen array:

```sindarin
var data: int[] = {1, 2, 3}
var r1: int = &sum(data)
var r2: int = &sum(data)       // OK - both read-only

data[0] = 99                    // COMPILE ERROR: frozen by r1 and r2

[r1, r2]!                       // sync releases both freezes
data[0] = 99                    // OK
```

### Shared Blocks and Threads

Parent thread writes to `shared` arenas are frozen while child threads hold references:

```sindarin
fn process(items: str[]): int =>
    return items.length

fn main(): int =>
    shared =>
        var items: str[] = {"a", "b", "c"}
        var r: int = &process(items)

        items.push("d")         // COMPILE ERROR: items frozen

        r!
        items.push("d")         // OK - unfrozen

    return 0
```

### Summary

| Scenario | Parent Read | Parent Write | Thread Read | Thread Write |
|----------|-------------|--------------|-------------|--------------|
| Array (default) | Yes | Frozen | Yes | Yes |
| Array `as val` | Yes | Yes | Yes | Yes (own copy) |
| Primitive | Yes | Yes | Yes | Yes (both have copies) |
| Primitive `as ref` | Yes | Frozen | Yes | Yes |
| String | Yes | N/A | Yes | N/A |

---

## Thread Arenas

Thread arena management follows the same `shared`, `private`, and default semantics as regular functions.

### Default (Own Arena)

Thread gets its own arena. Return value promoted to caller's arena on sync:

```sindarin
fn build(): str[] =>
    var result: str[] = {"a", "b", "c"}  // thread's arena
    return result                         // promoted on sync

var r: str[] = &build()
r!                        // result promoted to caller's arena
print(r)                  // safe - lives in caller's arena
```

### `shared` (Caller's Arena)

Thread allocates directly in caller's arena. Parent's writes frozen until sync:

```sindarin
fn build() shared: str[] =>
    var result: str[] = {"a", "b", "c"}  // caller's arena
    return result                         // no promotion needed

var data: str[] = {}
var r: str[] = &build()

data.push("x")            // COMPILE ERROR: caller's arena frozen
r!
data.push("x")            // OK - unfrozen
```

### `private` (Isolated Arena)

Thread has isolated arena. Only primitives can be returned:

```sindarin
fn count_lines(path: str) private: int =>
    var contents: str = read_file(path)  // thread's private arena
    var lines: str[] = contents.split("\n")
    return lines.length                   // primitive escapes, rest freed

var r: int = &count_lines("big.txt")
r!
print(r)                  // just the count, file contents already freed
```

```sindarin
// COMPILE ERROR: can't return array from private function
fn bad(path: str) private: str[] =>
    return read_file(path).split("\n")
```

### Summary

| Function Type | Thread Arena | Return Behavior | Parent Arena |
|---------------|--------------|-----------------|--------------|
| default | Own arena | Promoted on `!` | Not frozen |
| `shared` | Caller's arena | No promotion | Frozen until `!` |
| `private` | Isolated arena | Primitives only | Not frozen |

---

## Error Handling

Thread panics propagate on sync. If you don't sync, the panic is lost.

### Panic Propagation

```sindarin
fn might_fail(x: int): int =>
    if x < 0 =>
        panic("negative value")
    return x * 2

var r: int = &might_fail(-1)
// ... thread panics, but we don't know yet ...
r!                            // PANIC propagates here
print(r)                      // never reached
```

### Fire and Forget: Panic Lost

```sindarin
fn risky(): void =>
    panic("something went wrong")

&risky()        // fire and forget
                // panic happens in background
                // no sync = panic lost
print("done")   // still executes
```

### Multiple Threads

If multiple threads panic, the first sync propagates its panic:

```sindarin
var r1: int = &might_fail(-1)
var r2: int = &might_fail(-2)

r1!             // PANIC from r1
r2!             // never reached
```

With array sync, first completed panic propagates:

```sindarin
var r1: int = &might_fail(-1)
var r2: int = &might_fail(-2)

[r1, r2]!       // PANIC from whichever fails first
```

---

## Implementation Considerations

### Code Generation

The `&` operator would generate pthread creation:

```sindarin
var r: int = &add(1, 2)
```

```c
// Generated C (conceptual)
typedef struct {
    int arg_a;
    int arg_b;
    int* result;
    bool done;
} add_thread_args;

void* add_thread_wrapper(void* arg) {
    add_thread_args* args = (add_thread_args*)arg;
    *args->result = add(args->arg_a, args->arg_b);
    args->done = true;
    return NULL;
}

// At spawn site
add_thread_args args = {1, 2, &r, false};
pthread_t thread;
pthread_create(&thread, NULL, add_thread_wrapper, &args);
```

### Synchronization Implementation

The `!` operator generates pthread_join or condition variable wait:

```sindarin
r!
```

```c
// Generated C (conceptual)
pthread_join(thread, NULL);
// or with condition variable for more complex cases
```

---

## Revision History

| Date | Changes |
|------|---------|
| 2025-01-02 | Initial draft - spawn/sync operators |
| 2025-01-02 | Added memory semantics, thread arenas, error handling |

---

## See Also

- [MEMORY.md](../language/MEMORY.md) - Arena memory management, `as ref`, `as val`, `shared`, `private`
- [INTEROP.md](INTEROP.md) - C interoperability (threading uses pthreads)
