#include "arena_tests.c"
#include "ast_tests.c"
#include "file_tests.c"
#include "lexer_tests.c"
#include "parser_tests.c"
#include "token_tests.c"

int main()
{

    // *** Debugging ***
    printf("Running tests with debug level: %d\n", DEBUG_LEVEL_ERROR);
    init_debug(DEBUG_LEVEL_ERROR);

    // *** Arena ***

    test_arena_init();
    test_arena_alloc_small();
    test_arena_alloc_large();
    test_arena_alloc_larger_than_double();
    test_arena_alloc_zero();
    test_arena_strdup();
    test_arena_strndup();
    test_arena_free();

    // *** AST ***

    test_ast_create_primitive_type();
    test_ast_create_array_type();
    test_ast_create_function_type();
    test_ast_clone_type();
    test_ast_type_equals();
    test_ast_type_to_string();
    test_ast_create_binary_expr();
    test_ast_create_unary_expr();
    test_ast_create_literal_expr();
    test_ast_create_variable_expr();
    test_ast_create_assign_expr();
    test_ast_create_call_expr();
    test_ast_create_array_expr();
    test_ast_create_array_access_expr();
    test_ast_create_increment_expr();
    test_ast_create_decrement_expr();
    test_ast_create_interpolated_expr();
    test_ast_create_member_expr();
    test_ast_create_comparison_expr();
    test_ast_create_expr_stmt();
    test_ast_create_var_decl_stmt();
    test_ast_create_function_stmt();
    test_ast_create_return_stmt();
    test_ast_create_block_stmt();
    test_ast_create_if_stmt();
    test_ast_create_while_stmt();
    test_ast_create_for_stmt();
    test_ast_create_import_stmt();
    test_ast_init_module();
    test_ast_module_add_statement();
    test_ast_clone_token();
    test_ast_print();

    // *** File ***

    test_file_read_null_arena();
    test_file_read_null_path();
    test_file_read_nonexistent_file();
    test_file_read_empty_file();
    test_file_read_small_file();
    test_file_read_large_file();
    test_file_read_seek_failure();
    test_file_read_read_failure();
    test_file_read_special_characters();

    // *** Lexer ***

    // *** Lexer ***

    test_lexer_array_empty();
    test_lexer_array_single_element();
    test_lexer_array_multi_element();
    test_lexer_inline_array_expression();
    test_lexer_array_assignment();
    test_lexer_array_method_calls();
    test_lexer_unmatched_brace_error();
    test_lexer_array_with_indentation();
    test_lexer_array_at_line_start();
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
    test_lexer_operators_single();
    test_lexer_operators_compound();
    test_lexer_operators_logical();
    test_lexer_brackets_parens_braces();
    test_lexer_punctuation();
    test_lexer_comments();
    test_lexer_indentation_basic();
    test_lexer_indentation_nested();
    test_lexer_indentation_error_inconsistent();
    test_lexer_multiple_newlines();
    test_lexer_line_with_only_comment();
    test_lexer_unexpected_character();
    test_lexer_mixed_tokens();
    test_lexer_tabs_as_indent();

    // *** Parser ***

    test_empty_program_parsing();
    test_var_decl_parsing();
    test_function_no_params_parsing();
    test_if_statement_parsing(); // NOT sure about this one, needs more debugging
    test_simple_program_parsing();
    test_while_loop_parsing();
    test_for_loop_parsing();
    test_interpolated_string_parsing();
    test_literal_types_parsing();
    test_recursive_function_parsing();
    test_full_program_parsing();

    // *** Token ***

    test_token_init_array_literal();
    test_token_init_int_literal();
    test_token_init_non_literal();
    test_token_init_zero_length();
    test_token_set_array_literal_null();
    test_token_set_array_literal_empty();
    test_token_set_array_literal_single();
    test_token_set_array_literal_multi();
    test_token_set_int_literal();
    test_token_set_long_literal();
    test_token_set_double_literal();
    test_token_set_char_literal();
    test_token_set_string_literal();
    test_token_set_interpol_string();
    test_token_set_bool_literal_true();
    test_token_set_bool_literal_false();
    test_token_type_to_string_array();
    test_token_type_to_string_all_literals();
    test_token_type_to_string_keywords();
    test_token_type_to_string_operators();
    test_token_type_to_string_special();
    test_token_type_to_string_invalid();
    test_token_print_array_integration();
    test_token_print_int_literal();
    test_token_print_long_literal();
    test_token_print_double_literal();
    test_token_print_char_literal();
    test_token_print_string_literal();
    test_token_print_interpol_string();
    test_token_print_bool_literal();
    test_token_print_non_literal();
    test_token_print_empty_lexeme();
    test_token_init_invalid_type();
    test_token_print_invalid_type();
    test_token_init_invalid_type();

    printf("All tests passed!\n");

    return 0;
}