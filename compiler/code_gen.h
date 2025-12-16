#ifndef CODE_GEN_H
#define CODE_GEN_H

#include "arena.h"
#include "ast.h"
#include "symbol_table.h"
#include <stdio.h>
#include <stdbool.h>

typedef struct {
    Arena *arena;
    int label_count;
    SymbolTable *symbol_table;
    FILE *output;
    char *current_function;
    Type *current_return_type;
    int temp_count;
    char *for_continue_label;  // Label to jump to for continue in for loops

    /* Arena context for memory management */
    int arena_depth;            // Current arena nesting level
    bool in_shared_context;     // Are we in a shared block/loop?
    bool in_private_context;    // Are we in a private block/function?
    char *current_arena_var;    // Name of current arena variable (e.g., "__arena__")
    FunctionModifier current_func_modifier;  // Current function's modifier

    /* Loop arena for per-iteration cleanup */
    char *loop_arena_var;       // Name of current loop's per-iteration arena (NULL if shared loop)
    char *loop_cleanup_label;   // Label for loop cleanup (used by break/continue)
} CodeGen;

void code_gen_init(Arena *arena, CodeGen *gen, SymbolTable *symbol_table, const char *output_file);
void code_gen_cleanup(CodeGen *gen);
int code_gen_new_label(CodeGen *gen);
void code_gen_module(CodeGen *gen, Module *module);
void code_gen_statement(CodeGen *gen, Stmt *stmt, int indent);
void code_gen_block(CodeGen *gen, BlockStmt *stmt, int indent);
void code_gen_function(CodeGen *gen, FunctionStmt *stmt);
void code_gen_return_statement(CodeGen *gen, ReturnStmt *stmt, int indent);
void code_gen_if_statement(CodeGen *gen, IfStmt *stmt, int indent);
void code_gen_while_statement(CodeGen *gen, WhileStmt *stmt, int indent);
void code_gen_for_statement(CodeGen *gen, ForStmt *stmt, int indent);

#endif