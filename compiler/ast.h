// ast.h
#ifndef AST_H
#define AST_H

#include "arena.h"
#include "token.h"
#include <stddef.h>
#include <stdbool.h>

typedef struct Expr Expr;
typedef struct Stmt Stmt;
typedef struct Type Type;

typedef enum
{
    TYPE_INT,
    TYPE_LONG,
    TYPE_DOUBLE,
    TYPE_CHAR,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_VOID,
    TYPE_ARRAY,
    TYPE_FUNCTION,
    TYPE_NIL,
    TYPE_ANY
} TypeKind;

/* Memory qualifier for variables and parameters */
typedef enum
{
    MEM_DEFAULT,    /* Default behavior (reference for arrays, value for primitives) */
    MEM_AS_VAL,     /* as val - explicit copy semantics */
    MEM_AS_REF      /* as ref - heap allocation for primitives */
} MemoryQualifier;

/* Block modifier for memory management */
typedef enum
{
    BLOCK_DEFAULT,  /* Normal block with own arena */
    BLOCK_SHARED,   /* shared block - uses parent's arena */
    BLOCK_PRIVATE   /* private block - isolated arena, only primitives escape */
} BlockModifier;

/* Function modifier for memory management */
typedef enum
{
    FUNC_DEFAULT,   /* Normal function with own arena */
    FUNC_SHARED,    /* shared function - uses caller's arena */
    FUNC_PRIVATE    /* private function - isolated arena, only primitives return */
} FunctionModifier;

struct Type
{
    TypeKind kind;

    union
    {
        struct
        {
            Type *element_type;
        } array;

        struct
        {
            Type *return_type;
            Type **param_types;
            int param_count;
        } function;
    } as;
};

typedef enum
{
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_LITERAL,
    EXPR_VARIABLE,
    EXPR_ASSIGN,
    EXPR_CALL,
    EXPR_ARRAY,
    EXPR_ARRAY_ACCESS,
    EXPR_INCREMENT,
    EXPR_DECREMENT,
    EXPR_INTERPOLATED,
    EXPR_MEMBER,
    EXPR_ARRAY_SLICE,
    EXPR_RANGE,
    EXPR_SPREAD
} ExprType;

typedef struct
{
    Expr *left;
    Expr *right;
    TokenType operator;
} BinaryExpr;

typedef struct
{
    Expr *operand;
    TokenType operator;
} UnaryExpr;

typedef struct
{
    LiteralValue value;
    Type *type;
    bool is_interpolated;
} LiteralExpr;

typedef struct
{
    Token name;
} VariableExpr;

typedef struct
{
    Token name;
    Expr *value;
} AssignExpr;

typedef struct
{
    Expr *callee;
    Expr **arguments;
    int arg_count;
} CallExpr;

typedef struct
{
    Expr **elements;
    int element_count;
} ArrayExpr;

typedef struct
{
    Expr *array;
    Expr *index;
} ArrayAccessExpr;

typedef struct
{
    Expr *array;
    Expr *start;  // NULL means from beginning
    Expr *end;    // NULL means to end
    Expr *step;   // NULL means step of 1
} ArraySliceExpr;

typedef struct
{
    Expr *start;  // Start of range (required)
    Expr *end;    // End of range (required)
} RangeExpr;

typedef struct
{
    Expr *array;  // The array being spread
} SpreadExpr;

typedef struct
{
    Expr **parts;
    int part_count;
} InterpolExpr;

typedef struct
{
    Expr *object;
    Token member_name;
} MemberExpr;

struct Expr
{
    ExprType type;
    Token *token;

    union
    {
        BinaryExpr binary;
        UnaryExpr unary;
        LiteralExpr literal;
        VariableExpr variable;
        AssignExpr assign;
        CallExpr call;
        ArrayExpr array;
        ArrayAccessExpr array_access;
        ArraySliceExpr array_slice;
        RangeExpr range;
        SpreadExpr spread;
        Expr *operand;
        MemberExpr member;
        InterpolExpr interpol;
    } as;

    Type *expr_type;
};

typedef enum
{
    STMT_EXPR,
    STMT_VAR_DECL,
    STMT_FUNCTION,
    STMT_RETURN,
    STMT_BLOCK,
    STMT_IF,
    STMT_WHILE,
    STMT_FOR,
    STMT_FOR_EACH,
    STMT_BREAK,
    STMT_CONTINUE,
    STMT_IMPORT
} StmtType;

typedef struct
{
    Expr *expression;
} ExprStmt;

typedef struct
{
    Token name;
    Type *type;
    Expr *initializer;
    MemoryQualifier mem_qualifier;  /* as val or as ref modifier */
} VarDeclStmt;

typedef struct
{
    Token name;
    Type *type;
    MemoryQualifier mem_qualifier;  /* as val modifier for copy semantics */
} Parameter;

typedef struct
{
    Token name;
    Parameter *params;
    int param_count;
    Type *return_type;
    Stmt **body;
    int body_count;
    FunctionModifier modifier;  /* shared or private modifier */
} FunctionStmt;

typedef struct
{
    Token keyword;
    Expr *value;
} ReturnStmt;

typedef struct
{
    Stmt **statements;
    int count;
    BlockModifier modifier;  /* shared or private block modifier */
} BlockStmt;

typedef struct
{
    Expr *condition;
    Stmt *then_branch;
    Stmt *else_branch;
} IfStmt;

typedef struct
{
    Expr *condition;
    Stmt *body;
    bool is_shared;  /* shared loop - no per-iteration arena */
} WhileStmt;

typedef struct
{
    Stmt *initializer;
    Expr *condition;
    Expr *increment;
    Stmt *body;
    bool is_shared;  /* shared loop - no per-iteration arena */
} ForStmt;

typedef struct
{
    Token var_name;
    Expr *iterable;
    Stmt *body;
    bool is_shared;  /* shared loop - no per-iteration arena */
} ForEachStmt;

typedef struct
{
    Token module_name;
} ImportStmt;

struct Stmt
{
    StmtType type;
    Token *token;

    union
    {
        ExprStmt expression;
        VarDeclStmt var_decl;
        FunctionStmt function;
        ReturnStmt return_stmt;
        BlockStmt block;
        IfStmt if_stmt;
        WhileStmt while_stmt;
        ForStmt for_stmt;
        ForEachStmt for_each_stmt;
        ImportStmt import;
    } as;
};

typedef struct
{
    Stmt **statements;
    int count;
    int capacity;
    const char *filename;
} Module;

void ast_print_stmt(Arena *arena, Stmt *stmt, int indent_level);
void ast_print_expr(Arena *arena, Expr *expr, int indent_level);

Token *ast_clone_token(Arena *arena, const Token *src);

Type *ast_clone_type(Arena *arena, Type *type);
Type *ast_create_primitive_type(Arena *arena, TypeKind kind);
Type *ast_create_array_type(Arena *arena, Type *element_type);
Type *ast_create_function_type(Arena *arena, Type *return_type, Type **param_types, int param_count);
int ast_type_equals(Type *a, Type *b);
const char *ast_type_to_string(Arena *arena, Type *type);

Expr *ast_create_binary_expr(Arena *arena, Expr *left, TokenType operator, Expr *right, const Token *loc_token);
Expr *ast_create_unary_expr(Arena *arena, TokenType operator, Expr *operand, const Token *loc_token);
Expr *ast_create_literal_expr(Arena *arena, LiteralValue value, Type *type, bool is_interpolated, const Token *loc_token);
Expr *ast_create_variable_expr(Arena *arena, Token name, const Token *loc_token);
Expr *ast_create_assign_expr(Arena *arena, Token name, Expr *value, const Token *loc_token);
Expr *ast_create_call_expr(Arena *arena, Expr *callee, Expr **arguments, int arg_count, const Token *loc_token);
Expr *ast_create_array_expr(Arena *arena, Expr **elements, int element_count, const Token *loc_token);
Expr *ast_create_array_access_expr(Arena *arena, Expr *array, Expr *index, const Token *loc_token);
Expr *ast_create_increment_expr(Arena *arena, Expr *operand, const Token *loc_token);
Expr *ast_create_decrement_expr(Arena *arena, Expr *operand, const Token *loc_token);
Expr *ast_create_interpolated_expr(Arena *arena, Expr **parts, int part_count, const Token *loc_token);
Expr *ast_create_member_expr(Arena *arena, Expr *object, Token member_name, const Token *loc_token);
Expr *ast_create_comparison_expr(Arena *arena, Expr *left, Expr *right, TokenType comparison_type, const Token *loc_token);
Expr *ast_create_array_slice_expr(Arena *arena, Expr *array, Expr *start, Expr *end, Expr *step, const Token *loc_token);
Expr *ast_create_range_expr(Arena *arena, Expr *start, Expr *end, const Token *loc_token);
Expr *ast_create_spread_expr(Arena *arena, Expr *array, const Token *loc_token);

Stmt *ast_create_expr_stmt(Arena *arena, Expr *expression, const Token *loc_token);
Stmt *ast_create_var_decl_stmt(Arena *arena, Token name, Type *type, Expr *initializer, const Token *loc_token);
Stmt *ast_create_function_stmt(Arena *arena, Token name, Parameter *params, int param_count,
                               Type *return_type, Stmt **body, int body_count, const Token *loc_token);
Stmt *ast_create_return_stmt(Arena *arena, Token keyword, Expr *value, const Token *loc_token);
Stmt *ast_create_block_stmt(Arena *arena, Stmt **statements, int count, const Token *loc_token);
Stmt *ast_create_if_stmt(Arena *arena, Expr *condition, Stmt *then_branch, Stmt *else_branch, const Token *loc_token);
Stmt *ast_create_while_stmt(Arena *arena, Expr *condition, Stmt *body, const Token *loc_token);
Stmt *ast_create_for_stmt(Arena *arena, Stmt *initializer, Expr *condition, Expr *increment, Stmt *body, const Token *loc_token);
Stmt *ast_create_for_each_stmt(Arena *arena, Token var_name, Expr *iterable, Stmt *body, const Token *loc_token);
Stmt *ast_create_import_stmt(Arena *arena, Token module_name, const Token *loc_token);

void ast_init_module(Arena *arena, Module *module, const char *filename);
void ast_module_add_statement(Arena *arena, Module *module, Stmt *stmt);

#endif