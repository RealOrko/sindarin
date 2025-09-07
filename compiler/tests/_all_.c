#include "arena_tests.c"
#include "ast_tests.c"
#include "file_tests.c"
#include "lexer_tests.c"
#include "parser_tests.c"
#include "symbol_table_tests.c"
#include "token_tests.c"
#include "type_checker_tests.c"

int main()
{

    // *** Debugging ***
    printf("Running tests with debug level: %d\n", DEBUG_LEVEL_VERBOSE);
    init_debug(DEBUG_LEVEL_VERBOSE);

    // *** Arena ***

    test_arena_main();

    // *** AST ***
    
    test_ast_main();

    // *** File ***

    test_file_main();

    // *** Lexer ***

    test_lexer_main();

    // *** Parser ***

    test_parser_main();

    // *** Symbol Table ***

    test_symbol_table_main();
    
    // *** Token ***

    test_token_main();

    // *** Type Checker ***

    test_type_checker_main();

    printf("All tests passed!\n");

    return 0;
}