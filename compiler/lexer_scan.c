#include "lexer_scan.h"
#include "lexer_util.h"
#include "debug.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static char error_buffer[128];

TokenType lexer_check_keyword(Lexer *lexer, int start, int length, const char *rest, TokenType type)
{
    int lexeme_length = (int)(lexer->current - lexer->start);
    if (lexeme_length == start + length &&
        memcmp(lexer->start + start, rest, length) == 0)
    {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

TokenType lexer_identifier_type(Lexer *lexer)
{
    switch (lexer->start[0])
    {
    case 'b':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'o':
                return lexer_check_keyword(lexer, 2, 2, "ol", TOKEN_BOOL);
            case 'r':
                return lexer_check_keyword(lexer, 2, 3, "eak", TOKEN_BREAK);
            }
        }
        break;
    case 'c':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'h':
                return lexer_check_keyword(lexer, 2, 2, "ar", TOKEN_CHAR);
            case 'o':
                return lexer_check_keyword(lexer, 2, 6, "ntinue", TOKEN_CONTINUE);
            }
        }
        break;
    case 'd':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'o':
                return lexer_check_keyword(lexer, 2, 4, "uble", TOKEN_DOUBLE);
            }
        }
        break;
    case 'e':
        return lexer_check_keyword(lexer, 1, 3, "lse", TOKEN_ELSE);
    case 'f':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'a':
                return lexer_check_keyword(lexer, 2, 3, "lse", TOKEN_BOOL_LITERAL);
            case 'n':
                return lexer_check_keyword(lexer, 2, 0, "", TOKEN_FN);
            case 'o':
                return lexer_check_keyword(lexer, 2, 1, "r", TOKEN_FOR);
            }
        }
        break;
    case 'i':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'f':
                return lexer_check_keyword(lexer, 2, 0, "", TOKEN_IF);
            case 'm':
                return lexer_check_keyword(lexer, 2, 4, "port", TOKEN_IMPORT);
            case 'n':
                // Check for "in" (2 chars) vs "int" (3 chars)
                if (lexer->current - lexer->start == 2)
                {
                    return TOKEN_IN;
                }
                return lexer_check_keyword(lexer, 2, 1, "t", TOKEN_INT);
            }
        }
        break;
    case 'l':
        return lexer_check_keyword(lexer, 1, 3, "ong", TOKEN_LONG);
    case 'n':
        return lexer_check_keyword(lexer, 1, 2, "il", TOKEN_NIL);
    case 'r':
        return lexer_check_keyword(lexer, 1, 5, "eturn", TOKEN_RETURN);
    case 's':
        return lexer_check_keyword(lexer, 1, 2, "tr", TOKEN_STR);
    case 't':
        return lexer_check_keyword(lexer, 1, 3, "rue", TOKEN_BOOL_LITERAL);
    case 'v':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'a':
                return lexer_check_keyword(lexer, 2, 1, "r", TOKEN_VAR);
            case 'o':
                return lexer_check_keyword(lexer, 2, 2, "id", TOKEN_VOID);
            }
        }
        break;
    case 'w':
        return lexer_check_keyword(lexer, 1, 4, "hile", TOKEN_WHILE);
    }
    return TOKEN_IDENTIFIER;
}

Token lexer_scan_identifier(Lexer *lexer)
{
    while (isalnum(lexer_peek(lexer)) || lexer_peek(lexer) == '_')
    {
        lexer_advance(lexer);
    }
    TokenType type = lexer_identifier_type(lexer);
    if (type == TOKEN_BOOL_LITERAL)
    {
        Token token = lexer_make_token(lexer, type);
        if (memcmp(lexer->start, "true", 4) == 0)
        {
            token_set_bool_literal(&token, 1);
        }
        else
        {
            token_set_bool_literal(&token, 0);
        }
        return token;
    }
    return lexer_make_token(lexer, type);
}

Token lexer_scan_number(Lexer *lexer)
{
    while (isdigit(lexer_peek(lexer)))
    {
        lexer_advance(lexer);
    }
    if (lexer_peek(lexer) == '.' && isdigit(lexer_peek_next(lexer)))
    {
        lexer_advance(lexer);
        while (isdigit(lexer_peek(lexer)))
        {
            lexer_advance(lexer);
        }
        if (lexer_peek(lexer) == 'd')
        {
            lexer_advance(lexer);
            Token token = lexer_make_token(lexer, TOKEN_DOUBLE_LITERAL);
            char buffer[256];
            int length = (int)(lexer->current - lexer->start - 1);
            if (length >= (int)sizeof(buffer))
            {
                snprintf(error_buffer, sizeof(error_buffer), "Number literal too long");
                return lexer_error_token(lexer, error_buffer);
            }
            strncpy(buffer, lexer->start, length);
            buffer[length] = '\0';
            double value = strtod(buffer, NULL);
            token_set_double_literal(&token, value);
            return token;
        }
        Token token = lexer_make_token(lexer, TOKEN_DOUBLE_LITERAL);
        char buffer[256];
        int length = (int)(lexer->current - lexer->start);
        if (length >= (int)sizeof(buffer))
        {
            snprintf(error_buffer, sizeof(error_buffer), "Number literal too long");
            return lexer_error_token(lexer, error_buffer);
        }
        strncpy(buffer, lexer->start, length);
        buffer[length] = '\0';
        double value = strtod(buffer, NULL);
        token_set_double_literal(&token, value);
        return token;
    }
    if (lexer_peek(lexer) == 'l')
    {
        lexer_advance(lexer);
        Token token = lexer_make_token(lexer, TOKEN_LONG_LITERAL);
        char buffer[256];
        int length = (int)(lexer->current - lexer->start - 1);
        if (length >= (int)sizeof(buffer))
        {
            snprintf(error_buffer, sizeof(error_buffer), "Number literal too long");
            return lexer_error_token(lexer, error_buffer);
        }
        strncpy(buffer, lexer->start, length);
        buffer[length] = '\0';
        int64_t value = strtoll(buffer, NULL, 10);
        token_set_int_literal(&token, value);
        return token;
    }
    Token token = lexer_make_token(lexer, TOKEN_INT_LITERAL);
    char buffer[256];
    int length = (int)(lexer->current - lexer->start);
    if (length >= (int)sizeof(buffer))
    {
        snprintf(error_buffer, sizeof(error_buffer), "Number literal too long");
        return lexer_error_token(lexer, error_buffer);
    }
    strncpy(buffer, lexer->start, length);
    buffer[length] = '\0';
    int64_t value = strtoll(buffer, NULL, 10);
    token_set_int_literal(&token, value);
    return token;
}

Token lexer_scan_string(Lexer *lexer)
{
    int buffer_size = 256;
    char *buffer = arena_alloc(lexer->arena, buffer_size);
    if (buffer == NULL)
    {
        snprintf(error_buffer, sizeof(error_buffer), "Memory allocation failed");
        return lexer_error_token(lexer, error_buffer);
    }
    int buffer_index = 0;
    int brace_depth = 0;  // Track depth inside {} for interpolated strings
    int in_nested_string = 0;  // Track if we're inside a string within {}

    while (!lexer_is_at_end(lexer))
    {
        char c = lexer_peek(lexer);

        // Only stop on " if we're not inside {} interpolation
        if (c == '"' && brace_depth == 0 && !in_nested_string)
        {
            break;
        }

        if (c == '\n')
        {
            lexer->line++;
        }

        if (c == '\\')
        {
            buffer[buffer_index++] = c;
            lexer_advance(lexer);
            if (!lexer_is_at_end(lexer))
            {
                // Handle escape sequences
                char escaped = lexer_peek(lexer);
                if (brace_depth == 0)
                {
                    // Outside braces, process escape sequences
                    switch (escaped)
                    {
                    case '\\':
                        buffer[buffer_index - 1] = '\\';
                        break;
                    case 'n':
                        buffer[buffer_index - 1] = '\n';
                        break;
                    case 'r':
                        buffer[buffer_index - 1] = '\r';
                        break;
                    case 't':
                        buffer[buffer_index - 1] = '\t';
                        break;
                    case '"':
                        buffer[buffer_index - 1] = '"';
                        break;
                    default:
                        snprintf(error_buffer, sizeof(error_buffer), "Invalid escape sequence");
                        return lexer_error_token(lexer, error_buffer);
                    }
                }
                else
                {
                    // Inside braces, keep escape sequence as-is for sub-parser
                    buffer[buffer_index++] = escaped;
                }
                lexer_advance(lexer);
            }
        }
        else
        {
            // Track brace depth for interpolation
            if (c == '{' && !in_nested_string)
            {
                brace_depth++;
            }
            else if (c == '}' && !in_nested_string)
            {
                if (brace_depth > 0) brace_depth--;
            }
            // Track nested strings inside braces
            else if (c == '"' && brace_depth > 0)
            {
                in_nested_string = !in_nested_string;
            }

            buffer[buffer_index++] = c;
            lexer_advance(lexer);
        }

        if (buffer_index >= buffer_size - 1)
        {
            buffer_size *= 2;
            char *new_buffer = arena_alloc(lexer->arena, buffer_size);
            if (new_buffer == NULL)
            {
                snprintf(error_buffer, sizeof(error_buffer), "Memory allocation failed");
                return lexer_error_token(lexer, error_buffer);
            }
            memcpy(new_buffer, buffer, buffer_index);
            buffer = new_buffer;
        }
    }
    if (lexer_is_at_end(lexer))
    {
        snprintf(error_buffer, sizeof(error_buffer), "Unterminated string");
        return lexer_error_token(lexer, error_buffer);
    }
    lexer_advance(lexer);
    buffer[buffer_index] = '\0';
    Token token = lexer_make_token(lexer, TOKEN_STRING_LITERAL);
    char *str_copy = arena_strdup(lexer->arena, buffer);
    if (str_copy == NULL)
    {
        snprintf(error_buffer, sizeof(error_buffer), "Memory allocation failed");
        return lexer_error_token(lexer, error_buffer);
    }
    token_set_string_literal(&token, str_copy);
    return token;
}

Token lexer_scan_char(Lexer *lexer)
{
    char value = '\0';
    if (lexer_peek(lexer) == '\\')
    {
        lexer_advance(lexer);
        switch (lexer_peek(lexer))
        {
        case '\\':
            value = '\\';
            break;
        case 'n':
            value = '\n';
            break;
        case 'r':
            value = '\r';
            break;
        case 't':
            value = '\t';
            break;
        case '\'':
            value = '\'';
            break;
        default:
            snprintf(error_buffer, sizeof(error_buffer), "Invalid escape sequence");
            return lexer_error_token(lexer, error_buffer);
        }
    }
    else if (lexer_peek(lexer) == '\'')
    {
        snprintf(error_buffer, sizeof(error_buffer), "Empty character literal");
        return lexer_error_token(lexer, error_buffer);
    }
    else
    {
        value = lexer_peek(lexer);
    }
    lexer_advance(lexer);
    if (lexer_peek(lexer) != '\'')
    {
        snprintf(error_buffer, sizeof(error_buffer), "Unterminated character literal");
        return lexer_error_token(lexer, error_buffer);
    }
    lexer_advance(lexer);
    Token token = lexer_make_token(lexer, TOKEN_CHAR_LITERAL);
    token_set_char_literal(&token, value);
    return token;
}
