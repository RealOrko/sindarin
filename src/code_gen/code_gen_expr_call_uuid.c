/**
 * code_gen_expr_call_uuid.c - Code generation for UUID method calls
 *
 * Contains implementations for generating C code from method calls on
 * UUID type, including toString, equals, version, and other property accessors.
 */

#include "code_gen/code_gen_expr_call.h"
#include "code_gen/code_gen_expr.h"
#include "code_gen/code_gen_util.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Dispatch UUID instance method calls */
char *code_gen_uuid_method_call(CodeGen *gen, Expr *expr, const char *method_name,
                                 Expr *object, int arg_count, Expr **arguments)
{
    char *object_str = code_gen_expression(gen, object);

    /* uuid.toString() -> rt_uuid_to_string(arena, uuid) */
    if (strcmp(method_name, "toString") == 0 && arg_count == 0)
    {
        return arena_sprintf(gen->arena, "rt_uuid_to_string(%s, %s)",
                             ARENA_VAR(gen), object_str);
    }

    /* uuid.toHex() -> rt_uuid_to_hex(arena, uuid) */
    if (strcmp(method_name, "toHex") == 0 && arg_count == 0)
    {
        return arena_sprintf(gen->arena, "rt_uuid_to_hex(%s, %s)",
                             ARENA_VAR(gen), object_str);
    }

    /* uuid.toBase64() -> rt_uuid_to_base64(arena, uuid) */
    if (strcmp(method_name, "toBase64") == 0 && arg_count == 0)
    {
        return arena_sprintf(gen->arena, "rt_uuid_to_base64(%s, %s)",
                             ARENA_VAR(gen), object_str);
    }

    /* uuid.toBytes() -> rt_uuid_to_bytes(arena, uuid) */
    if (strcmp(method_name, "toBytes") == 0 && arg_count == 0)
    {
        return arena_sprintf(gen->arena, "rt_uuid_to_bytes(%s, %s)",
                             ARENA_VAR(gen), object_str);
    }

    /* uuid.version() -> rt_uuid_get_version(uuid) */
    if (strcmp(method_name, "version") == 0 && arg_count == 0)
    {
        return arena_sprintf(gen->arena, "rt_uuid_get_version(%s)", object_str);
    }

    /* uuid.variant() -> rt_uuid_get_variant(uuid) */
    if (strcmp(method_name, "variant") == 0 && arg_count == 0)
    {
        return arena_sprintf(gen->arena, "rt_uuid_get_variant(%s)", object_str);
    }

    /* uuid.isNil() -> rt_uuid_is_nil(uuid) */
    if (strcmp(method_name, "isNil") == 0 && arg_count == 0)
    {
        return arena_sprintf(gen->arena, "rt_uuid_is_nil(%s)", object_str);
    }

    /* uuid.timestamp() -> rt_uuid_get_timestamp(uuid) */
    if (strcmp(method_name, "timestamp") == 0 && arg_count == 0)
    {
        return arena_sprintf(gen->arena, "rt_uuid_get_timestamp(%s)", object_str);
    }

    /* uuid.time() -> rt_uuid_get_time(arena, uuid) */
    if (strcmp(method_name, "time") == 0 && arg_count == 0)
    {
        return arena_sprintf(gen->arena, "rt_uuid_get_time(%s, %s)",
                             ARENA_VAR(gen), object_str);
    }

    /* uuid.equals(other) -> rt_uuid_equals(uuid, other) */
    if (strcmp(method_name, "equals") == 0 && arg_count == 1)
    {
        char *other_str = code_gen_expression(gen, arguments[0]);
        return arena_sprintf(gen->arena, "rt_uuid_equals(%s, %s)",
                             object_str, other_str);
    }

    /* Method not handled here */
    (void)expr; /* Reserved for future use */
    return NULL;
}
