// tests/unit/runtime_random_tests_basic.c
// Tests for runtime random basic (instance) value generation: int, long, double, bool, byte, gaussian

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "../../src/arena.h"
#include "../../src/runtime/runtime_random.h"
#include "../../src/runtime/runtime_arena.h"
#include "../../src/debug.h"
#include "../test_utils.h"

/* ============================================================================
 * Instance Int Tests
 * ============================================================================ */

void test_rt_random_basic_int_range()
{
    printf("Testing rt_random_int range validation...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    /* Test basic range */
    for (int i = 0; i < 100; i++) {
        long val = rt_random_int(rng, 1, 10);
        TEST_ASSERT(val >= 1 && val <= 10, "Value should be in range [1, 10]");
    }

    /* Test inverted range */
    for (int i = 0; i < 100; i++) {
        long val = rt_random_int(rng, 10, 1);
        TEST_ASSERT(val >= 1 && val <= 10, "Inverted range should work");
    }

    /* Test single value */
    long single = rt_random_int(rng, 42, 42);
    TEST_ASSERT(single == 42, "Single value should return that value");

    printf("  Instance int range tests passed\n");
    rt_arena_destroy(arena);
}

void test_rt_random_basic_int_power_of_two_range()
{
    printf("Testing rt_random_int with power-of-two ranges...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 42);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    /* Test range of size 2 (power of 2) */
    for (int i = 0; i < 100; i++) {
        long val = rt_random_int(rng, 0, 1);
        TEST_ASSERT(val >= 0 && val <= 1, "Value should be in range [0, 1]");
    }

    /* Test range of size 256 (power of 2) */
    for (int i = 0; i < 100; i++) {
        long val = rt_random_int(rng, 0, 255);
        TEST_ASSERT(val >= 0 && val <= 255, "Value should be in range [0, 255]");
    }

    /* Test range of size 1024 (power of 2) */
    for (int i = 0; i < 100; i++) {
        long val = rt_random_int(rng, 100, 1123);  /* Range size 1024 */
        TEST_ASSERT(val >= 100 && val <= 1123, "Value should be in range [100, 1123]");
    }

    printf("  Instance power-of-two ranges work correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_random_basic_int_large_range()
{
    printf("Testing rt_random_int with large ranges...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    /* Test large range with seeded PRNG */
    long min = -1000000000L;
    long max = 1000000000L;
    for (int i = 0; i < 100; i++) {
        long val = rt_random_int(rng, min, max);
        TEST_ASSERT(val >= min && val <= max, "Value should be in large range");
    }

    printf("  Instance large ranges work correctly\n");
    rt_arena_destroy(arena);
}

/* ============================================================================
 * Instance Long Tests
 * ============================================================================ */

void test_rt_random_basic_long_range()
{
    printf("Testing rt_random_long range validation...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    for (int i = 0; i < 100; i++) {
        long long val = rt_random_long(rng, 1000000000LL, 2000000000LL);
        TEST_ASSERT(val >= 1000000000LL && val <= 2000000000LL, "Long should be in range");
    }

    printf("  Instance long range tests passed\n");
    rt_arena_destroy(arena);
}

void test_rt_random_basic_long_power_of_two_range()
{
    printf("Testing rt_random_long with power-of-two ranges...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 42);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    /* Test range of size 2 (power of 2) */
    for (int i = 0; i < 100; i++) {
        long long val = rt_random_long(rng, 0, 1);
        TEST_ASSERT(val >= 0 && val <= 1, "Value should be in range [0, 1]");
    }

    /* Test range of size 2^16 (power of 2) */
    for (int i = 0; i < 100; i++) {
        long long val = rt_random_long(rng, 0, 65535);
        TEST_ASSERT(val >= 0 && val <= 65535, "Value should be in range [0, 65535]");
    }

    printf("  Instance power-of-two ranges for long work correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_random_basic_long_large_range()
{
    printf("Testing rt_random_long with very large ranges...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    /* Test very large range with seeded PRNG */
    long long min = -4000000000000000000LL;
    long long max = 4000000000000000000LL;
    for (int i = 0; i < 100; i++) {
        long long val = rt_random_long(rng, min, max);
        TEST_ASSERT(val >= min && val <= max, "Value should be in very large range");
    }

    printf("  Instance very large long ranges work correctly\n");
    rt_arena_destroy(arena);
}

/* ============================================================================
 * Instance Double Tests
 * ============================================================================ */

void test_rt_random_basic_double_range()
{
    printf("Testing rt_random_double range validation...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    for (int i = 0; i < 100; i++) {
        double val = rt_random_double(rng, 0.0, 1.0);
        TEST_ASSERT(val >= 0.0 && val < 1.0, "Double should be in [0, 1)");
    }

    for (int i = 0; i < 100; i++) {
        double val = rt_random_double(rng, 20.5, 10.5);  /* Inverted */
        TEST_ASSERT(val >= 10.5 && val < 20.5, "Inverted double range should work");
    }

    printf("  Instance double range tests passed\n");
    rt_arena_destroy(arena);
}

void test_rt_random_basic_double_small_range()
{
    printf("Testing rt_random_double with very small ranges...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 42);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    /* Test very small range */
    double min = 0.0;
    double max = 0.0001;
    for (int i = 0; i < 100; i++) {
        double val = rt_random_double(rng, min, max);
        TEST_ASSERT(val >= min && val < max, "Value should be in small range");
    }

    printf("  Instance very small double ranges work correctly\n");
    rt_arena_destroy(arena);
}

/* ============================================================================
 * Instance Bool/Byte/Bytes Tests
 * ============================================================================ */

void test_rt_random_basic_bool_instance()
{
    printf("Testing rt_random_bool instance method...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    int true_count = 0;
    for (int i = 0; i < 1000; i++) {
        int val = rt_random_bool(rng);
        TEST_ASSERT(val == 0 || val == 1, "Bool should be 0 or 1");
        if (val) true_count++;
    }

    /* Should be roughly 50/50 */
    TEST_ASSERT(true_count > 350 && true_count < 650, "Bool should be roughly 50/50");

    printf("  Instance bool test passed (true=%d/1000)\n", true_count);
    rt_arena_destroy(arena);
}

void test_rt_random_basic_byte_instance()
{
    printf("Testing rt_random_byte instance method...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    int byte_counts[256] = {0};
    for (int i = 0; i < 2560; i++) {
        unsigned char val = rt_random_byte(rng);
        byte_counts[val]++;
    }

    int unique = 0;
    for (int i = 0; i < 256; i++) {
        if (byte_counts[i] > 0) unique++;
    }

    TEST_ASSERT(unique > 200, "Should see many unique byte values");

    printf("  Instance byte test passed (unique=%d/256)\n", unique);
    rt_arena_destroy(arena);
}

void test_rt_random_basic_bytes_instance()
{
    printf("Testing rt_random_bytes instance method...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    unsigned char *buf = rt_random_bytes(arena, rng, 32);
    TEST_ASSERT_NOT_NULL(buf, "Bytes buffer should be created");

    int non_zero = 0;
    for (int i = 0; i < 32; i++) {
        if (buf[i] != 0) non_zero++;
    }
    TEST_ASSERT(non_zero > 0, "Bytes should contain non-zero values");

    /* Test NULL arena */
    unsigned char *null_buf = rt_random_bytes(NULL, rng, 32);
    TEST_ASSERT(null_buf == NULL, "NULL arena should return NULL");

    /* Test zero/negative count */
    unsigned char *zero_buf = rt_random_bytes(arena, rng, 0);
    TEST_ASSERT(zero_buf == NULL, "Zero count should return NULL");

    printf("  Instance bytes test passed\n");
    rt_arena_destroy(arena);
}

/* ============================================================================
 * Instance Gaussian Tests
 * ============================================================================ */

void test_rt_random_basic_gaussian_instance()
{
    printf("Testing rt_random_gaussian instance method...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    double mean = 0.0;
    double stddev = 1.0;
    int iterations = 10000;

    double sum = 0.0;
    double sum_sq = 0.0;

    for (int i = 0; i < iterations; i++) {
        double val = rt_random_gaussian(rng, mean, stddev);
        sum += val;
        sum_sq += val * val;
    }

    double actual_mean = sum / iterations;
    double variance = (sum_sq / iterations) - (actual_mean * actual_mean);
    double actual_stddev = sqrt(variance);

    /* Standard normal should have mean ~0 and stddev ~1 */
    TEST_ASSERT(fabs(actual_mean) < 0.1, "Mean should be close to 0");
    TEST_ASSERT(fabs(actual_stddev - 1.0) < 0.1, "Stddev should be close to 1");

    printf("  Instance gaussian: mean=%.3f, stddev=%.3f\n", actual_mean, actual_stddev);
    rt_arena_destroy(arena);
}

void test_rt_random_basic_gaussian_extreme_stddev()
{
    printf("Testing rt_random_gaussian with extreme stddev values...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 42);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    /* Test with very small stddev */
    double mean = 100.0;
    double stddev = 0.001;
    double sum = 0;
    for (int i = 0; i < 1000; i++) {
        double val = rt_random_gaussian(rng, mean, stddev);
        sum += val;
        /* Values should be very close to mean */
        TEST_ASSERT(fabs(val - mean) < 1.0, "Value should be close to mean with small stddev");
    }
    double actual_mean = sum / 1000;
    TEST_ASSERT(fabs(actual_mean - mean) < 0.1, "Mean should be close to target");

    /* Test with large stddev */
    stddev = 1000.0;
    int in_1_stddev = 0;
    RtRandom *rng2 = rt_random_create_with_seed(arena, 43);
    for (int i = 0; i < 1000; i++) {
        double val = rt_random_gaussian(rng2, mean, stddev);
        if (fabs(val - mean) < stddev) in_1_stddev++;
    }
    /* About 68% should be within 1 stddev */
    TEST_ASSERT(in_1_stddev > 500 && in_1_stddev < 850, "Distribution should follow normal curve");

    printf("  Extreme stddev values handled correctly\n");
    rt_arena_destroy(arena);
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

void test_rt_random_basic_main()
{
    printf("\n");
    printf("================================================\n");
    printf(" Runtime Random Basic Tests\n");
    printf("================================================\n");

    /* Instance int tests */
    test_rt_random_basic_int_range();
    test_rt_random_basic_int_power_of_two_range();
    test_rt_random_basic_int_large_range();

    /* Instance long tests */
    test_rt_random_basic_long_range();
    test_rt_random_basic_long_power_of_two_range();
    test_rt_random_basic_long_large_range();

    /* Instance double tests */
    test_rt_random_basic_double_range();
    test_rt_random_basic_double_small_range();

    /* Instance bool/byte/bytes tests */
    test_rt_random_basic_bool_instance();
    test_rt_random_basic_byte_instance();
    test_rt_random_basic_bytes_instance();

    /* Instance gaussian tests */
    test_rt_random_basic_gaussian_instance();
    test_rt_random_basic_gaussian_extreme_stddev();

    printf("------------------------------------------------\n");
    printf(" All runtime random basic tests passed!\n");
    printf("================================================\n");
}
