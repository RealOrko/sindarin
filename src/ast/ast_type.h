#ifndef AST_TYPE_H
#define AST_TYPE_H

#include "ast.h"
#include "arena.h"

/* Type creation functions */
Type *ast_clone_type(Arena *arena, Type *type);
Type *ast_create_primitive_type(Arena *arena, TypeKind kind);
Type *ast_create_array_type(Arena *arena, Type *element_type);
Type *ast_create_function_type(Arena *arena, Type *return_type, Type **param_types, int param_count);

/* Type utilities */
int ast_type_equals(Type *a, Type *b);
const char *ast_type_to_string(Arena *arena, Type *type);

#endif /* AST_TYPE_H */
