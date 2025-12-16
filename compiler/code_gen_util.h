#ifndef CODE_GEN_UTIL_H
#define CODE_GEN_UTIL_H

#include "arena.h"
#include "ast.h"
#include "token.h"
#include "code_gen.h"
#include <stdarg.h>
#include <stdbool.h>

/* Arena-based string formatting */
char *arena_vsprintf(Arena *arena, const char *fmt, va_list args);
char *arena_sprintf(Arena *arena, const char *fmt, ...);

/* C code escaping */
char *escape_char_literal(Arena *arena, char ch);
char *escape_c_string(Arena *arena, const char *str);

/* Type conversion helpers */
const char *get_c_type(Arena *arena, Type *type);
const char *get_rt_to_string_func(TypeKind kind);
const char *get_default_value(Type *type);

/* Name helpers */
char *get_var_name(Arena *arena, Token name);

/* Code generation helpers */
void indented_fprintf(CodeGen *gen, int indent, const char *fmt, ...);
char *code_gen_binary_op_str(TokenType op);
char *code_gen_type_suffix(Type *type);

#endif /* CODE_GEN_UTIL_H */
