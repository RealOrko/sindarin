# Known Issues

## Function Pointer Calls Cause SEGV at Runtime

**Status:** RESOLVED
**Severity:** High
**Found in:** test_146_time_patterns.sn
**Fixed in:** code_gen_expr_call.c

### Description

Calling a function passed as a parameter causes a segmentation fault at runtime. The code compiles successfully but crashes when the function pointer is invoked.

### Resolution

The issue was that named functions were passed directly as closure pointers, but:
1. Closures are called with the closure pointer as the first argument
2. Named functions don't expect this extra argument

The fix generates thin wrapper functions that adapt the closure calling convention
to the named function's signature. The wrapper takes `(void*, params...)` and
forwards to the actual function, ignoring the closure pointer.

### Reproduction

```sn
fn testWork(): void =>
    var sum: int = 0
    var i: int = 0
    while i < 1000 =>
        sum = sum + i
        i = i + 1

fn benchmark(name: str, iterations: int, f: fn(): void): void =>
    var i: int = 0
    while i < iterations =>
        f()  // <-- SEGV crash occurs here
        i = i + 1

fn main(): int =>
    benchmark("test", 100, testWork)  // Crashes at runtime
    return 0
```

### Expected Behavior

The function `testWork` should be called 100 times through the function pointer `f`.

### Actual Behavior

Segmentation fault when `f()` is called inside the loop.

### Workaround

Use direct function calls instead of function pointers:

```sn
fn benchmarkTestWork(name: str, iterations: int): void =>
    var i: int = 0
    while i < iterations =>
        testWork()  // Direct call works fine
        i = i + 1
```

### Technical Notes

- Function pointer type syntax `fn(): void` parses correctly
- Function pointer parameters are accepted by the type checker
- The issue appears to be in code generation or runtime handling of function pointer calls

---

## Boolean Equality Operator Missing Runtime Function

**Status:** Open
**Severity:** Medium
**Found in:** test_146_time_patterns.sn

### Description

Using `==` operator with boolean values causes a linker error due to missing `rt_eq_bool` function.

### Reproduction

```sn
fn main(): int =>
    var done: bool = true
    if done == true =>  // <-- Linker error
        print("done\n")
    return 0
```

### Error Message

```
undefined reference to 'rt_eq_bool'
```

### Workaround

Use the boolean value directly without comparison:

```sn
fn main(): int =>
    var done: bool = true
    if done =>  // Works correctly
        print("done\n")
    return 0
```

### Technical Notes

- The runtime likely needs an `rt_eq_bool` function added
- Alternatively, code generation could handle bool equality inline without a runtime call
