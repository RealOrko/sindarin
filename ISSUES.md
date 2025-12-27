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

### String Interpolation with Escaped Quotes (Fixed)

**Problem**: Escaped quotes inside interpolated string expressions caused parsing errors.

**Example that previously failed**:
```sn
print($"Result: {f("hello")}\n")  # Failed - escaped quotes in interpolation
```

**Root Cause**: Two-pass problem in string interpolation handling:
1. The lexer preserved escape sequences (`\"`) inside interpolation braces for the sub-parser
2. The parser's expression extraction loop didn't track nested strings, so it would misidentify `}` inside nested strings as interpolation boundaries
3. The sub-lexer received raw escape sequences but didn't understand them outside string context

**Fix**: Made two changes:
- `compiler/lexer_scan.c` - Updated escape sequence handling to preserve `\` and escaped character together when inside braces or nested strings, and to not toggle `in_nested_string` when encountering escaped quotes
- `compiler/parser_expr.c` - Updated interpolation expression extraction to track nested strings and skip over escape sequences to correctly identify expression boundaries

**Working examples after fix**:
```sn
print($"Direct: {"hello"}\n")           # String literal in interpolation
print($"Upper: {"test".toUpper()}\n")   # String method on literal
print($"Tab: {"\ttab"}\n")              # Escape sequence in nested string
```

---

### Nested Interpolated Strings (Fixed)

**Feature**: Support interpolated strings inside interpolated strings, up to 4 levels deep.

**Implementation**:
- `compiler/lexer_scan.c` - Added `string_depth` and `interpol_depth` tracking to handle nested `$"..."` patterns
- Lexer maintains depth counters to correctly match braces at each nesting level

**Working examples**:
```sn
var x: int = 42
print($"Outer: {$"Inner: {x}"}\n")                    # 2-level nesting
print($"L1: {$"L2: {$"L3: {x}"}"}\n")                 # 3-level nesting
print($"A{$"B{$"C{$"D{x}"}"}"}\n")                    # 4-level nesting
```

**Tests**: `compiler/tests/integration/test_string_interpol_nested.sn`

---

### Format Specifiers in String Interpolation (Fixed)

**Feature**: Added format specifiers for controlling output format using `{expr:format}` syntax.

**Implementation**:
- `compiler/lexer_scan.c` - Lexer recognizes `:` followed by format specifier in interpolation expressions
- `compiler/parser_expr.c` - Parser extracts format specifier and creates FORMAT_SPEC AST node
- `compiler/code_gen_expr.c` - Code generator emits appropriate printf format strings
- `compiler/runtime.c` - Runtime support for format conversions

**Supported format specifiers**:
| Specifier | Description | Example |
|-----------|-------------|---------|
| `.Nf` | Float with N decimal places | `{pi:.2f}` → `3.14` |
| `:x` | Lowercase hexadecimal | `{255:x}` → `ff` |
| `:X` | Uppercase hexadecimal | `{255:X}` → `FF` |
| `:Nd` | Zero-padded decimal | `{7:03d}` → `007` |
| `:0Nd` | Zero-padded to N digits | `{42:05d}` → `00042` |

**Working examples**:
```sn
var pi: double = 3.14159
print($"Pi: {pi:.2f}\n")        # Output: Pi: 3.14
print($"Hex: {255:x}\n")        # Output: Hex: ff
print($"Padded: {7:05d}\n")     # Output: Padded: 00007
```

**Tests**: `compiler/tests/integration/test_string_interpol_format.sn`
**Documentation**: `docs/format_specifiers.md`

---

### Multi-line Interpolated Strings (Fixed)

**Feature**: Support interpolated strings spanning multiple lines with proper whitespace preservation.

**Implementation**:
- `compiler/lexer_scan.c` - Lexer tracks line numbers within multi-line strings using `start_line` variable
- Whitespace (spaces, tabs, newlines) is preserved exactly as written
- Line tracking enables accurate error reporting for unterminated strings

**Working examples**:
```sn
var name: str = "Alice"
var age: int = 30
var profile: str = $"User Profile:
  Name: {name}
  Age: {age}
  Status: Active"
print(profile)
```

**Tests**: `compiler/tests/integration/test_string_interpol_multiline.sn`

---

## Open Issues

(No open issues at this time)

---

## Future Improvements

### String Interpolation Enhancements

The following features would further improve string interpolation:

#### 1. Expression Type Coercion for Complex Types

Improve automatic type coercion for arrays and structs in interpolations:
```sn
var arr: int[] = {1, 2, 3}
print($"Array: {arr}\n")      # Should print array contents
```

Currently only primitive types (int, double, bool, str, char) are automatically converted to strings.

#### 2. Ternary Expressions with String Literals in Interpolation

Currently, ternary operators with embedded string literals inside interpolation are not supported:
```sn
// This does NOT work:
print($"Status: {age >= 18 ? "Adult" : "Minor"}\n")

// Workaround - assign first:
var status: str = age >= 18 ? "Adult" : "Minor"
print($"Status: {status}\n")
```

#### 3. Additional Format Specifiers

Potential additions:
- `:b` - Binary format
- `:o` - Octal format
- `:e` - Scientific notation
- `:s` - String width/alignment
- `:+d` - Always show sign

---

## Parser Robustness

### Panic Mode Token Advancement

**Observation**: When `panic_mode` is set and a subsequent error is encountered, `parser_error_at` returns early without advancing past the problematic token. This can lead to infinite loops in parsing loops that don't explicitly check for this condition.

**Affected areas**:
- `parser_indented_block` while loop
- `parser_block_statement` while loop
- Any parsing loop that calls `parser_declaration` or `parser_statement`

**Potential fix**: Consider always advancing past the current token when an error occurs, even in panic mode, or add explicit checks in parsing loops to detect and break out of stuck states.
