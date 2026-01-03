#include "ast/ast_type.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

Type *ast_clone_type(Arena *arena, Type *type)
{
    if (type == NULL)
        return NULL;

    Type *clone = arena_alloc(arena, sizeof(Type));
    if (clone == NULL)
    {
        DEBUG_ERROR("Out of memory when cloning type");
        exit(1);
    }
    memset(clone, 0, sizeof(Type));

    clone->kind = type->kind;

    switch (type->kind)
    {
    case TYPE_INT:
    case TYPE_LONG:
    case TYPE_DOUBLE:
    case TYPE_CHAR:
    case TYPE_STRING:
    case TYPE_BOOL:
    case TYPE_BYTE:
    case TYPE_VOID:
    case TYPE_NIL:
    case TYPE_ANY:
    case TYPE_TEXT_FILE:
    case TYPE_BINARY_FILE:
    case TYPE_DATE:
    case TYPE_TIME:
    case TYPE_PROCESS:
        break;

    case TYPE_ARRAY:
        clone->as.array.element_type = ast_clone_type(arena, type->as.array.element_type);
        break;

    case TYPE_FUNCTION:
        clone->as.function.return_type = ast_clone_type(arena, type->as.function.return_type);
        clone->as.function.param_count = type->as.function.param_count;

        if (type->as.function.param_count > 0)
        {
            clone->as.function.param_types = arena_alloc(arena, sizeof(Type *) * type->as.function.param_count);
            if (clone->as.function.param_types == NULL)
            {
                DEBUG_ERROR("Out of memory when cloning function param types");
                exit(1);
            }
            for (int i = 0; i < type->as.function.param_count; i++)
            {
                clone->as.function.param_types[i] = ast_clone_type(arena, type->as.function.param_types[i]);
            }

            /* Clone param_mem_quals if present */
            if (type->as.function.param_mem_quals != NULL)
            {
                clone->as.function.param_mem_quals = arena_alloc(arena, sizeof(MemoryQualifier) * type->as.function.param_count);
                if (clone->as.function.param_mem_quals == NULL)
                {
                    DEBUG_ERROR("Out of memory when cloning function param qualifiers");
                    exit(1);
                }
                for (int i = 0; i < type->as.function.param_count; i++)
                {
                    clone->as.function.param_mem_quals[i] = type->as.function.param_mem_quals[i];
                }
            }
            else
            {
                clone->as.function.param_mem_quals = NULL;
            }
        }
        else
        {
            clone->as.function.param_types = NULL;
            clone->as.function.param_mem_quals = NULL;
        }
        break;
    }

    return clone;
}

Type *ast_create_primitive_type(Arena *arena, TypeKind kind)
{
    Type *type = arena_alloc(arena, sizeof(Type));
    if (type == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(type, 0, sizeof(Type));
    type->kind = kind;
    return type;
}

Type *ast_create_array_type(Arena *arena, Type *element_type)
{
    Type *type = arena_alloc(arena, sizeof(Type));
    if (type == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(type, 0, sizeof(Type));
    type->kind = TYPE_ARRAY;
    type->as.array.element_type = element_type;
    return type;
}

Type *ast_create_function_type(Arena *arena, Type *return_type, Type **param_types, int param_count)
{
    Type *type = arena_alloc(arena, sizeof(Type));
    if (type == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(type, 0, sizeof(Type));
    type->kind = TYPE_FUNCTION;

    type->as.function.return_type = ast_clone_type(arena, return_type);

    type->as.function.param_count = param_count;
    if (param_count > 0)
    {
        type->as.function.param_types = arena_alloc(arena, sizeof(Type *) * param_count);
        if (type->as.function.param_types == NULL)
        {
            DEBUG_ERROR("Out of memory");
            exit(1);
        }
        if (param_types == NULL && param_count > 0)
        {
            DEBUG_ERROR("Cannot create function type: param_types is NULL but param_count > 0");
            return NULL;
        }
        for (int i = 0; i < param_count; i++)
        {
            type->as.function.param_types[i] = ast_clone_type(arena, param_types[i]);
        }
    }
    else
    {
        type->as.function.param_types = NULL;
    }

    /* Initialize param_mem_quals to NULL - set separately if needed */
    type->as.function.param_mem_quals = NULL;

    return type;
}

int ast_type_equals(Type *a, Type *b)
{
    if (a == NULL && b == NULL)
        return 1;
    if (a == NULL || b == NULL)
        return 0;
    // TYPE_NIL is compatible with any type (used for empty arrays)
    if (a->kind == TYPE_NIL || b->kind == TYPE_NIL)
        return 1;
    // Allow int literals to be assigned to byte variables (implicit narrowing)
    if ((a->kind == TYPE_BYTE && b->kind == TYPE_INT) ||
        (a->kind == TYPE_INT && b->kind == TYPE_BYTE))
        return 1;
    if (a->kind != b->kind)
        return 0;

    switch (a->kind)
    {
    case TYPE_ARRAY:
        return ast_type_equals(a->as.array.element_type, b->as.array.element_type);
    case TYPE_FUNCTION:
        if (!ast_type_equals(a->as.function.return_type, b->as.function.return_type))
            return 0;
        if (a->as.function.param_count != b->as.function.param_count)
            return 0;
        for (int i = 0; i < a->as.function.param_count; i++)
        {
            if (!ast_type_equals(a->as.function.param_types[i], b->as.function.param_types[i]))
                return 0;
        }
        return 1;
    default:
        return 1;
    }
}

const char *ast_type_to_string(Arena *arena, Type *type)
{
    if (type == NULL)
    {
        return NULL;
    }

    switch (type->kind)
    {
    case TYPE_INT:
        return arena_strdup(arena, "int");
    case TYPE_LONG:
        return arena_strdup(arena, "long");
    case TYPE_DOUBLE:
        return arena_strdup(arena, "double");
    case TYPE_CHAR:
        return arena_strdup(arena, "char");
    case TYPE_STRING:
        return arena_strdup(arena, "string");
    case TYPE_BOOL:
        return arena_strdup(arena, "bool");
    case TYPE_BYTE:
        return arena_strdup(arena, "byte");
    case TYPE_VOID:
        return arena_strdup(arena, "void");
    case TYPE_NIL:
        return arena_strdup(arena, "nil");
    case TYPE_ANY:
        return arena_strdup(arena, "any");
    case TYPE_TEXT_FILE:
        return arena_strdup(arena, "TextFile");
    case TYPE_BINARY_FILE:
        return arena_strdup(arena, "BinaryFile");
    case TYPE_DATE:
        return arena_strdup(arena, "Date");
    case TYPE_TIME:
        return arena_strdup(arena, "Time");
    case TYPE_PROCESS:
        return arena_strdup(arena, "Process");

    case TYPE_ARRAY:
    {
        const char *elem_str = ast_type_to_string(arena, type->as.array.element_type);
        size_t len = strlen("array of ") + strlen(elem_str) + 1;
        char *str = arena_alloc(arena, len);
        if (str == NULL)
        {
            DEBUG_ERROR("Out of memory");
            exit(1);
        }
        snprintf(str, len, "array of %s", elem_str);
        return str;
    }

    case TYPE_FUNCTION:
    {
        size_t params_len = 0;
        for (int i = 0; i < type->as.function.param_count; i++)
        {
            const char *param_str = ast_type_to_string(arena, type->as.function.param_types[i]);
            params_len += strlen(param_str);
            if (i < type->as.function.param_count - 1)
            {
                params_len += 2;
            }
        }

        char *params_buf = arena_alloc(arena, params_len + 1);
        if (params_buf == NULL && type->as.function.param_count > 0)
        {
            DEBUG_ERROR("Out of memory");
            exit(1);
        }
        char *ptr = params_buf;
        for (int i = 0; i < type->as.function.param_count; i++)
        {
            const char *param_str = ast_type_to_string(arena, type->as.function.param_types[i]);
            strcpy(ptr, param_str);
            ptr += strlen(param_str);
            if (i < type->as.function.param_count - 1)
            {
                strcpy(ptr, ", ");
                ptr += 2;
            }
        }
        if (params_buf != NULL)
        {
            *ptr = '\0';
        }
        else
        {
            params_buf = arena_strdup(arena, "");
        }

        const char *ret_str = ast_type_to_string(arena, type->as.function.return_type);
        size_t total_len = strlen("function(") + strlen(params_buf) + strlen(") -> ") + strlen(ret_str) + 1;
        char *str = arena_alloc(arena, total_len);
        if (str == NULL)
        {
            DEBUG_ERROR("Out of memory");
            exit(1);
        }
        snprintf(str, total_len, "function(%s) -> %s", params_buf, ret_str);
        return str;
    }

    default:
        return arena_strdup(arena, "unknown");
    }
}
