# Sindarin Exploratory Test Plan

## Language Features Summary

### Types
- `int`, `double`, `str`, `char`, `bool`, `void`
- Arrays: `T[]` for any type
- Function types: `fn(params): return_type`

### Operators
- Arithmetic: `+`, `-`, `*`, `/`, `%`
- Comparison: `==`, `!=`, `<`, `>`, `<=`, `>=`
- Logical: `&&`, `||`, `!`
- Increment/Decrement: `++`, `--`
- Range: `..`
- Spread: `...`

### Control Flow
- `if` / `else`
- `while`
- `for` (C-style)
- `for x in array` (for-each)
- `break`, `continue`

### Functions
- Regular: `fn name(params): return_type => body`
- Lambda: `fn(params) => expr`
- Modifiers: `shared`, `private`

### Memory
- `as val` (copy semantics)
- `as ref` (reference semantics)
- `shared` blocks
- `private` blocks
- `shared` loops

### Strings
- Literals: `"text"`
- Interpolation: `$"text {expr}"`
- Methods: `len()`, `.length`, `.toUpper()`, `.toLower()`, `.trim()`, `.substring()`, `.indexOf()`, `.startsWith()`, `.endsWith()`, `.contains()`, `.replace()`, `.split()`

### Arrays
- Literals: `{1, 2, 3}`
- Methods: `.length`, `.push()`, `.pop()`, `.insert()`, `.remove()`, `.reverse()`, `.clone()`, `.concat()`, `.indexOf()`, `.contains()`, `.join()`, `.clear()`
- Slicing: `arr[start..end]`, `arr[..end]`, `arr[start..]`, `arr[..]`, `arr[..:step]`
- Negative indexing: `arr[-1]`
- Range literals: `1..5`
- Spread: `{...arr}`

---

## Test Scenarios

### 1. Basic Types and Literals
- [ ] test_01_int_literals.sn - Integer literals, operations, edge cases
- [ ] test_02_double_literals.sn - Double literals, precision, operations
- [ ] test_03_str_literals.sn - String literals, escapes, empty strings
- [ ] test_04_char_literals.sn - Character literals, escapes
- [ ] test_05_bool_literals.sn - Boolean literals, operations

### 2. Operators and Expressions
- [ ] test_06_arithmetic_ops.sn - All arithmetic operators, precedence
- [ ] test_07_comparison_ops.sn - All comparison operators
- [ ] test_08_logical_ops.sn - AND, OR, NOT, short-circuit evaluation
- [ ] test_09_inc_dec_ops.sn - Increment/decrement in various contexts
- [ ] test_10_operator_precedence.sn - Complex expressions testing precedence

### 3. Control Flow
- [ ] test_11_if_else.sn - Simple and nested if/else
- [ ] test_12_while_loops.sn - While loops, conditions, break/continue
- [ ] test_13_for_loops.sn - C-style for loops, various increments
- [ ] test_14_foreach_loops.sn - For-each over different array types
- [ ] test_15_nested_loops.sn - Nested loops with break/continue

### 4. Functions
- [ ] test_16_void_functions.sn - Void functions, no return
- [ ] test_17_return_values.sn - Functions returning all primitive types
- [ ] test_18_recursion.sn - Recursive functions
- [ ] test_19_function_params.sn - Multiple parameters, different types
- [ ] test_20_forward_decl.sn - Forward declarations

### 5. Lambdas
- [ ] test_21_lambda_basic.sn - Basic lambda expressions
- [ ] test_22_lambda_infer.sn - Type inference in lambdas
- [ ] test_23_lambda_capture.sn - Variable capture in closures
- [ ] test_24_lambda_modifiers.sn - Shared/private lambdas
- [ ] test_25_lambda_higher_order.sn - Lambdas as params and returns

### 6. Arrays
- [ ] test_26_array_basic.sn - Declaration, initialization, access
- [ ] test_27_array_methods.sn - All array methods
- [ ] test_28_array_slicing.sn - All slice variants
- [ ] test_29_array_negative_idx.sn - Negative indexing
- [ ] test_30_array_range.sn - Range literals in arrays
- [ ] test_31_array_spread.sn - Spread operator
- [ ] test_32_array_types.sn - Arrays of different types

### 7. Strings
- [ ] test_33_string_basic.sn - Basic string operations
- [ ] test_34_string_interpol.sn - String interpolation variants
- [ ] test_35_string_methods.sn - All string methods
- [ ] test_36_string_escape.sn - Escape sequences

### 8. Memory Management
- [ ] test_37_shared_func.sn - Shared function modifier
- [ ] test_38_private_func.sn - Private function modifier
- [ ] test_39_shared_block.sn - Shared blocks
- [ ] test_40_private_block.sn - Private blocks
- [ ] test_41_shared_loops.sn - Shared loops
- [ ] test_42_as_val.sn - Copy semantics
- [ ] test_43_as_ref.sn - Reference semantics

### 9. Complex Combinations
- [ ] test_44_nested_arrays.sn - Multi-dimensional arrays
- [ ] test_45_array_of_lambdas.sn - Arrays containing lambdas
- [ ] test_46_lambda_returning_array.sn - Lambdas that return arrays
- [ ] test_47_string_array_ops.sn - String arrays with all operations
- [ ] test_48_complex_interpolation.sn - Complex expressions in interpolation
- [ ] test_49_closure_in_loop.sn - Closures created in loops
- [ ] test_50_method_chaining.sn - Chained method calls

### 10. Edge Cases and Boundary Conditions
- [ ] test_51_empty_arrays.sn - Empty array operations
- [ ] test_52_empty_strings.sn - Empty string operations
- [ ] test_53_zero_iterations.sn - Loops with zero iterations
- [ ] test_54_single_element.sn - Single element arrays
- [ ] test_55_large_numbers.sn - Large integer/double values
- [ ] test_56_deep_nesting.sn - Deeply nested structures
- [ ] test_57_many_params.sn - Functions with many parameters
- [ ] test_58_long_chains.sn - Long method chains
- [ ] test_59_boundary_slices.sn - Boundary conditions in slicing
- [ ] test_60_mixed_types.sn - Mixed type expressions (int/double)
