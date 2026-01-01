#include "lexer/lexer_scan.h"
#include "lexer/lexer_util.h"
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
    case 'a':
        return lexer_check_keyword(lexer, 1, 1, "s", TOKEN_AS);
    case 'b':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'o':
                return lexer_check_keyword(lexer, 2, 2, "ol", TOKEN_BOOL);
            case 'r':
                return lexer_check_keyword(lexer, 2, 3, "eak", TOKEN_BREAK);
            case 'y':
                return lexer_check_keyword(lexer, 2, 2, "te", TOKEN_BYTE);
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
    case 'p':
        return lexer_check_keyword(lexer, 1, 6, "rivate", TOKEN_PRIVATE);
    case 'r':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'e':
                // Check for "ref" (3 chars) vs "return" (6 chars)
                if (lexer->current - lexer->start == 3)
                {
                    return lexer_check_keyword(lexer, 2, 1, "f", TOKEN_REF);
                }
                return lexer_check_keyword(lexer, 2, 4, "turn", TOKEN_RETURN);
            }
        }
        break;
    case 's':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 't':
                return lexer_check_keyword(lexer, 2, 1, "r", TOKEN_STR);
            case 'h':
                return lexer_check_keyword(lexer, 2, 4, "ared", TOKEN_SHARED);
            }
        }
        break;
    case 't':
        return lexer_check_keyword(lexer, 1, 3, "rue", TOKEN_BOOL_LITERAL);
    case 'v':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'a':
                // Check for "val" (3 chars) vs "var" (3 chars)
                if (lexer->current - lexer->start == 3)
                {
                    if (lexer->start[2] == 'l')
                    {
                        return TOKEN_VAL;
                    }
                    else if (lexer->start[2] == 'r')
                    {
                        return TOKEN_VAR;
                    }
                }
                break;
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
    int start_line = lexer->line;  // Save the line where the string starts for error reporting

    // Stack to track nested string states (simple strings vs interpolated strings)
    // Each entry: 0 = regular string, 1 = interpolated string ($"...")
    // We use a simple depth counter for regular strings and track interpolated ones
    int string_depth = 0;  // How many nested strings deep we are (within braces)
    int interpol_depth = 0;  // How many nested interpolated strings deep (within braces)

    while (!lexer_is_at_end(lexer))
    {
        char c = lexer_peek(lexer);

        // Only stop on " if we're not inside {} interpolation and not in nested string
        if (c == '"' && brace_depth == 0 && string_depth == 0)
        {
            break;
        }

        if (c == '\n')
        {
            lexer->line++;
        }

        if (c == '\\')
        {
            lexer_advance(lexer);
            if (!lexer_is_at_end(lexer))
            {
                // Handle escape sequences
                char escaped = lexer_peek(lexer);
                if (brace_depth == 0 && string_depth == 0)
                {
                    // Outside braces and nested strings, process escape sequences
                    switch (escaped)
                    {
                    case '\\':
                        buffer[buffer_index++] = '\\';
                        break;
                    case 'n':
                        buffer[buffer_index++] = '\n';
                        break;
                    case 'r':
                        buffer[buffer_index++] = '\r';
                        break;
                    case 't':
                        buffer[buffer_index++] = '\t';
                        break;
                    case '"':
                        buffer[buffer_index++] = '"';
                        break;
                    default:
                        snprintf(error_buffer, sizeof(error_buffer), "Invalid escape sequence");
                        return lexer_error_token(lexer, error_buffer);
                    }
                }
                else
                {
                    // Inside braces or nested strings, keep escape sequence as-is for sub-parser
                    buffer[buffer_index++] = '\\';
                    buffer[buffer_index++] = escaped;
                }
                lexer_advance(lexer);
            }
            else
            {
                // Backslash at end of string
                buffer[buffer_index++] = '\\';
            }
        }
        else if (c == '$' && brace_depth > 0 && string_depth == 0 && lexer_peek_next(lexer) == '"')
        {
            // Nested interpolated string: $"..." inside braces
            // Copy $" and track we're entering an interpolated string
            buffer[buffer_index++] = '$';
            lexer_advance(lexer);
            buffer[buffer_index++] = '"';
            lexer_advance(lexer);
            string_depth++;
            interpol_depth++;
        }
        else if (c == '"' && brace_depth > 0)
        {
            // Quote inside braces - could be start or end of nested string
            if (string_depth > 0)
            {
                // Closing a nested string
                buffer[buffer_index++] = '"';
                lexer_advance(lexer);
                string_depth--;
                if (interpol_depth > 0) interpol_depth--;
            }
            else
            {
                // Opening a regular nested string (not interpolated)
                buffer[buffer_index++] = '"';
                lexer_advance(lexer);
                string_depth++;
            }
        }
        else if (c == '{' && string_depth == 0)
        {
            // Only track braces when not inside a nested string literal
            brace_depth++;
            buffer[buffer_index++] = c;
            lexer_advance(lexer);
        }
        else if (c == '}' && string_depth == 0)
        {
            // Only track braces when not inside a nested string literal
            if (brace_depth > 0) brace_depth--;
            buffer[buffer_index++] = c;
            lexer_advance(lexer);
        }
        else
        {
            buffer[buffer_index++] = c;
            lexer_advance(lexer);
        }

        if (buffer_index >= buffer_size - 2)  // -2 for potential $" pair
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
        // Report error at the line where the string started
        int saved_line = lexer->line;
        lexer->line = start_line;
        snprintf(error_buffer, sizeof(error_buffer), "Unterminated string starting at line %d", start_line);
        Token error_tok = lexer_error_token(lexer, error_buffer);
        lexer->line = saved_line;
        return error_tok;
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
