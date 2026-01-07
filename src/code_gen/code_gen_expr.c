#include "code_gen/code_gen_expr.h"
#include "code_gen/code_gen_expr_core.h"
#include "code_gen/code_gen_expr_binary.h"
#include "code_gen/code_gen_expr_lambda.h"
#include "code_gen/code_gen_expr_call.h"
#include "code_gen/code_gen_expr_thread.h"
#include "code_gen/code_gen_expr_array.h"
#include "code_gen/code_gen_expr_static.h"
#include "code_gen/code_gen_expr_string.h"
#include "code_gen/code_gen_util.h"
#include "code_gen/code_gen_stmt.h"
#include "debug.h"
#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

/* Forward declarations */
char *code_gen_range_expression(CodeGen *gen, Expr *expr);

char *code_gen_increment_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_increment_expression");
    if (expr->as.operand->type != EXPR_VARIABLE)
    {
        exit(1);
    }
    char *var_name = get_var_name(gen->arena, expr->as.operand->as.variable.name);
    // For 'as ref' variables, they're already pointers, so pass directly
    Symbol *symbol = symbol_table_lookup_symbol(gen->symbol_table, expr->as.operand->as.variable.name);
    if (symbol && symbol->mem_qual == MEM_AS_REF)
    {
        return arena_sprintf(gen->arena, "rt_post_inc_long(%s)", var_name);
    }
    return arena_sprintf(gen->arena, "rt_post_inc_long(&%s)", var_name);
}

char *code_gen_decrement_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_decrement_expression");
    if (expr->as.operand->type != EXPR_VARIABLE)
    {
        exit(1);
    }
    char *var_name = get_var_name(gen->arena, expr->as.operand->as.variable.name);
    // For 'as ref' variables, they're already pointers, so pass directly
    Symbol *symbol = symbol_table_lookup_symbol(gen->symbol_table, expr->as.operand->as.variable.name);
    if (symbol && symbol->mem_qual == MEM_AS_REF)
    {
        return arena_sprintf(gen->arena, "rt_post_dec_long(%s)", var_name);
    }
    return arena_sprintf(gen->arena, "rt_post_dec_long(&%s)", var_name);
}

char *code_gen_member_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_member_expression");
    MemberExpr *member = &expr->as.member;
    char *member_name_str = get_var_name(gen->arena, member->member_name);
    Type *object_type = member->object->expr_type;

    /* Handle namespace member access (namespace.symbol).
     * If the object has no type (expr_type is NULL) and is a variable,
     * this is a namespace member reference. Return just the function name
     * since C functions are referenced by name without namespace prefix. */
    if (object_type == NULL && member->object->type == EXPR_VARIABLE)
    {
        /* Namespace member access - emit just the function name */
        return member_name_str;
    }

    char *object_str = code_gen_expression(gen, member->object);

    // Handle array.length
    if (object_type->kind == TYPE_ARRAY && strcmp(member_name_str, "length") == 0) {
        return arena_sprintf(gen->arena, "rt_array_length(%s)", object_str);
    }

    // Handle string.length
    if (object_type->kind == TYPE_STRING && strcmp(member_name_str, "length") == 0) {
        return arena_sprintf(gen->arena, "rt_str_length(%s)", object_str);
    }

    /* TextFile and BinaryFile properties - also in code_gen_expr_call_file.c */
    // Handle TextFile.path
    if (object_type->kind == TYPE_TEXT_FILE && strcmp(member_name_str, "path") == 0) {
        return arena_sprintf(gen->arena, "rt_text_file_get_path(%s, %s)",
            ARENA_VAR(gen), object_str);
    }

    // Handle TextFile.name
    if (object_type->kind == TYPE_TEXT_FILE && strcmp(member_name_str, "name") == 0) {
        return arena_sprintf(gen->arena, "rt_text_file_get_name(%s, %s)",
            ARENA_VAR(gen), object_str);
    }

    // Handle TextFile.size
    if (object_type->kind == TYPE_TEXT_FILE && strcmp(member_name_str, "size") == 0) {
        return arena_sprintf(gen->arena, "rt_text_file_get_size(%s)", object_str);
    }

    // Handle BinaryFile.path
    if (object_type->kind == TYPE_BINARY_FILE && strcmp(member_name_str, "path") == 0) {
        return arena_sprintf(gen->arena, "rt_binary_file_get_path(%s, %s)",
            ARENA_VAR(gen), object_str);
    }

    // Handle BinaryFile.name
    if (object_type->kind == TYPE_BINARY_FILE && strcmp(member_name_str, "name") == 0) {
        return arena_sprintf(gen->arena, "rt_binary_file_get_name(%s, %s)",
            ARENA_VAR(gen), object_str);
    }

    // Handle BinaryFile.size
    if (object_type->kind == TYPE_BINARY_FILE && strcmp(member_name_str, "size") == 0) {
        return arena_sprintf(gen->arena, "rt_binary_file_get_size(%s)", object_str);
    }

    /* Process properties - direct struct member access */
    // Handle Process.exitCode
    if (object_type->kind == TYPE_PROCESS && strcmp(member_name_str, "exitCode") == 0) {
        return arena_sprintf(gen->arena, "(%s)->exit_code", object_str);
    }

    // Handle Process.stdout
    if (object_type->kind == TYPE_PROCESS && strcmp(member_name_str, "stdout") == 0) {
        return arena_sprintf(gen->arena, "(%s)->stdout_data", object_str);
    }

    // Handle Process.stderr
    if (object_type->kind == TYPE_PROCESS && strcmp(member_name_str, "stderr") == 0) {
        return arena_sprintf(gen->arena, "(%s)->stderr_data", object_str);
    }

    /* TcpListener properties */
    // Handle TcpListener.port
    if (object_type->kind == TYPE_TCP_LISTENER && strcmp(member_name_str, "port") == 0) {
        return arena_sprintf(gen->arena, "(%s)->port", object_str);
    }

    /* TcpStream properties */
    // Handle TcpStream.remoteAddress
    if (object_type->kind == TYPE_TCP_STREAM && strcmp(member_name_str, "remoteAddress") == 0) {
        return arena_sprintf(gen->arena, "(%s)->remote_address", object_str);
    }

    /* UdpSocket properties */
    // Handle UdpSocket.port
    if (object_type->kind == TYPE_UDP_SOCKET && strcmp(member_name_str, "port") == 0) {
        return arena_sprintf(gen->arena, "(%s)->port", object_str);
    }
    // Handle UdpSocket.lastSender
    if (object_type->kind == TYPE_UDP_SOCKET && strcmp(member_name_str, "lastSender") == 0) {
        return arena_sprintf(gen->arena, "rt_udp_socket_get_last_sender(%s)", object_str);
    }

    // Generic struct member access (not currently supported)
    fprintf(stderr, "Error: Unsupported member access on type\n");
    exit(1);
}

char *code_gen_range_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_range_expression");
    RangeExpr *range = &expr->as.range;

    char *start_str = code_gen_expression(gen, range->start);
    char *end_str = code_gen_expression(gen, range->end);

    return arena_sprintf(gen->arena, "rt_array_range(%s, %s, %s)", ARENA_VAR(gen), start_str, end_str);
}

char *code_gen_spread_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_spread_expression");
    // Spread expressions are typically handled in array literal context
    // but if used standalone, just return the array
    return code_gen_expression(gen, expr->as.spread.array);
}

static char *code_gen_sized_array_alloc_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_sized_array_alloc_expression");

    /* Extract fields from sized_array_alloc */
    SizedArrayAllocExpr *alloc = &expr->as.sized_array_alloc;
    Type *element_type = alloc->element_type;
    Expr *size_expr = alloc->size_expr;
    Expr *default_value = alloc->default_value;

    /* Determine the runtime function suffix based on element type */
    const char *suffix = NULL;
    switch (element_type->kind) {
        case TYPE_INT: suffix = "long"; break;
        case TYPE_DOUBLE: suffix = "double"; break;
        case TYPE_CHAR: suffix = "char"; break;
        case TYPE_BOOL: suffix = "bool"; break;
        case TYPE_BYTE: suffix = "byte"; break;
        case TYPE_STRING: suffix = "string"; break;
        default:
            fprintf(stderr, "Error: Unsupported element type for sized array allocation\n");
            exit(1);
    }

    /* Generate code for the size expression */
    char *size_str = code_gen_expression(gen, size_expr);

    /* Generate code for the default value */
    char *default_str;
    if (default_value != NULL) {
        default_str = code_gen_expression(gen, default_value);
    } else {
        /* Use type-appropriate zero value when no default provided */
        switch (element_type->kind) {
            case TYPE_INT: default_str = "0"; break;
            case TYPE_DOUBLE: default_str = "0.0"; break;
            case TYPE_CHAR: default_str = "'\\0'"; break;
            case TYPE_BOOL: default_str = "0"; break;
            case TYPE_BYTE: default_str = "0"; break;
            case TYPE_STRING: default_str = "NULL"; break;
            default: default_str = "0"; break;
        }
    }

    /* Construct the runtime function call: rt_array_alloc_{suffix}(arena, size, default) */
    return arena_sprintf(gen->arena, "rt_array_alloc_%s(%s, %s, %s)",
                         suffix, ARENA_VAR(gen), size_str, default_str);
}

/**
 * Generate code for 'as val' expression - pointer dereference/value extraction.
 * For *int, *double, etc. - dereferences pointer to get value
 * For *char - converts null-terminated C string to Sn str
 */
static char *code_gen_as_val_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Generating as_val expression");

    AsValExpr *as_val = &expr->as.as_val;
    char *operand_code = code_gen_expression(gen, as_val->operand);

    if (as_val->is_noop)
    {
        /* Operand is already an array type (e.g., from ptr[0..len] slice).
         * Just pass through without any transformation. */
        return operand_code;
    }
    else if (as_val->is_cstr_to_str)
    {
        /* *char => str: use rt_arena_strdup to copy null-terminated C string to arena.
         * Handle NULL pointer by returning empty string. */
        return arena_sprintf(gen->arena, "((%s) ? rt_arena_strdup(%s, %s) : rt_arena_strdup(%s, \"\"))",
                            operand_code, ARENA_VAR(gen), operand_code, ARENA_VAR(gen));
    }
    else
    {
        /* Primitive pointer dereference: *int, *double, *float, etc. */
        return arena_sprintf(gen->arena, "(*(%s))", operand_code);
    }
}

char *code_gen_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_expression");
    if (expr == NULL)
    {
        return arena_strdup(gen->arena, "0L");
    }
    switch (expr->type)
    {
    case EXPR_BINARY:
        return code_gen_binary_expression(gen, &expr->as.binary);
    case EXPR_UNARY:
        return code_gen_unary_expression(gen, &expr->as.unary);
    case EXPR_LITERAL:
        return code_gen_literal_expression(gen, &expr->as.literal);
    case EXPR_VARIABLE:
        return code_gen_variable_expression(gen, &expr->as.variable);
    case EXPR_ASSIGN:
        return code_gen_assign_expression(gen, &expr->as.assign);
    case EXPR_INDEX_ASSIGN:
        return code_gen_index_assign_expression(gen, &expr->as.index_assign);
    case EXPR_CALL:
        return code_gen_call_expression(gen, expr);
    case EXPR_ARRAY:
        return code_gen_array_expression(gen, expr);
    case EXPR_ARRAY_ACCESS:
        return code_gen_array_access_expression(gen, &expr->as.array_access);
    case EXPR_INCREMENT:
        return code_gen_increment_expression(gen, expr);
    case EXPR_DECREMENT:
        return code_gen_decrement_expression(gen, expr);
    case EXPR_INTERPOLATED:
        return code_gen_interpolated_expression(gen, &expr->as.interpol);
    case EXPR_MEMBER:
        return code_gen_member_expression(gen, expr);
    case EXPR_ARRAY_SLICE:
        return code_gen_array_slice_expression(gen, expr);
    case EXPR_RANGE:
        return code_gen_range_expression(gen, expr);
    case EXPR_SPREAD:
        return code_gen_spread_expression(gen, expr);
    case EXPR_LAMBDA:
        return code_gen_lambda_expression(gen, expr);
    case EXPR_STATIC_CALL:
        return code_gen_static_call_expression(gen, expr);
    case EXPR_SIZED_ARRAY_ALLOC:
        return code_gen_sized_array_alloc_expression(gen, expr);
    case EXPR_THREAD_SPAWN:
        return code_gen_thread_spawn_expression(gen, expr);
    case EXPR_THREAD_SYNC:
        return code_gen_thread_sync_expression(gen, expr);
    case EXPR_SYNC_LIST:
        /* Sync lists are only valid as part of thread sync [r1, r2]! */
        fprintf(stderr, "Error: Sync list without sync operator\n");
        exit(1);
    case EXPR_AS_VAL:
        return code_gen_as_val_expression(gen, expr);
    default:
        exit(1);
    }
    return NULL;
}
