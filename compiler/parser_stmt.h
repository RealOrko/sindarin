#ifndef PARSER_STMT_H
#define PARSER_STMT_H

#include "parser.h"
#include "ast.h"
#include "ast_stmt.h"

/* Block/indentation helpers */
int is_at_function_boundary(Parser *parser);
Stmt *parser_indented_block(Parser *parser);

/* Statement parsing functions */
Stmt *parser_statement(Parser *parser);
Stmt *parser_declaration(Parser *parser);
Stmt *parser_var_declaration(Parser *parser);
Stmt *parser_function_declaration(Parser *parser);
Stmt *parser_return_statement(Parser *parser);
Stmt *parser_if_statement(Parser *parser);
Stmt *parser_while_statement(Parser *parser, bool is_shared);
Stmt *parser_for_statement(Parser *parser, bool is_shared);
Stmt *parser_block_statement(Parser *parser);
Stmt *parser_expression_statement(Parser *parser);
Stmt *parser_import_statement(Parser *parser);

#endif /* PARSER_STMT_H */
