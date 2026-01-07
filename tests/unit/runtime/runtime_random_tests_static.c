// tests/unit/runtime_random_tests_static.c
// Tests for runtime random static methods using OS entropy

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
 * Static Value Generation Tests
 * ============================================================================
 * Tests for the static methods that use OS entropy directly.
 * ============================================================================ */

void test_rt_random_static_int_range()
{
    printf("Testing rt_random_static_int range validation...\n");

    /* Test basic range */
    for (int i = 0; i < 100; i++) {
        long val = rt_random_static_int(1, 10);
        TEST_ASSERT(val >= 1 && val <= 10, "Value should be in range [1, 10]");
    }

    /* Test inverted range (min > max) should still work */
    for (int i = 0; i < 100; i++) {
        long val = rt_random_static_int(10, 1);  /* Inverted */
        TEST_ASSERT(val >= 1 && val <= 10, "Inverted range should still work");
    }

    /* Test single value range */
    long single = rt_random_static_int(42, 42);
    TEST_ASSERT(single == 42, "Single value range should return that value");

    /* Test negative range */
    for (int i = 0; i < 100; i++) {
        long val = rt_random_static_int(-100, -50);
        TEST_ASSERT(val >= -100 && val <= -50, "Negative range should work");
    }

    /* Test range crossing zero */
    for (int i = 0; i < 100; i++) {
        long val = rt_random_static_int(-50, 50);
        TEST_ASSERT(val >= -50 && val <= 50, "Zero-crossing range should work");
    }

    printf("  All range tests passed\n");
}

void test_rt_random_static_int_distribution()
{
    printf("Testing rt_random_static_int distribution...\n");

    int count = 10000;
    int buckets[10] = {0};

    for (int i = 0; i < count; i++) {
        long val = rt_random_static_int(0, 9);
        TEST_ASSERT(val >= 0 && val <= 9, "Value should be in range");
        buckets[val]++;
    }

    int expected = count / 10;
    int tolerance = expected / 3;

    for (int i = 0; i < 10; i++) {
        int deviation = abs(buckets[i] - expected);
        TEST_ASSERT(deviation < tolerance, "Distribution should be roughly uniform");
    }

    printf("  Distribution is uniform\n");
}

void test_rt_random_static_int_power_of_two_range()
{
    printf("Testing rt_random_static_int with power-of-two ranges...\n");

    /* Test range of size 2 (power of 2) */
    for (int i = 0; i < 100; i++) {
        long val = rt_random_static_int(0, 1);  /* Range size 2 */
        TEST_ASSERT(val >= 0 && val <= 1, "Value should be in range [0, 1]");
    }

    /* Test range of size 4 (power of 2) */
    for (int i = 0; i < 100; i++) {
        long val = rt_random_static_int(0, 3);  /* Range size 4 */
        TEST_ASSERT(val >= 0 && val <= 3, "Value should be in range [0, 3]");
    }

    /* Test range of size 8 (power of 2) */
    for (int i = 0; i < 100; i++) {
        long val = rt_random_static_int(0, 7);  /* Range size 8 */
        TEST_ASSERT(val >= 0 && val <= 7, "Value should be in range [0, 7]");
    }

    /* Test range of size 16 (power of 2) */
    for (int i = 0; i < 100; i++) {
        long val = rt_random_static_int(10, 25);  /* Range size 16 (10 to 25 inclusive) */
        TEST_ASSERT(val >= 10 && val <= 25, "Value should be in range [10, 25]");
    }

    /* Test range of size 256 (power of 2) */
    for (int i = 0; i < 100; i++) {
        long val = rt_random_static_int(0, 255);  /* Range size 256 */
        TEST_ASSERT(val >= 0 && val <= 255, "Value should be in range [0, 255]");
    }

    printf("  Power-of-two ranges work correctly\n");
}

void test_rt_random_static_int_large_range()
{
    printf("Testing rt_random_static_int with large ranges...\n");

    /* Test a very large range (close to max long range) */
    long min = -1000000000L;
    long max = 1000000000L;
    for (int i = 0; i < 100; i++) {
        long val = rt_random_static_int(min, max);
        TEST_ASSERT(val >= min && val <= max, "Value should be in large range");
    }

    /* Test positive large range */
    for (int i = 0; i < 100; i++) {
        long val = rt_random_static_int(0, 2000000000L);
        TEST_ASSERT(val >= 0 && val <= 2000000000L, "Value should be in positive large range");
    }

    printf("  Large ranges work correctly\n");
}

void test_rt_random_static_long_range()
{
    printf("Testing rt_random_static_long range validation...\n");

    /* Test basic range */
    for (int i = 0; i < 100; i++) {
        long long val = rt_random_static_long(1000000000LL, 2000000000LL);
        TEST_ASSERT(val >= 1000000000LL && val <= 2000000000LL, "Long should be in range");
    }

    /* Test inverted range */
    for (int i = 0; i < 100; i++) {
        long long val = rt_random_static_long(2000000000LL, 1000000000LL);
        TEST_ASSERT(val >= 1000000000LL && val <= 2000000000LL, "Inverted long range should work");
    }

    /* Test single value */
    long long single = rt_random_static_long(123456789012345LL, 123456789012345LL);
    TEST_ASSERT(single == 123456789012345LL, "Single value should return that value");

    printf("  Long range tests passed\n");
}

void test_rt_random_static_long_power_of_two_range()
{
    printf("Testing rt_random_static_long with power-of-two ranges...\n");

    /* Test range of size 2 (power of 2) */
    for (int i = 0; i < 100; i++) {
        long long val = rt_random_static_long(0, 1);  /* Range size 2 */
        TEST_ASSERT(val >= 0 && val <= 1, "Value should be in range [0, 1]");
    }

    /* Test range of size 4 (power of 2) */
    for (int i = 0; i < 100; i++) {
        long long val = rt_random_static_long(0, 3);  /* Range size 4 */
        TEST_ASSERT(val >= 0 && val <= 3, "Value should be in range [0, 3]");
    }

    /* Test large power-of-two range */
    for (int i = 0; i < 100; i++) {
        long long val = rt_random_static_long(0, (1LL << 32) - 1);  /* Range size 2^32 */
        TEST_ASSERT(val >= 0 && val <= (1LL << 32) - 1, "Value should be in large power-of-two range");
    }

    printf("  Power-of-two ranges for long work correctly\n");
}

void test_rt_random_static_long_large_range()
{
    printf("Testing rt_random_static_long with very large ranges...\n");

    /* Test very large 64-bit ranges */
    long long min = -4000000000000000000LL;
    long long max = 4000000000000000000LL;
    for (int i = 0; i < 100; i++) {
        long long val = rt_random_static_long(min, max);
        TEST_ASSERT(val >= min && val <= max, "Value should be in very large range");
    }

    printf("  Very large long ranges work correctly\n");
}

void test_rt_random_static_double_range()
{
    printf("Testing rt_random_static_double range validation...\n");

    /* Test basic range [0, 1) */
    for (int i = 0; i < 100; i++) {
        double val = rt_random_static_double(0.0, 1.0);
        TEST_ASSERT(val >= 0.0 && val < 1.0, "Double should be in [0, 1)");
    }

    /* Test custom range */
    for (int i = 0; i < 100; i++) {
        double val = rt_random_static_double(10.5, 20.5);
        TEST_ASSERT(val >= 10.5 && val < 20.5, "Double should be in [10.5, 20.5)");
    }

    /* Test inverted range */
    for (int i = 0; i < 100; i++) {
        double val = rt_random_static_double(20.5, 10.5);
        TEST_ASSERT(val >= 10.5 && val < 20.5, "Inverted double range should work");
    }

    /* Test single value */
    double single = rt_random_static_double(3.14159, 3.14159);
    TEST_ASSERT(single == 3.14159, "Single value should return that value");

    /* Test negative range */
    for (int i = 0; i < 100; i++) {
        double val = rt_random_static_double(-100.0, -50.0);
        TEST_ASSERT(val >= -100.0 && val < -50.0, "Negative double range should work");
    }

    printf("  Double range tests passed\n");
}

void test_rt_random_static_double_small_range()
{
    printf("Testing rt_random_static_double with very small ranges...\n");

    /* Test very small range to verify precision */
    double min = 0.0;
    double max = 0.0001;
    for (int i = 0; i < 100; i++) {
        double val = rt_random_static_double(min, max);
        TEST_ASSERT(val >= min && val < max, "Value should be in small range");
    }

    /* Test range around a specific value */
    min = 100.0;
    max = 100.0001;
    for (int i = 0; i < 100; i++) {
        double val = rt_random_static_double(min, max);
        TEST_ASSERT(val >= min && val < max, "Value should be in range around 100");
    }

    printf("  Very small double ranges work correctly\n");
}

void test_rt_random_static_double_large_range()
{
    printf("Testing rt_random_static_double with large ranges...\n");

    /* Test large range */
    double min = -1e15;
    double max = 1e15;
    for (int i = 0; i < 100; i++) {
        double val = rt_random_static_double(min, max);
        TEST_ASSERT(val >= min && val < max, "Value should be in large range");
    }

    printf("  Large double ranges work correctly\n");
}

void test_rt_random_static_bool()
{
    printf("Testing rt_random_static_bool...\n");

    int true_count = 0;
    int false_count = 0;
    int iterations = 10000;

    for (int i = 0; i < iterations; i++) {
        int val = rt_random_static_bool();
        TEST_ASSERT(val == 0 || val == 1, "Bool should be 0 or 1");
        if (val) true_count++;
        else false_count++;
    }

    /* Should be roughly 50/50 */
    int expected = iterations / 2;
    int tolerance = expected / 5;  /* 20% tolerance */

    int deviation = abs(true_count - expected);
    TEST_ASSERT(deviation < tolerance, "Bool distribution should be roughly 50/50");

    printf("  Bool distribution: true=%d, false=%d\n", true_count, false_count);
}

void test_rt_random_static_byte()
{
    printf("Testing rt_random_static_byte...\n");

    int byte_counts[256] = {0};
    int iterations = 25600;

    for (int i = 0; i < iterations; i++) {
        unsigned char val = rt_random_static_byte();
        byte_counts[val]++;
    }

    /* Count unique values */
    int unique = 0;
    for (int i = 0; i < 256; i++) {
        if (byte_counts[i] > 0) unique++;
    }

    /* Should see most byte values with 25600 samples */
    TEST_ASSERT(unique > 240, "Should see most byte values");

    printf("  Unique byte values: %d / 256\n", unique);
}

void test_rt_random_static_bytes()
{
    printf("Testing rt_random_static_bytes...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Test basic generation */
    unsigned char *buf = rt_random_static_bytes(arena, 32);
    TEST_ASSERT_NOT_NULL(buf, "Bytes buffer should be created");

    /* Verify not all zeros */
    int non_zero = 0;
    for (int i = 0; i < 32; i++) {
        if (buf[i] != 0) non_zero++;
    }
    TEST_ASSERT(non_zero > 0, "Bytes should contain non-zero values");

    /* Test NULL arena */
    unsigned char *null_buf = rt_random_static_bytes(NULL, 32);
    TEST_ASSERT(null_buf == NULL, "NULL arena should return NULL");

    /* Test zero count */
    unsigned char *zero_buf = rt_random_static_bytes(arena, 0);
    TEST_ASSERT(zero_buf == NULL, "Zero count should return NULL");

    /* Test negative count */
    unsigned char *neg_buf = rt_random_static_bytes(arena, -1);
    TEST_ASSERT(neg_buf == NULL, "Negative count should return NULL");

    printf("  Static bytes generation passed\n");
    rt_arena_destroy(arena);
}

void test_rt_random_static_gaussian()
{
    printf("Testing rt_random_static_gaussian (Box-Muller)...\n");

    double mean = 100.0;
    double stddev = 15.0;
    int iterations = 10000;

    double sum = 0.0;
    double sum_sq = 0.0;
    double min_val = 1e9;
    double max_val = -1e9;

    for (int i = 0; i < iterations; i++) {
        double val = rt_random_static_gaussian(mean, stddev);
        sum += val;
        sum_sq += val * val;
        if (val < min_val) min_val = val;
        if (val > max_val) max_val = val;
    }

    double actual_mean = sum / iterations;
    double variance = (sum_sq / iterations) - (actual_mean * actual_mean);
    double actual_stddev = sqrt(variance);

    /* Mean should be close to target */
    double mean_error = fabs(actual_mean - mean);
    TEST_ASSERT(mean_error < 1.0, "Mean should be close to target");

    /* Standard deviation should be close to target */
    double stddev_error = fabs(actual_stddev - stddev);
    TEST_ASSERT(stddev_error < 1.0, "Stddev should be close to target");

    printf("  Actual mean: %.2f (expected %.2f)\n", actual_mean, mean);
    printf("  Actual stddev: %.2f (expected %.2f)\n", actual_stddev, stddev);
    printf("  Range: [%.2f, %.2f]\n", min_val, max_val);
}

void test_rt_random_static_gaussian_zero_stddev()
{
    printf("Testing rt_random_static_gaussian with zero stddev...\n");

    /* Zero stddev should always return the mean */
    for (int i = 0; i < 100; i++) {
        double val = rt_random_static_gaussian(42.0, 0.0);
        TEST_ASSERT(val == 42.0, "Zero stddev should return mean");
    }

    printf("  Zero stddev returns mean\n");
}

void test_rt_random_static_gaussian_extreme_stddev()
{
    printf("Testing rt_random_static_gaussian with extreme stddev...\n");

    /* Test with very small stddev */
    double mean = 50.0;
    double stddev = 0.0001;
    for (int i = 0; i < 100; i++) {
        double val = rt_random_static_gaussian(mean, stddev);
        TEST_ASSERT(fabs(val - mean) < 1.0, "Value should be very close to mean");
    }

    /* Test with negative stddev (should still work - absolute value behavior) */
    /* Note: Implementation may vary - this tests current behavior */

    printf("  Static gaussian extreme stddev handled correctly\n");
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

void test_rt_random_static_main()
{
    printf("\n");
    printf("================================================\n");
    printf(" Runtime Random Static Tests\n");
    printf("================================================\n");

    /* Static int tests */
    test_rt_random_static_int_range();
    test_rt_random_static_int_distribution();
    test_rt_random_static_int_power_of_two_range();
    test_rt_random_static_int_large_range();

    /* Static long tests */
    test_rt_random_static_long_range();
    test_rt_random_static_long_power_of_two_range();
    test_rt_random_static_long_large_range();

    /* Static double tests */
    test_rt_random_static_double_range();
    test_rt_random_static_double_small_range();
    test_rt_random_static_double_large_range();

    /* Static bool/byte/bytes tests */
    test_rt_random_static_bool();
    test_rt_random_static_byte();
    test_rt_random_static_bytes();

    /* Static gaussian tests */
    test_rt_random_static_gaussian();
    test_rt_random_static_gaussian_zero_stddev();
    test_rt_random_static_gaussian_extreme_stddev();

    printf("------------------------------------------------\n");
    printf(" All runtime random static tests passed!\n");
    printf("================================================\n");
}
