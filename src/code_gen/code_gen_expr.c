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
        case TYPE_INT:
        case TYPE_LONG: suffix = "long"; break;
        case TYPE_INT32: suffix = "int32"; break;
        case TYPE_UINT: suffix = "uint"; break;
        case TYPE_UINT32: suffix = "uint32"; break;
        case TYPE_FLOAT: suffix = "float"; break;
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
            case TYPE_INT:
            case TYPE_LONG: default_str = "0"; break;
            case TYPE_INT32: default_str = "0"; break;
            case TYPE_UINT: default_str = "0"; break;
            case TYPE_UINT32: default_str = "0"; break;
            case TYPE_FLOAT: default_str = "0.0f"; break;
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

/**
 * Get the runtime type tag constant for a type.
 */
static const char *get_type_tag_constant(TypeKind kind)
{
    switch (kind)
    {
        case TYPE_NIL: return "RT_ANY_NIL";
        case TYPE_INT: return "RT_ANY_INT";
        case TYPE_LONG: return "RT_ANY_LONG";
        case TYPE_INT32: return "RT_ANY_INT32";
        case TYPE_UINT: return "RT_ANY_UINT";
        case TYPE_UINT32: return "RT_ANY_UINT32";
        case TYPE_DOUBLE: return "RT_ANY_DOUBLE";
        case TYPE_FLOAT: return "RT_ANY_FLOAT";
        case TYPE_STRING: return "RT_ANY_STRING";
        case TYPE_CHAR: return "RT_ANY_CHAR";
        case TYPE_BOOL: return "RT_ANY_BOOL";
        case TYPE_BYTE: return "RT_ANY_BYTE";
        case TYPE_ARRAY: return "RT_ANY_ARRAY";
        case TYPE_FUNCTION: return "RT_ANY_FUNCTION";
        case TYPE_TEXT_FILE: return "RT_ANY_TEXT_FILE";
        case TYPE_BINARY_FILE: return "RT_ANY_BINARY_FILE";
        case TYPE_DATE: return "RT_ANY_DATE";
        case TYPE_TIME: return "RT_ANY_TIME";
        case TYPE_PROCESS: return "RT_ANY_PROCESS";
        case TYPE_TCP_LISTENER: return "RT_ANY_TCP_LISTENER";
        case TYPE_TCP_STREAM: return "RT_ANY_TCP_STREAM";
        case TYPE_UDP_SOCKET: return "RT_ANY_UDP_SOCKET";
        case TYPE_RANDOM: return "RT_ANY_RANDOM";
        case TYPE_UUID: return "RT_ANY_UUID";
        case TYPE_ANY: return "RT_ANY_NIL";  /* any has no fixed tag */
        default: return "RT_ANY_NIL";
    }
}

/**
 * Generate code for typeof expression.
 * typeof(value) - returns runtime type tag of any value
 * typeof(Type) - returns compile-time type tag constant
 */
static char *code_gen_typeof_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Generating typeof expression");

    TypeofExpr *typeof_expr = &expr->as.typeof_expr;

    if (typeof_expr->type_literal != NULL)
    {
        /* typeof(int), typeof(str), etc. - compile-time constant */
        return arena_sprintf(gen->arena, "%s", get_type_tag_constant(typeof_expr->type_literal->kind));
    }
    else
    {
        /* typeof(value) - get runtime type tag */
        char *operand_code = code_gen_expression(gen, typeof_expr->operand);
        Type *operand_type = typeof_expr->operand->expr_type;

        if (operand_type->kind == TYPE_ANY)
        {
            /* For any type, get the runtime tag */
            return arena_sprintf(gen->arena, "rt_any_get_tag(%s)", operand_code);
        }
        else
        {
            /* For concrete types, return compile-time constant */
            return arena_sprintf(gen->arena, "%s", get_type_tag_constant(operand_type->kind));
        }
    }
}

/**
 * Generate code for 'is' type check expression.
 * expr is Type - checks if any value is of the specified type
 */
static char *code_gen_is_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Generating is expression");

    IsExpr *is_expr = &expr->as.is_expr;
    char *operand_code = code_gen_expression(gen, is_expr->operand);
    const char *type_tag = get_type_tag_constant(is_expr->check_type->kind);

    /* For array types, also check the element type tag */
    if (is_expr->check_type->kind == TYPE_ARRAY &&
        is_expr->check_type->as.array.element_type != NULL)
    {
        Type *elem_type = is_expr->check_type->as.array.element_type;
        const char *elem_tag = get_type_tag_constant(elem_type->kind);
        return arena_sprintf(gen->arena, "((%s).tag == %s && (%s).element_tag == %s)",
                             operand_code, type_tag, operand_code, elem_tag);
    }

    return arena_sprintf(gen->arena, "((%s).tag == %s)", operand_code, type_tag);
}

/**
 * Generate code for 'as Type' cast expression.
 * expr as Type - casts any value to concrete type (panics on mismatch)
 */
static char *code_gen_as_type_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Generating as type expression");

    AsTypeExpr *as_type = &expr->as.as_type;
    char *operand_code = code_gen_expression(gen, as_type->operand);
    Type *target_type = as_type->target_type;
    Type *operand_type = as_type->operand->expr_type;

    /* Check if this is an any[] to T[] cast */
    if (operand_type != NULL &&
        operand_type->kind == TYPE_ARRAY &&
        operand_type->as.array.element_type != NULL &&
        operand_type->as.array.element_type->kind == TYPE_ANY &&
        target_type->kind == TYPE_ARRAY &&
        target_type->as.array.element_type != NULL)
    {
        Type *target_elem = target_type->as.array.element_type;
        const char *conv_func = NULL;
        switch (target_elem->kind)
        {
        case TYPE_INT:
        case TYPE_INT32:
        case TYPE_UINT:
        case TYPE_UINT32:
        case TYPE_LONG:
            conv_func = "rt_array_from_any_long";
            break;
        case TYPE_DOUBLE:
        case TYPE_FLOAT:
            conv_func = "rt_array_from_any_double";
            break;
        case TYPE_CHAR:
            conv_func = "rt_array_from_any_char";
            break;
        case TYPE_BOOL:
            conv_func = "rt_array_from_any_bool";
            break;
        case TYPE_BYTE:
            conv_func = "rt_array_from_any_byte";
            break;
        case TYPE_STRING:
            conv_func = "rt_array_from_any_string";
            break;
        default:
            break;
        }
        if (conv_func != NULL)
        {
            return arena_sprintf(gen->arena, "%s(%s, %s)", conv_func, ARENA_VAR(gen), operand_code);
        }
    }

    /* Use the unbox helper function for single any values */
    return code_gen_unbox_value(gen, operand_code, target_type);
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
    case EXPR_TYPEOF:
        return code_gen_typeof_expression(gen, expr);
    case EXPR_IS:
        return code_gen_is_expression(gen, expr);
    case EXPR_AS_TYPE:
        return code_gen_as_type_expression(gen, expr);
    default:
        exit(1);
    }
    return NULL;
}
