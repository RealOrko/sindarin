// tests/lexer_tests_array.c
// Array-related lexer tests

void test_lexer_array_empty()
{
    DEBUG_INFO("Starting test_lexer_array_empty");
    printf("Testing lexer with empty array '{}'\n");

    const char *source = "{}";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_LEFT_BRACE);
    assert(t1.length == 1);
    assert(strcmp(t1.start, "{") == 0);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_RIGHT_BRACE);
    assert(t2.length == 1);
    assert(strcmp(t2.start, "}") == 0);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_array_empty");
}

void test_lexer_array_single_element()
{
    DEBUG_INFO("Starting test_lexer_array_single_element");
    printf("Testing lexer with single-element array '{1}'\n");

    const char *source = "{1}";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_LEFT_BRACE);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_INT_LITERAL);
    assert(t2.length == 1);
    assert(strcmp(t2.start, "1") == 0);
    assert(t2.literal.int_value == 1);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_RIGHT_BRACE);

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_array_single_element");
}

void test_lexer_array_multi_element()
{
    DEBUG_INFO("Starting test_lexer_array_multi_element");
    printf("Testing lexer with multi-element array '{1, 2, 3}' (with whitespace)\n");

    const char *source = "{1, 2, 3}";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_LEFT_BRACE);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_INT_LITERAL);
    assert(t2.literal.int_value == 1);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_COMMA);

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_INT_LITERAL);
    assert(t4.literal.int_value == 2);

    Token t5 = lexer_scan_token(&lexer);
    assert(t5.type == TOKEN_COMMA);

    Token t6 = lexer_scan_token(&lexer);
    assert(t6.type == TOKEN_INT_LITERAL);
    assert(t6.literal.int_value == 3);

    Token t7 = lexer_scan_token(&lexer);
    assert(t7.type == TOKEN_RIGHT_BRACE);

    Token t8 = lexer_scan_token(&lexer);
    assert(t8.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_array_multi_element");
}

void test_lexer_inline_array_expression()
{
    DEBUG_INFO("Starting test_lexer_inline_array_expression");
    printf("Testing lexer with inline array like 'arr.concat({1, 2})'\n");

    const char *source = "arr.concat({1, 2})";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_IDENTIFIER); // arr

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_DOT);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_IDENTIFIER); // concat

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_LEFT_PAREN);

    Token t5 = lexer_scan_token(&lexer);
    assert(t5.type == TOKEN_LEFT_BRACE);

    Token t6 = lexer_scan_token(&lexer);
    assert(t6.type == TOKEN_INT_LITERAL);
    assert(t6.literal.int_value == 1);

    Token t7 = lexer_scan_token(&lexer);
    assert(t7.type == TOKEN_COMMA);

    Token t8 = lexer_scan_token(&lexer);
    assert(t8.type == TOKEN_INT_LITERAL);
    assert(t8.literal.int_value == 2);

    Token t9 = lexer_scan_token(&lexer);
    assert(t9.type == TOKEN_RIGHT_BRACE);

    Token t10 = lexer_scan_token(&lexer);
    assert(t10.type == TOKEN_RIGHT_PAREN);

    Token t11 = lexer_scan_token(&lexer);
    assert(t11.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_inline_array_expression");
}

void test_lexer_array_assignment()
{
    DEBUG_INFO("Starting test_lexer_array_assignment");
    printf("Testing lexer with array assignment 'var arr: int[] = {1, 2}'\n");

    const char *source = "var arr: int[] = {1, 2}";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    // var (identifier)
    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_VAR);

    // arr (identifier)
    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_IDENTIFIER);

    // : (colon)
    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_COLON);

    // int (identifier/keyword)
    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_INT);

    // [ (left bracket)
    Token t5 = lexer_scan_token(&lexer);
    assert(t5.type == TOKEN_LEFT_BRACKET); // Assuming [] for type, but lexer needs [ ] too? Wait, add if missing.

    // ] (right bracket)
    Token t6 = lexer_scan_token(&lexer);
    assert(t6.type == TOKEN_RIGHT_BRACKET);

    // = (equal)
    Token t7 = lexer_scan_token(&lexer);
    assert(t7.type == TOKEN_EQUAL);

    // {1, 2}
    Token t8 = lexer_scan_token(&lexer);
    assert(t8.type == TOKEN_LEFT_BRACE);

    Token t9 = lexer_scan_token(&lexer);
    assert(t9.type == TOKEN_INT_LITERAL);
    assert(t9.literal.int_value == 1);

    Token t10 = lexer_scan_token(&lexer);
    assert(t10.type == TOKEN_COMMA);

    Token t11 = lexer_scan_token(&lexer);
    assert(t11.type == TOKEN_INT_LITERAL);
    assert(t11.literal.int_value == 2);

    Token t12 = lexer_scan_token(&lexer);
    assert(t12.type == TOKEN_RIGHT_BRACE);

    Token t13 = lexer_scan_token(&lexer);
    assert(t13.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_array_assignment");
}

void test_lexer_array_method_calls()
{
    DEBUG_INFO("Starting test_lexer_array_method_calls");
    printf("Testing lexer with array methods 'arr.push(1); arr.length; arr.pop()'\n");

    const char *source = "arr.push(1); arr.length; arr.pop()";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    // arr.push(1);
    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_IDENTIFIER); // arr

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_DOT);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_IDENTIFIER); // push

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_LEFT_PAREN);

    Token t5 = lexer_scan_token(&lexer);
    assert(t5.type == TOKEN_INT_LITERAL);
    assert(t5.literal.int_value == 1);

    Token t6 = lexer_scan_token(&lexer);
    assert(t6.type == TOKEN_RIGHT_PAREN);

    Token t7 = lexer_scan_token(&lexer);
    assert(t7.type == TOKEN_SEMICOLON);

    // arr.length
    Token t8 = lexer_scan_token(&lexer);
    assert(t8.type == TOKEN_IDENTIFIER); // arr

    Token t9 = lexer_scan_token(&lexer);
    assert(t9.type == TOKEN_DOT);

    Token t10 = lexer_scan_token(&lexer);
    assert(t10.type == TOKEN_IDENTIFIER); // length

    Token t11 = lexer_scan_token(&lexer);
    assert(t11.type == TOKEN_SEMICOLON);

    // arr.pop()
    Token t12 = lexer_scan_token(&lexer);
    assert(t12.type == TOKEN_IDENTIFIER); // arr

    Token t13 = lexer_scan_token(&lexer);
    assert(t13.type == TOKEN_DOT);

    Token t14 = lexer_scan_token(&lexer);
    assert(t14.type == TOKEN_IDENTIFIER); // pop

    Token t15 = lexer_scan_token(&lexer);
    assert(t15.type == TOKEN_LEFT_PAREN);

    Token t16 = lexer_scan_token(&lexer);
    assert(t16.type == TOKEN_RIGHT_PAREN);

    Token t17 = lexer_scan_token(&lexer);
    assert(t17.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_array_method_calls");
}

void test_lexer_unmatched_brace_error()
{
    DEBUG_INFO("Starting test_lexer_unmatched_brace_error");
    printf("Testing lexer error on unmatched '{' (now expecting EOF, as mismatch is parser concern)...\n");

    const char *source = "{1";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_LEFT_BRACE);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_INT_LITERAL);
    assert(t2.literal.int_value == 1);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_EOF); // Changed: EOF is correct; parser handles mismatch

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_unmatched_brace_error");
}

void test_lexer_array_with_indentation()
{
    DEBUG_INFO("Starting test_lexer_array_with_indentation");
    printf("Testing lexer with multi-line array under indentation (INDENT once per level)\n");

    const char *source = "  var arr = {\n    1,\n    2\n  }";
    Arena arena;
    arena_init(&arena, 1024 * 2); // Larger for multi-line
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    // t1: INDENT (2 spaces for block)
    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_INDENT);

    // Line 1: var arr = {
    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_VAR);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_IDENTIFIER); // arr

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_EQUAL);

    Token t5 = lexer_scan_token(&lexer);
    assert(t5.type == TOKEN_LEFT_BRACE);

    // t6: NEWLINE (end line 1)
    Token t6 = lexer_scan_token(&lexer);
    assert(t6.type == TOKEN_NEWLINE);

    // t7: INDENT (4 spaces for array elements block)
    Token t7 = lexer_scan_token(&lexer);
    assert(t7.type == TOKEN_INDENT);

    // Line 2: 1,
    Token t8 = lexer_scan_token(&lexer);
    assert(t8.type == TOKEN_INT_LITERAL);
    assert(t8.literal.int_value == 1);

    Token t9 = lexer_scan_token(&lexer);
    assert(t9.type == TOKEN_COMMA);

    // t10: NEWLINE (end line 2)
    Token t10 = lexer_scan_token(&lexer);
    assert(t10.type == TOKEN_NEWLINE);

    // Line 3: 2 (same indent level 4, no extra INDENT)
    Token t11 = lexer_scan_token(&lexer);
    assert(t11.type == TOKEN_INT_LITERAL); // Changed: Directly the number "2"
    assert(t11.literal.int_value == 2);

    // t12: NEWLINE (end line 3)
    Token t12 = lexer_scan_token(&lexer);
    assert(t12.type == TOKEN_NEWLINE);

    // t13: DEDENT (back to 2 spaces for line 4)
    Token t13 = lexer_scan_token(&lexer);
    assert(t13.type == TOKEN_DEDENT);

    // No second DEDENT yet (line 4 indent=2 == previous 2)

    // Line 4: }
    Token t14 = lexer_scan_token(&lexer);
    assert(t14.type == TOKEN_RIGHT_BRACE);

    // t15: EOF (end, with possible final DEDENT if needed, but since base, direct EOF)
    Token t15 = lexer_scan_token(&lexer);
    assert(t15.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_array_with_indentation");
}

void test_lexer_array_at_line_start()
{
    DEBUG_INFO("Starting test_lexer_array_at_line_start");
    printf("Testing lexer with array at line start (indent handling)\n");

    const char *source = "\n{1, 2}";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    // Newline (empty line, no indent)
    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_NEWLINE);

    // {1, 2} at line start, no indent
    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_LEFT_BRACE);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_INT_LITERAL);
    assert(t3.literal.int_value == 1);

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_COMMA);

    Token t5 = lexer_scan_token(&lexer);
    assert(t5.type == TOKEN_INT_LITERAL);
    assert(t5.literal.int_value == 2);

    Token t6 = lexer_scan_token(&lexer);
    assert(t6.type == TOKEN_RIGHT_BRACE);

    Token t7 = lexer_scan_token(&lexer);
    assert(t7.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_array_at_line_start");
}

// New comprehensive tests for lexer.c


void test_lexer_array_main()
{
    test_lexer_array_empty();
    test_lexer_array_single_element();
    test_lexer_array_multi_element();
    test_lexer_inline_array_expression();
    test_lexer_array_assignment();
    test_lexer_array_method_calls();
    test_lexer_unmatched_brace_error();
    test_lexer_array_with_indentation();
    test_lexer_array_at_line_start();
}
