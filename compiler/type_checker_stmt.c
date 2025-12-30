#include "type_checker_stmt.h"
#include "type_checker_util.h"
#include "type_checker_expr.h"
#include "debug.h"

/* Infer missing lambda types from a function type annotation */
static void infer_lambda_types(Expr *lambda_expr, Type *func_type)
{
    if (lambda_expr == NULL || lambda_expr->type != EXPR_LAMBDA)
        return;
    if (func_type == NULL || func_type->kind != TYPE_FUNCTION)
        return;

    LambdaExpr *lambda = &lambda_expr->as.lambda;

    /* Check parameter count matches */
    if (lambda->param_count != func_type->as.function.param_count)
    {
        DEBUG_VERBOSE("Lambda param count %d doesn't match function type param count %d",
                      lambda->param_count, func_type->as.function.param_count);
        return;
    }

    /* Infer missing parameter types */
    for (int i = 0; i < lambda->param_count; i++)
    {
        if (lambda->params[i].type == NULL)
        {
            lambda->params[i].type = func_type->as.function.param_types[i];
            DEBUG_VERBOSE("Inferred parameter %d type from function type", i);
        }
    }

    /* Infer missing return type */
    if (lambda->return_type == NULL)
    {
        lambda->return_type = func_type->as.function.return_type;
        DEBUG_VERBOSE("Inferred return type from function type");
    }
}

static void type_check_var_decl(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    (void)return_type;
    DEBUG_VERBOSE("Type checking variable declaration: %.*s", stmt->as.var_decl.name.length, stmt->as.var_decl.name.start);
    Type *decl_type = stmt->as.var_decl.type;
    Type *init_type = NULL;
    if (stmt->as.var_decl.initializer)
    {
        /* If initializer is a lambda with missing types, infer from declared type */
        if (stmt->as.var_decl.initializer->type == EXPR_LAMBDA &&
            decl_type != NULL && decl_type->kind == TYPE_FUNCTION)
        {
            infer_lambda_types(stmt->as.var_decl.initializer, decl_type);
        }

        init_type = type_check_expr(stmt->as.var_decl.initializer, table);
        if (init_type == NULL)
        {
            // If we have a declared type, use it; otherwise use NIL as placeholder
            Type *fallback = decl_type ? decl_type : ast_create_primitive_type(table->arena, TYPE_NIL);
            symbol_table_add_symbol_with_kind(table, stmt->as.var_decl.name, fallback, SYMBOL_LOCAL);
            return;
        }
        // For empty array literals, adopt the declared type for code generation
        if (decl_type && init_type->kind == TYPE_ARRAY &&
            init_type->as.array.element_type->kind == TYPE_NIL &&
            decl_type->kind == TYPE_ARRAY)
        {
            // Update the expression's type to match the declared type
            stmt->as.var_decl.initializer->expr_type = decl_type;
            init_type = decl_type;
        }
        // For int[] assigned to byte[], update the expression type to byte[]
        // This allows int literals to be used in byte array literals
        if (decl_type && decl_type->kind == TYPE_ARRAY &&
            decl_type->as.array.element_type->kind == TYPE_BYTE &&
            init_type->kind == TYPE_ARRAY &&
            init_type->as.array.element_type->kind == TYPE_INT)
        {
            stmt->as.var_decl.initializer->expr_type = decl_type;
            init_type = decl_type;
        }
    }

    // Type inference: if no declared type, infer from initializer
    if (decl_type == NULL)
    {
        if (init_type == NULL)
        {
            type_error(&stmt->as.var_decl.name, "Cannot infer type without initializer");
            decl_type = ast_create_primitive_type(table->arena, TYPE_NIL);
        }
        else
        {
            decl_type = init_type;
            // Update the statement's type for code generation
            stmt->as.var_decl.type = decl_type;
        }
    }

    // Validate memory qualifier usage
    MemoryQualifier mem_qual = stmt->as.var_decl.mem_qualifier;
    if (mem_qual == MEM_AS_REF)
    {
        // 'as ref' can only be used with primitive types
        if (!is_primitive_type(decl_type))
        {
            type_error(&stmt->as.var_decl.name, "'as ref' can only be used with primitive types");
        }
    }
    else if (mem_qual == MEM_AS_VAL)
    {
        // 'as val' is meaningful only for reference types (arrays, strings)
        // For primitives, it's a no-op but we allow it
        if (is_primitive_type(decl_type))
        {
            DEBUG_VERBOSE("Warning: 'as val' on primitive type has no effect");
        }
    }

    symbol_table_add_symbol_with_kind(table, stmt->as.var_decl.name, decl_type, SYMBOL_LOCAL);
    if (init_type && !ast_type_equals(init_type, decl_type))
    {
        type_error(&stmt->as.var_decl.name, "Initializer type does not match variable type");
    }
}

static void type_check_function(Stmt *stmt, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking function with %d parameters", stmt->as.function.param_count);

    /* Create function type from declaration */
    Arena *arena = table->arena;
    Type **param_types = (Type **)arena_alloc(arena, sizeof(Type *) * stmt->as.function.param_count);
    for (int i = 0; i < stmt->as.function.param_count; i++) {
        Type *param_type = stmt->as.function.params[i].type;
        /* Handle null parameter type - use NIL as placeholder */
        if (param_type == NULL) {
            param_type = ast_create_primitive_type(arena, TYPE_NIL);
        }
        param_types[i] = param_type;
    }
    Type *func_type = ast_create_function_type(arena, stmt->as.function.return_type, param_types, stmt->as.function.param_count);

    /* Validate private function return type */
    FunctionModifier modifier = stmt->as.function.modifier;
    if (modifier == FUNC_PRIVATE)
    {
        Type *return_type = stmt->as.function.return_type;
        if (!can_escape_private(return_type))
        {
            type_error(&stmt->as.function.name,
                       "Private function can only return primitive types (int, double, bool, char)");
        }
    }

    /* Functions returning heap-allocated types (closures, strings, arrays) must be
     * implicitly shared to avoid arena lifetime issues - the returned value must
     * live in caller's arena, not the function's arena which is destroyed on return.
     * This matches the implicit sharing logic in code_gen_stmt.c:301-307. */
    FunctionModifier effective_modifier = modifier;
    if (stmt->as.function.return_type &&
        (stmt->as.function.return_type->kind == TYPE_FUNCTION ||
         stmt->as.function.return_type->kind == TYPE_STRING ||
         stmt->as.function.return_type->kind == TYPE_ARRAY) &&
        modifier != FUNC_PRIVATE)
    {
        effective_modifier = FUNC_SHARED;
    }

    /* Add function symbol to current scope (e.g., global) with its modifier */
    symbol_table_add_function(table, stmt->as.function.name, func_type, effective_modifier);

    symbol_table_push_scope(table);

    for (int i = 0; i < stmt->as.function.param_count; i++)
    {
        Parameter param = stmt->as.function.params[i];
        DEBUG_VERBOSE("Adding parameter %d: %.*s", i, param.name.length, param.name.start);

        /* Check for null parameter type - report error and use placeholder */
        if (param.type == NULL)
        {
            type_error(&param.name, "Parameter type is missing");
            param.type = ast_create_primitive_type(arena, TYPE_NIL);
        }

        /* Validate parameter memory qualifier */
        if (param.mem_qualifier == MEM_AS_VAL)
        {
            /* 'as val' on parameters is meaningful only for reference types */
            if (is_primitive_type(param.type))
            {
                DEBUG_VERBOSE("Warning: 'as val' on primitive parameter has no effect");
            }
        }
        else if (param.mem_qualifier == MEM_AS_REF)
        {
            /* 'as ref' doesn't make sense for parameters - they're already references by default */
            type_error(&param.name, "'as ref' cannot be used on function parameters");
        }

        symbol_table_add_symbol_with_kind(table, param.name, param.type, SYMBOL_PARAM);
    }

    table->current->next_local_offset = table->current->next_param_offset;

    for (int i = 0; i < stmt->as.function.body_count; i++)
    {
        type_check_stmt(stmt->as.function.body[i], table, stmt->as.function.return_type);
    }
    symbol_table_pop_scope(table);
}

static void type_check_return(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    DEBUG_VERBOSE("Type checking return statement");
    Type *value_type;
    if (stmt->as.return_stmt.value)
    {
        value_type = type_check_expr(stmt->as.return_stmt.value, table);
        if (value_type == NULL)
            return;
    }
    else
    {
        value_type = ast_create_primitive_type(table->arena, TYPE_VOID);
    }
    if (!ast_type_equals(value_type, return_type))
    {
        type_error(stmt->token, "Return type does not match function return type");
    }
}

static void type_check_block(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    DEBUG_VERBOSE("Type checking block with %d statements", stmt->as.block.count);

    BlockModifier modifier = stmt->as.block.modifier;
    bool is_private = modifier == BLOCK_PRIVATE;

    if (is_private)
    {
        DEBUG_VERBOSE("Entering private block - escape analysis will be enforced");
        symbol_table_enter_arena(table);
    }
    else if (modifier == BLOCK_SHARED)
    {
        DEBUG_VERBOSE("Entering shared block - using parent's arena");
        /* Shared block: allocations use parent's arena, no special restrictions */
    }

    symbol_table_push_scope(table);
    for (int i = 0; i < stmt->as.block.count; i++)
    {
        type_check_stmt(stmt->as.block.statements[i], table, return_type);
    }
    symbol_table_pop_scope(table);

    if (is_private)
    {
        symbol_table_exit_arena(table);
    }
}

static void type_check_if(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    DEBUG_VERBOSE("Type checking if statement");
    Type *cond_type = type_check_expr(stmt->as.if_stmt.condition, table);
    if (cond_type && cond_type->kind != TYPE_BOOL)
    {
        type_error(stmt->as.if_stmt.condition->token, "If condition must be boolean");
    }
    type_check_stmt(stmt->as.if_stmt.then_branch, table, return_type);
    if (stmt->as.if_stmt.else_branch)
    {
        DEBUG_VERBOSE("Type checking else branch");
        type_check_stmt(stmt->as.if_stmt.else_branch, table, return_type);
    }
}

static void type_check_while(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    DEBUG_VERBOSE("Type checking while statement");
    Type *cond_type = type_check_expr(stmt->as.while_stmt.condition, table);
    if (cond_type && cond_type->kind != TYPE_BOOL)
    {
        type_error(stmt->as.while_stmt.condition->token, "While condition must be boolean");
    }

    // Non-shared loops have per-iteration arenas - enter arena context for escape analysis
    bool is_shared = stmt->as.while_stmt.is_shared;
    if (!is_shared)
    {
        symbol_table_enter_arena(table);
    }

    type_check_stmt(stmt->as.while_stmt.body, table, return_type);

    if (!is_shared)
    {
        symbol_table_exit_arena(table);
    }
}

static void type_check_for(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    DEBUG_VERBOSE("Type checking for statement");
    symbol_table_push_scope(table);
    if (stmt->as.for_stmt.initializer)
    {
        type_check_stmt(stmt->as.for_stmt.initializer, table, return_type);
    }
    if (stmt->as.for_stmt.condition)
    {
        Type *cond_type = type_check_expr(stmt->as.for_stmt.condition, table);
        if (cond_type && cond_type->kind != TYPE_BOOL)
        {
            type_error(stmt->as.for_stmt.condition->token, "For condition must be boolean");
        }
    }
    if (stmt->as.for_stmt.increment)
    {
        type_check_expr(stmt->as.for_stmt.increment, table);
    }

    // Non-shared loops have per-iteration arenas - enter arena context for escape analysis
    bool is_shared = stmt->as.for_stmt.is_shared;
    if (!is_shared)
    {
        symbol_table_enter_arena(table);
    }

    type_check_stmt(stmt->as.for_stmt.body, table, return_type);

    if (!is_shared)
    {
        symbol_table_exit_arena(table);
    }

    symbol_table_pop_scope(table);
}

static void type_check_for_each(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    DEBUG_VERBOSE("Type checking for-each statement");

    // Type check the iterable expression
    Type *iterable_type = type_check_expr(stmt->as.for_each_stmt.iterable, table);
    if (iterable_type == NULL)
    {
        return;
    }

    // Verify the iterable is an array type
    if (iterable_type->kind != TYPE_ARRAY)
    {
        type_error(stmt->as.for_each_stmt.iterable->token, "For-each iterable must be an array");
        return;
    }

    // Get the element type from the array
    Type *element_type = iterable_type->as.array.element_type;

    // Create a new scope and add the loop variable
    // Use SYMBOL_PARAM so it's not freed - loop var is a reference to array element, not owned
    symbol_table_push_scope(table);
    symbol_table_add_symbol_with_kind(table, stmt->as.for_each_stmt.var_name, element_type, SYMBOL_PARAM);

    // Non-shared loops have per-iteration arenas - enter arena context for escape analysis
    bool is_shared = stmt->as.for_each_stmt.is_shared;
    if (!is_shared)
    {
        symbol_table_enter_arena(table);
    }

    // Type check the body
    type_check_stmt(stmt->as.for_each_stmt.body, table, return_type);

    if (!is_shared)
    {
        symbol_table_exit_arena(table);
    }

    symbol_table_pop_scope(table);
}

void type_check_stmt(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    if (stmt == NULL)
    {
        DEBUG_VERBOSE("Statement is NULL");
        return;
    }
    DEBUG_VERBOSE("Type checking statement type: %d", stmt->type);
    switch (stmt->type)
    {
    case STMT_EXPR:
        type_check_expr(stmt->as.expression.expression, table);
        break;
    case STMT_VAR_DECL:
        type_check_var_decl(stmt, table, return_type);
        break;
    case STMT_FUNCTION:
        type_check_function(stmt, table);
        break;
    case STMT_RETURN:
        type_check_return(stmt, table, return_type);
        break;
    case STMT_BLOCK:
        type_check_block(stmt, table, return_type);
        break;
    case STMT_IF:
        type_check_if(stmt, table, return_type);
        break;
    case STMT_WHILE:
        type_check_while(stmt, table, return_type);
        break;
    case STMT_FOR:
        type_check_for(stmt, table, return_type);
        break;
    case STMT_FOR_EACH:
        type_check_for_each(stmt, table, return_type);
        break;
    case STMT_BREAK:
        DEBUG_VERBOSE("Type checking break statement");
        // TODO: Verify break is inside a loop
        break;
    case STMT_CONTINUE:
        DEBUG_VERBOSE("Type checking continue statement");
        // TODO: Verify continue is inside a loop
        break;
    case STMT_IMPORT:
        DEBUG_VERBOSE("Skipping type check for import statement");
        break;
    }
}
