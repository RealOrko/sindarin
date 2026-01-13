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

**Status:** RESOLVED
**Severity:** Medium
**Found in:** test_146_time_patterns.sn
**Fixed in:** code_gen_util.c (gen_native_arithmetic)

### Description

Using `==` operator with boolean values causes a linker error due to missing `rt_eq_bool` function.

### Resolution

The issue was fixed by adding special handling for boolean types in `gen_native_arithmetic()`.
Boolean equality now uses native C operators (`==`, `!=`) instead of runtime functions,
generating code like `((done) == (1L))` which is efficient and doesn't require `rt_eq_bool`.

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

---

## Recursive Lambdas Cause SEGV at Runtime

**Status:** RESOLVED
**Severity:** High
**Fixed in:** type_checker_stmt.c, code_gen_stmt_core.c, code_gen_expr_lambda.c, code_gen.h, code_gen.c

### Description

Recursive lambdas (lambdas that reference themselves) would cause a segmentation fault
at runtime. The code compiles successfully but crashes when the lambda is invoked.

### Resolution

The fix required changes in both the type checker and code generator:

1. **Type Checker (type_checker_stmt.c):** Add the variable to the symbol table
   BEFORE type-checking the lambda body, so the lambda can reference itself.
   Mark the symbol as `is_function = true` so it can be called like a function.

2. **Code Generator (code_gen_stmt_core.c, code_gen_expr_lambda.c):**
   - Track the current variable being declared (`current_decl_var_name`)
   - When generating closure captures, skip the self-reference (it's not initialized yet)
   - After the variable declaration, add a "self-fix" statement to set the self-reference

   This transforms code like:
   ```c
   // Before (broken - uses uninitialized variable):
   __Closure__ *factorial = ({
       __cl__->factorial = factorial;  // BAD: factorial not initialized
       __cl__;
   });

   // After (fixed - deferred self-reference):
   __Closure__ *factorial = ({
       // Don't capture factorial here
       __cl__;
   });
   ((__closure_0__ *)factorial)->factorial = factorial;  // Fixed after assignment
   ```

### Reproduction

```sn
fn main(): void =>
    var factorial: fn(int): int = fn(n: int): int =>
        if n <= 1 =>
            return 1
        else =>
            return n * factorial(n - 1)  // Recursive call

    print($"factorial(5) = {factorial(5)}\n")  // Crashes
```

### Expected Behavior

The recursive lambda should compute `factorial(5) = 120`.

### Actual Behavior

Before fix: Segmentation fault when `factorial(n - 1)` is called.
After fix: Works correctly, outputs `factorial(5) = 120`.

### Technical Notes

- The issue was a circular dependency: the closure initialization tried to capture
  the variable being declared before it was assigned
- The fix uses a two-phase approach: create closure without self-reference, then
  patch the self-reference after the variable is assigned
- This pattern works for any recursive lambda, including mutual recursion between
  lambdas declared in the same scope
