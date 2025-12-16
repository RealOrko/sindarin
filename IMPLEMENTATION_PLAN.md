# Memory Management Implementation Plan

This document outlines the implementation plan for Sindarin's arena-based memory management system as specified in MEMORY.md.

---

## Overview

### Goals
1. Implement block-scoped arenas in the runtime
2. Add new keywords: `shared`, `private`, `as val`, `as ref`
3. Modify code generation to use arenas instead of manual malloc/free
4. Add escape analysis for `private` block validation
5. Maintain backward compatibility - existing code compiles unchanged

### Non-Goals (for initial implementation)
- Optimization passes (can be added later)
- Debug allocation tracking (can be added later)

---

## Phase 1: Runtime Arena System

### 1.1 Arena Data Structure

**File**: `compiler/runtime.h`, `compiler/runtime.c`

```c
typedef struct Arena {
    struct Arena *parent;       // Parent arena (for hierarchy)
    char *base;                 // Base memory pointer
    char *current;              // Current allocation pointer
    char *end;                  // End of current block
    size_t block_size;          // Size of each block (default 64KB)
    struct ArenaBlock *blocks;  // Linked list of blocks
    int shared_depth;           // Track shared nesting depth
} Arena;

typedef struct ArenaBlock {
    struct ArenaBlock *next;
    char data[];                // Flexible array member
} ArenaBlock;
```

### 1.2 Arena Functions

```c
// Create a new arena (optionally with parent)
Arena *rt_arena_create(Arena *parent);

// Allocate memory from arena (bump allocation)
void *rt_arena_alloc(Arena *arena, size_t size);

// Allocate and zero memory
void *rt_arena_calloc(Arena *arena, size_t count, size_t size);

// Duplicate string into arena
char *rt_arena_strdup(Arena *arena, const char *str);

// Destroy arena and free all memory
void rt_arena_destroy(Arena *arena);

// Promote value from child arena to parent arena (copy)
void *rt_arena_promote(Arena *parent, void *ptr, size_t size);

// Get current arena (thread-local or passed explicitly)
Arena *rt_arena_current(void);
```

### 1.3 Modified Array/String Functions

All existing `rt_*` functions need arena-aware versions:

```c
// Old (to be deprecated internally)
char *rt_str_concat(const char *a, const char *b);

// New (arena-aware)
char *rt_str_concat_arena(Arena *arena, const char *a, const char *b);

// Array creation with arena
long *rt_array_create_long_arena(Arena *arena, size_t count, const long *data);
char **rt_array_create_string_arena(Arena *arena, size_t count, const char **data);
// ... etc for all array types
```

### 1.4 Tasks

- [ ] Define Arena struct in runtime.h
- [ ] Implement rt_arena_create()
- [ ] Implement rt_arena_alloc() with block growth
- [ ] Implement rt_arena_destroy()
- [ ] Implement rt_arena_promote()
- [ ] Add arena parameter to all rt_str_* functions
- [ ] Add arena parameter to all rt_array_* functions
- [ ] Add arena parameter to rt_to_string_* functions
- [ ] Write unit tests for arena operations

---

## Phase 2: Lexer Changes

### 2.1 New Tokens

**File**: `compiler/token.h`

```c
typedef enum {
    // ... existing tokens ...

    // New keywords
    TOKEN_SHARED,      // shared
    TOKEN_PRIVATE,     // private
    TOKEN_AS,          // as
    TOKEN_VAL,         // val
    TOKEN_REF,         // ref
} TokenType;
```

### 2.2 Keyword Recognition

**File**: `compiler/lexer_scan.c`

Add to keyword table:
```c
{"shared", TOKEN_SHARED},
{"private", TOKEN_PRIVATE},
{"as", TOKEN_AS},
{"val", TOKEN_VAL},
{"ref", TOKEN_REF},
```

### 2.3 Tasks

- [ ] Add TOKEN_SHARED, TOKEN_PRIVATE, TOKEN_AS, TOKEN_VAL, TOKEN_REF to token.h
- [ ] Add keywords to lexer keyword table
- [ ] Update token_type_to_string() for new tokens
- [ ] Write lexer tests for new tokens

---

## Phase 3: AST Changes

### 3.1 New AST Node Types

**File**: `compiler/ast.h`

```c
// Memory qualifier for variables/parameters
typedef enum {
    MEM_DEFAULT,    // Default behavior
    MEM_AS_VAL,     // as val - copy semantics
    MEM_AS_REF,     // as ref - reference semantics (for primitives)
} MemoryQualifier;

// Block modifier
typedef enum {
    BLOCK_DEFAULT,   // Normal block with own arena
    BLOCK_SHARED,    // shared block
    BLOCK_PRIVATE,   // private block
} BlockModifier;

// Function modifier
typedef enum {
    FUNC_DEFAULT,    // Normal function with own arena
    FUNC_SHARED,     // shared function (uses caller's arena)
    FUNC_PRIVATE,    // private function (isolated arena, primitives only return)
} FunctionModifier;
```

### 3.2 Modified Structures

```c
// Update VarDeclStmt
typedef struct {
    Token *name;
    Type *type;
    Expr *initializer;
    MemoryQualifier mem_qualifier;  // NEW
} VarDeclStmt;

// Update FunctionParam
typedef struct {
    Token *name;
    Type *type;
    MemoryQualifier mem_qualifier;  // NEW
} FunctionParam;

// Update FunctionStmt
typedef struct {
    Token *name;
    FunctionParam *params;
    int param_count;
    Type *return_type;
    Stmt **body;
    int body_count;
    FunctionModifier modifier;      // NEW
} FunctionStmt;

// Update BlockStmt
typedef struct {
    Stmt **statements;
    int count;
    Token *token;
    BlockModifier modifier;         // NEW
} BlockStmt;

// Update ForStmt, WhileStmt, ForEachStmt
typedef struct {
    // ... existing fields ...
    bool is_shared;                 // NEW: shared loop
} ForStmt;
```

### 3.3 New Statement Type

```c
// Add to StmtType enum
STMT_SHARED_BLOCK,
STMT_PRIVATE_BLOCK,

// Or reuse STMT_BLOCK with modifier
```

### 3.4 Tasks

- [ ] Add MemoryQualifier enum
- [ ] Add BlockModifier enum
- [ ] Add FunctionModifier enum
- [ ] Update VarDeclStmt with mem_qualifier
- [ ] Update FunctionParam with mem_qualifier
- [ ] Update FunctionStmt with modifier
- [ ] Update BlockStmt with modifier
- [ ] Update ForStmt with is_shared
- [ ] Update WhileStmt with is_shared
- [ ] Update ForEachStmt with is_shared
- [ ] Add AST creation functions for new nodes
- [ ] Update AST print functions

---

## Phase 4: Parser Changes

### 4.1 Parse Memory Qualifiers

**File**: `compiler/parser_stmt.c`, `compiler/parser_expr.c`

```c
// Parse "as val" or "as ref" after type
MemoryQualifier parse_memory_qualifier(Parser *parser) {
    if (match(parser, TOKEN_AS)) {
        if (match(parser, TOKEN_VAL)) {
            return MEM_AS_VAL;
        } else if (match(parser, TOKEN_REF)) {
            return MEM_AS_REF;
        }
        error(parser, "Expected 'val' or 'ref' after 'as'");
    }
    return MEM_DEFAULT;
}
```

### 4.2 Parse Function Modifiers

```c
// fn name() shared: ReturnType =>
// fn name() private: ReturnType =>
FunctionModifier parse_function_modifier(Parser *parser) {
    if (match(parser, TOKEN_SHARED)) {
        return FUNC_SHARED;
    } else if (match(parser, TOKEN_PRIVATE)) {
        return FUNC_PRIVATE;
    }
    return FUNC_DEFAULT;
}
```

### 4.3 Parse Block Modifiers

```c
// shared =>
//   ...
// private =>
//   ...
Stmt *parse_modified_block(Parser *parser) {
    if (match(parser, TOKEN_SHARED)) {
        consume(parser, TOKEN_ARROW, "Expected '=>' after 'shared'");
        // Parse block with BLOCK_SHARED modifier
    } else if (match(parser, TOKEN_PRIVATE)) {
        consume(parser, TOKEN_ARROW, "Expected '=>' after 'private'");
        // Parse block with BLOCK_PRIVATE modifier
    }
}
```

### 4.4 Parse Shared Loops

```c
// for ... shared =>
// while ... shared =>
bool parse_loop_shared(Parser *parser) {
    if (match(parser, TOKEN_SHARED)) {
        return true;
    }
    return false;
}
```

### 4.5 Parse Assignment with `as val`

```c
// var x: int[] = arr as val
Expr *parse_assignment_expr(Parser *parser) {
    Expr *expr = parse_or_expr(parser);
    if (match(parser, TOKEN_AS)) {
        if (match(parser, TOKEN_VAL)) {
            // Wrap expr in a "copy" expression
            return create_copy_expr(parser->arena, expr);
        }
        // TOKEN_REF in expression context is an error
    }
    return expr;
}
```

### 4.6 Tasks

- [ ] Implement parse_memory_qualifier()
- [ ] Modify parse_var_declaration() to handle `as val`/`as ref`
- [ ] Modify parse_function_param() to handle `as val`/`as ref`
- [ ] Modify parse_function() to handle `shared`/`private` modifiers
- [ ] Add parsing for `shared =>` blocks
- [ ] Add parsing for `private =>` blocks
- [ ] Modify parse_for_statement() to handle `shared`
- [ ] Modify parse_while_statement() to handle `shared`
- [ ] Modify parse_for_each_statement() to handle `shared`
- [ ] Modify expression parsing for `as val` in assignments
- [ ] Write parser tests for all new syntax

---

## Phase 5: Type Checker Changes

### 5.1 Escape Analysis for Private Blocks

**File**: `compiler/type_checker_stmt.c`

```c
// Track current context
typedef struct {
    bool in_private_block;
    bool in_private_function;
    int shared_depth;
} MemoryContext;

// Check if type can escape private
bool can_escape_private(Type *type) {
    return type->kind == TYPE_INT ||
           type->kind == TYPE_DOUBLE ||
           type->kind == TYPE_BOOL ||
           type->kind == TYPE_CHAR;
}
```

### 5.2 Validate Private Block Escapes

```c
void type_check_assignment_in_private(Stmt *stmt, MemoryContext *ctx) {
    if (ctx->in_private_block) {
        // Check if target variable is from outer scope
        // Check if value type can escape private
        if (!can_escape_private(value_type)) {
            type_error("Cannot assign %s to outer scope from private block",
                       type_to_string(value_type));
        }
    }
}
```

### 5.3 Validate Private Function Returns

```c
void type_check_private_function(FunctionStmt *fn) {
    if (fn->modifier == FUNC_PRIVATE) {
        if (!can_escape_private(fn->return_type)) {
            type_error("Private function cannot return %s, only primitives allowed",
                       type_to_string(fn->return_type));
        }
    }
}
```

### 5.4 Validate `as ref` Usage

```c
void type_check_as_ref(VarDeclStmt *stmt) {
    if (stmt->mem_qualifier == MEM_AS_REF) {
        // Only primitives can use 'as ref'
        if (!is_primitive_type(stmt->type)) {
            type_error("'as ref' can only be used with primitive types");
        }
    }
}
```

### 5.5 Tasks

- [ ] Add MemoryContext to type checker state
- [ ] Implement can_escape_private()
- [ ] Add escape checking for assignments in private blocks
- [ ] Add return type validation for private functions
- [ ] Add validation for `as ref` (primitives only)
- [ ] Add validation for `as val` (arrays/strings)
- [ ] Track scope depth for escape analysis
- [ ] Write type checker tests for all error cases

---

## Phase 6: Code Generator Changes

### 6.1 Arena Context Tracking

**File**: `compiler/code_gen.h`

```c
typedef struct {
    Arena *arena;
    int label_count;
    SymbolTable *symbol_table;
    FILE *output;
    char *current_function;
    Type *current_return_type;
    int temp_count;
    char *for_continue_label;

    // NEW: Arena context
    int arena_depth;            // Current arena nesting level
    bool in_shared_context;     // Are we in a shared block/loop?
    bool in_private_context;    // Are we in a private block?
    char *current_arena_var;    // Name of current arena variable
} CodeGen;
```

### 6.2 Generate Arena Lifecycle

```c
// At function entry
void code_gen_function_prologue(CodeGen *gen, FunctionStmt *fn) {
    if (fn->modifier == FUNC_SHARED) {
        // Use caller's arena (passed as parameter)
        indented_fprintf(gen, 1, "// shared function - using caller's arena\n");
    } else {
        // Create new arena
        indented_fprintf(gen, 1, "Arena *__arena__ = rt_arena_create(__parent_arena__);\n");
    }
}

// At function exit
void code_gen_function_epilogue(CodeGen *gen, FunctionStmt *fn) {
    if (fn->modifier != FUNC_SHARED) {
        indented_fprintf(gen, 1, "rt_arena_destroy(__arena__);\n");
    }
}
```

### 6.3 Generate Block Arenas

```c
void code_gen_block(CodeGen *gen, BlockStmt *stmt, int indent) {
    if (stmt->modifier == BLOCK_SHARED || gen->in_shared_context) {
        // No new arena - use parent's
        bool was_shared = gen->in_shared_context;
        gen->in_shared_context = true;
        // Generate statements...
        gen->in_shared_context = was_shared;
    } else if (stmt->modifier == BLOCK_PRIVATE) {
        // Isolated arena with no parent
        indented_fprintf(gen, indent, "{\n");
        indented_fprintf(gen, indent + 1, "Arena *__private_arena__ = rt_arena_create(NULL);\n");
        char *old_arena = gen->current_arena_var;
        gen->current_arena_var = "__private_arena__";
        gen->in_private_context = true;
        // Generate statements...
        gen->in_private_context = false;
        gen->current_arena_var = old_arena;
        indented_fprintf(gen, indent + 1, "rt_arena_destroy(__private_arena__);\n");
        indented_fprintf(gen, indent, "}\n");
    } else {
        // Default: new arena with parent
        indented_fprintf(gen, indent, "{\n");
        indented_fprintf(gen, indent + 1, "Arena *__block_arena_%d__ = rt_arena_create(%s);\n",
                         gen->arena_depth, gen->current_arena_var);
        // ...
    }
}
```

### 6.4 Generate Loop Arenas

```c
void code_gen_for_statement(CodeGen *gen, ForStmt *stmt, int indent) {
    if (stmt->is_shared || gen->in_shared_context) {
        // No per-iteration arena
        // Generate loop without arena management
    } else {
        // Default: arena per iteration
        indented_fprintf(gen, indent + 1, "Arena *__loop_arena__ = rt_arena_create(%s);\n",
                         gen->current_arena_var);
        // ... generate body ...
        indented_fprintf(gen, indent + 1, "rt_arena_destroy(__loop_arena__);\n");
    }
}
```

### 6.5 Generate Allocations with Arena

```c
// Before (current):
// char *temp = rt_str_concat(a, b);

// After (with arena):
// char *temp = rt_str_concat_arena(__arena__, a, b);
```

### 6.6 Generate `as val` Copies

```c
void code_gen_as_val_expr(CodeGen *gen, Expr *expr) {
    // Generate: rt_array_clone_TYPE_arena(__arena__, source)
    char *source = code_gen_expression(gen, expr->as.copy.source);
    return arena_sprintf(gen->arena, "rt_array_clone_%s_arena(%s, %s)",
                         get_type_suffix(expr->expr_type),
                         gen->current_arena_var,
                         source);
}
```

### 6.7 Generate Auto-Promotion

```c
// When assigning from inner arena to outer scope variable
void code_gen_promoting_assignment(CodeGen *gen, Expr *target, Expr *value, int indent) {
    // Check if value's arena != target's arena
    // If so, generate: target = rt_arena_promote(__outer_arena__, value, size);
}
```

### 6.8 Tasks

- [ ] Add arena context fields to CodeGen struct
- [ ] Implement code_gen_function_prologue() with arena creation
- [ ] Implement code_gen_function_epilogue() with arena destruction
- [ ] Modify code_gen_block() for shared/private/default cases
- [ ] Modify code_gen_for_statement() for arena management
- [ ] Modify code_gen_while_statement() for arena management
- [ ] Modify code_gen_for_each_statement() for arena management
- [ ] Update all expression generation to use arena-aware runtime functions
- [ ] Implement code_gen_as_val_expr() for copy semantics
- [ ] Implement promotion detection and generation
- [ ] Update string concatenation generation to use arenas
- [ ] Update array creation generation to use arenas
- [ ] Update all rt_to_string_* calls to use arenas
- [ ] Write code generation tests

---

## Phase 7: Symbol Table Changes

### 7.1 Track Variable Arena Context

```c
typedef struct Symbol {
    Token *name;
    Type *type;
    SymbolKind kind;
    int offset;
    struct Symbol *next;

    // NEW
    int arena_depth;            // Which arena depth owns this
    MemoryQualifier mem_qual;   // as val, as ref, or default
} Symbol;
```

### 7.2 Tasks

- [ ] Add arena_depth to Symbol struct
- [ ] Add mem_qual to Symbol struct
- [ ] Track arena depth when pushing scopes
- [ ] Update symbol lookup to include arena info

---

## Phase 8: Integration and Testing

### 8.1 Backward Compatibility Tests

Ensure all existing samples compile and run unchanged:

- [ ] samples/main.sn
- [ ] samples/lib/*.sn
- [ ] compiler/tests/integration/*.sn

### 8.2 New Feature Tests

Create integration tests for new features:

- [ ] test_shared_function.sn
- [ ] test_shared_loop.sn
- [ ] test_shared_block.sn
- [ ] test_private_block.sn
- [ ] test_private_function.sn
- [ ] test_as_val_assignment.sn
- [ ] test_as_val_parameter.sn
- [ ] test_as_ref_primitive.sn
- [ ] test_escape_promotion.sn
- [ ] test_private_escape_error.sn (should fail to compile)

### 8.3 Performance Tests

- [ ] Benchmark default vs shared loops
- [ ] Benchmark promotion overhead
- [ ] Benchmark arena allocation vs malloc

---

## Implementation Order

### Milestone 1: Runtime Foundation
1. Phase 1: Runtime Arena System
2. Basic tests for arena operations

### Milestone 2: Language Syntax
3. Phase 2: Lexer Changes
4. Phase 3: AST Changes
5. Phase 4: Parser Changes
6. Parser tests for new syntax

### Milestone 3: Semantic Analysis
7. Phase 5: Type Checker Changes
8. Type checker tests for escape analysis

### Milestone 4: Code Generation
9. Phase 7: Symbol Table Changes
10. Phase 6: Code Generator Changes
11. Code generation tests

### Milestone 5: Integration
12. Phase 8: Integration and Testing
13. Update documentation
14. Performance benchmarks

---

## Estimated Scope

| Phase | Files Modified | New Functions | Complexity |
|-------|---------------|---------------|------------|
| Phase 1 | 2 | ~15 | Medium |
| Phase 2 | 3 | ~5 | Low |
| Phase 3 | 2 | ~10 | Low |
| Phase 4 | 3 | ~10 | Medium |
| Phase 5 | 3 | ~10 | Medium |
| Phase 6 | 4 | ~20 | High |
| Phase 7 | 2 | ~5 | Low |
| Phase 8 | - | - | Medium |

**Total**: ~75 new functions across ~15 files

---

## Risks and Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| Breaking existing code | High | Extensive backward compat testing |
| Arena memory leaks | Medium | Valgrind/ASAN testing |
| Performance regression | Medium | Benchmarking before/after |
| Complex promotion logic | High | Start simple, add optimization later |
| Parser ambiguity | Low | Careful grammar design |

---

## Success Criteria

1. All existing samples compile and run unchanged
2. All existing tests pass
3. New features work as documented in MEMORY.md
4. No memory leaks detected by ASAN
5. `private` escape errors caught at compile time
6. Performance within 10% of current implementation for default cases
