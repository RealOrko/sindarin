#include "code_gen_stmt.h"
#include "code_gen_expr.h"
#include "code_gen_util.h"
#include "debug.h"
#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void code_gen_expression_statement(CodeGen *gen, ExprStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_expression_statement");
    char *expr_str = code_gen_expression(gen, stmt->expression);
    if (stmt->expression->expr_type->kind == TYPE_STRING && expression_produces_temp(stmt->expression))
    {
        // Skip freeing in arena context - arena handles cleanup
        if (gen->current_arena_var == NULL)
        {
            indented_fprintf(gen, indent, "{\n");
            indented_fprintf(gen, indent + 1, "char *_tmp = %s;\n", expr_str);
            indented_fprintf(gen, indent + 1, "(void)_tmp;\n");
            indented_fprintf(gen, indent + 1, "rt_free_string(_tmp);\n");
            indented_fprintf(gen, indent, "}\n");
        }
        else
        {
            indented_fprintf(gen, indent, "%s;\n", expr_str);
        }
    }
    else if (stmt->expression->type == EXPR_CALL && stmt->expression->expr_type->kind == TYPE_VOID)
    {
        // Statement expressions need a semicolon after them
        indented_fprintf(gen, indent, "%s;\n", expr_str);
    }
    else
    {
        indented_fprintf(gen, indent, "%s;\n", expr_str);
    }
}

void code_gen_var_declaration(CodeGen *gen, VarDeclStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_var_declaration");
    symbol_table_add_symbol_full(gen->symbol_table, stmt->name, stmt->type, SYMBOL_LOCAL, stmt->mem_qualifier);
    const char *type_c = get_c_type(gen->arena, stmt->type);
    char *var_name = get_var_name(gen->arena, stmt->name);
    char *init_str;
    if (stmt->initializer)
    {
        init_str = code_gen_expression(gen, stmt->initializer);
        // Wrap string literals in rt_to_string_string to create heap-allocated copies
        // This is needed because string variables may be freed/reassigned later
        if (stmt->type->kind == TYPE_STRING && stmt->initializer->type == EXPR_LITERAL)
        {
            init_str = arena_sprintf(gen->arena, "rt_to_string_string(%s, %s)", ARENA_VAR(gen), init_str);
        }

        // Handle 'as val' - create a copy for arrays and strings
        if (stmt->mem_qualifier == MEM_AS_VAL)
        {
            if (stmt->type->kind == TYPE_ARRAY)
            {
                // Get element type suffix for the clone function
                Type *elem_type = stmt->type->as.array.element_type;
                const char *suffix = code_gen_type_suffix(elem_type);
                init_str = arena_sprintf(gen->arena, "rt_array_clone_%s(%s, %s)", suffix, ARENA_VAR(gen), init_str);
            }
            else if (stmt->type->kind == TYPE_STRING)
            {
                init_str = arena_sprintf(gen->arena, "rt_to_string_string(%s, %s)", ARENA_VAR(gen), init_str);
            }
        }
    }
    else
    {
        init_str = arena_strdup(gen->arena, get_default_value(stmt->type));
    }

    // Handle 'as ref' - heap-allocate primitives via arena
    if (stmt->mem_qualifier == MEM_AS_REF)
    {
        // Allocate on arena and store pointer
        // e.g., long *x = (long *)rt_arena_alloc(__arena_1__, sizeof(long)); *x = 42L;
        indented_fprintf(gen, indent, "%s *%s = (%s *)rt_arena_alloc(%s, sizeof(%s));\n",
                         type_c, var_name, type_c, ARENA_VAR(gen), type_c);
        indented_fprintf(gen, indent, "*%s = %s;\n", var_name, init_str);
    }
    else
    {
        indented_fprintf(gen, indent, "%s %s = %s;\n", type_c, var_name, init_str);
    }
}

void code_gen_free_locals(CodeGen *gen, Scope *scope, bool is_function, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_free_locals");

    // Skip manual freeing when in arena context - arena handles all deallocation
    if (gen->current_arena_var != NULL)
    {
        return;
    }

    Symbol *sym = scope->symbols;
    while (sym)
    {
        if (sym->type && sym->type->kind == TYPE_STRING && sym->kind == SYMBOL_LOCAL)
        {
            char *var_name = get_var_name(gen->arena, sym->name);
            indented_fprintf(gen, indent, "if (%s) {\n", var_name);
            if (is_function && gen->current_return_type && gen->current_return_type->kind == TYPE_STRING)
            {
                indented_fprintf(gen, indent + 1, "if (%s != _return_value) {\n", var_name);
                indented_fprintf(gen, indent + 2, "rt_free_string(%s);\n", var_name);
                indented_fprintf(gen, indent + 1, "}\n");
            }
            else
            {
                indented_fprintf(gen, indent + 1, "rt_free_string(%s);\n", var_name);
            }
            indented_fprintf(gen, indent, "}\n");
        }
        else if (sym->type && sym->type->kind == TYPE_ARRAY && sym->kind == SYMBOL_LOCAL)
        {
            char *var_name = get_var_name(gen->arena, sym->name);
            Type *elem_type = sym->type->as.array.element_type;
            indented_fprintf(gen, indent, "if (%s) {\n", var_name);
            if (is_function && gen->current_return_type && gen->current_return_type->kind == TYPE_ARRAY)
            {
                indented_fprintf(gen, indent + 1, "if (%s != _return_value) {\n", var_name);
                if (elem_type && elem_type->kind == TYPE_STRING)
                {
                    indented_fprintf(gen, indent + 2, "rt_array_free_string(%s);\n", var_name);
                }
                else
                {
                    indented_fprintf(gen, indent + 2, "rt_array_free(%s);\n", var_name);
                }
                indented_fprintf(gen, indent + 1, "}\n");
            }
            else
            {
                if (elem_type && elem_type->kind == TYPE_STRING)
                {
                    indented_fprintf(gen, indent + 1, "rt_array_free_string(%s);\n", var_name);
                }
                else
                {
                    indented_fprintf(gen, indent + 1, "rt_array_free(%s);\n", var_name);
                }
            }
            indented_fprintf(gen, indent, "}\n");
        }
        sym = sym->next;
    }
}

void code_gen_block(CodeGen *gen, BlockStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_block");

    bool old_in_shared_context = gen->in_shared_context;
    bool old_in_private_context = gen->in_private_context;
    char *old_arena_var = gen->current_arena_var;
    int old_arena_depth = gen->arena_depth;

    bool is_shared = stmt->modifier == BLOCK_SHARED;
    bool is_private = stmt->modifier == BLOCK_PRIVATE;

    symbol_table_push_scope(gen->symbol_table);

    // Handle private block - create new arena
    if (is_private)
    {
        gen->in_private_context = true;
        gen->in_shared_context = false;
        gen->arena_depth++;
        gen->current_arena_var = arena_sprintf(gen->arena, "__arena_%d__", gen->arena_depth);
        symbol_table_enter_arena(gen->symbol_table);
    }
    // Handle shared block - uses parent's arena
    else if (is_shared)
    {
        gen->in_shared_context = true;
    }

    indented_fprintf(gen, indent, "{\n");

    // For private blocks, create a local arena
    if (is_private)
    {
        indented_fprintf(gen, indent + 1, "RtArena *%s = rt_arena_create(NULL);\n", gen->current_arena_var);
    }

    for (int i = 0; i < stmt->count; i++)
    {
        code_gen_statement(gen, stmt->statements[i], indent + 1);
    }
    code_gen_free_locals(gen, gen->symbol_table->current, false, indent + 1);

    // For private blocks, destroy the arena
    if (is_private)
    {
        indented_fprintf(gen, indent + 1, "rt_arena_destroy(%s);\n", gen->current_arena_var);
        symbol_table_exit_arena(gen->symbol_table);
    }

    indented_fprintf(gen, indent, "}\n");
    symbol_table_pop_scope(gen->symbol_table);

    // Restore context
    gen->in_shared_context = old_in_shared_context;
    gen->in_private_context = old_in_private_context;
    gen->current_arena_var = old_arena_var;
    gen->arena_depth = old_arena_depth;
}

void code_gen_function(CodeGen *gen, FunctionStmt *stmt)
{
    DEBUG_VERBOSE("Entering code_gen_function");
    char *old_function = gen->current_function;
    Type *old_return_type = gen->current_return_type;
    FunctionModifier old_func_modifier = gen->current_func_modifier;
    bool old_in_private_context = gen->in_private_context;
    char *old_arena_var = gen->current_arena_var;
    int old_arena_depth = gen->arena_depth;

    gen->current_function = get_var_name(gen->arena, stmt->name);
    gen->current_return_type = stmt->return_type;
    gen->current_func_modifier = stmt->modifier;

    bool is_main = strcmp(gen->current_function, "main") == 0;
    bool is_private = stmt->modifier == FUNC_PRIVATE;
    bool is_shared = stmt->modifier == FUNC_SHARED;
    // All functions have their own arena except shared (which uses caller's arena)
    bool needs_arena = is_main || !is_shared;

    // Non-shared functions and main have their own arena context
    if (needs_arena)
    {
        if (is_private) gen->in_private_context = true;
        gen->arena_depth++;
        gen->current_arena_var = arena_sprintf(gen->arena, "__arena_%d__", gen->arena_depth);
    }

    // Special case for main: always use "int" return type in C for standard entry point.
    const char *ret_c = is_main ? "int" : get_c_type(gen->arena, gen->current_return_type);
    // Determine if we need a _return_value variable: only for non-void or main.
    bool has_return_value = (gen->current_return_type && gen->current_return_type->kind != TYPE_VOID) || is_main;
    symbol_table_push_scope(gen->symbol_table);

    // For private functions and main, enter arena scope in symbol table
    if (needs_arena)
    {
        symbol_table_enter_arena(gen->symbol_table);
    }

    for (int i = 0; i < stmt->param_count; i++)
    {
        symbol_table_add_symbol_with_kind(gen->symbol_table, stmt->params[i].name, stmt->params[i].type, SYMBOL_PARAM);
    }
    indented_fprintf(gen, 0, "%s %s(", ret_c, gen->current_function);
    for (int i = 0; i < stmt->param_count; i++)
    {
        const char *param_type_c = get_c_type(gen->arena, stmt->params[i].type);
        char *param_name = get_var_name(gen->arena, stmt->params[i].name);
        fprintf(gen->output, "%s %s", param_type_c, param_name);
        if (i < stmt->param_count - 1)
        {
            fprintf(gen->output, ", ");
        }
    }
    indented_fprintf(gen, 0, ") {\n");

    // For private functions and main, create a local arena
    if (needs_arena)
    {
        indented_fprintf(gen, 1, "RtArena *%s = rt_arena_create(NULL);\n", gen->current_arena_var);
    }

    // Add _return_value only if needed (non-void or main).
    if (has_return_value)
    {
        const char *default_val = is_main ? "0" : get_default_value(gen->current_return_type);
        indented_fprintf(gen, 1, "%s _return_value = %s;\n", ret_c, default_val);
    }
    bool has_return = false;
    if (stmt->body_count > 0 && stmt->body[stmt->body_count - 1]->type == STMT_RETURN)
    {
        has_return = true;
    }
    for (int i = 0; i < stmt->body_count; i++)
    {
        code_gen_statement(gen, stmt->body[i], 1);
    }
    if (!has_return)
    {
        indented_fprintf(gen, 1, "goto %s_return;\n", gen->current_function);
    }
    indented_fprintf(gen, 0, "%s_return:\n", gen->current_function);
    code_gen_free_locals(gen, gen->symbol_table->current, true, 1);

    // For private functions and main, destroy the arena before returning
    if (needs_arena)
    {
        indented_fprintf(gen, 1, "rt_arena_destroy(%s);\n", gen->current_arena_var);
    }

    // Return _return_value only if needed; otherwise, plain return.
    if (has_return_value)
    {
        indented_fprintf(gen, 1, "return _return_value;\n");
    }
    else
    {
        indented_fprintf(gen, 1, "return;\n");
    }
    indented_fprintf(gen, 0, "}\n\n");

    // Exit arena scope in symbol table for private functions and main
    if (needs_arena)
    {
        symbol_table_exit_arena(gen->symbol_table);
    }

    symbol_table_pop_scope(gen->symbol_table);
    gen->current_function = old_function;
    gen->current_return_type = old_return_type;
    gen->current_func_modifier = old_func_modifier;
    gen->in_private_context = old_in_private_context;
    gen->current_arena_var = old_arena_var;
    gen->arena_depth = old_arena_depth;
}

void code_gen_return_statement(CodeGen *gen, ReturnStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_return_statement");
    if (stmt->value)
    {
        char *value_str = code_gen_expression(gen, stmt->value);
        indented_fprintf(gen, indent, "_return_value = %s;\n", value_str);
    }
    indented_fprintf(gen, indent, "goto %s_return;\n", gen->current_function);
}

void code_gen_if_statement(CodeGen *gen, IfStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_if_statement");
    char *cond_str = code_gen_expression(gen, stmt->condition);
    indented_fprintf(gen, indent, "if (%s) {\n", cond_str);
    code_gen_statement(gen, stmt->then_branch, indent + 1);
    indented_fprintf(gen, indent, "}\n");
    if (stmt->else_branch)
    {
        indented_fprintf(gen, indent, "else {\n");
        code_gen_statement(gen, stmt->else_branch, indent + 1);
        indented_fprintf(gen, indent, "}\n");
    }
}

void code_gen_while_statement(CodeGen *gen, WhileStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_while_statement");

    bool old_in_shared_context = gen->in_shared_context;
    char *old_loop_arena_var = gen->loop_arena_var;
    char *old_loop_cleanup_label = gen->loop_cleanup_label;
    char *old_current_arena_var = gen->current_arena_var;

    bool is_shared = stmt->is_shared;
    bool needs_loop_arena = !is_shared && gen->current_arena_var != NULL;

    // Shared loops don't create per-iteration arenas
    if (is_shared)
    {
        gen->in_shared_context = true;
    }

    // For non-shared loops with arena context, create per-iteration arena
    if (needs_loop_arena)
    {
        int label_num = code_gen_new_label(gen);
        gen->loop_arena_var = arena_sprintf(gen->arena, "__loop_arena_%d__", label_num);
        gen->loop_cleanup_label = arena_sprintf(gen->arena, "__loop_cleanup_%d__", label_num);
    }
    else
    {
        gen->loop_arena_var = NULL;
        gen->loop_cleanup_label = NULL;
    }

    char *cond_str = code_gen_expression(gen, stmt->condition);
    indented_fprintf(gen, indent, "while (%s) {\n", cond_str);

    // Create per-iteration arena at start of loop body
    if (needs_loop_arena)
    {
        indented_fprintf(gen, indent + 1, "RtArena *%s = rt_arena_create(%s);\n",
                         gen->loop_arena_var, ARENA_VAR(gen));
        // Switch to using the loop arena for allocations inside the loop body
        gen->current_arena_var = gen->loop_arena_var;
    }

    code_gen_statement(gen, stmt->body, indent + 1);

    // Restore parent arena before cleanup (for cleanup label context)
    if (needs_loop_arena)
    {
        gen->current_arena_var = old_current_arena_var;
    }

    // Cleanup label and arena destruction
    if (needs_loop_arena)
    {
        indented_fprintf(gen, indent, "%s:\n", gen->loop_cleanup_label);
        indented_fprintf(gen, indent + 1, "rt_arena_destroy(%s);\n", gen->loop_arena_var);
    }

    indented_fprintf(gen, indent, "}\n");

    gen->in_shared_context = old_in_shared_context;
    gen->loop_arena_var = old_loop_arena_var;
    gen->loop_cleanup_label = old_loop_cleanup_label;
}

void code_gen_for_statement(CodeGen *gen, ForStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_for_statement");

    bool old_in_shared_context = gen->in_shared_context;
    char *old_loop_arena_var = gen->loop_arena_var;
    char *old_loop_cleanup_label = gen->loop_cleanup_label;
    char *old_current_arena_var = gen->current_arena_var;

    bool is_shared = stmt->is_shared;
    bool needs_loop_arena = !is_shared && gen->current_arena_var != NULL;

    // Shared loops don't create per-iteration arenas
    if (is_shared)
    {
        gen->in_shared_context = true;
    }

    // For non-shared loops with arena context, create per-iteration arena
    if (needs_loop_arena)
    {
        int arena_label_num = code_gen_new_label(gen);
        gen->loop_arena_var = arena_sprintf(gen->arena, "__loop_arena_%d__", arena_label_num);
        gen->loop_cleanup_label = arena_sprintf(gen->arena, "__loop_cleanup_%d__", arena_label_num);
    }
    else
    {
        gen->loop_arena_var = NULL;
        gen->loop_cleanup_label = NULL;
    }

    symbol_table_push_scope(gen->symbol_table);
    indented_fprintf(gen, indent, "{\n");
    if (stmt->initializer)
    {
        code_gen_statement(gen, stmt->initializer, indent + 1);
    }
    char *cond_str = NULL;
    if (stmt->condition)
    {
        cond_str = code_gen_expression(gen, stmt->condition);
    }

    // Save old continue label and create new one for this for loop
    char *old_continue_label = gen->for_continue_label;
    int label_num = code_gen_new_label(gen);
    char *continue_label = arena_sprintf(gen->arena, "__for_continue_%d__", label_num);
    gen->for_continue_label = continue_label;

    indented_fprintf(gen, indent + 1, "while (%s) {\n", cond_str ? cond_str : "1");

    // Create per-iteration arena at start of loop body
    if (needs_loop_arena)
    {
        indented_fprintf(gen, indent + 2, "RtArena *%s = rt_arena_create(%s);\n",
                         gen->loop_arena_var, ARENA_VAR(gen));
        // Switch to using the loop arena for allocations inside the loop body
        gen->current_arena_var = gen->loop_arena_var;
    }

    code_gen_statement(gen, stmt->body, indent + 2);

    // Restore parent arena before cleanup
    if (needs_loop_arena)
    {
        gen->current_arena_var = old_current_arena_var;
    }

    // Cleanup label and arena destruction (before increment)
    if (needs_loop_arena)
    {
        indented_fprintf(gen, indent + 1, "%s:\n", gen->loop_cleanup_label);
        indented_fprintf(gen, indent + 2, "rt_arena_destroy(%s);\n", gen->loop_arena_var);
    }

    // Generate continue label before increment
    indented_fprintf(gen, indent + 1, "%s:;\n", continue_label);

    if (stmt->increment)
    {
        char *inc_str = code_gen_expression(gen, stmt->increment);
        indented_fprintf(gen, indent + 2, "%s;\n", inc_str);
    }
    indented_fprintf(gen, indent + 1, "}\n");

    // Restore old continue label
    gen->for_continue_label = old_continue_label;

    code_gen_free_locals(gen, gen->symbol_table->current, false, indent + 1);
    indented_fprintf(gen, indent, "}\n");
    symbol_table_pop_scope(gen->symbol_table);

    gen->in_shared_context = old_in_shared_context;
    gen->loop_arena_var = old_loop_arena_var;
    gen->loop_cleanup_label = old_loop_cleanup_label;
}

void code_gen_for_each_statement(CodeGen *gen, ForEachStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_for_each_statement");

    bool old_in_shared_context = gen->in_shared_context;
    char *old_loop_arena_var = gen->loop_arena_var;
    char *old_loop_cleanup_label = gen->loop_cleanup_label;

    bool is_shared = stmt->is_shared;
    bool needs_loop_arena = !is_shared && gen->current_arena_var != NULL;

    // Shared loops don't create per-iteration arenas
    if (is_shared)
    {
        gen->in_shared_context = true;
    }

    // For non-shared loops with arena context, create per-iteration arena
    if (needs_loop_arena)
    {
        int arena_label_num = code_gen_new_label(gen);
        gen->loop_arena_var = arena_sprintf(gen->arena, "__loop_arena_%d__", arena_label_num);
        gen->loop_cleanup_label = arena_sprintf(gen->arena, "__loop_cleanup_%d__", arena_label_num);
    }
    else
    {
        gen->loop_arena_var = NULL;
        gen->loop_cleanup_label = NULL;
    }

    // Generate a unique index variable name
    int temp_idx = gen->temp_count++;
    char *idx_var = arena_sprintf(gen->arena, "__idx_%d__", temp_idx);
    char *len_var = arena_sprintf(gen->arena, "__len_%d__", temp_idx);
    char *arr_var = arena_sprintf(gen->arena, "__arr_%d__", temp_idx);

    // Get the iterable expression
    char *iterable_str = code_gen_expression(gen, stmt->iterable);

    // Get the element type from the iterable's type
    Type *iterable_type = stmt->iterable->expr_type;
    Type *elem_type = iterable_type->as.array.element_type;
    const char *elem_c_type = get_c_type(gen->arena, elem_type);
    const char *arr_c_type = get_c_type(gen->arena, iterable_type);

    // Get the loop variable name
    char *var_name = get_var_name(gen->arena, stmt->var_name);

    // Create a new scope
    symbol_table_push_scope(gen->symbol_table);

    // Add the loop variable to the symbol table
    // Use SYMBOL_PARAM so it's not freed - loop var is a reference to array element, not owned
    symbol_table_add_symbol_with_kind(gen->symbol_table, stmt->var_name, elem_type, SYMBOL_PARAM);

    // Generate the for-each loop desugared to:
    // {
    //     arr_type __arr__ = iterable;
    //     long __len__ = rt_array_length(__arr__);
    //     for (long __idx__ = 0; __idx__ < __len__; __idx__++) {
    //         [RtArena *__loop_arena__ = rt_arena_create(parent);]  // if needs_loop_arena
    //         elem_type var = __arr__[__idx__];
    //         body
    //         [__loop_cleanup__: rt_arena_destroy(__loop_arena__);] // if needs_loop_arena
    //     }
    // }
    indented_fprintf(gen, indent, "{\n");
    indented_fprintf(gen, indent + 1, "%s %s = %s;\n", arr_c_type, arr_var, iterable_str);
    indented_fprintf(gen, indent + 1, "long %s = rt_array_length(%s);\n", len_var, arr_var);
    indented_fprintf(gen, indent + 1, "for (long %s = 0; %s < %s; %s++) {\n", idx_var, idx_var, len_var, idx_var);

    // Create per-iteration arena at start of loop body
    char *old_current_arena_var = gen->current_arena_var;
    if (needs_loop_arena)
    {
        indented_fprintf(gen, indent + 2, "RtArena *%s = rt_arena_create(%s);\n",
                         gen->loop_arena_var, ARENA_VAR(gen));
        // Switch to using the loop arena for allocations inside the loop body
        gen->current_arena_var = gen->loop_arena_var;
    }

    indented_fprintf(gen, indent + 2, "%s %s = %s[%s];\n", elem_c_type, var_name, arr_var, idx_var);

    // Generate the body
    code_gen_statement(gen, stmt->body, indent + 2);

    // Restore parent arena before cleanup
    if (needs_loop_arena)
    {
        gen->current_arena_var = old_current_arena_var;
    }

    // Cleanup label and arena destruction
    if (needs_loop_arena)
    {
        indented_fprintf(gen, indent + 1, "%s:\n", gen->loop_cleanup_label);
        indented_fprintf(gen, indent + 2, "rt_arena_destroy(%s);\n", gen->loop_arena_var);
    }

    indented_fprintf(gen, indent + 1, "}\n");
    code_gen_free_locals(gen, gen->symbol_table->current, false, indent + 1);
    indented_fprintf(gen, indent, "}\n");

    gen->in_shared_context = old_in_shared_context;
    gen->loop_arena_var = old_loop_arena_var;
    gen->loop_cleanup_label = old_loop_cleanup_label;

    symbol_table_pop_scope(gen->symbol_table);
}

void code_gen_statement(CodeGen *gen, Stmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_statement");
    switch (stmt->type)
    {
    case STMT_EXPR:
        code_gen_expression_statement(gen, &stmt->as.expression, indent);
        break;
    case STMT_VAR_DECL:
        code_gen_var_declaration(gen, &stmt->as.var_decl, indent);
        break;
    case STMT_FUNCTION:
        code_gen_function(gen, &stmt->as.function);
        break;
    case STMT_RETURN:
        code_gen_return_statement(gen, &stmt->as.return_stmt, indent);
        break;
    case STMT_BLOCK:
        code_gen_block(gen, &stmt->as.block, indent);
        break;
    case STMT_IF:
        code_gen_if_statement(gen, &stmt->as.if_stmt, indent);
        break;
    case STMT_WHILE:
        code_gen_while_statement(gen, &stmt->as.while_stmt, indent);
        break;
    case STMT_FOR:
        code_gen_for_statement(gen, &stmt->as.for_stmt, indent);
        break;
    case STMT_FOR_EACH:
        code_gen_for_each_statement(gen, &stmt->as.for_each_stmt, indent);
        break;
    case STMT_BREAK:
        // If in a loop with per-iteration arena, destroy it before breaking
        if (gen->loop_arena_var)
        {
            indented_fprintf(gen, indent, "{ rt_arena_destroy(%s); break; }\n", gen->loop_arena_var);
        }
        else
        {
            indented_fprintf(gen, indent, "break;\n");
        }
        break;
    case STMT_CONTINUE:
        // If there's a loop cleanup label (for per-iteration arena), jump to it
        // The cleanup label destroys the arena and falls through to continue/increment
        if (gen->loop_cleanup_label)
        {
            indented_fprintf(gen, indent, "goto %s;\n", gen->loop_cleanup_label);
        }
        // In for loops without arena, continue needs to jump to the continue label (before increment)
        else if (gen->for_continue_label)
        {
            indented_fprintf(gen, indent, "goto %s;\n", gen->for_continue_label);
        }
        // In while/for-each loops without arena, regular continue works fine
        else
        {
            indented_fprintf(gen, indent, "continue;\n");
        }
        break;
    case STMT_IMPORT:
        break;
    }
}
