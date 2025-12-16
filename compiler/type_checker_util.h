#ifndef TYPE_CHECKER_UTIL_H
#define TYPE_CHECKER_UTIL_H

#include "ast.h"
#include "symbol_table.h"
#include <stdbool.h>

/* Error state management */
void type_checker_reset_error(void);
int type_checker_had_error(void);
void type_checker_set_error(void);

/* Error reporting */
const char *type_name(Type *type);
void type_error(Token *token, const char *msg);
void type_mismatch_error(Token *token, Type *expected, Type *actual, const char *context);

/* Type predicates */
bool is_numeric_type(Type *type);
bool is_comparison_operator(TokenType op);
bool is_arithmetic_operator(TokenType op);
bool is_printable_type(Type *type);

#endif /* TYPE_CHECKER_UTIL_H */
