# Known Issues and Future Improvements

## Resolved Issues

### Hash Comment Support (Fixed)

**Problem**: The lexer did not support `#` as a comment character, causing compilation to hang indefinitely when processing files with `#` comments.

**Root Cause**: When the lexer encountered `#`:
1. It returned `TOKEN_ERROR` for the unrecognized character
2. Error handling reset `indent_size` to 1, corrupting the indent stack
3. The parser entered `panic_mode`
4. Subsequent INDENT tokens couldn't be parsed correctly
5. When already in `panic_mode`, `parser_error_at` returned early without advancing the token
6. This caused an infinite loop in `parser_indented_block` repeatedly trying to parse the same INDENT token

**Fix**: Added `#` comment support to:
- `compiler/lexer_util.c` - `lexer_skip_whitespace()` now skips `#` comments
- `compiler/lexer.c` - Comment-only line detection now recognizes `#`

---

## Open Issues

### String Interpolation with Escaped Quotes

**Status**: Not working

**Description**: Escaped quotes inside interpolated string expressions cause parsing errors.

**Example that fails**:
```sn
print($"Result: {f("hello")}\n")  # Fails - escaped quotes in interpolation
```

**Current workaround**: Use variables instead of string literals inside interpolations:
```sn
var s: str = "hello"
print($"Result: {f(s)}\n")  # Works
```

**Error message**:
```
[interpolated:1] Error: Unexpected character '\'
```

**Location**: The issue is in the interpolated string sub-parser in `compiler/parser_expr.c` which doesn't properly handle escape sequences within `{}` expressions.

---

## Future Improvements

### String Interpolation Enhancements

The following features would improve the string interpolation functionality:

#### 1. Escaped Quotes in Interpolation Expressions

Allow string literals with escaped quotes inside `$"{...}"` expressions:
```sn
print($"Greeting: {greet("World")}\n")  # Should work
print($"Name: {"Alice"}\n")              # Should work
```

**Implementation notes**: The lexer's `lexer_scan_string()` function tracks brace depth for interpolation but doesn't correctly handle escape sequences inside the braces. The sub-parser receives the raw escape sequences and fails to parse them.

#### 2. Nested Interpolated Strings

Support interpolated strings inside interpolated strings:
```sn
var inner: str = $"x={x}"
print($"Result: {$"nested {y}"}\n")  # Nested interpolation
```

#### 3. Format Specifiers

Add optional format specifiers for controlling output:
```sn
var pi: double = 3.14159
print($"Pi: {pi:.2f}\n")      # Format to 2 decimal places
print($"Hex: {value:x}\n")    # Hexadecimal format
print($"Padded: {n:05d}\n")   # Zero-padded to 5 digits
```

#### 4. Expression Type Coercion

Improve automatic type coercion in interpolations:
```sn
var arr: int[] = {1, 2, 3}
print($"Array: {arr}\n")      # Should print array contents
```

Currently only primitive types are automatically converted to strings.

#### 5. Multi-line Interpolated Strings

Support interpolated strings spanning multiple lines:
```sn
var message: str = $"
    Hello {name},
    Your balance is {balance}.
    "
```

---

## Parser Robustness

### Panic Mode Token Advancement

**Observation**: When `panic_mode` is set and a subsequent error is encountered, `parser_error_at` returns early without advancing past the problematic token. This can lead to infinite loops in parsing loops that don't explicitly check for this condition.

**Affected areas**:
- `parser_indented_block` while loop
- `parser_block_statement` while loop
- Any parsing loop that calls `parser_declaration` or `parser_statement`

**Potential fix**: Consider always advancing past the current token when an error occurs, even in panic mode, or add explicit checks in parsing loops to detect and break out of stuck states.
