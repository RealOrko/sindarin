#include "standalone/arena_tests.c"
#include "ast/ast_tests.c"
#include "code_gen/code_gen_tests.c"
#include "standalone/file_tests.c"
#include "lexer/lexer_tests.c"
#include "optimizer/optimizer_tests.c"
#include "parser/parser_tests.c"
#include "runtime/runtime_arena_tests.c"
#include "runtime/runtime_arithmetic_tests.c"
#include "runtime/runtime_string_tests.c"
#include "runtime/runtime_byte_tests.c"
#include "runtime/runtime_array_tests.c"
#include "runtime/runtime_path_tests.c"
#include "runtime/runtime_thread_tests.c"
#include "runtime/runtime_process_tests.c"
#include "runtime/runtime_net_tests.c"
#include "runtime/runtime_random_tests_core.c"
#include "runtime/runtime_random_tests_basic.c"
#include "runtime/runtime_random_tests_static.c"
#include "runtime/runtime_random_tests_many.c"
#include "runtime/runtime_random_tests_choice.c"
#include "runtime/runtime_random_tests_collection.c"
#include "runtime/runtime_uuid_tests.c"
#include "runtime/runtime_sha1_tests.c"
#include "runtime/runtime_env_tests.c"
#include "standalone/symbol_table_tests_core.c"
#include "standalone/symbol_table_tests_thread.c"
#include "standalone/symbol_table_tests_namespace.c"
#include "standalone/token_tests.c"
#include "type_checker/type_checker_tests.c"

int main()
{

    // *** Debugging ***
    printf("Running tests with debug level: %d\n", DEBUG_LEVEL_ERROR);
    init_debug(DEBUG_LEVEL_ERROR);

    // *** Arena ***

    test_arena_main();

    // *** Runtime Arena ***

    test_rt_arena_main();

    // *** Runtime Arithmetic ***

    test_rt_arithmetic_main();

    // *** Runtime String ***

    test_rt_string_main();

    // *** Runtime Byte ***

    test_rt_byte_main();

    // *** Runtime Array ***

    test_rt_array_main();

    // *** Runtime Path ***

    test_rt_path_main();

    // *** Runtime Thread ***

    test_rt_thread_main();

    // *** Runtime Process ***

    test_rt_process_main();

    // *** Runtime Network ***

    test_rt_net_main();

    // *** Runtime Random ***

    test_rt_random_core_main();
    test_rt_random_basic_main();
    test_rt_random_static_main();
    test_rt_random_many_main();
    test_rt_random_choice_main();
    test_rt_random_collection_main();

    // *** Runtime UUID ***

    test_rt_uuid_main();

    // *** Runtime SHA-1 ***

    test_rt_sha1_main();

    // *** Runtime Environment ***

    test_rt_env_main();

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

    test_symbol_table_core_main();
    test_symbol_table_thread_main();
    test_symbol_table_namespace_main();
    
    // *** Token ***

    test_token_main();

    // *** Type Checker ***

    test_type_checker_main();

    // *** Optimizer ***

    run_optimizer_tests();

    // *** Complete ***

    printf("All tests passed!\n");

    return 0;
}