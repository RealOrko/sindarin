#include "code_gen_util.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char *arena_vsprintf(Arena *arena, const char *fmt, va_list args)
{
    va_list args_copy;
    va_copy(args_copy, args);
    int size = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);
    if (size < 0)
    {
        exit(1);
    }
    char *buf = arena_alloc(arena, size + 1);
    if (buf == NULL)
    {
        exit(1);
    }
    vsnprintf(buf, size + 1, fmt, args);
    return buf;
}

char *arena_sprintf(Arena *arena, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char *result = arena_vsprintf(arena, fmt, args);
    va_end(args);
    return result;
}

char *escape_char_literal(Arena *arena, char ch)
{
    DEBUG_VERBOSE("Entering escape_char_literal");
    char buf[16];
    if (ch == '\'') {
        sprintf(buf, "'\\''");
    } else if (ch == '\\') {
        sprintf(buf, "'\\\\'");
    } else if (ch == '\n') {
        sprintf(buf, "'\\n'");
    } else if (ch == '\t') {
        sprintf(buf, "'\\t'");
    } else if (ch == '\r') {
        sprintf(buf, "'\\r'");
    } else if (ch == '\0') {
        sprintf(buf, "'\\0'");
    } else if ((unsigned char)ch >= 0x80 || ch < 0) {
        sprintf(buf, "'\\x%02x'", (unsigned char)ch);
    } else if (ch < ' ' || ch > '~') {
        sprintf(buf, "'\\x%02x'", (unsigned char)ch);
    } else {
        sprintf(buf, "'%c'", ch);
    }
    return arena_strdup(arena, buf);
}

char *escape_c_string(Arena *arena, const char *str)
{
    DEBUG_VERBOSE("Entering escape_c_string");
    if (str == NULL)
    {
        return arena_strdup(arena, "NULL");
    }
    size_t len = strlen(str);
    char *buf = arena_alloc(arena, len * 2 + 3);
    if (buf == NULL)
    {
        exit(1);
    }
    char *p = buf;
    *p++ = '"';
    for (const char *s = str; *s; s++)
    {
        switch (*s)
        {
        case '"':
            *p++ = '\\';
            *p++ = '"';
            break;
        case '\\':
            *p++ = '\\';
            *p++ = '\\';
            break;
        case '\n':
            *p++ = '\\';
            *p++ = 'n';
            break;
        case '\t':
            *p++ = '\\';
            *p++ = 't';
            break;
        case '\r':
            *p++ = '\\';
            *p++ = 'r';
            break;
        default:
            *p++ = *s;
            break;
        }
    }
    *p++ = '"';
    *p = '\0';
    return buf;
}

const char *get_c_type(Arena *arena, Type *type)
{
    DEBUG_VERBOSE("Entering get_c_type");

    if (type == NULL)
    {
        return arena_strdup(arena, "void");
    }

    switch (type->kind)
    {
    case TYPE_INT:
    case TYPE_LONG:
        return arena_strdup(arena, "long");
    case TYPE_DOUBLE:
        return arena_strdup(arena, "double");
    case TYPE_CHAR:
        return arena_strdup(arena, "char");
    case TYPE_STRING:
        return arena_strdup(arena, "char *");
    case TYPE_BOOL:
        return arena_strdup(arena, "bool");
    case TYPE_VOID:
        return arena_strdup(arena, "void");
    case TYPE_NIL:
        return arena_strdup(arena, "void *");
    case TYPE_ANY:
        return arena_strdup(arena, "void *");
    case TYPE_ARRAY:
    {
        // For bool arrays, use int* since runtime stores bools as int internally
        const char *element_c_type;
        if (type->as.array.element_type->kind == TYPE_BOOL)
        {
            element_c_type = "int";
        }
        else
        {
            element_c_type = get_c_type(arena, type->as.array.element_type);
        }
        if (type->as.array.element_type->kind == TYPE_ARRAY)
        {
            size_t len = strlen(element_c_type) + 10;
            char *result = arena_alloc(arena, len);
            snprintf(result, len, "%s (*)[]", element_c_type);
            return result;
        }
        else
        {
            size_t len = strlen(element_c_type) + 3;
            char *result = arena_alloc(arena, len);
            snprintf(result, len, "%s *", element_c_type);
            return result;
        }
    }
    case TYPE_FUNCTION:
    {
        const char *return_c_type = get_c_type(arena, type->as.function.return_type);
        size_t len = strlen(return_c_type) + 10;
        for (int i = 0; i < type->as.function.param_count; i++)
        {
            len += strlen(get_c_type(arena, type->as.function.param_types[i]));
            if (i < type->as.function.param_count - 1)
            {
                len += 2;
            }
        }
        char *result = arena_alloc(arena, len);
        char *current = result;
        current += sprintf(current, "%s (*)(", return_c_type);
        for (int i = 0; i < type->as.function.param_count; i++)
        {
            current += sprintf(current, "%s", get_c_type(arena, type->as.function.param_types[i]));
            if (i < type->as.function.param_count - 1)
            {
                current += sprintf(current, ", ");
            }
        }
        sprintf(current, ")");
        return result;
    }
    default:
        fprintf(stderr, "Error: Unknown type kind %d\n", type->kind);
        exit(1);
    }

    return NULL;
}

const char *get_rt_to_string_func(TypeKind kind)
{
    DEBUG_VERBOSE("Entering get_rt_to_string_func");
    switch (kind)
    {
    case TYPE_INT:
    case TYPE_LONG:
        return "rt_to_string_long";
    case TYPE_DOUBLE:
        return "rt_to_string_double";
    case TYPE_CHAR:
        return "rt_to_string_char";
    case TYPE_STRING:
        return "rt_to_string_string";
    case TYPE_BOOL:
        return "rt_to_string_bool";
    case TYPE_VOID:
        return "rt_to_string_void";
    case TYPE_NIL:
    case TYPE_ANY:
    case TYPE_ARRAY:
    case TYPE_FUNCTION:
        return "rt_to_string_pointer";
    default:
        exit(1);
    }
    return NULL;
}

const char *get_default_value(Type *type)
{
    DEBUG_VERBOSE("Entering get_default_value");
    if (type->kind == TYPE_STRING || type->kind == TYPE_ARRAY)
    {
        return "NULL";
    }
    else
    {
        return "0";
    }
}

char *get_var_name(Arena *arena, Token name)
{
    DEBUG_VERBOSE("Entering get_var_name");
    char *var_name = arena_alloc(arena, name.length + 1);
    if (var_name == NULL)
    {
        exit(1);
    }
    strncpy(var_name, name.start, name.length);
    var_name[name.length] = '\0';
    return var_name;
}

void indented_fprintf(CodeGen *gen, int indent, const char *fmt, ...)
{
    for (int i = 0; i < indent; i++)
    {
        fprintf(gen->output, "    ");
    }
    va_list args;
    va_start(args, fmt);
    vfprintf(gen->output, fmt, args);
    va_end(args);
}

char *code_gen_binary_op_str(TokenType op)
{
    DEBUG_VERBOSE("Entering code_gen_binary_op_str");
    switch (op)
    {
    case TOKEN_PLUS:
        return "add";
    case TOKEN_MINUS:
        return "sub";
    case TOKEN_STAR:
        return "mul";
    case TOKEN_SLASH:
        return "div";
    case TOKEN_MODULO:
        return "mod";
    case TOKEN_EQUAL_EQUAL:
        return "eq";
    case TOKEN_BANG_EQUAL:
        return "ne";
    case TOKEN_LESS:
        return "lt";
    case TOKEN_LESS_EQUAL:
        return "le";
    case TOKEN_GREATER:
        return "gt";
    case TOKEN_GREATER_EQUAL:
        return "ge";
    default:
        return NULL;
    }
}

char *code_gen_type_suffix(Type *type)
{
    DEBUG_VERBOSE("Entering code_gen_type_suffix");
    if (type == NULL)
    {
        return "void";
    }
    switch (type->kind)
    {
    case TYPE_INT:
    case TYPE_LONG:
        return "long";
    case TYPE_DOUBLE:
        return "double";
    case TYPE_STRING:
        return "string";
    case TYPE_BOOL:
        return "bool";
    default:
        return "void";
    }
}
