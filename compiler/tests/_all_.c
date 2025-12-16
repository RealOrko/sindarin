#include "arena_tests.c"
#include "ast_tests.c"
#include "code_gen_tests.c"
#include "file_tests.c"
#include "lexer_tests.c"
#include "parser_tests.c"
#include "runtime_arena_tests.c"
#include "symbol_table_tests.c"
#include "token_tests.c"
#include "type_checker_tests.c"

int main()
{

    // *** Debugging ***
    printf("Running tests with debug level: %d\n", DEBUG_LEVEL_ERROR);
    init_debug(DEBUG_LEVEL_ERROR);

    // *** Arena ***

    test_arena_main();

    // *** Runtime Arena ***

    test_rt_arena_main();

    // *** AST ***
    
    test_ast_main();

    // *** Code Gen ***
    
    test_code_gen_main();

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

    // *** Complete ***

    printf("All tests passed!\n");

    return 0;
}