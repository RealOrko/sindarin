#ifndef CODE_GEN_STMT_H
#define CODE_GEN_STMT_H

#include "code_gen.h"
#include "ast.h"

/* Statement code generation functions */
void code_gen_expression_statement(CodeGen *gen, ExprStmt *stmt, int indent);
void code_gen_var_declaration(CodeGen *gen, VarDeclStmt *stmt, int indent);
void code_gen_free_locals(CodeGen *gen, Scope *scope, bool is_function, int indent);
void code_gen_block(CodeGen *gen, BlockStmt *stmt, int indent);
void code_gen_function(CodeGen *gen, FunctionStmt *stmt);
void code_gen_return_statement(CodeGen *gen, ReturnStmt *stmt, int indent);
void code_gen_if_statement(CodeGen *gen, IfStmt *stmt, int indent);
void code_gen_while_statement(CodeGen *gen, WhileStmt *stmt, int indent);
void code_gen_for_statement(CodeGen *gen, ForStmt *stmt, int indent);
void code_gen_statement(CodeGen *gen, Stmt *stmt, int indent);

/* Pre-pass to identify primitives captured by closures in a function body.
 * These need to be declared as heap-allocated pointers so mutations persist. */
void code_gen_scan_captured_primitives(CodeGen *gen, Stmt **stmts, int stmt_count);

/* Check if a variable name is a captured primitive */
bool code_gen_is_captured_primitive(CodeGen *gen, const char *name);

/* Clear the captured primitives list (call at end of function) */
void code_gen_clear_captured_primitives(CodeGen *gen);

/* Private block arena stack operations */
void push_arena_to_stack(CodeGen *gen, const char *arena_name);
const char *pop_arena_from_stack(CodeGen *gen);

#endif /* CODE_GEN_STMT_H */
