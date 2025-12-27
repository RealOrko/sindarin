// tests/lexer_tests_literal.c
// Literal-related lexer tests (keywords, numbers, strings, chars)

void test_lexer_empty_source()
{
    DEBUG_INFO("Starting test_lexer_empty_source");
    printf("Testing lexer with empty source\n");

    const char *source = "";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_empty_source");
}

void test_lexer_only_whitespace()
{
    DEBUG_INFO("Starting test_lexer_only_whitespace");
    printf("Testing lexer with only whitespace\n");

    const char *source = "   \t  \n";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_NEWLINE);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_only_whitespace");
}

void test_lexer_single_identifier()
{
    DEBUG_INFO("Starting test_lexer_single_identifier");
    printf("Testing lexer with single identifier 'var'\n");

    const char *source = "var";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_VAR);
    assert(t1.length == 3);
    assert(strcmp(t1.start, "var") == 0);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_single_identifier");
}

void test_lexer_keywords()
{
    DEBUG_INFO("Starting test_lexer_keywords");
    printf("Testing lexer with various keywords\n");

    const char *source = "fn if else for while return var int bool str char double long void nil import byte";
    Arena arena;
    arena_init(&arena, 1024 * 2);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_FN);
    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_IF);
    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_ELSE);
    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_FOR);
    Token t5 = lexer_scan_token(&lexer);
    assert(t5.type == TOKEN_WHILE);
    Token t6 = lexer_scan_token(&lexer);
    assert(t6.type == TOKEN_RETURN);
    Token t7 = lexer_scan_token(&lexer);
    assert(t7.type == TOKEN_VAR);
    Token t8 = lexer_scan_token(&lexer);
    assert(t8.type == TOKEN_INT);
    Token t9 = lexer_scan_token(&lexer);
    assert(t9.type == TOKEN_BOOL);
    Token t10 = lexer_scan_token(&lexer);
    assert(t10.type == TOKEN_STR);
    Token t11 = lexer_scan_token(&lexer);
    assert(t11.type == TOKEN_CHAR);
    Token t12 = lexer_scan_token(&lexer);
    assert(t12.type == TOKEN_DOUBLE);
    Token t13 = lexer_scan_token(&lexer);
    assert(t13.type == TOKEN_LONG);
    Token t14 = lexer_scan_token(&lexer);
    assert(t14.type == TOKEN_VOID);
    Token t15 = lexer_scan_token(&lexer);
    assert(t15.type == TOKEN_NIL);
    Token t16 = lexer_scan_token(&lexer);
    assert(t16.type == TOKEN_IMPORT);
    Token t17 = lexer_scan_token(&lexer);
    assert(t17.type == TOKEN_BYTE);

    Token t18 = lexer_scan_token(&lexer);
    assert(t18.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_keywords");
}

void test_lexer_bool_literals()
{
    DEBUG_INFO("Starting test_lexer_bool_literals");
    printf("Testing lexer with bool literals 'true false'\n");

    const char *source = "true false";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_BOOL_LITERAL);
    assert(t1.literal.bool_value == 1);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_BOOL_LITERAL);
    assert(t2.literal.bool_value == 0);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_bool_literals");
}

void test_lexer_int_literal()
{
    DEBUG_INFO("Starting test_lexer_int_literal");
    printf("Testing lexer with int literal '42'\n");

    const char *source = "42";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_INT_LITERAL);
    assert(t1.literal.int_value == 42);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_int_literal");
}

void test_lexer_long_literal()
{
    DEBUG_INFO("Starting test_lexer_long_literal");
    printf("Testing lexer with long literal '42l'\n");

    const char *source = "42l";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_LONG_LITERAL);
    assert(t1.literal.int_value == 42);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_long_literal");
}

void test_lexer_double_literal_decimal()
{
    DEBUG_INFO("Starting test_lexer_double_literal_decimal");
    printf("Testing lexer with double literal '3.14'\n");

    const char *source = "3.14";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_DOUBLE_LITERAL);
    assert(t1.literal.double_value == 3.14);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_double_literal_decimal");
}

void test_lexer_double_literal_with_d()
{
    DEBUG_INFO("Starting test_lexer_double_literal_with_d");
    printf("Testing lexer with double literal '3.14d'\n");

    const char *source = "3.14d";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_DOUBLE_LITERAL);
    assert(t1.literal.double_value == 3.14);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_double_literal_with_d");
}

void test_lexer_string_literal()
{
    DEBUG_INFO("Starting test_lexer_string_literal");
    printf("Testing lexer with string literal '\"hello\"'\n");

    const char *source = "\"hello\"";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_STRING_LITERAL);
    assert(t1.literal.string_value != NULL);
    assert(strcmp(t1.literal.string_value, "hello") == 0);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_string_literal");
}

void test_lexer_string_with_escapes()
{
    DEBUG_INFO("Starting test_lexer_string_with_escapes");
    printf("Testing lexer with string escapes '\\n \\t \"'\n");

    const char *source = "\"hello\\n\\t\\\"world\"";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_STRING_LITERAL);
    assert(t1.literal.string_value != NULL);
    assert(strcmp(t1.literal.string_value, "hello\n\t\"world") == 0);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_string_with_escapes");
}

void test_lexer_unterminated_string()
{
    DEBUG_INFO("Starting test_lexer_unterminated_string");
    printf("Testing lexer with unterminated string (should error)\n");

    const char *source = "\"unterminated";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_ERROR);
    assert(strstr(t1.start, "Unterminated string") != NULL);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_unterminated_string");
}

void test_lexer_interpolated_string()
{
    DEBUG_INFO("Starting test_lexer_interpolated_string");
    printf("Testing lexer with interpolated string '$\"hello ${var}\"' (basic recognition)\n");

    const char *source = "$\"hello\"";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_INTERPOL_STRING);
    assert(t1.literal.string_value != NULL);
    assert(strcmp(t1.literal.string_value, "hello") == 0); // Assuming escapes handled similarly

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_interpolated_string");
}

void test_lexer_char_literal()
{
    DEBUG_INFO("Starting test_lexer_char_literal");
    printf("Testing lexer with char literal \"'a'\"\n");

    const char *source = "'a'";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_CHAR_LITERAL);
    assert(t1.literal.char_value == 'a');

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_char_literal");
}

void test_lexer_char_escape()
{
    DEBUG_INFO("Starting test_lexer_char_escape");
    printf("Testing lexer with char escape \"'\\n'\"\n");

    const char *source = "'\\n'";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_CHAR_LITERAL);
    assert(t1.literal.char_value == '\n');

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_char_escape");
}

void test_lexer_unterminated_char()
{
    DEBUG_INFO("Starting test_lexer_unterminated_char");
    printf("Testing lexer with unterminated char (should error)\n");

    const char *source = "'unterminated";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_ERROR);
    assert(strstr(t1.start, "Unterminated character literal") != NULL);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_unterminated_char");
}


void test_lexer_literal_main()
{
    test_lexer_empty_source();
    test_lexer_only_whitespace();
    test_lexer_single_identifier();
    test_lexer_keywords();
    test_lexer_bool_literals();
    test_lexer_int_literal();
    test_lexer_long_literal();
    test_lexer_double_literal_decimal();
    test_lexer_double_literal_with_d();
    test_lexer_string_literal();
    test_lexer_string_with_escapes();
    test_lexer_unterminated_string();
    test_lexer_interpolated_string();
    test_lexer_char_literal();
    test_lexer_char_escape();
    test_lexer_unterminated_char();
}
