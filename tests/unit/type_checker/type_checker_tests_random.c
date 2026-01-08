// tests/type_checker_tests_random.c
// Type checker tests for Random type - main entry point
// Tests are split into separate files for maintainability:
// - type_checker_tests_random_basic.c: Factory methods and value generation
// - type_checker_tests_random_many.c: Batch generation (*Many methods)
// - type_checker_tests_random_choice.c: choice() and weightedChoice() operations
// - type_checker_tests_random_collection.c: shuffle() and sample() operations

#include "../test_harness.h"
#include "type_checker_tests_random_basic.c"
#include "type_checker_tests_random_many.c"
#include "type_checker_tests_random_choice.c"
#include "type_checker_tests_random_collection.c"

void test_type_checker_random_main(void)
{
    TEST_SECTION("Type Checker Random");
    test_tc_random_basic_main();
    test_tc_random_many_main();
    test_tc_random_choice_main();
    test_tc_random_collection_main();
}
