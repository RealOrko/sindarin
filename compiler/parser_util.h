#ifndef PARSER_UTIL_H
#define PARSER_UTIL_H

#include "parser.h"
#include "ast.h"

/* Low-level parser utilities */
int parser_is_at_end(Parser *parser);
void skip_newlines(Parser *parser);
int skip_newlines_and_check_end(Parser *parser);

/* Error handling */
void parser_error(Parser *parser, const char *message);
void parser_error_at_current(Parser *parser, const char *message);
void parser_error_at(Parser *parser, Token *token, const char *message);

/* Token operations */
void parser_advance(Parser *parser);
void parser_consume(Parser *parser, TokenType type, const char *message);
int parser_check(Parser *parser, TokenType type);
int parser_match(Parser *parser, TokenType type);

/* Error recovery */
void synchronize(Parser *parser);

/* Type parsing */
Type *parser_type(Parser *parser);

#endif /* PARSER_UTIL_H */
