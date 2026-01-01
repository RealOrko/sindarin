# String Interpolation Technical Analysis

## Overview

This document provides a technical analysis of the string interpolation implementation in the Sn compiler, documenting how the `in_nested_string` flag works, the flow through the compiler, and the exact failure point for escaped quotes in interpolation expressions.

## Current Implementation Flow

### 1. Lexer Stage (`compiler/lexer.c:302-310`, `compiler/lexer_scan.c:260-379`)

When the lexer encounters `$"`, it:
1. Advances past the `$` (line 305)
2. Calls `lexer_scan_string()` (line 306)
3. Changes the token type to `TOKEN_INTERPOL_STRING` (line 307)

The `lexer_scan_string()` function in `lexer_scan.c:260-379` handles the scanning.

### 2. Parser Stage (`compiler/parser_expr.c:412-545`)

When the parser encounters `TOKEN_INTERPOL_STRING`:
1. Extracts the string content from `parser->previous.literal.string_value`
2. Iterates through the content character by character
3. For `{{` and `}}`: Converts to literal `{` and `}`
4. For `{`: Extracts the expression content, creates a sub-lexer and sub-parser, parses the expression
5. For other characters: Accumulates them into text segments

### 3. Type Checker Stage (`compiler/type_checker_expr.c:125-144`)

The `type_check_interpolated()` function:
1. Iterates through all parts
2. Type-checks each part expression
3. Verifies each part has a "printable" type
4. Returns `TYPE_STRING` for the overall expression

### 4. Code Generation Stage (`compiler/code_gen_expr.c:719-845`)

The `code_gen_interpolated_expression()` function:
1. Generates code for each part
2. Converts non-string types using `rt_to_string_*` functions
3. Concatenates all parts using `rt_str_concat()`

---

## How `in_nested_string` Flag Works

**Location**: `compiler/lexer_scan.c:271`

```c
int in_nested_string = 0;  // Track if we're inside a string within {}
```

### Purpose

The `in_nested_string` flag is designed to track whether the lexer is currently inside a nested string literal within an interpolation expression (i.e., inside `{...}`).

### State Transitions

| Condition | Action |
|-----------|--------|
| Encounter `{` when `!in_nested_string` | Increment `brace_depth` |
| Encounter `}` when `!in_nested_string` | Decrement `brace_depth` |
| Encounter `"` when `brace_depth > 0` | Toggle `in_nested_string` |
| Encounter `"` when `brace_depth == 0 && !in_nested_string` | End of string |

### Code Reference (`lexer_scan.c:331-344`)

```c
// Track brace depth for interpolation
if (c == '{' && !in_nested_string)
{
    brace_depth++;
}
else if (c == '}' && !in_nested_string)
{
    if (brace_depth > 0) brace_depth--;
}
// Track nested strings inside braces
else if (c == '"' && brace_depth > 0)
{
    in_nested_string = !in_nested_string;
}
```

### Current Behavior

1. When entering `{`, `brace_depth` increases
2. When seeing `"` inside `{}`, `in_nested_string` toggles ON
3. When seeing closing `"` inside `{}`, `in_nested_string` toggles OFF
4. While `in_nested_string` is true, `{` and `}` are not counted as interpolation boundaries

### Limitation

The `in_nested_string` toggle works for simple cases like `$"result: {"hello"}"`, but **fails when there are escaped quotes** because the escape sequence handling doesn't interact properly with the `in_nested_string` tracking.

---

## Exact Failure Point for Escaped Quotes

### The Problem

When processing a string like:
```sn
print($"Result: {f("hello")}\n")
```

The flow is:
1. **Lexer scans the entire string**: The lexer correctly handles the full string content because `in_nested_string` tracking works at the lexer level.
2. **Parser extracts interpolation expression**: The content `f("hello")` is extracted as a substring.
3. **Sub-parser is created**: A new lexer and parser are created to parse `f("hello")`.
4. **Sub-lexer encounters `\"`**: Here is the failure point!

### Failure Location (`compiler/parser_expr.c:498-510`)

```c
Lexer sub_lexer;
lexer_init(parser->arena, &sub_lexer, expr_src, "interpolated");
Parser sub_parser;
parser_init(parser->arena, &sub_parser, &sub_lexer, parser->symbol_table);
sub_parser.symbol_table = parser->symbol_table;

Expr *inner = parser_expression(&sub_parser);
if (inner == NULL || sub_parser.had_error)
{
    parser_error_at_current(parser, "Invalid expression in interpolation");
    // ...
}
```

### Root Cause Analysis

The issue is a **two-pass problem**:

**Pass 1 (Main Lexer) - `lexer_scan.c:288-326`:**
When `brace_depth > 0` (inside interpolation braces), the lexer keeps escape sequences as-is:
```c
else  // brace_depth > 0
{
    // Inside braces, keep escape sequence as-is for sub-parser
    buffer[buffer_index++] = escaped;
}
```

So for input: `$"Result: {f(\"hello\")}\n"`
The lexer stores: `Result: {f(\"hello\")}\n` (with backslash-quote preserved)

**Pass 2 (Sub-Parser extraction) - `parser_expr.c:480-496`:**
The parser extracts the expression by simple brace matching:
```c
p++; // skip {
const char *expr_start = p;
int brace_depth = 1;
while (*p && brace_depth > 0)
{
    if (*p == '{') brace_depth++;
    else if (*p == '}') brace_depth--;
    if (brace_depth > 0) p++;
}
```

This extracts `f(\"hello\")` - still with the backslash.

**Pass 3 (Sub-Lexer) - `lexer_scan.c`:**
When the sub-lexer tries to scan `f(\"hello\")`:
1. It sees `f` - identifier
2. It sees `(` - left paren
3. It sees `\` - **ERROR!** The sub-lexer's `lexer_scan_token()` doesn't handle `\` as a standalone character.

The sub-lexer encounters the `\` at `lexer.c:312-315`:
```c
default:
    snprintf(error_buffer, sizeof(error_buffer), "Unexpected character '%c'", c);
    DEBUG_VERBOSE("Line %d: Error - %s", lexer->line, error_buffer);
    return lexer_error_token(lexer, error_buffer);
```

**This produces the error**: `[interpolated:1] Error: Unexpected character '\'`

### Why `in_nested_string` Doesn't Help Here

The `in_nested_string` flag is only used within the *main* lexer's string scanning. When the expression is extracted and passed to the sub-lexer:

1. The sub-lexer doesn't know it's parsing an interpolation expression
2. The sub-lexer sees `\"` as a backslash followed by a quote, not as an escape sequence
3. The backslash is an invalid character in the normal token scanning context

---

## Current Limitations Summary

### 1. Escaped Quotes in Interpolation (`ISSUES.md:25-47`)
- **Status**: Not working
- **Cause**: Escape sequences are preserved for sub-parser, but sub-lexer doesn't handle them
- **Workaround**: Use variables instead of string literals

### 2. Nested Interpolated Strings (`ISSUES.md:68-74`)
- **Status**: Not supported
- **Example**: `$"Result: {$"nested {y}"}\n"`
- **Cause**: Would require recursive handling in the parser

### 3. No Format Specifiers (`ISSUES.md:76-85`)
- **Status**: Not implemented
- **Example**: `$"Pi: {pi:.2f}\n"`

### 4. Limited Type Coercion (`ISSUES.md:87-94`)
- **Status**: Only primitive types supported
- **Example**: Arrays cannot be directly interpolated

### 5. No Multi-line Interpolated Strings (`ISSUES.md:96-104`)
- **Status**: Not supported

---

## Proposed Fixes

### Fix for Escaped Quotes in Interpolation

The fundamental issue is that the lexer preserves `\"` for the sub-parser, but the sub-lexer doesn't understand escape sequences outside of string context.

**Solution Options:**

#### Option A: Pre-process escape sequences in parser
Before creating the sub-lexer, convert escape sequences to actual characters:
- `\"` → `"`
- `\\` → `\`

**Risk**: Need to handle nested escaping carefully.

#### Option B: Modify sub-lexer behavior
Pass a flag to the sub-lexer indicating it's parsing an interpolation expression, allowing it to handle escape sequences appropriately.

#### Option C: Different tokenization strategy
Instead of extracting expression source and re-lexing, have the main lexer tokenize the expression directly and store tokens for later parsing.

### Recommended Approach

**Option A** is the simplest and least invasive:

```c
// In parser_expr.c, after extracting expr_src:
char *processed_src = preprocess_interp_escapes(parser->arena, expr_src, expr_len);

Lexer sub_lexer;
lexer_init(parser->arena, &sub_lexer, processed_src, "interpolated");
```

Where `preprocess_interp_escapes` would convert `\"` to actual quote characters, allowing the sub-lexer to see valid string literals.

---

## Test Coverage Gaps

Current tests do NOT cover:
1. String literals inside interpolation expressions
2. Function calls with string arguments in interpolation
3. Nested string operations in interpolation
4. Escaped characters in interpolation expressions

Recommended additional tests:
```sn
// Test escaped quotes (should work after fix)
fn greet(s: str): str => $"Hello {s}"
print($"Greeting: {greet("World")}\n")

// Test string literal in interpolation
print($"Direct: {"hello"}\n")

// Test nested operations
var s: str = "test"
print($"Upper: {s.toUpper()}\n")
```
