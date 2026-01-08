// tests/unit/runtime_random_tests_choice.c
// Tests for runtime random choice and weighted_choice functions

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "../../src/arena.h"
#include "../../src/runtime/runtime_random.h"
#include "../../src/runtime/runtime_arena.h"
#include "../../src/runtime/runtime_array.h"
#include "../../src/debug.h"
#include "../test_utils.h"
#include "../test_harness.h"

/* ============================================================================
 * Static Choice Tests
 * ============================================================================ */

static void test_rt_random_static_choice_long_basic(void)
{

    long arr[] = {10, 20, 30, 40, 50};
    long len = 5;

    /* Generate multiple choices and verify they are from the array */
    for (int i = 0; i < 100; i++) {
        long val = rt_random_static_choice_long(arr, len);
        int found = 0;
        for (long j = 0; j < len; j++) {
            if (val == arr[j]) {
                found = 1;
                break;
            }
        }
        TEST_ASSERT(found, "Choice should be from array");
    }

}

static void test_rt_random_static_choice_long_single_element(void)
{

    long arr[] = {42};
    for (int i = 0; i < 10; i++) {
        long val = rt_random_static_choice_long(arr, 1);
        TEST_ASSERT(val == 42, "Single element should always return that element");
    }

}

static void test_rt_random_static_choice_long_null_empty(void)
{

    long arr[] = {1, 2, 3};

    /* NULL array should return 0 */
    long val1 = rt_random_static_choice_long(NULL, 3);
    TEST_ASSERT(val1 == 0, "NULL array should return 0");

    /* Empty array (len <= 0) should return 0 */
    long val2 = rt_random_static_choice_long(arr, 0);
    TEST_ASSERT(val2 == 0, "Empty array should return 0");

    long val3 = rt_random_static_choice_long(arr, -1);
    TEST_ASSERT(val3 == 0, "Negative length should return 0");

}

static void test_rt_random_static_choice_long_distribution(void)
{

    long arr[] = {0, 1, 2, 3, 4};
    long len = 5;
    int counts[5] = {0};
    int iterations = 5000;

    for (int i = 0; i < iterations; i++) {
        long val = rt_random_static_choice_long(arr, len);
        TEST_ASSERT(val >= 0 && val < len, "Value should be valid index");
        counts[val]++;
    }

    /* Each element should be chosen roughly iterations/len times */
    int expected = iterations / len;
    int tolerance = expected / 2;  /* Allow 50% deviation */

    for (int i = 0; i < len; i++) {
        int deviation = abs(counts[i] - expected);
        TEST_ASSERT(deviation < tolerance, "Distribution should be roughly uniform");
    }

}

static void test_rt_random_static_choice_double_basic(void)
{

    double arr[] = {1.1, 2.2, 3.3, 4.4, 5.5};
    long len = 5;

    for (int i = 0; i < 100; i++) {
        double val = rt_random_static_choice_double(arr, len);
        int found = 0;
        for (long j = 0; j < len; j++) {
            if (val == arr[j]) {
                found = 1;
                break;
            }
        }
        TEST_ASSERT(found, "Choice should be from array");
    }

}

static void test_rt_random_static_choice_double_null_empty(void)
{

    double arr[] = {1.0, 2.0, 3.0};

    double val1 = rt_random_static_choice_double(NULL, 3);
    TEST_ASSERT(val1 == 0.0, "NULL array should return 0.0");

    double val2 = rt_random_static_choice_double(arr, 0);
    TEST_ASSERT(val2 == 0.0, "Empty array should return 0.0");

}

static void test_rt_random_static_choice_string_basic(void)
{

    char *arr[] = {"red", "green", "blue", "yellow"};
    long len = 4;

    for (int i = 0; i < 100; i++) {
        char *val = rt_random_static_choice_string(arr, len);
        int found = 0;
        for (long j = 0; j < len; j++) {
            if (val == arr[j]) {  /* Pointer comparison is fine here */
                found = 1;
                break;
            }
        }
        TEST_ASSERT(found, "Choice should be from array");
    }

}

static void test_rt_random_static_choice_string_null_empty(void)
{

    char *arr[] = {"a", "b", "c"};

    char *val1 = rt_random_static_choice_string(NULL, 3);
    TEST_ASSERT(val1 == NULL, "NULL array should return NULL");

    char *val2 = rt_random_static_choice_string(arr, 0);
    TEST_ASSERT(val2 == NULL, "Empty array should return NULL");

}

static void test_rt_random_static_choice_bool_basic(void)
{

    int arr[] = {0, 1, 0, 1, 1};
    long len = 5;

    for (int i = 0; i < 100; i++) {
        int val = rt_random_static_choice_bool(arr, len);
        TEST_ASSERT(val == 0 || val == 1, "Choice should be 0 or 1");
    }

}

static void test_rt_random_static_choice_bool_null_empty(void)
{

    int arr[] = {1, 0, 1};

    int val1 = rt_random_static_choice_bool(NULL, 3);
    TEST_ASSERT(val1 == 0, "NULL array should return 0");

    int val2 = rt_random_static_choice_bool(arr, 0);
    TEST_ASSERT(val2 == 0, "Empty array should return 0");

}

static void test_rt_random_static_choice_byte_basic(void)
{

    unsigned char arr[] = {0x10, 0x20, 0x30, 0x40, 0x50};
    long len = 5;

    for (int i = 0; i < 100; i++) {
        unsigned char val = rt_random_static_choice_byte(arr, len);
        int found = 0;
        for (long j = 0; j < len; j++) {
            if (val == arr[j]) {
                found = 1;
                break;
            }
        }
        TEST_ASSERT(found, "Choice should be from array");
    }

}

static void test_rt_random_static_choice_byte_null_empty(void)
{

    unsigned char arr[] = {0xAA, 0xBB, 0xCC};

    unsigned char val1 = rt_random_static_choice_byte(NULL, 3);
    TEST_ASSERT(val1 == 0, "NULL array should return 0");

    unsigned char val2 = rt_random_static_choice_byte(arr, 0);
    TEST_ASSERT(val2 == 0, "Empty array should return 0");

}

/* ============================================================================
 * Instance Choice Tests
 * ============================================================================ */

static void test_rt_random_choice_long_basic(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    long arr[] = {10, 20, 30, 40, 50};
    long len = 5;

    for (int i = 0; i < 100; i++) {
        long val = rt_random_choice_long(rng, arr, len);
        int found = 0;
        for (long j = 0; j < len; j++) {
            if (val == arr[j]) {
                found = 1;
                break;
            }
        }
        TEST_ASSERT(found, "Choice should be from array");
    }

    rt_arena_destroy(arena);
}

static void test_rt_random_choice_long_reproducibility(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng1 = rt_random_create_with_seed(arena, 42);
    RtRandom *rng2 = rt_random_create_with_seed(arena, 42);

    long arr[] = {100, 200, 300, 400, 500};
    long len = 5;

    for (int i = 0; i < 50; i++) {
        long v1 = rt_random_choice_long(rng1, arr, len);
        long v2 = rt_random_choice_long(rng2, arr, len);
        TEST_ASSERT(v1 == v2, "Same seed should produce same choices");
    }

    rt_arena_destroy(arena);
}

static void test_rt_random_choice_long_null_args(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    long arr[] = {1, 2, 3};

    long val1 = rt_random_choice_long(NULL, arr, 3);
    TEST_ASSERT(val1 == 0, "NULL rng should return 0");

    long val2 = rt_random_choice_long(rng, NULL, 3);
    TEST_ASSERT(val2 == 0, "NULL array should return 0");

    long val3 = rt_random_choice_long(rng, arr, 0);
    TEST_ASSERT(val3 == 0, "Empty array should return 0");

    rt_arena_destroy(arena);
}

static void test_rt_random_choice_long_distribution(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);

    long arr[] = {0, 1, 2, 3, 4};
    long len = 5;
    int counts[5] = {0};
    int iterations = 5000;

    for (int i = 0; i < iterations; i++) {
        long val = rt_random_choice_long(rng, arr, len);
        TEST_ASSERT(val >= 0 && val < len, "Value should be valid index");
        counts[val]++;
    }

    int expected = iterations / len;
    int tolerance = expected / 2;

    for (int i = 0; i < len; i++) {
        int deviation = abs(counts[i] - expected);
        TEST_ASSERT(deviation < tolerance, "Distribution should be roughly uniform");
    }

    rt_arena_destroy(arena);
}

static void test_rt_random_choice_double_basic(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    double arr[] = {1.1, 2.2, 3.3, 4.4, 5.5};
    long len = 5;

    for (int i = 0; i < 100; i++) {
        double val = rt_random_choice_double(rng, arr, len);
        int found = 0;
        for (long j = 0; j < len; j++) {
            if (val == arr[j]) {
                found = 1;
                break;
            }
        }
        TEST_ASSERT(found, "Choice should be from array");
    }

    rt_arena_destroy(arena);
}

static void test_rt_random_choice_double_null_args(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    double arr[] = {1.0, 2.0, 3.0};

    double val1 = rt_random_choice_double(NULL, arr, 3);
    TEST_ASSERT(val1 == 0.0, "NULL rng should return 0.0");

    double val2 = rt_random_choice_double(rng, NULL, 3);
    TEST_ASSERT(val2 == 0.0, "NULL array should return 0.0");

    rt_arena_destroy(arena);
}

static void test_rt_random_choice_string_basic(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    char *arr[] = {"red", "green", "blue", "yellow"};
    long len = 4;

    for (int i = 0; i < 100; i++) {
        char *val = rt_random_choice_string(rng, arr, len);
        int found = 0;
        for (long j = 0; j < len; j++) {
            if (val == arr[j]) {
                found = 1;
                break;
            }
        }
        TEST_ASSERT(found, "Choice should be from array");
    }

    rt_arena_destroy(arena);
}

static void test_rt_random_choice_string_null_args(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    char *arr[] = {"a", "b", "c"};

    char *val1 = rt_random_choice_string(NULL, arr, 3);
    TEST_ASSERT(val1 == NULL, "NULL rng should return NULL");

    char *val2 = rt_random_choice_string(rng, NULL, 3);
    TEST_ASSERT(val2 == NULL, "NULL array should return NULL");

    rt_arena_destroy(arena);
}

static void test_rt_random_choice_bool_basic(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    int arr[] = {0, 1, 0, 1, 1};
    long len = 5;

    for (int i = 0; i < 100; i++) {
        int val = rt_random_choice_bool(rng, arr, len);
        TEST_ASSERT(val == 0 || val == 1, "Choice should be 0 or 1");
    }

    rt_arena_destroy(arena);
}

static void test_rt_random_choice_bool_null_args(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    int arr[] = {1, 0, 1};

    int val1 = rt_random_choice_bool(NULL, arr, 3);
    TEST_ASSERT(val1 == 0, "NULL rng should return 0");

    int val2 = rt_random_choice_bool(rng, NULL, 3);
    TEST_ASSERT(val2 == 0, "NULL array should return 0");

    rt_arena_destroy(arena);
}

static void test_rt_random_choice_byte_basic(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    unsigned char arr[] = {0x10, 0x20, 0x30, 0x40, 0x50};
    long len = 5;

    for (int i = 0; i < 100; i++) {
        unsigned char val = rt_random_choice_byte(rng, arr, len);
        int found = 0;
        for (long j = 0; j < len; j++) {
            if (val == arr[j]) {
                found = 1;
                break;
            }
        }
        TEST_ASSERT(found, "Choice should be from array");
    }

    rt_arena_destroy(arena);
}

static void test_rt_random_choice_byte_null_args(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    unsigned char arr[] = {0xAA, 0xBB, 0xCC};

    unsigned char val1 = rt_random_choice_byte(NULL, arr, 3);
    TEST_ASSERT(val1 == 0, "NULL rng should return 0");

    unsigned char val2 = rt_random_choice_byte(rng, NULL, 3);
    TEST_ASSERT(val2 == 0, "NULL array should return 0");

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Statistical Distribution Tests for Choice Functions
 * ============================================================================ */

static void test_rt_random_static_choice_double_distribution(void)
{

    double arr[] = {0.0, 1.0, 2.0, 3.0};
    long len = 4;
    int counts[4] = {0};
    int iterations = 4000;

    for (int i = 0; i < iterations; i++) {
        double val = rt_random_static_choice_double(arr, len);
        int idx = (int)val;
        TEST_ASSERT(idx >= 0 && idx < len, "Value should be valid");
        counts[idx]++;
    }

    int expected = iterations / len;
    int tolerance = expected / 2;

    for (int i = 0; i < len; i++) {
        int deviation = abs(counts[i] - expected);
        TEST_ASSERT(deviation < tolerance, "Distribution should be roughly uniform");
    }

}

static void test_rt_random_static_choice_string_distribution(void)
{

    char *arr[] = {"a", "b", "c", "d"};
    long len = 4;
    int counts[4] = {0};
    int iterations = 4000;

    for (int i = 0; i < iterations; i++) {
        char *val = rt_random_static_choice_string(arr, len);
        for (long j = 0; j < len; j++) {
            if (val == arr[j]) {
                counts[j]++;
                break;
            }
        }
    }

    int expected = iterations / len;
    int tolerance = expected / 2;

    for (int i = 0; i < len; i++) {
        int deviation = abs(counts[i] - expected);
        TEST_ASSERT(deviation < tolerance, "Distribution should be roughly uniform");
    }

}

static void test_rt_random_static_choice_byte_distribution(void)
{

    unsigned char arr[] = {0x00, 0x55, 0xAA, 0xFF};
    long len = 4;
    int counts[4] = {0};
    int iterations = 4000;

    for (int i = 0; i < iterations; i++) {
        unsigned char val = rt_random_static_choice_byte(arr, len);
        for (long j = 0; j < len; j++) {
            if (val == arr[j]) {
                counts[j]++;
                break;
            }
        }
    }

    int expected = iterations / len;
    int tolerance = expected / 2;

    for (int i = 0; i < len; i++) {
        int deviation = abs(counts[i] - expected);
        TEST_ASSERT(deviation < tolerance, "Distribution should be roughly uniform");
    }

}

static void test_rt_random_choice_double_distribution(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);

    double arr[] = {0.0, 1.0, 2.0, 3.0};
    long len = 4;
    int counts[4] = {0};
    int iterations = 4000;

    for (int i = 0; i < iterations; i++) {
        double val = rt_random_choice_double(rng, arr, len);
        int idx = (int)val;
        TEST_ASSERT(idx >= 0 && idx < len, "Value should be valid");
        counts[idx]++;
    }

    int expected = iterations / len;
    int tolerance = expected / 2;

    for (int i = 0; i < len; i++) {
        int deviation = abs(counts[i] - expected);
        TEST_ASSERT(deviation < tolerance, "Distribution should be roughly uniform");
    }

    rt_arena_destroy(arena);
}

static void test_rt_random_choice_string_distribution(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);

    char *arr[] = {"a", "b", "c", "d"};
    long len = 4;
    int counts[4] = {0};
    int iterations = 4000;

    for (int i = 0; i < iterations; i++) {
        char *val = rt_random_choice_string(rng, arr, len);
        for (long j = 0; j < len; j++) {
            if (val == arr[j]) {
                counts[j]++;
                break;
            }
        }
    }

    int expected = iterations / len;
    int tolerance = expected / 2;

    for (int i = 0; i < len; i++) {
        int deviation = abs(counts[i] - expected);
        TEST_ASSERT(deviation < tolerance, "Distribution should be roughly uniform");
    }

    rt_arena_destroy(arena);
}

static void test_rt_random_choice_byte_distribution(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);

    unsigned char arr[] = {0x00, 0x55, 0xAA, 0xFF};
    long len = 4;
    int counts[4] = {0};
    int iterations = 4000;

    for (int i = 0; i < iterations; i++) {
        unsigned char val = rt_random_choice_byte(rng, arr, len);
        for (long j = 0; j < len; j++) {
            if (val == arr[j]) {
                counts[j]++;
                break;
            }
        }
    }

    int expected = iterations / len;
    int tolerance = expected / 2;

    for (int i = 0; i < len; i++) {
        int deviation = abs(counts[i] - expected);
        TEST_ASSERT(deviation < tolerance, "Distribution should be roughly uniform");
    }

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Weight Validation Helper Tests
 * ============================================================================
 * Tests for rt_random_validate_weights() function.
 * ============================================================================ */

static void test_rt_random_validate_weights_valid(void)
{

    /* Basic valid weights */
    double weights1[] = {1.0, 2.0, 3.0};
    TEST_ASSERT(rt_random_validate_weights(weights1, 3) == 1, "Valid weights should pass");

    /* Single element */
    double weights2[] = {0.5};
    TEST_ASSERT(rt_random_validate_weights(weights2, 1) == 1, "Single positive weight should pass");

    /* Very small positive weights */
    double weights3[] = {0.001, 0.002, 0.003};
    TEST_ASSERT(rt_random_validate_weights(weights3, 3) == 1, "Small positive weights should pass");

    /* Large weights */
    double weights4[] = {1000000.0, 2000000.0};
    TEST_ASSERT(rt_random_validate_weights(weights4, 2) == 1, "Large weights should pass");

}

static void test_rt_random_validate_weights_negative(void)
{

    /* Single negative weight */
    double weights1[] = {-1.0, 2.0, 3.0};
    TEST_ASSERT(rt_random_validate_weights(weights1, 3) == 0, "Negative weight should fail");

    /* Negative in middle */
    double weights2[] = {1.0, -0.5, 3.0};
    TEST_ASSERT(rt_random_validate_weights(weights2, 3) == 0, "Negative weight in middle should fail");

    /* Negative at end */
    double weights3[] = {1.0, 2.0, -3.0};
    TEST_ASSERT(rt_random_validate_weights(weights3, 3) == 0, "Negative weight at end should fail");

    /* All negative */
    double weights4[] = {-1.0, -2.0, -3.0};
    TEST_ASSERT(rt_random_validate_weights(weights4, 3) == 0, "All negative weights should fail");

}

static void test_rt_random_validate_weights_zero(void)
{

    /* Zero weight in array */
    double weights1[] = {0.0, 2.0, 3.0};
    TEST_ASSERT(rt_random_validate_weights(weights1, 3) == 0, "Zero weight should fail");

    /* Zero weight in middle */
    double weights2[] = {1.0, 0.0, 3.0};
    TEST_ASSERT(rt_random_validate_weights(weights2, 3) == 0, "Zero weight in middle should fail");

    /* Zero weight at end */
    double weights3[] = {1.0, 2.0, 0.0};
    TEST_ASSERT(rt_random_validate_weights(weights3, 3) == 0, "Zero weight at end should fail");

    /* All zeros */
    double weights4[] = {0.0, 0.0, 0.0};
    TEST_ASSERT(rt_random_validate_weights(weights4, 3) == 0, "All zero weights should fail");

}

static void test_rt_random_validate_weights_empty(void)
{

    double weights[] = {1.0, 2.0, 3.0};  /* dummy, won't be accessed */

    /* Zero length */
    TEST_ASSERT(rt_random_validate_weights(weights, 0) == 0, "Zero length should fail");

    /* Negative length */
    TEST_ASSERT(rt_random_validate_weights(weights, -1) == 0, "Negative length should fail");

}

static void test_rt_random_validate_weights_null(void)
{

    TEST_ASSERT(rt_random_validate_weights(NULL, 3) == 0, "NULL pointer should fail");
    TEST_ASSERT(rt_random_validate_weights(NULL, 0) == 0, "NULL with zero length should fail");

}

/* ============================================================================
 * Cumulative Distribution Helper Tests
 * ============================================================================
 * Tests for rt_random_build_cumulative() function.
 * ============================================================================ */

static void test_rt_random_build_cumulative_basic(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Test with typical probability weights */
    double weights[] = {0.7, 0.25, 0.05};
    double *cumulative = rt_random_build_cumulative(arena, weights, 3);

    TEST_ASSERT_NOT_NULL(cumulative, "Cumulative array should be created");

    /* Check cumulative distribution values */
    /* cumulative[0] = 0.7/1.0 = 0.7 */
    TEST_ASSERT(fabs(cumulative[0] - 0.7) < 0.0001, "First cumulative should be ~0.7");
    /* cumulative[1] = (0.7 + 0.25)/1.0 = 0.95 */
    TEST_ASSERT(fabs(cumulative[1] - 0.95) < 0.0001, "Second cumulative should be ~0.95");
    /* cumulative[2] = 1.0 (guaranteed) */
    TEST_ASSERT(cumulative[2] == 1.0, "Last cumulative should be exactly 1.0");


    rt_arena_destroy(arena);
}

static void test_rt_random_build_cumulative_normalization(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Weights that don't sum to 1.0 should be normalized */
    double weights[] = {2.0, 4.0, 4.0};  /* Sum = 10.0 */
    double *cumulative = rt_random_build_cumulative(arena, weights, 3);

    TEST_ASSERT_NOT_NULL(cumulative, "Cumulative array should be created");

    /* After normalization: [0.2, 0.4, 0.4] -> cumulative: [0.2, 0.6, 1.0] */
    TEST_ASSERT(fabs(cumulative[0] - 0.2) < 0.0001, "First cumulative should be ~0.2");
    TEST_ASSERT(fabs(cumulative[1] - 0.6) < 0.0001, "Second cumulative should be ~0.6");
    TEST_ASSERT(cumulative[2] == 1.0, "Last cumulative should be exactly 1.0");


    rt_arena_destroy(arena);
}

static void test_rt_random_build_cumulative_single_element(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Single element should produce cumulative [1.0] */
    double weights[] = {5.0};
    double *cumulative = rt_random_build_cumulative(arena, weights, 1);

    TEST_ASSERT_NOT_NULL(cumulative, "Cumulative array should be created");
    TEST_ASSERT(cumulative[0] == 1.0, "Single element cumulative should be 1.0");


    rt_arena_destroy(arena);
}

static void test_rt_random_build_cumulative_two_elements(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Two equal weights */
    double weights[] = {1.0, 1.0};
    double *cumulative = rt_random_build_cumulative(arena, weights, 2);

    TEST_ASSERT_NOT_NULL(cumulative, "Cumulative array should be created");
    TEST_ASSERT(fabs(cumulative[0] - 0.5) < 0.0001, "First cumulative should be ~0.5");
    TEST_ASSERT(cumulative[1] == 1.0, "Second cumulative should be exactly 1.0");


    rt_arena_destroy(arena);
}

static void test_rt_random_build_cumulative_null_arena(void)
{

    double weights[] = {1.0, 2.0, 3.0};
    double *cumulative = rt_random_build_cumulative(NULL, weights, 3);

    TEST_ASSERT(cumulative == NULL, "Should return NULL with NULL arena");

}

static void test_rt_random_build_cumulative_null_weights(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    double *cumulative = rt_random_build_cumulative(arena, NULL, 3);

    TEST_ASSERT(cumulative == NULL, "Should return NULL with NULL weights");


    rt_arena_destroy(arena);
}

static void test_rt_random_build_cumulative_empty_array(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    double weights[] = {1.0, 2.0, 3.0};  /* Dummy, won't be accessed */

    /* Zero length */
    double *cumulative1 = rt_random_build_cumulative(arena, weights, 0);
    TEST_ASSERT(cumulative1 == NULL, "Should return NULL with zero length");

    /* Negative length */
    double *cumulative2 = rt_random_build_cumulative(arena, weights, -1);
    TEST_ASSERT(cumulative2 == NULL, "Should return NULL with negative length");


    rt_arena_destroy(arena);
}

static void test_rt_random_build_cumulative_large_weights(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Large weights should still normalize correctly */
    double weights[] = {1000000.0, 2000000.0, 3000000.0, 4000000.0};  /* Sum = 10M */
    double *cumulative = rt_random_build_cumulative(arena, weights, 4);

    TEST_ASSERT_NOT_NULL(cumulative, "Cumulative array should be created");

    /* After normalization: [0.1, 0.2, 0.3, 0.4] -> cumulative: [0.1, 0.3, 0.6, 1.0] */
    TEST_ASSERT(fabs(cumulative[0] - 0.1) < 0.0001, "First cumulative should be ~0.1");
    TEST_ASSERT(fabs(cumulative[1] - 0.3) < 0.0001, "Second cumulative should be ~0.3");
    TEST_ASSERT(fabs(cumulative[2] - 0.6) < 0.0001, "Third cumulative should be ~0.6");
    TEST_ASSERT(cumulative[3] == 1.0, "Last cumulative should be exactly 1.0");

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Weighted Index Selection Helper Tests
 * ============================================================================
 * Tests for rt_random_select_weighted_index() function.
 * ============================================================================ */

static void test_rt_random_select_weighted_index_basic(void)
{

    /* Cumulative distribution: [0.7, 0.95, 1.0] */
    double cumulative[] = {0.7, 0.95, 1.0};
    long len = 3;

    /* Test values in first range [0, 0.7) -> index 0 */
    TEST_ASSERT(rt_random_select_weighted_index(0.0, cumulative, len) == 0, "0.0 should select index 0");
    TEST_ASSERT(rt_random_select_weighted_index(0.35, cumulative, len) == 0, "0.35 should select index 0");
    TEST_ASSERT(rt_random_select_weighted_index(0.69, cumulative, len) == 0, "0.69 should select index 0");

    /* Test values in second range [0.7, 0.95) -> index 1 */
    TEST_ASSERT(rt_random_select_weighted_index(0.7, cumulative, len) == 1, "0.7 should select index 1");
    TEST_ASSERT(rt_random_select_weighted_index(0.8, cumulative, len) == 1, "0.8 should select index 1");
    TEST_ASSERT(rt_random_select_weighted_index(0.94, cumulative, len) == 1, "0.94 should select index 1");

    /* Test values in third range [0.95, 1.0) -> index 2 */
    TEST_ASSERT(rt_random_select_weighted_index(0.95, cumulative, len) == 2, "0.95 should select index 2");
    TEST_ASSERT(rt_random_select_weighted_index(0.99, cumulative, len) == 2, "0.99 should select index 2");

}

static void test_rt_random_select_weighted_index_edge_zero(void)
{

    double cumulative[] = {0.25, 0.5, 0.75, 1.0};
    long len = 4;

    /* Value 0.0 should always select first element */
    TEST_ASSERT(rt_random_select_weighted_index(0.0, cumulative, len) == 0, "0.0 should select index 0");

    /* Negative value should also select first element (safety) */
    TEST_ASSERT(rt_random_select_weighted_index(-0.1, cumulative, len) == 0, "Negative should select index 0");

}

static void test_rt_random_select_weighted_index_edge_near_one(void)
{

    double cumulative[] = {0.25, 0.5, 0.75, 1.0};
    long len = 4;

    /* Values very close to 1.0 should select last element */
    TEST_ASSERT(rt_random_select_weighted_index(0.9999, cumulative, len) == 3, "0.9999 should select index 3");
    TEST_ASSERT(rt_random_select_weighted_index(0.999999, cumulative, len) == 3, "0.999999 should select index 3");

    /* Value exactly 1.0 should select last element (edge case) */
    TEST_ASSERT(rt_random_select_weighted_index(1.0, cumulative, len) == 3, "1.0 should select index 3");

    /* Values > 1.0 should select last element (safety) */
    TEST_ASSERT(rt_random_select_weighted_index(1.5, cumulative, len) == 3, ">1.0 should select index 3");

}

static void test_rt_random_select_weighted_index_single_element(void)
{

    double cumulative[] = {1.0};
    long len = 1;

    /* Any value should return index 0 */
    TEST_ASSERT(rt_random_select_weighted_index(0.0, cumulative, len) == 0, "0.0 should select index 0");
    TEST_ASSERT(rt_random_select_weighted_index(0.5, cumulative, len) == 0, "0.5 should select index 0");
    TEST_ASSERT(rt_random_select_weighted_index(0.99, cumulative, len) == 0, "0.99 should select index 0");

}

static void test_rt_random_select_weighted_index_two_elements(void)
{

    /* Equal weights -> [0.5, 1.0] */
    double cumulative[] = {0.5, 1.0};
    long len = 2;

    /* Values < 0.5 should select index 0 */
    TEST_ASSERT(rt_random_select_weighted_index(0.0, cumulative, len) == 0, "0.0 should select index 0");
    TEST_ASSERT(rt_random_select_weighted_index(0.49, cumulative, len) == 0, "0.49 should select index 0");

    /* Values >= 0.5 should select index 1 */
    TEST_ASSERT(rt_random_select_weighted_index(0.5, cumulative, len) == 1, "0.5 should select index 1");
    TEST_ASSERT(rt_random_select_weighted_index(0.99, cumulative, len) == 1, "0.99 should select index 1");

}

static void test_rt_random_select_weighted_index_boundary_values(void)
{

    /* Cumulative distribution: [0.25, 0.50, 0.75, 1.0] */
    double cumulative[] = {0.25, 0.50, 0.75, 1.0};
    long len = 4;

    /* Test at exact boundaries - value should go to next index */
    TEST_ASSERT(rt_random_select_weighted_index(0.25, cumulative, len) == 1, "0.25 (boundary) should select index 1");
    TEST_ASSERT(rt_random_select_weighted_index(0.50, cumulative, len) == 2, "0.50 (boundary) should select index 2");
    TEST_ASSERT(rt_random_select_weighted_index(0.75, cumulative, len) == 3, "0.75 (boundary) should select index 3");

    /* Test just below boundaries */
    TEST_ASSERT(rt_random_select_weighted_index(0.24, cumulative, len) == 0, "0.24 should select index 0");
    TEST_ASSERT(rt_random_select_weighted_index(0.49, cumulative, len) == 1, "0.49 should select index 1");
    TEST_ASSERT(rt_random_select_weighted_index(0.74, cumulative, len) == 2, "0.74 should select index 2");

}

static void test_rt_random_select_weighted_index_null(void)
{

    TEST_ASSERT(rt_random_select_weighted_index(0.5, NULL, 3) == 0, "NULL cumulative should return 0");

}

static void test_rt_random_select_weighted_index_invalid_len(void)
{

    double cumulative[] = {1.0};

    TEST_ASSERT(rt_random_select_weighted_index(0.5, cumulative, 0) == 0, "Zero length should return 0");
    TEST_ASSERT(rt_random_select_weighted_index(0.5, cumulative, -1) == 0, "Negative length should return 0");

}

static void test_rt_random_select_weighted_index_large_array(void)
{

    /* 10-element cumulative distribution [0.1, 0.2, 0.3, ..., 1.0] */
    double cumulative[] = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0};
    long len = 10;

    /* Test several positions */
    TEST_ASSERT(rt_random_select_weighted_index(0.05, cumulative, len) == 0, "0.05 should select index 0");
    TEST_ASSERT(rt_random_select_weighted_index(0.15, cumulative, len) == 1, "0.15 should select index 1");
    TEST_ASSERT(rt_random_select_weighted_index(0.45, cumulative, len) == 4, "0.45 should select index 4");
    TEST_ASSERT(rt_random_select_weighted_index(0.85, cumulative, len) == 8, "0.85 should select index 8");
    TEST_ASSERT(rt_random_select_weighted_index(0.95, cumulative, len) == 9, "0.95 should select index 9");

}

/* ============================================================================
 * Static Weighted Choice Tests
 * ============================================================================
 * Tests for rt_random_static_weighted_choice_long() function.
 * ============================================================================ */

static void test_rt_random_static_weighted_choice_long_basic(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Create array with values {10, 20, 30} */
    long data[] = {10, 20, 30};
    long *arr = rt_array_create_long(arena, 3, data);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Create weights {0.7, 0.25, 0.05} */
    double weight_data[] = {0.7, 0.25, 0.05};
    double *weights = rt_array_create_double(arena, 3, weight_data);
    TEST_ASSERT_NOT_NULL(weights, "Weights should be created");

    /* Call multiple times and verify result is always from array */
    int found_10 = 0, found_20 = 0, found_30 = 0;
    for (int i = 0; i < 100; i++) {
        long result = rt_random_static_weighted_choice_long(arr, weights);
        if (result == 10) found_10++;
        else if (result == 20) found_20++;
        else if (result == 30) found_30++;
        else {
            TEST_ASSERT(0, "Result should be from array");
        }
    }

    /* With weights {0.7, 0.25, 0.05}, 10 should appear most often */
    TEST_ASSERT(found_10 > found_30, "10 (weight 0.7) should appear more than 30 (weight 0.05)");


    rt_arena_destroy(arena);
}

static void test_rt_random_static_weighted_choice_long_single_element(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Single element array */
    long data[] = {42};
    long *arr = rt_array_create_long(arena, 1, data);
    double weight_data[] = {1.0};
    double *weights = rt_array_create_double(arena, 1, weight_data);

    /* Should always return the single element */
    for (int i = 0; i < 10; i++) {
        long result = rt_random_static_weighted_choice_long(arr, weights);
        TEST_ASSERT(result == 42, "Should always return single element");
    }


    rt_arena_destroy(arena);
}

static void test_rt_random_static_weighted_choice_long_null_arr(void)
{

    RtArena *arena = rt_arena_create(NULL);
    double weight_data[] = {1.0, 2.0};
    double *weights = rt_array_create_double(arena, 2, weight_data);

    long result = rt_random_static_weighted_choice_long(NULL, weights);
    TEST_ASSERT(result == 0, "Should return 0 for NULL array");


    rt_arena_destroy(arena);
}

static void test_rt_random_static_weighted_choice_long_null_weights(void)
{

    RtArena *arena = rt_arena_create(NULL);
    long data[] = {10, 20, 30};
    long *arr = rt_array_create_long(arena, 3, data);

    long result = rt_random_static_weighted_choice_long(arr, NULL);
    TEST_ASSERT(result == 0, "Should return 0 for NULL weights");


    rt_arena_destroy(arena);
}

static void test_rt_random_static_weighted_choice_long_invalid_weights(void)
{

    RtArena *arena = rt_arena_create(NULL);

    long data[] = {10, 20, 30};
    long *arr = rt_array_create_long(arena, 3, data);

    /* Negative weight */
    double neg_weight_data[] = {1.0, -1.0, 1.0};
    double *neg_weights = rt_array_create_double(arena, 3, neg_weight_data);
    long result1 = rt_random_static_weighted_choice_long(arr, neg_weights);
    TEST_ASSERT(result1 == 0, "Should return 0 for negative weights");

    /* Zero weight */
    double zero_weight_data[] = {1.0, 0.0, 1.0};
    double *zero_weights = rt_array_create_double(arena, 3, zero_weight_data);
    long result2 = rt_random_static_weighted_choice_long(arr, zero_weights);
    TEST_ASSERT(result2 == 0, "Should return 0 for zero weight");


    rt_arena_destroy(arena);
}

static void test_rt_random_static_weighted_choice_long_distribution(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Create array with values {1, 2, 3, 4} */
    long data[] = {1, 2, 3, 4};
    long *arr = rt_array_create_long(arena, 4, data);

    /* Equal weights -> should be roughly equal distribution */
    double weight_data[] = {1.0, 1.0, 1.0, 1.0};
    double *weights = rt_array_create_double(arena, 4, weight_data);

    int counts[4] = {0, 0, 0, 0};
    int iterations = 4000;

    for (int i = 0; i < iterations; i++) {
        long result = rt_random_static_weighted_choice_long(arr, weights);
        if (result >= 1 && result <= 4) {
            counts[result - 1]++;
        }
    }

    /* With equal weights, each should appear roughly 1/4 of the time */
    int expected = iterations / 4;
    int tolerance = expected / 2;  /* Allow 50% deviation */

    for (int i = 0; i < 4; i++) {
        int deviation = abs(counts[i] - expected);
        TEST_ASSERT(deviation < tolerance, "Distribution should be roughly uniform");
    }


    rt_arena_destroy(arena);
}

/* ============================================================================
 * Static Weighted Choice Double Tests
 * ============================================================================
 * Tests for rt_random_static_weighted_choice_double() function.
 * ============================================================================ */

static void test_rt_random_static_weighted_choice_double_basic(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Create array with values {1.5, 2.5, 3.5} */
    double data[] = {1.5, 2.5, 3.5};
    double *arr = rt_array_create_double(arena, 3, data);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Create weights {0.7, 0.25, 0.05} */
    double weight_data[] = {0.7, 0.25, 0.05};
    double *weights = rt_array_create_double(arena, 3, weight_data);
    TEST_ASSERT_NOT_NULL(weights, "Weights should be created");

    /* Call multiple times and verify result is always from array */
    int found_1_5 = 0, found_2_5 = 0, found_3_5 = 0;
    for (int i = 0; i < 100; i++) {
        double result = rt_random_static_weighted_choice_double(arr, weights);
        if (fabs(result - 1.5) < 0.001) found_1_5++;
        else if (fabs(result - 2.5) < 0.001) found_2_5++;
        else if (fabs(result - 3.5) < 0.001) found_3_5++;
        else {
            TEST_ASSERT(0, "Result should be from array");
        }
    }

    /* With weights {0.7, 0.25, 0.05}, 1.5 should appear most often */
    TEST_ASSERT(found_1_5 > found_3_5, "1.5 (weight 0.7) should appear more than 3.5 (weight 0.05)");


    rt_arena_destroy(arena);
}

static void test_rt_random_static_weighted_choice_double_single_element(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Single element array */
    double data[] = {3.14159};
    double *arr = rt_array_create_double(arena, 1, data);
    double weight_data[] = {1.0};
    double *weights = rt_array_create_double(arena, 1, weight_data);

    /* Should always return the single element */
    for (int i = 0; i < 10; i++) {
        double result = rt_random_static_weighted_choice_double(arr, weights);
        TEST_ASSERT(fabs(result - 3.14159) < 0.00001, "Should always return single element");
    }


    rt_arena_destroy(arena);
}

static void test_rt_random_static_weighted_choice_double_null_arr(void)
{

    RtArena *arena = rt_arena_create(NULL);
    double weight_data[] = {1.0, 2.0};
    double *weights = rt_array_create_double(arena, 2, weight_data);

    double result = rt_random_static_weighted_choice_double(NULL, weights);
    TEST_ASSERT(result == 0.0, "Should return 0.0 for NULL array");


    rt_arena_destroy(arena);
}

static void test_rt_random_static_weighted_choice_double_null_weights(void)
{

    RtArena *arena = rt_arena_create(NULL);
    double data[] = {1.0, 2.0, 3.0};
    double *arr = rt_array_create_double(arena, 3, data);

    double result = rt_random_static_weighted_choice_double(arr, NULL);
    TEST_ASSERT(result == 0.0, "Should return 0.0 for NULL weights");


    rt_arena_destroy(arena);
}

static void test_rt_random_static_weighted_choice_double_invalid_weights(void)
{

    RtArena *arena = rt_arena_create(NULL);

    double data[] = {1.0, 2.0, 3.0};
    double *arr = rt_array_create_double(arena, 3, data);

    /* Negative weight */
    double neg_weight_data[] = {1.0, -1.0, 1.0};
    double *neg_weights = rt_array_create_double(arena, 3, neg_weight_data);
    double result1 = rt_random_static_weighted_choice_double(arr, neg_weights);
    TEST_ASSERT(result1 == 0.0, "Should return 0.0 for negative weights");

    /* Zero weight */
    double zero_weight_data[] = {1.0, 0.0, 1.0};
    double *zero_weights = rt_array_create_double(arena, 3, zero_weight_data);
    double result2 = rt_random_static_weighted_choice_double(arr, zero_weights);
    TEST_ASSERT(result2 == 0.0, "Should return 0.0 for zero weight");


    rt_arena_destroy(arena);
}

static void test_rt_random_static_weighted_choice_double_distribution(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Create array with values {0.1, 0.2, 0.3, 0.4} */
    double data[] = {0.1, 0.2, 0.3, 0.4};
    double *arr = rt_array_create_double(arena, 4, data);

    /* Equal weights -> should be roughly equal distribution */
    double weight_data[] = {1.0, 1.0, 1.0, 1.0};
    double *weights = rt_array_create_double(arena, 4, weight_data);

    int counts[4] = {0, 0, 0, 0};
    int iterations = 4000;

    for (int i = 0; i < iterations; i++) {
        double result = rt_random_static_weighted_choice_double(arr, weights);
        if (fabs(result - 0.1) < 0.001) counts[0]++;
        else if (fabs(result - 0.2) < 0.001) counts[1]++;
        else if (fabs(result - 0.3) < 0.001) counts[2]++;
        else if (fabs(result - 0.4) < 0.001) counts[3]++;
    }

    /* With equal weights, each should appear roughly 1/4 of the time */
    int expected = iterations / 4;
    int tolerance = expected / 2;  /* Allow 50% deviation */

    for (int i = 0; i < 4; i++) {
        int deviation = abs(counts[i] - expected);
        TEST_ASSERT(deviation < tolerance, "Distribution should be roughly uniform");
    }


    rt_arena_destroy(arena);
}

/* ============================================================================
 * Static Weighted Choice String Tests
 * ============================================================================
 * Tests for rt_random_static_weighted_choice_string() function.
 * ============================================================================ */

static void test_rt_random_static_weighted_choice_string_basic(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Create array with string values */
    const char *data[] = {"apple", "banana", "cherry"};
    char **arr = rt_array_create_string(arena, 3, data);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Create weights {0.7, 0.25, 0.05} */
    double weight_data[] = {0.7, 0.25, 0.05};
    double *weights = rt_array_create_double(arena, 3, weight_data);
    TEST_ASSERT_NOT_NULL(weights, "Weights should be created");

    /* Call multiple times and verify result is always from array */
    int found_apple = 0, found_banana = 0, found_cherry = 0;
    for (int i = 0; i < 100; i++) {
        char *result = rt_random_static_weighted_choice_string(arr, weights);
        TEST_ASSERT_NOT_NULL(result, "Result should not be NULL");
        if (strcmp(result, "apple") == 0) found_apple++;
        else if (strcmp(result, "banana") == 0) found_banana++;
        else if (strcmp(result, "cherry") == 0) found_cherry++;
        else {
            TEST_ASSERT(0, "Result should be from array");
        }
    }

    /* With weights {0.7, 0.25, 0.05}, apple should appear most often */
    TEST_ASSERT(found_apple > found_cherry, "apple (weight 0.7) should appear more than cherry (weight 0.05)");


    rt_arena_destroy(arena);
}

static void test_rt_random_static_weighted_choice_string_single_element(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Single element array */
    const char *data[] = {"only_one"};
    char **arr = rt_array_create_string(arena, 1, data);
    double weight_data[] = {1.0};
    double *weights = rt_array_create_double(arena, 1, weight_data);

    /* Should always return the single element */
    for (int i = 0; i < 10; i++) {
        char *result = rt_random_static_weighted_choice_string(arr, weights);
        TEST_ASSERT_NOT_NULL(result, "Result should not be NULL");
        TEST_ASSERT(strcmp(result, "only_one") == 0, "Should always return single element");
    }


    rt_arena_destroy(arena);
}

static void test_rt_random_static_weighted_choice_string_null_arr(void)
{

    RtArena *arena = rt_arena_create(NULL);
    double weight_data[] = {1.0, 2.0};
    double *weights = rt_array_create_double(arena, 2, weight_data);

    char *result = rt_random_static_weighted_choice_string(NULL, weights);
    TEST_ASSERT(result == NULL, "Should return NULL for NULL array");


    rt_arena_destroy(arena);
}

static void test_rt_random_static_weighted_choice_string_null_weights(void)
{

    RtArena *arena = rt_arena_create(NULL);
    const char *data[] = {"a", "b", "c"};
    char **arr = rt_array_create_string(arena, 3, data);

    char *result = rt_random_static_weighted_choice_string(arr, NULL);
    TEST_ASSERT(result == NULL, "Should return NULL for NULL weights");


    rt_arena_destroy(arena);
}

static void test_rt_random_static_weighted_choice_string_invalid_weights(void)
{

    RtArena *arena = rt_arena_create(NULL);

    const char *data[] = {"a", "b", "c"};
    char **arr = rt_array_create_string(arena, 3, data);

    /* Negative weight */
    double neg_weight_data[] = {1.0, -1.0, 1.0};
    double *neg_weights = rt_array_create_double(arena, 3, neg_weight_data);
    char *result1 = rt_random_static_weighted_choice_string(arr, neg_weights);
    TEST_ASSERT(result1 == NULL, "Should return NULL for negative weights");

    /* Zero weight */
    double zero_weight_data[] = {1.0, 0.0, 1.0};
    double *zero_weights = rt_array_create_double(arena, 3, zero_weight_data);
    char *result2 = rt_random_static_weighted_choice_string(arr, zero_weights);
    TEST_ASSERT(result2 == NULL, "Should return NULL for zero weight");


    rt_arena_destroy(arena);
}

static void test_rt_random_static_weighted_choice_string_distribution(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Create array with string values */
    const char *data[] = {"one", "two", "three", "four"};
    char **arr = rt_array_create_string(arena, 4, data);

    /* Equal weights -> should be roughly equal distribution */
    double weight_data[] = {1.0, 1.0, 1.0, 1.0};
    double *weights = rt_array_create_double(arena, 4, weight_data);

    int counts[4] = {0, 0, 0, 0};
    int iterations = 4000;

    for (int i = 0; i < iterations; i++) {
        char *result = rt_random_static_weighted_choice_string(arr, weights);
        TEST_ASSERT_NOT_NULL(result, "Result should not be NULL");
        if (strcmp(result, "one") == 0) counts[0]++;
        else if (strcmp(result, "two") == 0) counts[1]++;
        else if (strcmp(result, "three") == 0) counts[2]++;
        else if (strcmp(result, "four") == 0) counts[3]++;
    }

    /* With equal weights, each should appear roughly 1/4 of the time */
    int expected = iterations / 4;
    int tolerance = expected / 2;  /* Allow 50% deviation */

    for (int i = 0; i < 4; i++) {
        int deviation = abs(counts[i] - expected);
        TEST_ASSERT(deviation < tolerance, "Distribution should be roughly uniform");
    }


    rt_arena_destroy(arena);
}

/* ============================================================================
 * Instance Weighted Choice Long Tests
 * ============================================================================
 * Tests for rt_random_weighted_choice_long() function.
 * ============================================================================ */

static void test_rt_random_weighted_choice_long_basic(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Create RNG with known seed */
    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    /* Create array with values {10, 20, 30} */
    long data[] = {10, 20, 30};
    long *arr = rt_array_create_long(arena, 3, data);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Create weights {0.7, 0.25, 0.05} */
    double weight_data[] = {0.7, 0.25, 0.05};
    double *weights = rt_array_create_double(arena, 3, weight_data);
    TEST_ASSERT_NOT_NULL(weights, "Weights should be created");

    /* Call multiple times and verify result is always from array */
    int found_10 = 0, found_20 = 0, found_30 = 0;
    for (int i = 0; i < 100; i++) {
        long result = rt_random_weighted_choice_long(rng, arr, weights);
        if (result == 10) found_10++;
        else if (result == 20) found_20++;
        else if (result == 30) found_30++;
        else {
            TEST_ASSERT(0, "Result should be from array");
        }
    }

    /* With weights {0.7, 0.25, 0.05}, 10 should appear most often */
    TEST_ASSERT(found_10 > found_30, "10 (weight 0.7) should appear more than 30 (weight 0.05)");


    rt_arena_destroy(arena);
}

static void test_rt_random_weighted_choice_long_single_element(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 42);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    /* Single element array */
    long data[] = {42};
    long *arr = rt_array_create_long(arena, 1, data);
    double weight_data[] = {1.0};
    double *weights = rt_array_create_double(arena, 1, weight_data);

    /* Should always return the single element */
    for (int i = 0; i < 10; i++) {
        long result = rt_random_weighted_choice_long(rng, arr, weights);
        TEST_ASSERT(result == 42, "Should always return single element");
    }


    rt_arena_destroy(arena);
}

static void test_rt_random_weighted_choice_long_null_rng(void)
{

    RtArena *arena = rt_arena_create(NULL);
    long data[] = {10, 20, 30};
    long *arr = rt_array_create_long(arena, 3, data);
    double weight_data[] = {1.0, 2.0, 3.0};
    double *weights = rt_array_create_double(arena, 3, weight_data);

    long result = rt_random_weighted_choice_long(NULL, arr, weights);
    TEST_ASSERT(result == 0, "Should return 0 for NULL rng");


    rt_arena_destroy(arena);
}

static void test_rt_random_weighted_choice_long_null_arr(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 42);
    double weight_data[] = {1.0, 2.0};
    double *weights = rt_array_create_double(arena, 2, weight_data);

    long result = rt_random_weighted_choice_long(rng, NULL, weights);
    TEST_ASSERT(result == 0, "Should return 0 for NULL array");


    rt_arena_destroy(arena);
}

static void test_rt_random_weighted_choice_long_null_weights(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 42);
    long data[] = {10, 20, 30};
    long *arr = rt_array_create_long(arena, 3, data);

    long result = rt_random_weighted_choice_long(rng, arr, NULL);
    TEST_ASSERT(result == 0, "Should return 0 for NULL weights");


    rt_arena_destroy(arena);
}

static void test_rt_random_weighted_choice_long_invalid_weights(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 42);

    long data[] = {10, 20, 30};
    long *arr = rt_array_create_long(arena, 3, data);

    /* Negative weight */
    double neg_weight_data[] = {1.0, -1.0, 1.0};
    double *neg_weights = rt_array_create_double(arena, 3, neg_weight_data);
    long result1 = rt_random_weighted_choice_long(rng, arr, neg_weights);
    TEST_ASSERT(result1 == 0, "Should return 0 for negative weights");

    /* Zero weight */
    double zero_weight_data[] = {1.0, 0.0, 1.0};
    double *zero_weights = rt_array_create_double(arena, 3, zero_weight_data);
    long result2 = rt_random_weighted_choice_long(rng, arr, zero_weights);
    TEST_ASSERT(result2 == 0, "Should return 0 for zero weight");


    rt_arena_destroy(arena);
}

static void test_rt_random_weighted_choice_long_reproducible(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    long data[] = {10, 20, 30, 40, 50};
    long *arr = rt_array_create_long(arena, 5, data);
    double weight_data[] = {1.0, 2.0, 3.0, 2.0, 1.0};
    double *weights = rt_array_create_double(arena, 5, weight_data);

    /* Create two RNGs with the same seed */
    RtRandom *rng1 = rt_random_create_with_seed(arena, 99999);
    RtRandom *rng2 = rt_random_create_with_seed(arena, 99999);

    /* They should produce the same sequence */
    int matches = 0;
    for (int i = 0; i < 20; i++) {
        long r1 = rt_random_weighted_choice_long(rng1, arr, weights);
        long r2 = rt_random_weighted_choice_long(rng2, arr, weights);
        if (r1 == r2) matches++;
    }

    TEST_ASSERT(matches == 20, "Same seed should produce same sequence");


    rt_arena_destroy(arena);
}

static void test_rt_random_weighted_choice_long_distribution(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 54321);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    /* Create array with values {1, 2, 3, 4} */
    long data[] = {1, 2, 3, 4};
    long *arr = rt_array_create_long(arena, 4, data);

    /* Equal weights -> should be roughly equal distribution */
    double weight_data[] = {1.0, 1.0, 1.0, 1.0};
    double *weights = rt_array_create_double(arena, 4, weight_data);

    int counts[4] = {0, 0, 0, 0};
    int iterations = 4000;

    for (int i = 0; i < iterations; i++) {
        long result = rt_random_weighted_choice_long(rng, arr, weights);
        if (result >= 1 && result <= 4) {
            counts[result - 1]++;
        }
    }

    /* With equal weights, each should appear roughly 1/4 of the time */
    int expected = iterations / 4;
    int tolerance = expected / 2;  /* Allow 50% deviation */

    for (int i = 0; i < 4; i++) {
        int deviation = abs(counts[i] - expected);
        TEST_ASSERT(deviation < tolerance, "Distribution should be roughly uniform");
    }


    rt_arena_destroy(arena);
}

/* ============================================================================
 * Instance Weighted Choice Double Tests
 * ============================================================================ */

static void test_rt_random_weighted_choice_double_basic(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Create RNG with known seed */
    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    /* Create array with values {1.5, 2.5, 3.5} */
    double data[] = {1.5, 2.5, 3.5};
    double *arr = rt_array_create_double(arena, 3, data);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Create weights {0.7, 0.25, 0.05} */
    double weight_data[] = {0.7, 0.25, 0.05};
    double *weights = rt_array_create_double(arena, 3, weight_data);
    TEST_ASSERT_NOT_NULL(weights, "Weights should be created");

    /* Call multiple times and verify result is always from array */
    int found_1_5 = 0, found_2_5 = 0, found_3_5 = 0;
    for (int i = 0; i < 100; i++) {
        double result = rt_random_weighted_choice_double(rng, arr, weights);
        if (result == 1.5) found_1_5++;
        else if (result == 2.5) found_2_5++;
        else if (result == 3.5) found_3_5++;
        else {
            TEST_ASSERT(0, "Result should be from array");
        }
    }

    /* With weights {0.7, 0.25, 0.05}, 1.5 should appear most often */
    TEST_ASSERT(found_1_5 > found_3_5, "1.5 (weight 0.7) should appear more than 3.5 (weight 0.05)");


    rt_arena_destroy(arena);
}

static void test_rt_random_weighted_choice_double_single_element(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 42);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    /* Single element array */
    double data[] = {3.14159};
    double *arr = rt_array_create_double(arena, 1, data);
    double weight_data[] = {1.0};
    double *weights = rt_array_create_double(arena, 1, weight_data);

    /* Should always return the single element */
    for (int i = 0; i < 10; i++) {
        double result = rt_random_weighted_choice_double(rng, arr, weights);
        TEST_ASSERT(result == 3.14159, "Should always return single element");
    }


    rt_arena_destroy(arena);
}

static void test_rt_random_weighted_choice_double_null_rng(void)
{

    RtArena *arena = rt_arena_create(NULL);
    double data[] = {1.0, 2.0, 3.0};
    double *arr = rt_array_create_double(arena, 3, data);
    double weight_data[] = {1.0, 2.0, 3.0};
    double *weights = rt_array_create_double(arena, 3, weight_data);

    double result = rt_random_weighted_choice_double(NULL, arr, weights);
    TEST_ASSERT(result == 0.0, "Should return 0.0 for NULL rng");


    rt_arena_destroy(arena);
}

static void test_rt_random_weighted_choice_double_null_arr(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 42);
    double weight_data[] = {1.0, 2.0};
    double *weights = rt_array_create_double(arena, 2, weight_data);

    double result = rt_random_weighted_choice_double(rng, NULL, weights);
    TEST_ASSERT(result == 0.0, "Should return 0.0 for NULL array");


    rt_arena_destroy(arena);
}

static void test_rt_random_weighted_choice_double_null_weights(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 42);
    double data[] = {1.0, 2.0, 3.0};
    double *arr = rt_array_create_double(arena, 3, data);

    double result = rt_random_weighted_choice_double(rng, arr, NULL);
    TEST_ASSERT(result == 0.0, "Should return 0.0 for NULL weights");


    rt_arena_destroy(arena);
}

static void test_rt_random_weighted_choice_double_invalid_weights(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 42);

    double data[] = {1.0, 2.0, 3.0};
    double *arr = rt_array_create_double(arena, 3, data);

    /* Negative weight */
    double neg_weight_data[] = {1.0, -1.0, 1.0};
    double *neg_weights = rt_array_create_double(arena, 3, neg_weight_data);
    double result1 = rt_random_weighted_choice_double(rng, arr, neg_weights);
    TEST_ASSERT(result1 == 0.0, "Should return 0.0 for negative weights");

    /* Zero weight */
    double zero_weight_data[] = {1.0, 0.0, 1.0};
    double *zero_weights = rt_array_create_double(arena, 3, zero_weight_data);
    double result2 = rt_random_weighted_choice_double(rng, arr, zero_weights);
    TEST_ASSERT(result2 == 0.0, "Should return 0.0 for zero weight");


    rt_arena_destroy(arena);
}

static void test_rt_random_weighted_choice_double_reproducible(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    double data[] = {1.1, 2.2, 3.3, 4.4, 5.5};
    double *arr = rt_array_create_double(arena, 5, data);
    double weight_data[] = {1.0, 2.0, 3.0, 2.0, 1.0};
    double *weights = rt_array_create_double(arena, 5, weight_data);

    /* Create two RNGs with the same seed */
    RtRandom *rng1 = rt_random_create_with_seed(arena, 99999);
    RtRandom *rng2 = rt_random_create_with_seed(arena, 99999);

    /* They should produce the same sequence */
    int matches = 0;
    for (int i = 0; i < 20; i++) {
        double r1 = rt_random_weighted_choice_double(rng1, arr, weights);
        double r2 = rt_random_weighted_choice_double(rng2, arr, weights);
        if (r1 == r2) matches++;
    }

    TEST_ASSERT(matches == 20, "Same seed should produce same sequence");


    rt_arena_destroy(arena);
}

static void test_rt_random_weighted_choice_double_distribution(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 54321);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    /* Create array with values {1.0, 2.0, 3.0, 4.0} */
    double data[] = {1.0, 2.0, 3.0, 4.0};
    double *arr = rt_array_create_double(arena, 4, data);

    /* Equal weights -> should be roughly equal distribution */
    double weight_data[] = {1.0, 1.0, 1.0, 1.0};
    double *weights = rt_array_create_double(arena, 4, weight_data);

    int counts[4] = {0, 0, 0, 0};
    int iterations = 4000;

    for (int i = 0; i < iterations; i++) {
        double result = rt_random_weighted_choice_double(rng, arr, weights);
        if (result == 1.0) counts[0]++;
        else if (result == 2.0) counts[1]++;
        else if (result == 3.0) counts[2]++;
        else if (result == 4.0) counts[3]++;
    }

    /* With equal weights, each should appear roughly 1/4 of the time */
    int expected = iterations / 4;
    int tolerance = expected / 2;  /* Allow 50% deviation */

    for (int i = 0; i < 4; i++) {
        int deviation = abs(counts[i] - expected);
        TEST_ASSERT(deviation < tolerance, "Distribution should be roughly uniform");
    }


    rt_arena_destroy(arena);
}

/* ============================================================================
 * Instance Weighted Choice String Tests
 * ============================================================================ */

static void test_rt_random_weighted_choice_string_basic(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Create RNG with known seed */
    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    /* Create array with values {"apple", "banana", "cherry"} */
    const char *data[] = {"apple", "banana", "cherry"};
    char **arr = rt_array_create_string(arena, 3, data);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Create weights {0.7, 0.25, 0.05} */
    double weight_data[] = {0.7, 0.25, 0.05};
    double *weights = rt_array_create_double(arena, 3, weight_data);
    TEST_ASSERT_NOT_NULL(weights, "Weights should be created");

    /* Call multiple times and verify result is always from array */
    int found_apple = 0, found_banana = 0, found_cherry = 0;
    for (int i = 0; i < 100; i++) {
        char *result = rt_random_weighted_choice_string(rng, arr, weights);
        TEST_ASSERT_NOT_NULL(result, "Result should not be NULL");
        if (strcmp(result, "apple") == 0) found_apple++;
        else if (strcmp(result, "banana") == 0) found_banana++;
        else if (strcmp(result, "cherry") == 0) found_cherry++;
        else {
            TEST_ASSERT(0, "Result should be from array");
        }
    }

    /* With weights {0.7, 0.25, 0.05}, apple should appear most often */
    TEST_ASSERT(found_apple > found_cherry, "apple (weight 0.7) should appear more than cherry (weight 0.05)");


    rt_arena_destroy(arena);
}

static void test_rt_random_weighted_choice_string_single_element(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 42);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    /* Single element array */
    const char *data[] = {"only_one"};
    char **arr = rt_array_create_string(arena, 1, data);
    double weight_data[] = {1.0};
    double *weights = rt_array_create_double(arena, 1, weight_data);

    /* Should always return the single element */
    for (int i = 0; i < 10; i++) {
        char *result = rt_random_weighted_choice_string(rng, arr, weights);
        TEST_ASSERT(strcmp(result, "only_one") == 0, "Should always return single element");
    }


    rt_arena_destroy(arena);
}

static void test_rt_random_weighted_choice_string_null_rng(void)
{

    RtArena *arena = rt_arena_create(NULL);
    const char *data[] = {"a", "b", "c"};
    char **arr = rt_array_create_string(arena, 3, data);
    double weight_data[] = {1.0, 2.0, 3.0};
    double *weights = rt_array_create_double(arena, 3, weight_data);

    char *result = rt_random_weighted_choice_string(NULL, arr, weights);
    TEST_ASSERT(result == NULL, "Should return NULL for NULL rng");


    rt_arena_destroy(arena);
}

static void test_rt_random_weighted_choice_string_null_arr(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 42);
    double weight_data[] = {1.0, 2.0};
    double *weights = rt_array_create_double(arena, 2, weight_data);

    char *result = rt_random_weighted_choice_string(rng, NULL, weights);
    TEST_ASSERT(result == NULL, "Should return NULL for NULL array");


    rt_arena_destroy(arena);
}

static void test_rt_random_weighted_choice_string_null_weights(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 42);
    const char *data[] = {"a", "b", "c"};
    char **arr = rt_array_create_string(arena, 3, data);

    char *result = rt_random_weighted_choice_string(rng, arr, NULL);
    TEST_ASSERT(result == NULL, "Should return NULL for NULL weights");


    rt_arena_destroy(arena);
}

static void test_rt_random_weighted_choice_string_invalid_weights(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 42);

    const char *data[] = {"a", "b", "c"};
    char **arr = rt_array_create_string(arena, 3, data);

    /* Negative weight */
    double neg_weight_data[] = {1.0, -1.0, 1.0};
    double *neg_weights = rt_array_create_double(arena, 3, neg_weight_data);
    char *result1 = rt_random_weighted_choice_string(rng, arr, neg_weights);
    TEST_ASSERT(result1 == NULL, "Should return NULL for negative weights");

    /* Zero weight */
    double zero_weight_data[] = {1.0, 0.0, 1.0};
    double *zero_weights = rt_array_create_double(arena, 3, zero_weight_data);
    char *result2 = rt_random_weighted_choice_string(rng, arr, zero_weights);
    TEST_ASSERT(result2 == NULL, "Should return NULL for zero weight");


    rt_arena_destroy(arena);
}

static void test_rt_random_weighted_choice_string_reproducible(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    const char *data[] = {"one", "two", "three", "four", "five"};
    char **arr = rt_array_create_string(arena, 5, data);
    double weight_data[] = {1.0, 2.0, 3.0, 2.0, 1.0};
    double *weights = rt_array_create_double(arena, 5, weight_data);

    /* Create two RNGs with the same seed */
    RtRandom *rng1 = rt_random_create_with_seed(arena, 99999);
    RtRandom *rng2 = rt_random_create_with_seed(arena, 99999);

    /* They should produce the same sequence */
    int matches = 0;
    for (int i = 0; i < 20; i++) {
        char *r1 = rt_random_weighted_choice_string(rng1, arr, weights);
        char *r2 = rt_random_weighted_choice_string(rng2, arr, weights);
        if (strcmp(r1, r2) == 0) matches++;
    }

    TEST_ASSERT(matches == 20, "Same seed should produce same sequence");


    rt_arena_destroy(arena);
}

static void test_rt_random_weighted_choice_string_distribution(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 54321);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    /* Create array with values {"a", "b", "c", "d"} */
    const char *data[] = {"a", "b", "c", "d"};
    char **arr = rt_array_create_string(arena, 4, data);

    /* Equal weights -> should be roughly equal distribution */
    double weight_data[] = {1.0, 1.0, 1.0, 1.0};
    double *weights = rt_array_create_double(arena, 4, weight_data);

    int counts[4] = {0, 0, 0, 0};
    int iterations = 4000;

    for (int i = 0; i < iterations; i++) {
        char *result = rt_random_weighted_choice_string(rng, arr, weights);
        if (strcmp(result, "a") == 0) counts[0]++;
        else if (strcmp(result, "b") == 0) counts[1]++;
        else if (strcmp(result, "c") == 0) counts[2]++;
        else if (strcmp(result, "d") == 0) counts[3]++;
    }

    /* With equal weights, each should appear roughly 1/4 of the time */
    int expected = iterations / 4;
    int tolerance = expected / 2;  /* Allow 50% deviation */

    for (int i = 0; i < 4; i++) {
        int deviation = abs(counts[i] - expected);
        TEST_ASSERT(deviation < tolerance, "Distribution should be roughly uniform");
    }


    rt_arena_destroy(arena);
}

/* ============================================================================
 * Weighted Selection Probability Distribution Tests
 * ============================================================================
 * Comprehensive tests for weighted random selection distribution accuracy.
 * ============================================================================ */

static void test_weighted_distribution_equal_weights_uniform(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Create RNG with seed for reproducibility */
    RtRandom *rng = rt_random_create_with_seed(arena, 42);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    /* Create array with 5 elements, all with equal weights */
    long data[] = {10, 20, 30, 40, 50};
    long *arr = rt_array_create_long(arena, 5, data);
    double weight_data[] = {1.0, 1.0, 1.0, 1.0, 1.0};
    double *weights = rt_array_create_double(arena, 5, weight_data);

    int counts[5] = {0, 0, 0, 0, 0};
    int iterations = 5000;

    for (int i = 0; i < iterations; i++) {
        long result = rt_random_weighted_choice_long(rng, arr, weights);
        if (result == 10) counts[0]++;
        else if (result == 20) counts[1]++;
        else if (result == 30) counts[2]++;
        else if (result == 40) counts[3]++;
        else if (result == 50) counts[4]++;
    }

    /* With equal weights, expect ~20% each (1000 per element) */
    int expected = iterations / 5;
    int tolerance = expected / 3;  /* Allow ~33% deviation */

    for (int i = 0; i < 5; i++) {
        int deviation = abs(counts[i] - expected);
        TEST_ASSERT(deviation < tolerance, "Equal weights should produce uniform distribution");
    }


    rt_arena_destroy(arena);
}

static void test_weighted_distribution_extreme_ratio(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Create RNG with seed for reproducibility */
    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    /* Create array with 2 elements: weight 1000 vs weight 1 */
    long data[] = {100, 200};
    long *arr = rt_array_create_long(arena, 2, data);
    double weight_data[] = {1000.0, 1.0};
    double *weights = rt_array_create_double(arena, 2, weight_data);

    int count_100 = 0, count_200 = 0;
    int iterations = 10010;  /* Divisible by 1001 for easier math */

    for (int i = 0; i < iterations; i++) {
        long result = rt_random_weighted_choice_long(rng, arr, weights);
        if (result == 100) count_100++;
        else if (result == 200) count_200++;
    }

    /* With 1000:1 ratio, expect ~99.9% vs ~0.1% */
    /* Expected: 100 should appear ~10000 times, 200 should appear ~10 times */
    int expected_100 = (int)(iterations * 1000.0 / 1001.0);  /* ~9990 */
    (void)iterations;  /* Mark as used */

    /* Verify 100 appears much more often */
    TEST_ASSERT(count_100 > count_200 * 100, "High-weight element should dominate");

    /* Allow generous tolerance for rare element */
    int tolerance_100 = expected_100 / 10;  /* 10% */
    int deviation_100 = abs(count_100 - expected_100);
    TEST_ASSERT(deviation_100 < tolerance_100, "High-weight element should be near expected");

    rt_arena_destroy(arena);
}

static void test_weighted_distribution_single_element(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Create RNG with seed */
    RtRandom *rng = rt_random_create_with_seed(arena, 99999);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    /* Single element array */
    long data[] = {42};
    long *arr = rt_array_create_long(arena, 1, data);
    double weight_data[] = {1.0};
    double *weights = rt_array_create_double(arena, 1, weight_data);

    /* Should always return 42, no matter how many times called */
    for (int i = 0; i < 100; i++) {
        long result = rt_random_weighted_choice_long(rng, arr, weights);
        TEST_ASSERT(result == 42, "Single element should always be returned");
    }

    /* Also test static version */
    for (int i = 0; i < 100; i++) {
        long result = rt_random_static_weighted_choice_long(arr, weights);
        TEST_ASSERT(result == 42, "Single element should always be returned (static)");
    }


    rt_arena_destroy(arena);
}

static void test_weighted_distribution_large_sample_accuracy(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Create RNG with seed for reproducibility */
    RtRandom *rng = rt_random_create_with_seed(arena, 777);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    /* Create array with specific weights: 50%, 30%, 15%, 5% */
    long data[] = {1, 2, 3, 4};
    long *arr = rt_array_create_long(arena, 4, data);
    double weight_data[] = {50.0, 30.0, 15.0, 5.0};  /* Total = 100 */
    double *weights = rt_array_create_double(arena, 4, weight_data);

    int counts[4] = {0, 0, 0, 0};
    int iterations = 10000;  /* Large sample for accuracy */

    for (int i = 0; i < iterations; i++) {
        long result = rt_random_weighted_choice_long(rng, arr, weights);
        if (result >= 1 && result <= 4) {
            counts[result - 1]++;
        }
    }

    /* Expected distribution: 5000, 3000, 1500, 500 */
    int expected[] = {5000, 3000, 1500, 500};
    /* Allow 15% tolerance from expected */
    double tolerance_pct = 0.15;

    for (int i = 0; i < 4; i++) {
        int tolerance = (int)(expected[i] * tolerance_pct);
        if (tolerance < 50) tolerance = 50;  /* Minimum tolerance for rare events */
        int deviation = abs(counts[i] - expected[i]);
        TEST_ASSERT(deviation < tolerance, "Distribution should match weights within tolerance");
    }

    rt_arena_destroy(arena);
}

static void test_weighted_distribution_seeded_prng_reproducible(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    long data[] = {10, 20, 30, 40, 50};
    long *arr = rt_array_create_long(arena, 5, data);
    double weight_data[] = {1.0, 2.0, 3.0, 2.0, 1.0};
    double *weights = rt_array_create_double(arena, 5, weight_data);

    /* Create two RNGs with the same seed */
    RtRandom *rng1 = rt_random_create_with_seed(arena, 54321);
    RtRandom *rng2 = rt_random_create_with_seed(arena, 54321);

    /* Generate sequences and verify they match exactly */
    int iterations = 100;
    int matches = 0;

    for (int i = 0; i < iterations; i++) {
        long r1 = rt_random_weighted_choice_long(rng1, arr, weights);
        long r2 = rt_random_weighted_choice_long(rng2, arr, weights);
        if (r1 == r2) matches++;
    }

    TEST_ASSERT(matches == iterations, "Same seed must produce identical sequence");


    rt_arena_destroy(arena);
}

static void test_weighted_distribution_os_entropy_varies(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    long data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    long *arr = rt_array_create_long(arena, 10, data);
    double weight_data[] = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
    double *weights = rt_array_create_double(arena, 10, weight_data);

    /* Generate a sequence using OS entropy (static function) */
    int iterations = 100;
    long results[100];

    for (int i = 0; i < iterations; i++) {
        results[i] = rt_random_static_weighted_choice_long(arr, weights);
    }

    /* Count unique values - with 10 elements and 100 samples, should see variety */
    int seen[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    for (int i = 0; i < iterations; i++) {
        if (results[i] >= 1 && results[i] <= 10) {
            seen[results[i] - 1] = 1;
        }
    }

    int unique_count = 0;
    for (int i = 0; i < 10; i++) {
        if (seen[i]) unique_count++;
    }

    /* With equal weights and 100 samples, should see most values */
    TEST_ASSERT(unique_count >= 5, "OS entropy should produce varied results");


    rt_arena_destroy(arena);
}

static void test_weighted_distribution_static_vs_instance(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    long data[] = {1, 2, 3};
    long *arr = rt_array_create_long(arena, 3, data);
    double weight_data[] = {1.0, 2.0, 3.0};  /* Total weight 6 */
    double *weights = rt_array_create_double(arena, 3, weight_data);

    /* Test static version (OS entropy) */
    int static_counts[3] = {0, 0, 0};
    int iterations = 6000;

    for (int i = 0; i < iterations; i++) {
        long result = rt_random_static_weighted_choice_long(arr, weights);
        if (result >= 1 && result <= 3) {
            static_counts[result - 1]++;
        }
    }

    /* Test instance version (seeded PRNG) */
    RtRandom *rng = rt_random_create_with_seed(arena, 11111);
    int instance_counts[3] = {0, 0, 0};

    for (int i = 0; i < iterations; i++) {
        long result = rt_random_weighted_choice_long(rng, arr, weights);
        if (result >= 1 && result <= 3) {
            instance_counts[result - 1]++;
        }
    }

    /* Expected distribution: 1/6, 2/6, 3/6 = ~1000, ~2000, ~3000 */
    int expected[] = {1000, 2000, 3000};
    int tolerance = 400;  /* Allow reasonable variance */

    /* Both should roughly match expected distribution */
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT(abs(static_counts[i] - expected[i]) < tolerance,
                   "Static distribution should match weights");
        TEST_ASSERT(abs(instance_counts[i] - expected[i]) < tolerance,
                   "Instance distribution should match weights");
    }

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Integration Test: Weighted Loot Drop Scenario
 * ============================================================================
 * This test demonstrates a real-world use case: game loot drops with
 * tiered rarity (common, rare, legendary).
 *
 * EXPECTED USAGE PATTERN:
 * -----------------------
 * In Sindarin (when Random module is exposed to language):
 *
 *   // Using static method (OS entropy - truly random):
 *   var items: str[] = {"common_sword", "rare_shield", "legendary_helm"}
 *   var weights: double[] = {70.0, 25.0, 5.0}  // 70%, 25%, 5%
 *   var drop: str = Random.weightedChoice(items, weights)
 *
 *   // Using instance method (seeded PRNG - reproducible):
 *   var rng: Random = Random.createWithSeed(player_seed)
 *   var drop: str = rng.weightedChoice(items, weights)
 *
 * This test verifies:
 * 1. Real-world weights (70%/25%/5%) work correctly
 * 2. Both static and instance methods produce correct distributions
 * 3. All items (including rare ones) can actually be selected
 * 4. Distribution matches expected probabilities within tolerance
 * ============================================================================ */

static void test_integration_weighted_loot_drop_static(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /*
     * Real-world loot drop scenario:
     * - common_sword:    70% drop rate
     * - rare_shield:     25% drop rate
     * - legendary_helm:   5% drop rate
     */
    const char *item_data[] = {"common_sword", "rare_shield", "legendary_helm"};
    char **items = rt_array_create_string(arena, 3, item_data);
    TEST_ASSERT_NOT_NULL(items, "Items array should be created");

    double weight_data[] = {70.0, 25.0, 5.0};  /* Percentages as weights */
    double *weights = rt_array_create_double(arena, 3, weight_data);
    TEST_ASSERT_NOT_NULL(weights, "Weights array should be created");

    /* Simulate many loot drops using OS entropy (static method) */
    int common_count = 0, rare_count = 0, legendary_count = 0;
    int total_drops = 10000;  /* Large sample for accuracy */

    for (int i = 0; i < total_drops; i++) {
        char *drop = rt_random_static_weighted_choice_string(items, weights);
        TEST_ASSERT_NOT_NULL(drop, "Drop should not be NULL");

        if (strcmp(drop, "common_sword") == 0) common_count++;
        else if (strcmp(drop, "rare_shield") == 0) rare_count++;
        else if (strcmp(drop, "legendary_helm") == 0) legendary_count++;
        else TEST_ASSERT(0, "Unknown item dropped");
    }

    /* Verify all items can be selected */
    TEST_ASSERT(common_count > 0, "Common items should be selectable");
    TEST_ASSERT(rare_count > 0, "Rare items should be selectable");
    TEST_ASSERT(legendary_count > 0, "Legendary items should be selectable");

    /* Expected: 7000 common, 2500 rare, 500 legendary */
    int expected_common = 7000;
    int expected_rare = 2500;
    int expected_legendary = 500;

    /* Allow 15% tolerance */
    int tolerance_common = expected_common * 15 / 100;     /* ~1050 */
    int tolerance_rare = expected_rare * 15 / 100;         /* ~375 */
    int tolerance_legendary = expected_legendary * 30 / 100; /* ~150 (generous for rare) */

    TEST_ASSERT(abs(common_count - expected_common) < tolerance_common,
               "Common drop rate should be ~70%");
    TEST_ASSERT(abs(rare_count - expected_rare) < tolerance_rare,
               "Rare drop rate should be ~25%");
    TEST_ASSERT(abs(legendary_count - expected_legendary) < tolerance_legendary,
               "Legendary drop rate should be ~5%");

    rt_arena_destroy(arena);
}

static void test_integration_weighted_loot_drop_seeded(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /*
     * Create seeded RNG - useful for:
     * - Procedural generation with save/load
     * - Testing where reproducibility is needed
     * - Replay systems (same seed = same loot sequence)
     */
    long player_seed = 12345;  /* Could be based on player ID, world seed, etc. */
    RtRandom *rng = rt_random_create_with_seed(arena, player_seed);
    TEST_ASSERT_NOT_NULL(rng, "Seeded RNG should be created");

    /* Same loot table */
    const char *item_data[] = {"common_sword", "rare_shield", "legendary_helm"};
    char **items = rt_array_create_string(arena, 3, item_data);
    double weight_data[] = {70.0, 25.0, 5.0};
    double *weights = rt_array_create_double(arena, 3, weight_data);

    int common_count = 0, rare_count = 0, legendary_count = 0;
    int total_drops = 10000;

    for (int i = 0; i < total_drops; i++) {
        char *drop = rt_random_weighted_choice_string(rng, items, weights);
        TEST_ASSERT_NOT_NULL(drop, "Drop should not be NULL");

        if (strcmp(drop, "common_sword") == 0) common_count++;
        else if (strcmp(drop, "rare_shield") == 0) rare_count++;
        else if (strcmp(drop, "legendary_helm") == 0) legendary_count++;
    }

    /* Verify all items can be selected */
    TEST_ASSERT(common_count > 0, "Common items should be selectable");
    TEST_ASSERT(rare_count > 0, "Rare items should be selectable");
    TEST_ASSERT(legendary_count > 0, "Legendary items should be selectable");

    /* Same distribution expectations */
    int expected_common = 7000;
    int expected_rare = 2500;
    int expected_legendary = 500;

    int tolerance_common = expected_common * 15 / 100;
    int tolerance_rare = expected_rare * 15 / 100;
    int tolerance_legendary = expected_legendary * 30 / 100;

    TEST_ASSERT(abs(common_count - expected_common) < tolerance_common,
               "Common drop rate should be ~70%");
    TEST_ASSERT(abs(rare_count - expected_rare) < tolerance_rare,
               "Rare drop rate should be ~25%");
    TEST_ASSERT(abs(legendary_count - expected_legendary) < tolerance_legendary,
               "Legendary drop rate should be ~5%");

    /*
     * Verify reproducibility: same seed should give same sequence
     */
    RtRandom *rng2 = rt_random_create_with_seed(arena, player_seed);
    RtRandom *rng_orig = rt_random_create_with_seed(arena, player_seed);

    int matches = 0;
    for (int i = 0; i < 10; i++) {
        char *drop1 = rt_random_weighted_choice_string(rng2, items, weights);
        char *drop2 = rt_random_weighted_choice_string(rng_orig, items, weights);
        if (strcmp(drop1, drop2) == 0) matches++;
    }
    TEST_ASSERT(matches == 10, "Same seed must produce identical loot sequence");

    rt_arena_destroy(arena);
}

static void test_integration_weighted_loot_drop_all_tiers(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /*
     * With a 5% legendary rate, we need enough samples to statistically
     * guarantee we see at least one legendary drop.
     * P(no legendary in N drops) = 0.95^N
     * For N=100: 0.95^100 ≈ 0.006 (0.6% chance of no legendary)
     * We'll use seeded RNG and verify all tiers appear.
     */
    RtRandom *rng = rt_random_create_with_seed(arena, 99999);

    const char *item_data[] = {"common_sword", "rare_shield", "legendary_helm"};
    char **items = rt_array_create_string(arena, 3, item_data);
    double weight_data[] = {70.0, 25.0, 5.0};
    double *weights = rt_array_create_double(arena, 3, weight_data);

    int found_common = 0, found_rare = 0, found_legendary = 0;

    for (int i = 0; i < 1000 && !(found_common && found_rare && found_legendary); i++) {
        char *drop = rt_random_weighted_choice_string(rng, items, weights);
        if (strcmp(drop, "common_sword") == 0) found_common = 1;
        else if (strcmp(drop, "rare_shield") == 0) found_rare = 1;
        else if (strcmp(drop, "legendary_helm") == 0) found_legendary = 1;
    }

    TEST_ASSERT(found_common, "Common tier must be reachable");
    TEST_ASSERT(found_rare, "Rare tier must be reachable");
    TEST_ASSERT(found_legendary, "Legendary tier must be reachable");


    rt_arena_destroy(arena);
}
static void test_rt_random_choice_statistical_chi_squared(void)
{

    RtArena *arena = rt_arena_create(NULL);

    long data[] = {10, 20, 30, 40, 50};
    long *arr = rt_array_create_long(arena, 5, data);

    int counts[5] = {0, 0, 0, 0, 0};
    int num_trials = 50000;

    for (int i = 0; i < num_trials; i++) {
        long choice = rt_random_static_choice_long(arr, 5);
        for (int j = 0; j < 5; j++) {
            if (choice == data[j]) {
                counts[j]++;
                break;
            }
        }
    }

    /* Calculate chi-squared statistic */
    double expected = num_trials / 5.0;
    double chi_squared = 0.0;
    for (int i = 0; i < 5; i++) {
        double diff = counts[i] - expected;
        chi_squared += (diff * diff) / expected;
    }

    /* Chi-squared with 4 degrees of freedom: p=0.01 critical value is ~13.28 */
    TEST_ASSERT(chi_squared < 15.0, "Choice should pass chi-squared test for uniformity");


    rt_arena_destroy(arena);
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

void test_rt_random_choice_main(void)
{
    TEST_SECTION("Runtime Random Choice");

    /* Static choice tests */
    TEST_RUN("static_choice_long_basic", test_rt_random_static_choice_long_basic);
    TEST_RUN("static_choice_long_single_element", test_rt_random_static_choice_long_single_element);
    TEST_RUN("static_choice_long_null_empty", test_rt_random_static_choice_long_null_empty);
    TEST_RUN("static_choice_long_distribution", test_rt_random_static_choice_long_distribution);
    TEST_RUN("static_choice_double_basic", test_rt_random_static_choice_double_basic);
    TEST_RUN("static_choice_double_null_empty", test_rt_random_static_choice_double_null_empty);
    TEST_RUN("static_choice_string_basic", test_rt_random_static_choice_string_basic);
    TEST_RUN("static_choice_string_null_empty", test_rt_random_static_choice_string_null_empty);
    TEST_RUN("static_choice_bool_basic", test_rt_random_static_choice_bool_basic);
    TEST_RUN("static_choice_bool_null_empty", test_rt_random_static_choice_bool_null_empty);
    TEST_RUN("static_choice_byte_basic", test_rt_random_static_choice_byte_basic);
    TEST_RUN("static_choice_byte_null_empty", test_rt_random_static_choice_byte_null_empty);

    /* Instance choice tests */
    TEST_RUN("choice_long_basic", test_rt_random_choice_long_basic);
    TEST_RUN("choice_long_reproducibility", test_rt_random_choice_long_reproducibility);
    TEST_RUN("choice_long_null_args", test_rt_random_choice_long_null_args);
    TEST_RUN("choice_long_distribution", test_rt_random_choice_long_distribution);
    TEST_RUN("choice_double_basic", test_rt_random_choice_double_basic);
    TEST_RUN("choice_double_null_args", test_rt_random_choice_double_null_args);
    TEST_RUN("choice_string_basic", test_rt_random_choice_string_basic);
    TEST_RUN("choice_string_null_args", test_rt_random_choice_string_null_args);
    TEST_RUN("choice_bool_basic", test_rt_random_choice_bool_basic);
    TEST_RUN("choice_bool_null_args", test_rt_random_choice_bool_null_args);
    TEST_RUN("choice_byte_basic", test_rt_random_choice_byte_basic);
    TEST_RUN("choice_byte_null_args", test_rt_random_choice_byte_null_args);

    /* Statistical distribution tests for choice */
    TEST_RUN("static_choice_double_distribution", test_rt_random_static_choice_double_distribution);
    TEST_RUN("static_choice_string_distribution", test_rt_random_static_choice_string_distribution);
    TEST_RUN("static_choice_byte_distribution", test_rt_random_static_choice_byte_distribution);
    TEST_RUN("choice_double_distribution", test_rt_random_choice_double_distribution);
    TEST_RUN("choice_string_distribution", test_rt_random_choice_string_distribution);
    TEST_RUN("choice_byte_distribution", test_rt_random_choice_byte_distribution);

    /* Weight validation helper tests */
    TEST_RUN("validate_weights_valid", test_rt_random_validate_weights_valid);
    TEST_RUN("validate_weights_negative", test_rt_random_validate_weights_negative);
    TEST_RUN("validate_weights_zero", test_rt_random_validate_weights_zero);
    TEST_RUN("validate_weights_empty", test_rt_random_validate_weights_empty);
    TEST_RUN("validate_weights_null", test_rt_random_validate_weights_null);

    /* Cumulative distribution helper tests */
    TEST_RUN("build_cumulative_basic", test_rt_random_build_cumulative_basic);
    TEST_RUN("build_cumulative_normalization", test_rt_random_build_cumulative_normalization);
    TEST_RUN("build_cumulative_single_element", test_rt_random_build_cumulative_single_element);
    TEST_RUN("build_cumulative_two_elements", test_rt_random_build_cumulative_two_elements);
    TEST_RUN("build_cumulative_null_arena", test_rt_random_build_cumulative_null_arena);
    TEST_RUN("build_cumulative_null_weights", test_rt_random_build_cumulative_null_weights);
    TEST_RUN("build_cumulative_empty_array", test_rt_random_build_cumulative_empty_array);
    TEST_RUN("build_cumulative_large_weights", test_rt_random_build_cumulative_large_weights);

    /* Weighted index selection helper tests */
    TEST_RUN("select_weighted_index_basic", test_rt_random_select_weighted_index_basic);
    TEST_RUN("select_weighted_index_edge_zero", test_rt_random_select_weighted_index_edge_zero);
    TEST_RUN("select_weighted_index_edge_near_one", test_rt_random_select_weighted_index_edge_near_one);
    TEST_RUN("select_weighted_index_single_element", test_rt_random_select_weighted_index_single_element);
    TEST_RUN("select_weighted_index_two_elements", test_rt_random_select_weighted_index_two_elements);
    TEST_RUN("select_weighted_index_boundary_values", test_rt_random_select_weighted_index_boundary_values);
    TEST_RUN("select_weighted_index_null", test_rt_random_select_weighted_index_null);
    TEST_RUN("select_weighted_index_invalid_len", test_rt_random_select_weighted_index_invalid_len);
    TEST_RUN("select_weighted_index_large_array", test_rt_random_select_weighted_index_large_array);

    /* Static weighted choice tests */
    TEST_RUN("static_weighted_choice_long_basic", test_rt_random_static_weighted_choice_long_basic);
    TEST_RUN("static_weighted_choice_long_single_element", test_rt_random_static_weighted_choice_long_single_element);
    TEST_RUN("static_weighted_choice_long_null_arr", test_rt_random_static_weighted_choice_long_null_arr);
    TEST_RUN("static_weighted_choice_long_null_weights", test_rt_random_static_weighted_choice_long_null_weights);
    TEST_RUN("static_weighted_choice_long_invalid_weights", test_rt_random_static_weighted_choice_long_invalid_weights);
    TEST_RUN("static_weighted_choice_long_distribution", test_rt_random_static_weighted_choice_long_distribution);
    TEST_RUN("static_weighted_choice_double_basic", test_rt_random_static_weighted_choice_double_basic);
    TEST_RUN("static_weighted_choice_double_single_element", test_rt_random_static_weighted_choice_double_single_element);
    TEST_RUN("static_weighted_choice_double_null_arr", test_rt_random_static_weighted_choice_double_null_arr);
    TEST_RUN("static_weighted_choice_double_null_weights", test_rt_random_static_weighted_choice_double_null_weights);
    TEST_RUN("static_weighted_choice_double_invalid_weights", test_rt_random_static_weighted_choice_double_invalid_weights);
    TEST_RUN("static_weighted_choice_double_distribution", test_rt_random_static_weighted_choice_double_distribution);
    TEST_RUN("static_weighted_choice_string_basic", test_rt_random_static_weighted_choice_string_basic);
    TEST_RUN("static_weighted_choice_string_single_element", test_rt_random_static_weighted_choice_string_single_element);
    TEST_RUN("static_weighted_choice_string_null_arr", test_rt_random_static_weighted_choice_string_null_arr);
    TEST_RUN("static_weighted_choice_string_null_weights", test_rt_random_static_weighted_choice_string_null_weights);
    TEST_RUN("static_weighted_choice_string_invalid_weights", test_rt_random_static_weighted_choice_string_invalid_weights);
    TEST_RUN("static_weighted_choice_string_distribution", test_rt_random_static_weighted_choice_string_distribution);

    /* Instance weighted choice tests */
    TEST_RUN("weighted_choice_long_basic", test_rt_random_weighted_choice_long_basic);
    TEST_RUN("weighted_choice_long_single_element", test_rt_random_weighted_choice_long_single_element);
    TEST_RUN("weighted_choice_long_null_rng", test_rt_random_weighted_choice_long_null_rng);
    TEST_RUN("weighted_choice_long_null_arr", test_rt_random_weighted_choice_long_null_arr);
    TEST_RUN("weighted_choice_long_null_weights", test_rt_random_weighted_choice_long_null_weights);
    TEST_RUN("weighted_choice_long_invalid_weights", test_rt_random_weighted_choice_long_invalid_weights);
    TEST_RUN("weighted_choice_long_reproducible", test_rt_random_weighted_choice_long_reproducible);
    TEST_RUN("weighted_choice_long_distribution", test_rt_random_weighted_choice_long_distribution);
    TEST_RUN("weighted_choice_double_basic", test_rt_random_weighted_choice_double_basic);
    TEST_RUN("weighted_choice_double_single_element", test_rt_random_weighted_choice_double_single_element);
    TEST_RUN("weighted_choice_double_null_rng", test_rt_random_weighted_choice_double_null_rng);
    TEST_RUN("weighted_choice_double_null_arr", test_rt_random_weighted_choice_double_null_arr);
    TEST_RUN("weighted_choice_double_null_weights", test_rt_random_weighted_choice_double_null_weights);
    TEST_RUN("weighted_choice_double_invalid_weights", test_rt_random_weighted_choice_double_invalid_weights);
    TEST_RUN("weighted_choice_double_reproducible", test_rt_random_weighted_choice_double_reproducible);
    TEST_RUN("weighted_choice_double_distribution", test_rt_random_weighted_choice_double_distribution);
    TEST_RUN("weighted_choice_string_basic", test_rt_random_weighted_choice_string_basic);
    TEST_RUN("weighted_choice_string_single_element", test_rt_random_weighted_choice_string_single_element);
    TEST_RUN("weighted_choice_string_null_rng", test_rt_random_weighted_choice_string_null_rng);
    TEST_RUN("weighted_choice_string_null_arr", test_rt_random_weighted_choice_string_null_arr);
    TEST_RUN("weighted_choice_string_null_weights", test_rt_random_weighted_choice_string_null_weights);
    TEST_RUN("weighted_choice_string_invalid_weights", test_rt_random_weighted_choice_string_invalid_weights);
    TEST_RUN("weighted_choice_string_reproducible", test_rt_random_weighted_choice_string_reproducible);
    TEST_RUN("weighted_choice_string_distribution", test_rt_random_weighted_choice_string_distribution);

    /* Weighted distribution tests */
    TEST_RUN("weighted_distribution_equal_weights_uniform", test_weighted_distribution_equal_weights_uniform);
    TEST_RUN("weighted_distribution_extreme_ratio", test_weighted_distribution_extreme_ratio);
    TEST_RUN("weighted_distribution_single_element", test_weighted_distribution_single_element);
    TEST_RUN("weighted_distribution_large_sample_accuracy", test_weighted_distribution_large_sample_accuracy);
    TEST_RUN("weighted_distribution_seeded_prng_reproducible", test_weighted_distribution_seeded_prng_reproducible);
    TEST_RUN("weighted_distribution_os_entropy_varies", test_weighted_distribution_os_entropy_varies);
    TEST_RUN("weighted_distribution_static_vs_instance", test_weighted_distribution_static_vs_instance);

    /* Integration tests for weighted loot */
    TEST_RUN("integration_weighted_loot_drop_static", test_integration_weighted_loot_drop_static);
    TEST_RUN("integration_weighted_loot_drop_seeded", test_integration_weighted_loot_drop_seeded);
    TEST_RUN("integration_weighted_loot_drop_all_tiers", test_integration_weighted_loot_drop_all_tiers);

    /* Statistical tests */
    TEST_RUN("choice_statistical_chi_squared", test_rt_random_choice_statistical_chi_squared);
}
