// tests/unit/runtime/runtime_random_tests.c
// Tests for the runtime random number generation system

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "../../src/runtime/runtime_random.h"
#include "../../src/runtime/runtime_arena.h"
#include "../../src/runtime/runtime_array.h"
#include "../test_utils.h"
#include "../debug.h"

/* ============================================================================
 * rt_random_fill_entropy() Tests
 * ============================================================================
 * Tests for the core entropy function that uses OS-provided randomness.
 * ============================================================================ */

void test_rt_random_fill_entropy_basic()
{
    printf("Testing rt_random_fill_entropy basic functionality...\n");

    uint8_t buf[32];
    memset(buf, 0, sizeof(buf));

    rt_random_fill_entropy(buf, sizeof(buf));

    /* Check that at least some bytes changed from zero */
    int non_zero_count = 0;
    for (size_t i = 0; i < sizeof(buf); i++) {
        if (buf[i] != 0) {
            non_zero_count++;
        }
    }

    /* With 32 random bytes, probability of all zeros is (1/256)^32, essentially impossible */
    TEST_ASSERT(non_zero_count > 0, "Entropy should produce non-zero bytes");

    printf("  Non-zero bytes: %d / %zu\n", non_zero_count, sizeof(buf));
}

void test_rt_random_fill_entropy_different_calls()
{
    printf("Testing rt_random_fill_entropy produces different values...\n");

    uint8_t buf1[32], buf2[32];

    rt_random_fill_entropy(buf1, sizeof(buf1));
    rt_random_fill_entropy(buf2, sizeof(buf2));

    /* Two calls should produce different sequences */
    int same_bytes = 0;
    for (size_t i = 0; i < sizeof(buf1); i++) {
        if (buf1[i] == buf2[i]) {
            same_bytes++;
        }
    }

    /* With 32 random bytes, expected matching is 32/256 = 0.125 bytes on average */
    /* Allow some tolerance but they shouldn't all match */
    TEST_ASSERT(same_bytes < (int)sizeof(buf1), "Two calls should produce different values");

    printf("  Matching bytes between calls: %d / %zu\n", same_bytes, sizeof(buf1));
}

void test_rt_random_fill_entropy_small_buffer()
{
    printf("Testing rt_random_fill_entropy with small buffer...\n");

    uint8_t buf[1];
    buf[0] = 0;

    /* This should work without errors */
    rt_random_fill_entropy(buf, sizeof(buf));

    printf("  Single byte generated: 0x%02x\n", buf[0]);
}

void test_rt_random_fill_entropy_large_buffer()
{
    printf("Testing rt_random_fill_entropy with large buffer...\n");

    /* Test with a larger buffer (4KB) to ensure retry loop works */
    size_t size = 4096;
    uint8_t *buf = malloc(size);
    TEST_ASSERT_NOT_NULL(buf, "malloc should succeed");

    memset(buf, 0, size);

    rt_random_fill_entropy(buf, size);

    /* Count unique bytes to verify distribution */
    int byte_counts[256] = {0};
    for (size_t i = 0; i < size; i++) {
        byte_counts[buf[i]]++;
    }

    int unique_bytes = 0;
    for (int i = 0; i < 256; i++) {
        if (byte_counts[i] > 0) {
            unique_bytes++;
        }
    }

    /* With 4096 random bytes, we should see nearly all 256 possible byte values */
    TEST_ASSERT(unique_bytes > 200, "Large buffer should contain many unique byte values");

    printf("  Unique byte values in 4KB: %d / 256\n", unique_bytes);

    free(buf);
}

void test_rt_random_fill_entropy_null_buffer()
{
    printf("Testing rt_random_fill_entropy with NULL buffer...\n");

    /* Should handle NULL gracefully without crashing */
    rt_random_fill_entropy(NULL, 32);

    printf("  NULL buffer handled gracefully\n");
}

void test_rt_random_fill_entropy_zero_length()
{
    printf("Testing rt_random_fill_entropy with zero length...\n");

    uint8_t buf[4] = {0xAA, 0xBB, 0xCC, 0xDD};

    /* Should handle zero length without modifying buffer */
    rt_random_fill_entropy(buf, 0);

    /* Buffer should be unchanged */
    TEST_ASSERT(buf[0] == 0xAA, "Buffer should be unchanged with zero length");
    TEST_ASSERT(buf[1] == 0xBB, "Buffer should be unchanged with zero length");
    TEST_ASSERT(buf[2] == 0xCC, "Buffer should be unchanged with zero length");
    TEST_ASSERT(buf[3] == 0xDD, "Buffer should be unchanged with zero length");

    printf("  Zero length handled correctly\n");
}

void test_rt_random_fill_entropy_statistical_distribution()
{
    printf("Testing rt_random_fill_entropy statistical distribution...\n");

    /* Generate a large sample and check distribution */
    size_t size = 16384;
    uint8_t *buf = malloc(size);
    TEST_ASSERT_NOT_NULL(buf, "malloc should succeed");

    rt_random_fill_entropy(buf, size);

    /* Count bytes in each quarter (0-63, 64-127, 128-191, 192-255) */
    int quarters[4] = {0, 0, 0, 0};
    for (size_t i = 0; i < size; i++) {
        quarters[buf[i] / 64]++;
    }

    /* Each quarter should have roughly 1/4 of the bytes (25% ± some variance) */
    int expected = (int)size / 4;
    int tolerance = expected / 4;  /* Allow 25% deviation */

    for (int q = 0; q < 4; q++) {
        int deviation = abs(quarters[q] - expected);
        TEST_ASSERT(deviation < tolerance, "Distribution should be roughly uniform");
    }

    printf("  Quarter distribution: [%d, %d, %d, %d] (expected ~%d each)\n",
           quarters[0], quarters[1], quarters[2], quarters[3], expected);

    free(buf);
}

/* ============================================================================
 * Factory Method Tests
 * ============================================================================
 * Tests for rt_random_create() and rt_random_create_with_seed()
 * ============================================================================ */

void test_rt_random_create_with_seed_basic()
{
    printf("Testing rt_random_create_with_seed basic functionality...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "Random generator should be created");
    TEST_ASSERT(rng->is_seeded == 1, "Should be marked as seeded");

    /* Verify state is non-zero after seeding */
    int has_nonzero = (rng->state[0] != 0 || rng->state[1] != 0 ||
                       rng->state[2] != 0 || rng->state[3] != 0);
    TEST_ASSERT(has_nonzero, "State should be initialized (not all zeros)");

    printf("  Seeded generator created successfully\n");
    rt_arena_destroy(arena);
}

void test_rt_random_create_with_seed_deterministic()
{
    printf("Testing rt_random_create_with_seed determinism...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Create two generators with the same seed */
    RtRandom *rng1 = rt_random_create_with_seed(arena, 42);
    RtRandom *rng2 = rt_random_create_with_seed(arena, 42);

    TEST_ASSERT_NOT_NULL(rng1, "First generator should be created");
    TEST_ASSERT_NOT_NULL(rng2, "Second generator should be created");

    /* They should have identical state */
    TEST_ASSERT(rng1->state[0] == rng2->state[0], "State[0] should match");
    TEST_ASSERT(rng1->state[1] == rng2->state[1], "State[1] should match");
    TEST_ASSERT(rng1->state[2] == rng2->state[2], "State[2] should match");
    TEST_ASSERT(rng1->state[3] == rng2->state[3], "State[3] should match");

    printf("  Same seed produces identical state\n");
    rt_arena_destroy(arena);
}

void test_rt_random_create_with_seed_different_seeds()
{
    printf("Testing rt_random_create_with_seed with different seeds...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Create two generators with different seeds */
    RtRandom *rng1 = rt_random_create_with_seed(arena, 42);
    RtRandom *rng2 = rt_random_create_with_seed(arena, 43);

    TEST_ASSERT_NOT_NULL(rng1, "First generator should be created");
    TEST_ASSERT_NOT_NULL(rng2, "Second generator should be created");

    /* They should have different state */
    int all_same = (rng1->state[0] == rng2->state[0] &&
                    rng1->state[1] == rng2->state[1] &&
                    rng1->state[2] == rng2->state[2] &&
                    rng1->state[3] == rng2->state[3]);
    TEST_ASSERT(!all_same, "Different seeds should produce different states");

    printf("  Different seeds produce different states\n");
    rt_arena_destroy(arena);
}

void test_rt_random_create_with_seed_not_all_zeros()
{
    printf("Testing rt_random_create_with_seed handles zero seed...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Even a zero seed should produce non-zero state */
    RtRandom *rng = rt_random_create_with_seed(arena, 0);
    TEST_ASSERT_NOT_NULL(rng, "Generator should be created");

    int has_nonzero = (rng->state[0] != 0 || rng->state[1] != 0 ||
                       rng->state[2] != 0 || rng->state[3] != 0);
    TEST_ASSERT(has_nonzero, "Zero seed should still produce non-zero state");

    printf("  Zero seed handled correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_random_create_with_seed_state_advances()
{
    printf("Testing xoshiro256** state advancement...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "Generator should be created");

    /* Save initial state */
    uint64_t initial_state[4];
    initial_state[0] = rng->state[0];
    initial_state[1] = rng->state[1];
    initial_state[2] = rng->state[2];
    initial_state[3] = rng->state[3];

    /* Generate a value (this uses the internal xoshiro256_next) */
    long val = rt_random_int(rng, 0, 1000);
    (void)val;  /* Suppress unused warning */

    /* State should have changed */
    int state_changed = (rng->state[0] != initial_state[0] ||
                         rng->state[1] != initial_state[1] ||
                         rng->state[2] != initial_state[2] ||
                         rng->state[3] != initial_state[3]);
    TEST_ASSERT(state_changed, "State should advance after generating value");

    printf("  State advances correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_random_create_with_seed_statistical()
{
    printf("Testing xoshiro256** statistical properties...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 42);
    TEST_ASSERT_NOT_NULL(rng, "Generator should be created");

    /* Generate many values and check distribution */
    int count = 10000;
    int buckets[10] = {0};  /* 10 buckets for values 0-9 */

    for (int i = 0; i < count; i++) {
        long val = rt_random_int(rng, 0, 9);
        TEST_ASSERT(val >= 0 && val <= 9, "Value should be in range");
        buckets[val]++;
    }

    /* Each bucket should have roughly 1000 values (10000/10) */
    int expected = count / 10;
    int tolerance = expected / 3;  /* Allow 33% deviation */

    int all_within_tolerance = 1;
    for (int i = 0; i < 10; i++) {
        int deviation = abs(buckets[i] - expected);
        if (deviation >= tolerance) {
            all_within_tolerance = 0;
            printf("  Bucket %d: %d (deviation %d exceeds tolerance %d)\n",
                   i, buckets[i], deviation, tolerance);
        }
    }
    TEST_ASSERT(all_within_tolerance, "Distribution should be roughly uniform");

    printf("  Distribution is roughly uniform\n");
    rt_arena_destroy(arena);
}

void test_rt_random_create_os_entropy()
{
    printf("Testing rt_random_create (OS entropy mode)...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create(arena);
    TEST_ASSERT_NOT_NULL(rng, "Generator should be created");
    TEST_ASSERT(rng->is_seeded == 0, "Should be marked as OS entropy mode");

    /* State should be initialized from OS entropy (not all zeros) */
    int has_nonzero = (rng->state[0] != 0 || rng->state[1] != 0 ||
                       rng->state[2] != 0 || rng->state[3] != 0);
    TEST_ASSERT(has_nonzero, "State should be initialized from OS entropy");

    /* Generate some values and verify they're in range */
    long val1 = rt_random_int(rng, 1, 100);
    TEST_ASSERT(val1 >= 1 && val1 <= 100, "Value should be in range [1, 100]");

    double dval = rt_random_double(rng, 0.0, 1.0);
    TEST_ASSERT(dval >= 0.0 && dval < 1.0, "Double should be in range [0, 1)");

    int bval = rt_random_bool(rng);
    TEST_ASSERT(bval == 0 || bval == 1, "Bool should be 0 or 1");

    printf("  OS entropy generator working correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_random_create_null_arena()
{
    printf("Testing rt_random_create with NULL arena...\n");

    /* rt_random_create should return NULL when arena is NULL */
    RtRandom *rng = rt_random_create(NULL);
    TEST_ASSERT(rng == NULL, "rt_random_create(NULL) should return NULL");

    printf("  NULL arena handled correctly\n");
}

void test_rt_random_create_with_seed_null_arena()
{
    printf("Testing rt_random_create_with_seed with NULL arena...\n");

    /* rt_random_create_with_seed should return NULL when arena is NULL */
    RtRandom *rng = rt_random_create_with_seed(NULL, 12345);
    TEST_ASSERT(rng == NULL, "rt_random_create_with_seed(NULL, seed) should return NULL");

    printf("  NULL arena handled correctly\n");
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

void test_rt_random_int_power_of_two_range()
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

void test_rt_random_long_power_of_two_range()
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

void test_rt_random_int_large_range()
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

void test_rt_random_long_large_range()
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

void test_rt_random_double_small_range()
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

void test_rt_random_gaussian_extreme_stddev()
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

/* ============================================================================
 * Instance Value Generation Tests
 * ============================================================================ */

void test_rt_random_int_range()
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

void test_rt_random_long_range()
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

void test_rt_random_double_range()
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

void test_rt_random_bool_instance()
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

void test_rt_random_byte_instance()
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

void test_rt_random_bytes_instance()
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

void test_rt_random_gaussian_instance()
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

/* ============================================================================
 * Reproducibility Tests
 * ============================================================================ */

void test_rt_random_seeded_reproducibility()
{
    printf("Testing seeded generator reproducibility...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Create two generators with same seed */
    RtRandom *rng1 = rt_random_create_with_seed(arena, 42);
    RtRandom *rng2 = rt_random_create_with_seed(arena, 42);

    /* Generate sequences and verify they match */
    for (int i = 0; i < 100; i++) {
        long v1 = rt_random_int(rng1, 0, 1000000);
        long v2 = rt_random_int(rng2, 0, 1000000);
        TEST_ASSERT(v1 == v2, "Same seed should produce same sequence");
    }

    printf("  Seeded reproducibility verified\n");
    rt_arena_destroy(arena);
}

void test_rt_random_seeded_different_types_reproducibility()
{
    printf("Testing seeded generator reproducibility across types...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng1 = rt_random_create_with_seed(arena, 42);
    RtRandom *rng2 = rt_random_create_with_seed(arena, 42);

    /* Generate mixed types and verify they match */
    TEST_ASSERT(rt_random_int(rng1, 0, 100) == rt_random_int(rng2, 0, 100), "int should match");
    TEST_ASSERT(rt_random_bool(rng1) == rt_random_bool(rng2), "bool should match");
    TEST_ASSERT(rt_random_double(rng1, 0.0, 1.0) == rt_random_double(rng2, 0.0, 1.0), "double should match");
    TEST_ASSERT(rt_random_byte(rng1) == rt_random_byte(rng2), "byte should match");
    TEST_ASSERT(rt_random_long(rng1, 0, 1000000) == rt_random_long(rng2, 0, 1000000), "long should match");

    printf("  Cross-type reproducibility verified\n");
    rt_arena_destroy(arena);
}

void test_rt_random_seeded_bytes_reproducibility()
{
    printf("Testing seeded bytes reproducibility...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng1 = rt_random_create_with_seed(arena, 42);
    RtRandom *rng2 = rt_random_create_with_seed(arena, 42);

    unsigned char *buf1 = rt_random_bytes(arena, rng1, 32);
    unsigned char *buf2 = rt_random_bytes(arena, rng2, 32);

    TEST_ASSERT_NOT_NULL(buf1, "buf1 should be created");
    TEST_ASSERT_NOT_NULL(buf2, "buf2 should be created");

    /* Verify byte-by-byte match */
    for (int i = 0; i < 32; i++) {
        TEST_ASSERT(buf1[i] == buf2[i], "Bytes should match for same seed");
    }

    printf("  Seeded bytes reproducibility verified\n");
    rt_arena_destroy(arena);
}

void test_rt_random_seeded_gaussian_reproducibility()
{
    printf("Testing seeded gaussian reproducibility...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng1 = rt_random_create_with_seed(arena, 42);
    RtRandom *rng2 = rt_random_create_with_seed(arena, 42);

    for (int i = 0; i < 100; i++) {
        double v1 = rt_random_gaussian(rng1, 0.0, 1.0);
        double v2 = rt_random_gaussian(rng2, 0.0, 1.0);
        TEST_ASSERT(v1 == v2, "Gaussian should match for same seed");
    }

    printf("  Seeded gaussian reproducibility verified\n");
    rt_arena_destroy(arena);
}

/* ============================================================================
 * Static Batch Generation Tests
 * ============================================================================ */

void test_rt_random_static_int_many_count_and_range()
{
    printf("Testing rt_random_static_int_many count and range...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    long count = 1000;
    long min = 10;
    long max = 100;

    long *arr = rt_random_static_int_many(arena, min, max, count);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Verify all values are in range */
    for (long i = 0; i < count; i++) {
        TEST_ASSERT(arr[i] >= min && arr[i] <= max,
                   "All values should be in range [min, max]");
    }

    /* Verify distribution is reasonable */
    int range_size = (int)(max - min + 1);
    int *buckets = rt_arena_calloc(arena, (size_t)range_size, sizeof(int));
    for (long i = 0; i < count; i++) {
        buckets[arr[i] - min]++;
    }

    /* Each bucket should have some values (expect ~11 per bucket for 91 buckets, 1000 samples) */
    int empty_buckets = 0;
    for (int i = 0; i < range_size; i++) {
        if (buckets[i] == 0) empty_buckets++;
    }
    TEST_ASSERT(empty_buckets < range_size / 4, "Distribution should cover most of range");

    printf("  Generated %ld integers in [%ld, %ld]\n", count, min, max);
    rt_arena_destroy(arena);
}

void test_rt_random_static_int_many_null_arena()
{
    printf("Testing rt_random_static_int_many with NULL arena...\n");

    long *arr = rt_random_static_int_many(NULL, 0, 100, 10);
    TEST_ASSERT(arr == NULL, "NULL arena should return NULL");

    printf("  NULL arena handled correctly\n");
}

void test_rt_random_static_int_many_zero_count()
{
    printf("Testing rt_random_static_int_many with zero/negative count...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    long *arr1 = rt_random_static_int_many(arena, 0, 100, 0);
    TEST_ASSERT(arr1 == NULL, "Zero count should return NULL");

    long *arr2 = rt_random_static_int_many(arena, 0, 100, -5);
    TEST_ASSERT(arr2 == NULL, "Negative count should return NULL");

    printf("  Zero/negative count handled correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_random_static_long_many_count_and_range()
{
    printf("Testing rt_random_static_long_many count and range...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    long count = 500;
    long long min = 1000000000LL;
    long long max = 2000000000LL;

    long long *arr = rt_random_static_long_many(arena, min, max, count);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Verify all values are in range */
    for (long i = 0; i < count; i++) {
        TEST_ASSERT(arr[i] >= min && arr[i] <= max,
                   "All longs should be in range [min, max]");
    }

    printf("  Generated %ld longs in [%lld, %lld]\n", count, min, max);
    rt_arena_destroy(arena);
}

void test_rt_random_static_long_many_null_arena()
{
    printf("Testing rt_random_static_long_many with NULL arena...\n");

    long long *arr = rt_random_static_long_many(NULL, 0, 100, 10);
    TEST_ASSERT(arr == NULL, "NULL arena should return NULL");

    printf("  NULL arena handled correctly\n");
}

void test_rt_random_static_double_many_count_and_range()
{
    printf("Testing rt_random_static_double_many count and range...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    long count = 1000;
    double min = 0.0;
    double max = 1.0;

    double *arr = rt_random_static_double_many(arena, min, max, count);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Verify all values are in range [min, max) */
    for (long i = 0; i < count; i++) {
        TEST_ASSERT(arr[i] >= min && arr[i] < max,
                   "All doubles should be in range [min, max)");
    }

    /* Verify distribution - check mean is roughly 0.5 */
    double sum = 0.0;
    for (long i = 0; i < count; i++) {
        sum += arr[i];
    }
    double mean = sum / count;
    TEST_ASSERT(mean > 0.4 && mean < 0.6, "Mean should be approximately 0.5");

    printf("  Generated %ld doubles in [%.1f, %.1f), mean=%.3f\n", count, min, max, mean);
    rt_arena_destroy(arena);
}

void test_rt_random_static_double_many_null_arena()
{
    printf("Testing rt_random_static_double_many with NULL arena...\n");

    double *arr = rt_random_static_double_many(NULL, 0.0, 1.0, 10);
    TEST_ASSERT(arr == NULL, "NULL arena should return NULL");

    printf("  NULL arena handled correctly\n");
}

void test_rt_random_static_bool_many_count()
{
    printf("Testing rt_random_static_bool_many count and distribution...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    long count = 1000;
    int *arr = rt_random_static_bool_many(arena, count);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Verify all values are 0 or 1, count trues */
    int true_count = 0;
    for (long i = 0; i < count; i++) {
        TEST_ASSERT(arr[i] == 0 || arr[i] == 1, "All bools should be 0 or 1");
        if (arr[i]) true_count++;
    }

    /* Should be roughly 50/50 */
    TEST_ASSERT(true_count > 400 && true_count < 600,
               "Bool distribution should be roughly 50/50");

    printf("  Generated %ld bools, true=%d, false=%ld\n", count, true_count, count - true_count);
    rt_arena_destroy(arena);
}

void test_rt_random_static_bool_many_null_arena()
{
    printf("Testing rt_random_static_bool_many with NULL arena...\n");

    int *arr = rt_random_static_bool_many(NULL, 10);
    TEST_ASSERT(arr == NULL, "NULL arena should return NULL");

    printf("  NULL arena handled correctly\n");
}

void test_rt_random_static_gaussian_many_count_and_distribution()
{
    printf("Testing rt_random_static_gaussian_many count and distribution...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    long count = 10000;
    double mean = 100.0;
    double stddev = 15.0;

    double *arr = rt_random_static_gaussian_many(arena, mean, stddev, count);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Calculate actual mean and stddev */
    double sum = 0.0;
    double sum_sq = 0.0;
    for (long i = 0; i < count; i++) {
        sum += arr[i];
        sum_sq += arr[i] * arr[i];
    }

    double actual_mean = sum / count;
    double variance = (sum_sq / count) - (actual_mean * actual_mean);
    double actual_stddev = sqrt(variance);

    /* Mean should be close to target */
    double mean_error = fabs(actual_mean - mean);
    TEST_ASSERT(mean_error < 1.0, "Mean should be close to target");

    /* Standard deviation should be close to target */
    double stddev_error = fabs(actual_stddev - stddev);
    TEST_ASSERT(stddev_error < 1.0, "Stddev should be close to target");

    printf("  Generated %ld gaussians: mean=%.2f (expected %.2f), stddev=%.2f (expected %.2f)\n",
           count, actual_mean, mean, actual_stddev, stddev);
    rt_arena_destroy(arena);
}

void test_rt_random_static_gaussian_many_null_arena()
{
    printf("Testing rt_random_static_gaussian_many with NULL arena...\n");

    double *arr = rt_random_static_gaussian_many(NULL, 0.0, 1.0, 10);
    TEST_ASSERT(arr == NULL, "NULL arena should return NULL");

    printf("  NULL arena handled correctly\n");
}

/* ============================================================================
 * Instance Batch Generation Tests
 * ============================================================================ */

void test_rt_random_int_many_count_and_range()
{
    printf("Testing rt_random_int_many count and range...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    long count = 1000;
    long min = 10;
    long max = 100;

    long *arr = rt_random_int_many(arena, rng, min, max, count);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Verify all values are in range */
    for (long i = 0; i < count; i++) {
        TEST_ASSERT(arr[i] >= min && arr[i] <= max,
                   "All values should be in range [min, max]");
    }

    printf("  Generated %ld integers in [%ld, %ld]\n", count, min, max);
    rt_arena_destroy(arena);
}

void test_rt_random_int_many_null_args()
{
    printf("Testing rt_random_int_many with NULL args...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    long *arr1 = rt_random_int_many(NULL, rng, 0, 100, 10);
    TEST_ASSERT(arr1 == NULL, "NULL arena should return NULL");

    long *arr2 = rt_random_int_many(arena, NULL, 0, 100, 10);
    TEST_ASSERT(arr2 == NULL, "NULL rng should return NULL");

    printf("  NULL args handled correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_random_int_many_reproducibility()
{
    printf("Testing rt_random_int_many reproducibility...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng1 = rt_random_create_with_seed(arena, 42);
    RtRandom *rng2 = rt_random_create_with_seed(arena, 42);

    long count = 100;
    long *arr1 = rt_random_int_many(arena, rng1, 0, 1000, count);
    long *arr2 = rt_random_int_many(arena, rng2, 0, 1000, count);

    TEST_ASSERT_NOT_NULL(arr1, "arr1 should be created");
    TEST_ASSERT_NOT_NULL(arr2, "arr2 should be created");

    /* Same seed should produce identical arrays */
    for (long i = 0; i < count; i++) {
        TEST_ASSERT(arr1[i] == arr2[i], "Same seed should produce identical arrays");
    }

    printf("  Reproducibility verified for int_many\n");
    rt_arena_destroy(arena);
}

void test_rt_random_long_many_count_and_range()
{
    printf("Testing rt_random_long_many count and range...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    long count = 500;
    long long min = 1000000000LL;
    long long max = 2000000000LL;

    long long *arr = rt_random_long_many(arena, rng, min, max, count);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Verify all values are in range */
    for (long i = 0; i < count; i++) {
        TEST_ASSERT(arr[i] >= min && arr[i] <= max,
                   "All longs should be in range [min, max]");
    }

    printf("  Generated %ld longs in [%lld, %lld]\n", count, min, max);
    rt_arena_destroy(arena);
}

void test_rt_random_long_many_null_args()
{
    printf("Testing rt_random_long_many with NULL args...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    long long *arr1 = rt_random_long_many(NULL, rng, 0, 100, 10);
    TEST_ASSERT(arr1 == NULL, "NULL arena should return NULL");

    long long *arr2 = rt_random_long_many(arena, NULL, 0, 100, 10);
    TEST_ASSERT(arr2 == NULL, "NULL rng should return NULL");

    printf("  NULL args handled correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_random_long_many_reproducibility()
{
    printf("Testing rt_random_long_many reproducibility...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng1 = rt_random_create_with_seed(arena, 42);
    RtRandom *rng2 = rt_random_create_with_seed(arena, 42);

    long count = 100;
    long long *arr1 = rt_random_long_many(arena, rng1, 0, 1000000000LL, count);
    long long *arr2 = rt_random_long_many(arena, rng2, 0, 1000000000LL, count);

    TEST_ASSERT_NOT_NULL(arr1, "arr1 should be created");
    TEST_ASSERT_NOT_NULL(arr2, "arr2 should be created");

    for (long i = 0; i < count; i++) {
        TEST_ASSERT(arr1[i] == arr2[i], "Same seed should produce identical arrays");
    }

    printf("  Reproducibility verified for long_many\n");
    rt_arena_destroy(arena);
}

void test_rt_random_double_many_count_and_range()
{
    printf("Testing rt_random_double_many count and range...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    long count = 1000;
    double min = 0.0;
    double max = 1.0;

    double *arr = rt_random_double_many(arena, rng, min, max, count);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Verify all values are in range [min, max) */
    for (long i = 0; i < count; i++) {
        TEST_ASSERT(arr[i] >= min && arr[i] < max,
                   "All doubles should be in range [min, max)");
    }

    printf("  Generated %ld doubles in [%.1f, %.1f)\n", count, min, max);
    rt_arena_destroy(arena);
}

void test_rt_random_double_many_null_args()
{
    printf("Testing rt_random_double_many with NULL args...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    double *arr1 = rt_random_double_many(NULL, rng, 0.0, 1.0, 10);
    TEST_ASSERT(arr1 == NULL, "NULL arena should return NULL");

    double *arr2 = rt_random_double_many(arena, NULL, 0.0, 1.0, 10);
    TEST_ASSERT(arr2 == NULL, "NULL rng should return NULL");

    printf("  NULL args handled correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_random_double_many_reproducibility()
{
    printf("Testing rt_random_double_many reproducibility...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng1 = rt_random_create_with_seed(arena, 42);
    RtRandom *rng2 = rt_random_create_with_seed(arena, 42);

    long count = 100;
    double *arr1 = rt_random_double_many(arena, rng1, 0.0, 1.0, count);
    double *arr2 = rt_random_double_many(arena, rng2, 0.0, 1.0, count);

    TEST_ASSERT_NOT_NULL(arr1, "arr1 should be created");
    TEST_ASSERT_NOT_NULL(arr2, "arr2 should be created");

    for (long i = 0; i < count; i++) {
        TEST_ASSERT(arr1[i] == arr2[i], "Same seed should produce identical arrays");
    }

    printf("  Reproducibility verified for double_many\n");
    rt_arena_destroy(arena);
}

void test_rt_random_bool_many_count()
{
    printf("Testing rt_random_bool_many count and distribution...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    long count = 1000;
    int *arr = rt_random_bool_many(arena, rng, count);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Verify all values are 0 or 1, count trues */
    int true_count = 0;
    for (long i = 0; i < count; i++) {
        TEST_ASSERT(arr[i] == 0 || arr[i] == 1, "All bools should be 0 or 1");
        if (arr[i]) true_count++;
    }

    /* Should be roughly 50/50 */
    TEST_ASSERT(true_count > 400 && true_count < 600,
               "Bool distribution should be roughly 50/50");

    printf("  Generated %ld bools, true=%d, false=%ld\n", count, true_count, count - true_count);
    rt_arena_destroy(arena);
}

void test_rt_random_bool_many_null_args()
{
    printf("Testing rt_random_bool_many with NULL args...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    int *arr1 = rt_random_bool_many(NULL, rng, 10);
    TEST_ASSERT(arr1 == NULL, "NULL arena should return NULL");

    int *arr2 = rt_random_bool_many(arena, NULL, 10);
    TEST_ASSERT(arr2 == NULL, "NULL rng should return NULL");

    printf("  NULL args handled correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_random_bool_many_reproducibility()
{
    printf("Testing rt_random_bool_many reproducibility...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng1 = rt_random_create_with_seed(arena, 42);
    RtRandom *rng2 = rt_random_create_with_seed(arena, 42);

    long count = 100;
    int *arr1 = rt_random_bool_many(arena, rng1, count);
    int *arr2 = rt_random_bool_many(arena, rng2, count);

    TEST_ASSERT_NOT_NULL(arr1, "arr1 should be created");
    TEST_ASSERT_NOT_NULL(arr2, "arr2 should be created");

    for (long i = 0; i < count; i++) {
        TEST_ASSERT(arr1[i] == arr2[i], "Same seed should produce identical arrays");
    }

    printf("  Reproducibility verified for bool_many\n");
    rt_arena_destroy(arena);
}

void test_rt_random_gaussian_many_count_and_distribution()
{
    printf("Testing rt_random_gaussian_many count and distribution...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    long count = 10000;
    double mean = 0.0;
    double stddev = 1.0;

    double *arr = rt_random_gaussian_many(arena, rng, mean, stddev, count);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Calculate actual mean and stddev */
    double sum = 0.0;
    double sum_sq = 0.0;
    for (long i = 0; i < count; i++) {
        sum += arr[i];
        sum_sq += arr[i] * arr[i];
    }

    double actual_mean = sum / count;
    double variance = (sum_sq / count) - (actual_mean * actual_mean);
    double actual_stddev = sqrt(variance);

    /* Standard normal should have mean ~0 and stddev ~1 */
    TEST_ASSERT(fabs(actual_mean) < 0.1, "Mean should be close to 0");
    TEST_ASSERT(fabs(actual_stddev - 1.0) < 0.1, "Stddev should be close to 1");

    printf("  Generated %ld gaussians: mean=%.3f, stddev=%.3f\n",
           count, actual_mean, actual_stddev);
    rt_arena_destroy(arena);
}

void test_rt_random_gaussian_many_null_args()
{
    printf("Testing rt_random_gaussian_many with NULL args...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    double *arr1 = rt_random_gaussian_many(NULL, rng, 0.0, 1.0, 10);
    TEST_ASSERT(arr1 == NULL, "NULL arena should return NULL");

    double *arr2 = rt_random_gaussian_many(arena, NULL, 0.0, 1.0, 10);
    TEST_ASSERT(arr2 == NULL, "NULL rng should return NULL");

    printf("  NULL args handled correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_random_gaussian_many_reproducibility()
{
    printf("Testing rt_random_gaussian_many reproducibility...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng1 = rt_random_create_with_seed(arena, 42);
    RtRandom *rng2 = rt_random_create_with_seed(arena, 42);

    long count = 100;
    double *arr1 = rt_random_gaussian_many(arena, rng1, 0.0, 1.0, count);
    double *arr2 = rt_random_gaussian_many(arena, rng2, 0.0, 1.0, count);

    TEST_ASSERT_NOT_NULL(arr1, "arr1 should be created");
    TEST_ASSERT_NOT_NULL(arr2, "arr2 should be created");

    for (long i = 0; i < count; i++) {
        TEST_ASSERT(arr1[i] == arr2[i], "Same seed should produce identical arrays");
    }

    printf("  Reproducibility verified for gaussian_many\n");
    rt_arena_destroy(arena);
}

/* ============================================================================
 * Performance Tests for Large Batches
 * ============================================================================ */

void test_rt_random_batch_large_count()
{
    printf("Testing batch generation with large counts...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    /* Generate 100,000 values of each type */
    long large_count = 100000;

    long *ints = rt_random_int_many(arena, rng, 0, 1000000, large_count);
    TEST_ASSERT_NOT_NULL(ints, "Large int array should be created");

    long long *longs = rt_random_long_many(arena, rng, 0, 1000000000LL, large_count);
    TEST_ASSERT_NOT_NULL(longs, "Large long array should be created");

    double *doubles = rt_random_double_many(arena, rng, 0.0, 1.0, large_count);
    TEST_ASSERT_NOT_NULL(doubles, "Large double array should be created");

    int *bools = rt_random_bool_many(arena, rng, large_count);
    TEST_ASSERT_NOT_NULL(bools, "Large bool array should be created");

    double *gaussians = rt_random_gaussian_many(arena, rng, 0.0, 1.0, large_count);
    TEST_ASSERT_NOT_NULL(gaussians, "Large gaussian array should be created");

    /* Verify a few random samples are in range */
    TEST_ASSERT(ints[0] >= 0 && ints[0] <= 1000000, "First int in range");
    TEST_ASSERT(ints[large_count-1] >= 0 && ints[large_count-1] <= 1000000, "Last int in range");

    TEST_ASSERT(longs[0] >= 0 && longs[0] <= 1000000000LL, "First long in range");
    TEST_ASSERT(longs[large_count-1] >= 0 && longs[large_count-1] <= 1000000000LL, "Last long in range");

    TEST_ASSERT(doubles[0] >= 0.0 && doubles[0] < 1.0, "First double in range");
    TEST_ASSERT(doubles[large_count-1] >= 0.0 && doubles[large_count-1] < 1.0, "Last double in range");

    TEST_ASSERT(bools[0] == 0 || bools[0] == 1, "First bool valid");
    TEST_ASSERT(bools[large_count-1] == 0 || bools[large_count-1] == 1, "Last bool valid");

    printf("  Successfully generated %ld values of each type\n", large_count);
    rt_arena_destroy(arena);
}

/* ============================================================================
 * Static Choice Tests
 * ============================================================================ */

void test_rt_random_static_choice_long_basic()
{
    printf("Testing rt_random_static_choice_long basic functionality...\n");

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

    printf("  Static choice_long returns elements from array\n");
}

void test_rt_random_static_choice_long_single_element()
{
    printf("Testing rt_random_static_choice_long with single element...\n");

    long arr[] = {42};
    for (int i = 0; i < 10; i++) {
        long val = rt_random_static_choice_long(arr, 1);
        TEST_ASSERT(val == 42, "Single element should always return that element");
    }

    printf("  Single element array always returns that element\n");
}

void test_rt_random_static_choice_long_null_empty()
{
    printf("Testing rt_random_static_choice_long with NULL/empty...\n");

    long arr[] = {1, 2, 3};

    /* NULL array should return 0 */
    long val1 = rt_random_static_choice_long(NULL, 3);
    TEST_ASSERT(val1 == 0, "NULL array should return 0");

    /* Empty array (len <= 0) should return 0 */
    long val2 = rt_random_static_choice_long(arr, 0);
    TEST_ASSERT(val2 == 0, "Empty array should return 0");

    long val3 = rt_random_static_choice_long(arr, -1);
    TEST_ASSERT(val3 == 0, "Negative length should return 0");

    printf("  NULL/empty handling correct\n");
}

void test_rt_random_static_choice_long_distribution()
{
    printf("Testing rt_random_static_choice_long distribution...\n");

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

    printf("  Distribution: [%d, %d, %d, %d, %d] (expected ~%d each)\n",
           counts[0], counts[1], counts[2], counts[3], counts[4], expected);
}

void test_rt_random_static_choice_double_basic()
{
    printf("Testing rt_random_static_choice_double basic functionality...\n");

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

    printf("  Static choice_double returns elements from array\n");
}

void test_rt_random_static_choice_double_null_empty()
{
    printf("Testing rt_random_static_choice_double with NULL/empty...\n");

    double arr[] = {1.0, 2.0, 3.0};

    double val1 = rt_random_static_choice_double(NULL, 3);
    TEST_ASSERT(val1 == 0.0, "NULL array should return 0.0");

    double val2 = rt_random_static_choice_double(arr, 0);
    TEST_ASSERT(val2 == 0.0, "Empty array should return 0.0");

    printf("  NULL/empty handling correct\n");
}

void test_rt_random_static_choice_string_basic()
{
    printf("Testing rt_random_static_choice_string basic functionality...\n");

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

    printf("  Static choice_string returns elements from array\n");
}

void test_rt_random_static_choice_string_null_empty()
{
    printf("Testing rt_random_static_choice_string with NULL/empty...\n");

    char *arr[] = {"a", "b", "c"};

    char *val1 = rt_random_static_choice_string(NULL, 3);
    TEST_ASSERT(val1 == NULL, "NULL array should return NULL");

    char *val2 = rt_random_static_choice_string(arr, 0);
    TEST_ASSERT(val2 == NULL, "Empty array should return NULL");

    printf("  NULL/empty handling correct\n");
}

void test_rt_random_static_choice_bool_basic()
{
    printf("Testing rt_random_static_choice_bool basic functionality...\n");

    int arr[] = {0, 1, 0, 1, 1};
    long len = 5;

    for (int i = 0; i < 100; i++) {
        int val = rt_random_static_choice_bool(arr, len);
        TEST_ASSERT(val == 0 || val == 1, "Choice should be 0 or 1");
    }

    printf("  Static choice_bool returns valid booleans\n");
}

void test_rt_random_static_choice_bool_null_empty()
{
    printf("Testing rt_random_static_choice_bool with NULL/empty...\n");

    int arr[] = {1, 0, 1};

    int val1 = rt_random_static_choice_bool(NULL, 3);
    TEST_ASSERT(val1 == 0, "NULL array should return 0");

    int val2 = rt_random_static_choice_bool(arr, 0);
    TEST_ASSERT(val2 == 0, "Empty array should return 0");

    printf("  NULL/empty handling correct\n");
}

void test_rt_random_static_choice_byte_basic()
{
    printf("Testing rt_random_static_choice_byte basic functionality...\n");

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

    printf("  Static choice_byte returns elements from array\n");
}

void test_rt_random_static_choice_byte_null_empty()
{
    printf("Testing rt_random_static_choice_byte with NULL/empty...\n");

    unsigned char arr[] = {0xAA, 0xBB, 0xCC};

    unsigned char val1 = rt_random_static_choice_byte(NULL, 3);
    TEST_ASSERT(val1 == 0, "NULL array should return 0");

    unsigned char val2 = rt_random_static_choice_byte(arr, 0);
    TEST_ASSERT(val2 == 0, "Empty array should return 0");

    printf("  NULL/empty handling correct\n");
}

/* ============================================================================
 * Instance Choice Tests
 * ============================================================================ */

void test_rt_random_choice_long_basic()
{
    printf("Testing rt_random_choice_long basic functionality...\n");

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

    printf("  Instance choice_long returns elements from array\n");
    rt_arena_destroy(arena);
}

void test_rt_random_choice_long_reproducibility()
{
    printf("Testing rt_random_choice_long reproducibility...\n");

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

    printf("  Reproducibility verified for choice_long\n");
    rt_arena_destroy(arena);
}

void test_rt_random_choice_long_null_args()
{
    printf("Testing rt_random_choice_long with NULL args...\n");

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

    printf("  NULL args handling correct\n");
    rt_arena_destroy(arena);
}

void test_rt_random_choice_long_distribution()
{
    printf("Testing rt_random_choice_long distribution...\n");

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

    printf("  Distribution: [%d, %d, %d, %d, %d] (expected ~%d each)\n",
           counts[0], counts[1], counts[2], counts[3], counts[4], expected);
    rt_arena_destroy(arena);
}

void test_rt_random_choice_double_basic()
{
    printf("Testing rt_random_choice_double basic functionality...\n");

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

    printf("  Instance choice_double returns elements from array\n");
    rt_arena_destroy(arena);
}

void test_rt_random_choice_double_null_args()
{
    printf("Testing rt_random_choice_double with NULL args...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    double arr[] = {1.0, 2.0, 3.0};

    double val1 = rt_random_choice_double(NULL, arr, 3);
    TEST_ASSERT(val1 == 0.0, "NULL rng should return 0.0");

    double val2 = rt_random_choice_double(rng, NULL, 3);
    TEST_ASSERT(val2 == 0.0, "NULL array should return 0.0");

    printf("  NULL args handling correct\n");
    rt_arena_destroy(arena);
}

void test_rt_random_choice_string_basic()
{
    printf("Testing rt_random_choice_string basic functionality...\n");

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

    printf("  Instance choice_string returns elements from array\n");
    rt_arena_destroy(arena);
}

void test_rt_random_choice_string_null_args()
{
    printf("Testing rt_random_choice_string with NULL args...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    char *arr[] = {"a", "b", "c"};

    char *val1 = rt_random_choice_string(NULL, arr, 3);
    TEST_ASSERT(val1 == NULL, "NULL rng should return NULL");

    char *val2 = rt_random_choice_string(rng, NULL, 3);
    TEST_ASSERT(val2 == NULL, "NULL array should return NULL");

    printf("  NULL args handling correct\n");
    rt_arena_destroy(arena);
}

void test_rt_random_choice_bool_basic()
{
    printf("Testing rt_random_choice_bool basic functionality...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    int arr[] = {0, 1, 0, 1, 1};
    long len = 5;

    for (int i = 0; i < 100; i++) {
        int val = rt_random_choice_bool(rng, arr, len);
        TEST_ASSERT(val == 0 || val == 1, "Choice should be 0 or 1");
    }

    printf("  Instance choice_bool returns valid booleans\n");
    rt_arena_destroy(arena);
}

void test_rt_random_choice_bool_null_args()
{
    printf("Testing rt_random_choice_bool with NULL args...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    int arr[] = {1, 0, 1};

    int val1 = rt_random_choice_bool(NULL, arr, 3);
    TEST_ASSERT(val1 == 0, "NULL rng should return 0");

    int val2 = rt_random_choice_bool(rng, NULL, 3);
    TEST_ASSERT(val2 == 0, "NULL array should return 0");

    printf("  NULL args handling correct\n");
    rt_arena_destroy(arena);
}

void test_rt_random_choice_byte_basic()
{
    printf("Testing rt_random_choice_byte basic functionality...\n");

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

    printf("  Instance choice_byte returns elements from array\n");
    rt_arena_destroy(arena);
}

void test_rt_random_choice_byte_null_args()
{
    printf("Testing rt_random_choice_byte with NULL args...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    unsigned char arr[] = {0xAA, 0xBB, 0xCC};

    unsigned char val1 = rt_random_choice_byte(NULL, arr, 3);
    TEST_ASSERT(val1 == 0, "NULL rng should return 0");

    unsigned char val2 = rt_random_choice_byte(rng, NULL, 3);
    TEST_ASSERT(val2 == 0, "NULL array should return 0");

    printf("  NULL args handling correct\n");
    rt_arena_destroy(arena);
}

/* ============================================================================
 * Statistical Distribution Tests for Choice Functions
 * ============================================================================ */

void test_rt_random_static_choice_double_distribution()
{
    printf("Testing rt_random_static_choice_double distribution...\n");

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

    printf("  Distribution: [%d, %d, %d, %d] (expected ~%d each)\n",
           counts[0], counts[1], counts[2], counts[3], expected);
}

void test_rt_random_static_choice_string_distribution()
{
    printf("Testing rt_random_static_choice_string distribution...\n");

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

    printf("  Distribution: [%d, %d, %d, %d] (expected ~%d each)\n",
           counts[0], counts[1], counts[2], counts[3], expected);
}

void test_rt_random_static_choice_byte_distribution()
{
    printf("Testing rt_random_static_choice_byte distribution...\n");

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

    printf("  Distribution: [%d, %d, %d, %d] (expected ~%d each)\n",
           counts[0], counts[1], counts[2], counts[3], expected);
}

void test_rt_random_choice_double_distribution()
{
    printf("Testing rt_random_choice_double distribution...\n");

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

    printf("  Distribution: [%d, %d, %d, %d] (expected ~%d each)\n",
           counts[0], counts[1], counts[2], counts[3], expected);
    rt_arena_destroy(arena);
}

void test_rt_random_choice_string_distribution()
{
    printf("Testing rt_random_choice_string distribution...\n");

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

    printf("  Distribution: [%d, %d, %d, %d] (expected ~%d each)\n",
           counts[0], counts[1], counts[2], counts[3], expected);
    rt_arena_destroy(arena);
}

void test_rt_random_choice_byte_distribution()
{
    printf("Testing rt_random_choice_byte distribution...\n");

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

    printf("  Distribution: [%d, %d, %d, %d] (expected ~%d each)\n",
           counts[0], counts[1], counts[2], counts[3], expected);
    rt_arena_destroy(arena);
}

/* ============================================================================
 * Weight Validation Helper Tests
 * ============================================================================
 * Tests for rt_random_validate_weights() function.
 * ============================================================================ */

void test_rt_random_validate_weights_valid()
{
    printf("Testing rt_random_validate_weights with valid weights...\n");

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

    printf("  Valid weights correctly accepted\n");
}

void test_rt_random_validate_weights_negative()
{
    printf("Testing rt_random_validate_weights with negative weights...\n");

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

    printf("  Negative weights correctly rejected\n");
}

void test_rt_random_validate_weights_zero()
{
    printf("Testing rt_random_validate_weights with zero weights...\n");

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

    printf("  Zero weights correctly rejected\n");
}

void test_rt_random_validate_weights_empty()
{
    printf("Testing rt_random_validate_weights with empty array...\n");

    double weights[] = {1.0, 2.0, 3.0};  /* dummy, won't be accessed */

    /* Zero length */
    TEST_ASSERT(rt_random_validate_weights(weights, 0) == 0, "Zero length should fail");

    /* Negative length */
    TEST_ASSERT(rt_random_validate_weights(weights, -1) == 0, "Negative length should fail");

    printf("  Empty array correctly rejected\n");
}

void test_rt_random_validate_weights_null()
{
    printf("Testing rt_random_validate_weights with NULL pointer...\n");

    TEST_ASSERT(rt_random_validate_weights(NULL, 3) == 0, "NULL pointer should fail");
    TEST_ASSERT(rt_random_validate_weights(NULL, 0) == 0, "NULL with zero length should fail");

    printf("  NULL pointer correctly rejected\n");
}

/* ============================================================================
 * Cumulative Distribution Helper Tests
 * ============================================================================
 * Tests for rt_random_build_cumulative() function.
 * ============================================================================ */

void test_rt_random_build_cumulative_basic()
{
    printf("Testing rt_random_build_cumulative basic functionality...\n");

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

    printf("  Cumulative: [%.4f, %.4f, %.4f]\n", cumulative[0], cumulative[1], cumulative[2]);

    rt_arena_destroy(arena);
}

void test_rt_random_build_cumulative_normalization()
{
    printf("Testing rt_random_build_cumulative normalizes weights...\n");

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

    printf("  Normalized cumulative: [%.4f, %.4f, %.4f]\n", cumulative[0], cumulative[1], cumulative[2]);

    rt_arena_destroy(arena);
}

void test_rt_random_build_cumulative_single_element()
{
    printf("Testing rt_random_build_cumulative with single element...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Single element should produce cumulative [1.0] */
    double weights[] = {5.0};
    double *cumulative = rt_random_build_cumulative(arena, weights, 1);

    TEST_ASSERT_NOT_NULL(cumulative, "Cumulative array should be created");
    TEST_ASSERT(cumulative[0] == 1.0, "Single element cumulative should be 1.0");

    printf("  Single element cumulative: [%.4f]\n", cumulative[0]);

    rt_arena_destroy(arena);
}

void test_rt_random_build_cumulative_two_elements()
{
    printf("Testing rt_random_build_cumulative with two elements...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Two equal weights */
    double weights[] = {1.0, 1.0};
    double *cumulative = rt_random_build_cumulative(arena, weights, 2);

    TEST_ASSERT_NOT_NULL(cumulative, "Cumulative array should be created");
    TEST_ASSERT(fabs(cumulative[0] - 0.5) < 0.0001, "First cumulative should be ~0.5");
    TEST_ASSERT(cumulative[1] == 1.0, "Second cumulative should be exactly 1.0");

    printf("  Two element cumulative: [%.4f, %.4f]\n", cumulative[0], cumulative[1]);

    rt_arena_destroy(arena);
}

void test_rt_random_build_cumulative_null_arena()
{
    printf("Testing rt_random_build_cumulative with NULL arena...\n");

    double weights[] = {1.0, 2.0, 3.0};
    double *cumulative = rt_random_build_cumulative(NULL, weights, 3);

    TEST_ASSERT(cumulative == NULL, "Should return NULL with NULL arena");

    printf("  NULL arena correctly rejected\n");
}

void test_rt_random_build_cumulative_null_weights()
{
    printf("Testing rt_random_build_cumulative with NULL weights...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    double *cumulative = rt_random_build_cumulative(arena, NULL, 3);

    TEST_ASSERT(cumulative == NULL, "Should return NULL with NULL weights");

    printf("  NULL weights correctly rejected\n");

    rt_arena_destroy(arena);
}

void test_rt_random_build_cumulative_empty_array()
{
    printf("Testing rt_random_build_cumulative with empty array...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    double weights[] = {1.0, 2.0, 3.0};  /* Dummy, won't be accessed */

    /* Zero length */
    double *cumulative1 = rt_random_build_cumulative(arena, weights, 0);
    TEST_ASSERT(cumulative1 == NULL, "Should return NULL with zero length");

    /* Negative length */
    double *cumulative2 = rt_random_build_cumulative(arena, weights, -1);
    TEST_ASSERT(cumulative2 == NULL, "Should return NULL with negative length");

    printf("  Empty array correctly rejected\n");

    rt_arena_destroy(arena);
}

void test_rt_random_build_cumulative_large_weights()
{
    printf("Testing rt_random_build_cumulative with large weights...\n");

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

    printf("  Large weights cumulative: [%.4f, %.4f, %.4f, %.4f]\n",
           cumulative[0], cumulative[1], cumulative[2], cumulative[3]);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Weighted Index Selection Helper Tests
 * ============================================================================
 * Tests for rt_random_select_weighted_index() function.
 * ============================================================================ */

void test_rt_random_select_weighted_index_basic()
{
    printf("Testing rt_random_select_weighted_index basic functionality...\n");

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

    printf("  Basic selection works correctly\n");
}

void test_rt_random_select_weighted_index_edge_zero()
{
    printf("Testing rt_random_select_weighted_index with 0.0...\n");

    double cumulative[] = {0.25, 0.5, 0.75, 1.0};
    long len = 4;

    /* Value 0.0 should always select first element */
    TEST_ASSERT(rt_random_select_weighted_index(0.0, cumulative, len) == 0, "0.0 should select index 0");

    /* Negative value should also select first element (safety) */
    TEST_ASSERT(rt_random_select_weighted_index(-0.1, cumulative, len) == 0, "Negative should select index 0");

    printf("  Edge value 0.0 handled correctly\n");
}

void test_rt_random_select_weighted_index_edge_near_one()
{
    printf("Testing rt_random_select_weighted_index with values near 1.0...\n");

    double cumulative[] = {0.25, 0.5, 0.75, 1.0};
    long len = 4;

    /* Values very close to 1.0 should select last element */
    TEST_ASSERT(rt_random_select_weighted_index(0.9999, cumulative, len) == 3, "0.9999 should select index 3");
    TEST_ASSERT(rt_random_select_weighted_index(0.999999, cumulative, len) == 3, "0.999999 should select index 3");

    /* Value exactly 1.0 should select last element (edge case) */
    TEST_ASSERT(rt_random_select_weighted_index(1.0, cumulative, len) == 3, "1.0 should select index 3");

    /* Values > 1.0 should select last element (safety) */
    TEST_ASSERT(rt_random_select_weighted_index(1.5, cumulative, len) == 3, ">1.0 should select index 3");

    printf("  Edge values near 1.0 handled correctly\n");
}

void test_rt_random_select_weighted_index_single_element()
{
    printf("Testing rt_random_select_weighted_index with single element...\n");

    double cumulative[] = {1.0};
    long len = 1;

    /* Any value should return index 0 */
    TEST_ASSERT(rt_random_select_weighted_index(0.0, cumulative, len) == 0, "0.0 should select index 0");
    TEST_ASSERT(rt_random_select_weighted_index(0.5, cumulative, len) == 0, "0.5 should select index 0");
    TEST_ASSERT(rt_random_select_weighted_index(0.99, cumulative, len) == 0, "0.99 should select index 0");

    printf("  Single element handled correctly\n");
}

void test_rt_random_select_weighted_index_two_elements()
{
    printf("Testing rt_random_select_weighted_index with two elements...\n");

    /* Equal weights -> [0.5, 1.0] */
    double cumulative[] = {0.5, 1.0};
    long len = 2;

    /* Values < 0.5 should select index 0 */
    TEST_ASSERT(rt_random_select_weighted_index(0.0, cumulative, len) == 0, "0.0 should select index 0");
    TEST_ASSERT(rt_random_select_weighted_index(0.49, cumulative, len) == 0, "0.49 should select index 0");

    /* Values >= 0.5 should select index 1 */
    TEST_ASSERT(rt_random_select_weighted_index(0.5, cumulative, len) == 1, "0.5 should select index 1");
    TEST_ASSERT(rt_random_select_weighted_index(0.99, cumulative, len) == 1, "0.99 should select index 1");

    printf("  Two elements handled correctly\n");
}

void test_rt_random_select_weighted_index_boundary_values()
{
    printf("Testing rt_random_select_weighted_index at exact boundaries...\n");

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

    printf("  Boundary values handled correctly\n");
}

void test_rt_random_select_weighted_index_null()
{
    printf("Testing rt_random_select_weighted_index with NULL cumulative...\n");

    TEST_ASSERT(rt_random_select_weighted_index(0.5, NULL, 3) == 0, "NULL cumulative should return 0");

    printf("  NULL cumulative handled correctly\n");
}

void test_rt_random_select_weighted_index_invalid_len()
{
    printf("Testing rt_random_select_weighted_index with invalid length...\n");

    double cumulative[] = {1.0};

    TEST_ASSERT(rt_random_select_weighted_index(0.5, cumulative, 0) == 0, "Zero length should return 0");
    TEST_ASSERT(rt_random_select_weighted_index(0.5, cumulative, -1) == 0, "Negative length should return 0");

    printf("  Invalid length handled correctly\n");
}

void test_rt_random_select_weighted_index_large_array()
{
    printf("Testing rt_random_select_weighted_index with larger array...\n");

    /* 10-element cumulative distribution [0.1, 0.2, 0.3, ..., 1.0] */
    double cumulative[] = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0};
    long len = 10;

    /* Test several positions */
    TEST_ASSERT(rt_random_select_weighted_index(0.05, cumulative, len) == 0, "0.05 should select index 0");
    TEST_ASSERT(rt_random_select_weighted_index(0.15, cumulative, len) == 1, "0.15 should select index 1");
    TEST_ASSERT(rt_random_select_weighted_index(0.45, cumulative, len) == 4, "0.45 should select index 4");
    TEST_ASSERT(rt_random_select_weighted_index(0.85, cumulative, len) == 8, "0.85 should select index 8");
    TEST_ASSERT(rt_random_select_weighted_index(0.95, cumulative, len) == 9, "0.95 should select index 9");

    printf("  Large array handled correctly with binary search\n");
}

/* ============================================================================
 * Static Weighted Choice Tests
 * ============================================================================
 * Tests for rt_random_static_weighted_choice_long() function.
 * ============================================================================ */

void test_rt_random_static_weighted_choice_long_basic()
{
    printf("Testing rt_random_static_weighted_choice_long basic functionality...\n");

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

    printf("  Distribution: 10=%d, 20=%d, 30=%d\n", found_10, found_20, found_30);

    rt_arena_destroy(arena);
}

void test_rt_random_static_weighted_choice_long_single_element()
{
    printf("Testing rt_random_static_weighted_choice_long with single element...\n");

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

    printf("  Single element correctly returns 42\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_weighted_choice_long_null_arr()
{
    printf("Testing rt_random_static_weighted_choice_long with NULL array...\n");

    RtArena *arena = rt_arena_create(NULL);
    double weight_data[] = {1.0, 2.0};
    double *weights = rt_array_create_double(arena, 2, weight_data);

    long result = rt_random_static_weighted_choice_long(NULL, weights);
    TEST_ASSERT(result == 0, "Should return 0 for NULL array");

    printf("  NULL array correctly returns 0\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_weighted_choice_long_null_weights()
{
    printf("Testing rt_random_static_weighted_choice_long with NULL weights...\n");

    RtArena *arena = rt_arena_create(NULL);
    long data[] = {10, 20, 30};
    long *arr = rt_array_create_long(arena, 3, data);

    long result = rt_random_static_weighted_choice_long(arr, NULL);
    TEST_ASSERT(result == 0, "Should return 0 for NULL weights");

    printf("  NULL weights correctly returns 0\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_weighted_choice_long_invalid_weights()
{
    printf("Testing rt_random_static_weighted_choice_long with invalid weights...\n");

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

    printf("  Invalid weights correctly return 0\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_weighted_choice_long_distribution()
{
    printf("Testing rt_random_static_weighted_choice_long distribution...\n");

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

    printf("  Distribution: [%d, %d, %d, %d] (expected ~%d each)\n",
           counts[0], counts[1], counts[2], counts[3], expected);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Static Weighted Choice Double Tests
 * ============================================================================
 * Tests for rt_random_static_weighted_choice_double() function.
 * ============================================================================ */

void test_rt_random_static_weighted_choice_double_basic()
{
    printf("Testing rt_random_static_weighted_choice_double basic functionality...\n");

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

    printf("  Distribution: 1.5=%d, 2.5=%d, 3.5=%d\n", found_1_5, found_2_5, found_3_5);

    rt_arena_destroy(arena);
}

void test_rt_random_static_weighted_choice_double_single_element()
{
    printf("Testing rt_random_static_weighted_choice_double with single element...\n");

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

    printf("  Single element correctly returns 3.14159\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_weighted_choice_double_null_arr()
{
    printf("Testing rt_random_static_weighted_choice_double with NULL array...\n");

    RtArena *arena = rt_arena_create(NULL);
    double weight_data[] = {1.0, 2.0};
    double *weights = rt_array_create_double(arena, 2, weight_data);

    double result = rt_random_static_weighted_choice_double(NULL, weights);
    TEST_ASSERT(result == 0.0, "Should return 0.0 for NULL array");

    printf("  NULL array correctly returns 0.0\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_weighted_choice_double_null_weights()
{
    printf("Testing rt_random_static_weighted_choice_double with NULL weights...\n");

    RtArena *arena = rt_arena_create(NULL);
    double data[] = {1.0, 2.0, 3.0};
    double *arr = rt_array_create_double(arena, 3, data);

    double result = rt_random_static_weighted_choice_double(arr, NULL);
    TEST_ASSERT(result == 0.0, "Should return 0.0 for NULL weights");

    printf("  NULL weights correctly returns 0.0\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_weighted_choice_double_invalid_weights()
{
    printf("Testing rt_random_static_weighted_choice_double with invalid weights...\n");

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

    printf("  Invalid weights correctly return 0.0\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_weighted_choice_double_distribution()
{
    printf("Testing rt_random_static_weighted_choice_double distribution...\n");

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

    printf("  Distribution: [%d, %d, %d, %d] (expected ~%d each)\n",
           counts[0], counts[1], counts[2], counts[3], expected);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Static Weighted Choice String Tests
 * ============================================================================
 * Tests for rt_random_static_weighted_choice_string() function.
 * ============================================================================ */

void test_rt_random_static_weighted_choice_string_basic()
{
    printf("Testing rt_random_static_weighted_choice_string basic functionality...\n");

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

    printf("  Distribution: apple=%d, banana=%d, cherry=%d\n", found_apple, found_banana, found_cherry);

    rt_arena_destroy(arena);
}

void test_rt_random_static_weighted_choice_string_single_element()
{
    printf("Testing rt_random_static_weighted_choice_string with single element...\n");

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

    printf("  Single element correctly returns 'only_one'\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_weighted_choice_string_null_arr()
{
    printf("Testing rt_random_static_weighted_choice_string with NULL array...\n");

    RtArena *arena = rt_arena_create(NULL);
    double weight_data[] = {1.0, 2.0};
    double *weights = rt_array_create_double(arena, 2, weight_data);

    char *result = rt_random_static_weighted_choice_string(NULL, weights);
    TEST_ASSERT(result == NULL, "Should return NULL for NULL array");

    printf("  NULL array correctly returns NULL\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_weighted_choice_string_null_weights()
{
    printf("Testing rt_random_static_weighted_choice_string with NULL weights...\n");

    RtArena *arena = rt_arena_create(NULL);
    const char *data[] = {"a", "b", "c"};
    char **arr = rt_array_create_string(arena, 3, data);

    char *result = rt_random_static_weighted_choice_string(arr, NULL);
    TEST_ASSERT(result == NULL, "Should return NULL for NULL weights");

    printf("  NULL weights correctly returns NULL\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_weighted_choice_string_invalid_weights()
{
    printf("Testing rt_random_static_weighted_choice_string with invalid weights...\n");

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

    printf("  Invalid weights correctly return NULL\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_weighted_choice_string_distribution()
{
    printf("Testing rt_random_static_weighted_choice_string distribution...\n");

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

    printf("  Distribution: [%d, %d, %d, %d] (expected ~%d each)\n",
           counts[0], counts[1], counts[2], counts[3], expected);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Instance Weighted Choice Long Tests
 * ============================================================================
 * Tests for rt_random_weighted_choice_long() function.
 * ============================================================================ */

void test_rt_random_weighted_choice_long_basic()
{
    printf("Testing rt_random_weighted_choice_long basic functionality...\n");

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

    printf("  Distribution: 10=%d, 20=%d, 30=%d\n", found_10, found_20, found_30);

    rt_arena_destroy(arena);
}

void test_rt_random_weighted_choice_long_single_element()
{
    printf("Testing rt_random_weighted_choice_long with single element...\n");

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

    printf("  Single element correctly returns 42\n");

    rt_arena_destroy(arena);
}

void test_rt_random_weighted_choice_long_null_rng()
{
    printf("Testing rt_random_weighted_choice_long with NULL rng...\n");

    RtArena *arena = rt_arena_create(NULL);
    long data[] = {10, 20, 30};
    long *arr = rt_array_create_long(arena, 3, data);
    double weight_data[] = {1.0, 2.0, 3.0};
    double *weights = rt_array_create_double(arena, 3, weight_data);

    long result = rt_random_weighted_choice_long(NULL, arr, weights);
    TEST_ASSERT(result == 0, "Should return 0 for NULL rng");

    printf("  NULL rng correctly returns 0\n");

    rt_arena_destroy(arena);
}

void test_rt_random_weighted_choice_long_null_arr()
{
    printf("Testing rt_random_weighted_choice_long with NULL array...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 42);
    double weight_data[] = {1.0, 2.0};
    double *weights = rt_array_create_double(arena, 2, weight_data);

    long result = rt_random_weighted_choice_long(rng, NULL, weights);
    TEST_ASSERT(result == 0, "Should return 0 for NULL array");

    printf("  NULL array correctly returns 0\n");

    rt_arena_destroy(arena);
}

void test_rt_random_weighted_choice_long_null_weights()
{
    printf("Testing rt_random_weighted_choice_long with NULL weights...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 42);
    long data[] = {10, 20, 30};
    long *arr = rt_array_create_long(arena, 3, data);

    long result = rt_random_weighted_choice_long(rng, arr, NULL);
    TEST_ASSERT(result == 0, "Should return 0 for NULL weights");

    printf("  NULL weights correctly returns 0\n");

    rt_arena_destroy(arena);
}

void test_rt_random_weighted_choice_long_invalid_weights()
{
    printf("Testing rt_random_weighted_choice_long with invalid weights...\n");

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

    printf("  Invalid weights correctly return 0\n");

    rt_arena_destroy(arena);
}

void test_rt_random_weighted_choice_long_reproducible()
{
    printf("Testing rt_random_weighted_choice_long reproducibility...\n");

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

    printf("  Reproducibility verified: %d/20 matches\n", matches);

    rt_arena_destroy(arena);
}

void test_rt_random_weighted_choice_long_distribution()
{
    printf("Testing rt_random_weighted_choice_long distribution...\n");

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

    printf("  Distribution: [%d, %d, %d, %d] (expected ~%d each)\n",
           counts[0], counts[1], counts[2], counts[3], expected);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Instance Weighted Choice Double Tests
 * ============================================================================ */

void test_rt_random_weighted_choice_double_basic()
{
    printf("Testing rt_random_weighted_choice_double basic functionality...\n");

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

    printf("  Distribution: 1.5=%d, 2.5=%d, 3.5=%d\n", found_1_5, found_2_5, found_3_5);

    rt_arena_destroy(arena);
}

void test_rt_random_weighted_choice_double_single_element()
{
    printf("Testing rt_random_weighted_choice_double with single element...\n");

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

    printf("  Single element correctly returns 3.14159\n");

    rt_arena_destroy(arena);
}

void test_rt_random_weighted_choice_double_null_rng()
{
    printf("Testing rt_random_weighted_choice_double with NULL rng...\n");

    RtArena *arena = rt_arena_create(NULL);
    double data[] = {1.0, 2.0, 3.0};
    double *arr = rt_array_create_double(arena, 3, data);
    double weight_data[] = {1.0, 2.0, 3.0};
    double *weights = rt_array_create_double(arena, 3, weight_data);

    double result = rt_random_weighted_choice_double(NULL, arr, weights);
    TEST_ASSERT(result == 0.0, "Should return 0.0 for NULL rng");

    printf("  NULL rng correctly returns 0.0\n");

    rt_arena_destroy(arena);
}

void test_rt_random_weighted_choice_double_null_arr()
{
    printf("Testing rt_random_weighted_choice_double with NULL array...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 42);
    double weight_data[] = {1.0, 2.0};
    double *weights = rt_array_create_double(arena, 2, weight_data);

    double result = rt_random_weighted_choice_double(rng, NULL, weights);
    TEST_ASSERT(result == 0.0, "Should return 0.0 for NULL array");

    printf("  NULL array correctly returns 0.0\n");

    rt_arena_destroy(arena);
}

void test_rt_random_weighted_choice_double_null_weights()
{
    printf("Testing rt_random_weighted_choice_double with NULL weights...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 42);
    double data[] = {1.0, 2.0, 3.0};
    double *arr = rt_array_create_double(arena, 3, data);

    double result = rt_random_weighted_choice_double(rng, arr, NULL);
    TEST_ASSERT(result == 0.0, "Should return 0.0 for NULL weights");

    printf("  NULL weights correctly returns 0.0\n");

    rt_arena_destroy(arena);
}

void test_rt_random_weighted_choice_double_invalid_weights()
{
    printf("Testing rt_random_weighted_choice_double with invalid weights...\n");

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

    printf("  Invalid weights correctly return 0.0\n");

    rt_arena_destroy(arena);
}

void test_rt_random_weighted_choice_double_reproducible()
{
    printf("Testing rt_random_weighted_choice_double reproducibility...\n");

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

    printf("  Reproducibility verified: %d/20 matches\n", matches);

    rt_arena_destroy(arena);
}

void test_rt_random_weighted_choice_double_distribution()
{
    printf("Testing rt_random_weighted_choice_double distribution...\n");

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

    printf("  Distribution: [%d, %d, %d, %d] (expected ~%d each)\n",
           counts[0], counts[1], counts[2], counts[3], expected);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Instance Weighted Choice String Tests
 * ============================================================================ */

void test_rt_random_weighted_choice_string_basic()
{
    printf("Testing rt_random_weighted_choice_string basic functionality...\n");

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

    printf("  Distribution: apple=%d, banana=%d, cherry=%d\n", found_apple, found_banana, found_cherry);

    rt_arena_destroy(arena);
}

void test_rt_random_weighted_choice_string_single_element()
{
    printf("Testing rt_random_weighted_choice_string with single element...\n");

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

    printf("  Single element correctly returns 'only_one'\n");

    rt_arena_destroy(arena);
}

void test_rt_random_weighted_choice_string_null_rng()
{
    printf("Testing rt_random_weighted_choice_string with NULL rng...\n");

    RtArena *arena = rt_arena_create(NULL);
    const char *data[] = {"a", "b", "c"};
    char **arr = rt_array_create_string(arena, 3, data);
    double weight_data[] = {1.0, 2.0, 3.0};
    double *weights = rt_array_create_double(arena, 3, weight_data);

    char *result = rt_random_weighted_choice_string(NULL, arr, weights);
    TEST_ASSERT(result == NULL, "Should return NULL for NULL rng");

    printf("  NULL rng correctly returns NULL\n");

    rt_arena_destroy(arena);
}

void test_rt_random_weighted_choice_string_null_arr()
{
    printf("Testing rt_random_weighted_choice_string with NULL array...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 42);
    double weight_data[] = {1.0, 2.0};
    double *weights = rt_array_create_double(arena, 2, weight_data);

    char *result = rt_random_weighted_choice_string(rng, NULL, weights);
    TEST_ASSERT(result == NULL, "Should return NULL for NULL array");

    printf("  NULL array correctly returns NULL\n");

    rt_arena_destroy(arena);
}

void test_rt_random_weighted_choice_string_null_weights()
{
    printf("Testing rt_random_weighted_choice_string with NULL weights...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 42);
    const char *data[] = {"a", "b", "c"};
    char **arr = rt_array_create_string(arena, 3, data);

    char *result = rt_random_weighted_choice_string(rng, arr, NULL);
    TEST_ASSERT(result == NULL, "Should return NULL for NULL weights");

    printf("  NULL weights correctly returns NULL\n");

    rt_arena_destroy(arena);
}

void test_rt_random_weighted_choice_string_invalid_weights()
{
    printf("Testing rt_random_weighted_choice_string with invalid weights...\n");

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

    printf("  Invalid weights correctly return NULL\n");

    rt_arena_destroy(arena);
}

void test_rt_random_weighted_choice_string_reproducible()
{
    printf("Testing rt_random_weighted_choice_string reproducibility...\n");

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

    printf("  Reproducibility verified: %d/20 matches\n", matches);

    rt_arena_destroy(arena);
}

void test_rt_random_weighted_choice_string_distribution()
{
    printf("Testing rt_random_weighted_choice_string distribution...\n");

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

    printf("  Distribution: [%d, %d, %d, %d] (expected ~%d each)\n",
           counts[0], counts[1], counts[2], counts[3], expected);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Weighted Selection Probability Distribution Tests
 * ============================================================================
 * Comprehensive tests for weighted random selection distribution accuracy.
 * ============================================================================ */

void test_weighted_distribution_equal_weights_uniform()
{
    printf("Testing weighted distribution with equal weights produces uniform...\n");

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

    printf("  Distribution: [%d, %d, %d, %d, %d] (expected ~%d each)\n",
           counts[0], counts[1], counts[2], counts[3], counts[4], expected);

    rt_arena_destroy(arena);
}

void test_weighted_distribution_extreme_ratio()
{
    printf("Testing weighted distribution with extreme weight ratio (1000:1)...\n");

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
    int expected_200 = iterations - expected_100;            /* ~10 */

    /* Verify 100 appears much more often */
    TEST_ASSERT(count_100 > count_200 * 100, "High-weight element should dominate");

    /* Allow generous tolerance for rare element */
    int tolerance_100 = expected_100 / 10;  /* 10% */
    int deviation_100 = abs(count_100 - expected_100);
    TEST_ASSERT(deviation_100 < tolerance_100, "High-weight element should be near expected");

    printf("  Count: 100=%d (expected ~%d), 200=%d (expected ~%d)\n",
           count_100, expected_100, count_200, expected_200);
    printf("  Ratio: %.1f:1 (expected 1000:1)\n",
           count_200 > 0 ? (double)count_100 / count_200 : (double)count_100);

    rt_arena_destroy(arena);
}

void test_weighted_distribution_single_element()
{
    printf("Testing weighted distribution single element always returns that element...\n");

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

    printf("  Single element correctly returned 200 times\n");

    rt_arena_destroy(arena);
}

void test_weighted_distribution_large_sample_accuracy()
{
    printf("Testing weighted distribution large sample accuracy...\n");

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

    printf("  Distribution: [%d, %d, %d, %d]\n", counts[0], counts[1], counts[2], counts[3]);
    printf("  Expected:     [%d, %d, %d, %d]\n", expected[0], expected[1], expected[2], expected[3]);

    /* Calculate actual percentages */
    printf("  Actual %%:     [%.1f%%, %.1f%%, %.1f%%, %.1f%%]\n",
           100.0 * counts[0] / iterations,
           100.0 * counts[1] / iterations,
           100.0 * counts[2] / iterations,
           100.0 * counts[3] / iterations);

    rt_arena_destroy(arena);
}

void test_weighted_distribution_seeded_prng_reproducible()
{
    printf("Testing weighted distribution seeded PRNG is reproducible...\n");

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

    printf("  Reproducibility: %d/%d matches (expected 100%%)\n", matches, iterations);

    rt_arena_destroy(arena);
}

void test_weighted_distribution_os_entropy_varies()
{
    printf("Testing weighted distribution OS entropy (static) varies...\n");

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

    printf("  Unique values seen: %d/10\n", unique_count);

    rt_arena_destroy(arena);
}

void test_weighted_distribution_static_vs_instance()
{
    printf("Testing weighted distribution static vs instance methods...\n");

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

    printf("  Static (OS entropy):  [%d, %d, %d]\n",
           static_counts[0], static_counts[1], static_counts[2]);
    printf("  Instance (seeded):    [%d, %d, %d]\n",
           instance_counts[0], instance_counts[1], instance_counts[2]);
    printf("  Expected:             [%d, %d, %d]\n", expected[0], expected[1], expected[2]);

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

void test_integration_weighted_loot_drop_static()
{
    printf("Testing integration: Weighted loot drop (static method)...\n");
    printf("  Scenario: Game loot system with common=70%%, rare=25%%, legendary=5%%\n");

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

    printf("  Results (static/OS entropy):\n");
    printf("    common_sword:    %d (expected ~%d, %.1f%%)\n",
           common_count, expected_common, 100.0 * common_count / total_drops);
    printf("    rare_shield:     %d (expected ~%d, %.1f%%)\n",
           rare_count, expected_rare, 100.0 * rare_count / total_drops);
    printf("    legendary_helm:  %d (expected ~%d, %.1f%%)\n",
           legendary_count, expected_legendary, 100.0 * legendary_count / total_drops);

    rt_arena_destroy(arena);
}

void test_integration_weighted_loot_drop_seeded()
{
    printf("Testing integration: Weighted loot drop (seeded instance)...\n");
    printf("  Scenario: Reproducible loot with seed for testing/replay\n");

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

    printf("  Results (seeded PRNG, seed=%ld):\n", player_seed);
    printf("    common_sword:    %d (expected ~%d, %.1f%%)\n",
           common_count, expected_common, 100.0 * common_count / total_drops);
    printf("    rare_shield:     %d (expected ~%d, %.1f%%)\n",
           rare_count, expected_rare, 100.0 * rare_count / total_drops);
    printf("    legendary_helm:  %d (expected ~%d, %.1f%%)\n",
           legendary_count, expected_legendary, 100.0 * legendary_count / total_drops);

    /*
     * Verify reproducibility: same seed should give same sequence
     */
    RtRandom *rng2 = rt_random_create_with_seed(arena, player_seed);
    RtRandom *rng_orig = rt_random_create_with_seed(arena, player_seed);

    printf("  Reproducibility test (10 drops with same seed):\n");
    printf("    ");
    int matches = 0;
    for (int i = 0; i < 10; i++) {
        char *drop1 = rt_random_weighted_choice_string(rng2, items, weights);
        char *drop2 = rt_random_weighted_choice_string(rng_orig, items, weights);
        if (strcmp(drop1, drop2) == 0) matches++;
        printf("%s ", drop1);
    }
    printf("\n");
    TEST_ASSERT(matches == 10, "Same seed must produce identical loot sequence");
    printf("    All 10 drops matched between two RNGs with same seed\n");

    rt_arena_destroy(arena);
}

void test_integration_weighted_loot_drop_all_tiers()
{
    printf("Testing integration: Verify all loot tiers are reachable...\n");

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

    printf("  All three tiers (common, rare, legendary) verified reachable\n");

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Shuffle Tests - Static Methods (OS Entropy)
 * ============================================================================
 * Tests for Fisher-Yates shuffle algorithm.
 * ============================================================================ */

void test_rt_random_static_shuffle_long_basic()
{
    printf("Testing rt_random_static_shuffle_long basic...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Create array {1, 2, 3, 4, 5} */
    long data[] = {1, 2, 3, 4, 5};
    long *arr = rt_array_create_long(arena, 5, data);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Calculate original sum */
    long original_sum = 0;
    for (int i = 0; i < 5; i++) original_sum += arr[i];

    /* Shuffle multiple times and verify all elements present */
    for (int trial = 0; trial < 10; trial++) {
        rt_random_static_shuffle_long(arr);

        /* Verify all elements still present (sum unchanged) */
        long sum = 0;
        int found[5] = {0, 0, 0, 0, 0};
        for (int i = 0; i < 5; i++) {
            sum += arr[i];
            if (arr[i] >= 1 && arr[i] <= 5) {
                found[arr[i] - 1] = 1;
            }
        }

        TEST_ASSERT(sum == original_sum, "Sum should be unchanged after shuffle");

        int all_found = 1;
        for (int i = 0; i < 5; i++) {
            if (!found[i]) all_found = 0;
        }
        TEST_ASSERT(all_found, "All elements should be present after shuffle");
    }

    printf("  Shuffle preserves all elements correctly\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_shuffle_double_basic()
{
    printf("Testing rt_random_static_shuffle_double basic...\n");

    RtArena *arena = rt_arena_create(NULL);
    double data[] = {1.1, 2.2, 3.3, 4.4, 5.5};
    double *arr = rt_array_create_double(arena, 5, data);

    double original_sum = 0;
    for (int i = 0; i < 5; i++) original_sum += arr[i];

    rt_random_static_shuffle_double(arr);

    double sum = 0;
    for (int i = 0; i < 5; i++) sum += arr[i];

    TEST_ASSERT(fabs(sum - original_sum) < 0.001, "Sum should be unchanged after shuffle");

    printf("  Double shuffle preserves elements\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_shuffle_string_basic()
{
    printf("Testing rt_random_static_shuffle_string basic...\n");

    RtArena *arena = rt_arena_create(NULL);
    const char *data[] = {"apple", "banana", "cherry", "date", "elderberry"};
    char **arr = rt_array_create_string(arena, 5, data);

    rt_random_static_shuffle_string(arr);

    /* Verify all strings still present */
    int found[5] = {0, 0, 0, 0, 0};
    for (int i = 0; i < 5; i++) {
        if (strcmp(arr[i], "apple") == 0) found[0] = 1;
        else if (strcmp(arr[i], "banana") == 0) found[1] = 1;
        else if (strcmp(arr[i], "cherry") == 0) found[2] = 1;
        else if (strcmp(arr[i], "date") == 0) found[3] = 1;
        else if (strcmp(arr[i], "elderberry") == 0) found[4] = 1;
    }

    for (int i = 0; i < 5; i++) {
        TEST_ASSERT(found[i], "All strings should be present after shuffle");
    }

    printf("  String shuffle preserves elements\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_shuffle_bool_basic()
{
    printf("Testing rt_random_static_shuffle_bool basic...\n");

    RtArena *arena = rt_arena_create(NULL);
    int data[] = {1, 1, 0, 0, 1};
    int *arr = rt_array_create_bool(arena, 5, data);

    int original_true_count = 0;
    for (int i = 0; i < 5; i++) if (arr[i]) original_true_count++;

    rt_random_static_shuffle_bool(arr);

    int true_count = 0;
    for (int i = 0; i < 5; i++) if (arr[i]) true_count++;

    TEST_ASSERT(true_count == original_true_count, "Bool count should be unchanged");

    printf("  Bool shuffle preserves elements\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_shuffle_byte_basic()
{
    printf("Testing rt_random_static_shuffle_byte basic...\n");

    RtArena *arena = rt_arena_create(NULL);
    unsigned char data[] = {10, 20, 30, 40, 50};
    unsigned char *arr = rt_array_create_byte(arena, 5, data);

    int original_sum = 0;
    for (int i = 0; i < 5; i++) original_sum += arr[i];

    rt_random_static_shuffle_byte(arr);

    int sum = 0;
    for (int i = 0; i < 5; i++) sum += arr[i];

    TEST_ASSERT(sum == original_sum, "Byte sum should be unchanged");

    printf("  Byte shuffle preserves elements\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_shuffle_null_handling()
{
    printf("Testing rt_random_static_shuffle NULL handling...\n");

    /* These should not crash */
    rt_random_static_shuffle_long(NULL);
    rt_random_static_shuffle_double(NULL);
    rt_random_static_shuffle_string(NULL);
    rt_random_static_shuffle_bool(NULL);
    rt_random_static_shuffle_byte(NULL);

    printf("  NULL arrays handled gracefully\n");
}

void test_rt_random_static_shuffle_single_element()
{
    printf("Testing rt_random_static_shuffle with single element...\n");

    RtArena *arena = rt_arena_create(NULL);

    long data[] = {42};
    long *arr = rt_array_create_long(arena, 1, data);

    rt_random_static_shuffle_long(arr);

    TEST_ASSERT(arr[0] == 42, "Single element should be unchanged");

    printf("  Single element unchanged\n");

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Shuffle Tests - Instance Methods (Seeded PRNG)
 * ============================================================================ */

void test_rt_random_shuffle_long_basic()
{
    printf("Testing rt_random_shuffle_long basic (seeded)...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 12345);

    long data[] = {1, 2, 3, 4, 5};
    long *arr = rt_array_create_long(arena, 5, data);

    long original_sum = 0;
    for (int i = 0; i < 5; i++) original_sum += arr[i];

    rt_random_shuffle_long(rng, arr);

    long sum = 0;
    int found[5] = {0, 0, 0, 0, 0};
    for (int i = 0; i < 5; i++) {
        sum += arr[i];
        if (arr[i] >= 1 && arr[i] <= 5) found[arr[i] - 1] = 1;
    }

    TEST_ASSERT(sum == original_sum, "Sum should be unchanged");
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT(found[i], "All elements present");
    }

    printf("  Seeded shuffle preserves elements\n");

    rt_arena_destroy(arena);
}

void test_rt_random_shuffle_reproducible()
{
    printf("Testing rt_random_shuffle reproducibility...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Two identical arrays with same seed should produce same shuffle */
    long data1[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    long data2[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    long *arr1 = rt_array_create_long(arena, 10, data1);
    long *arr2 = rt_array_create_long(arena, 10, data2);

    RtRandom *rng1 = rt_random_create_with_seed(arena, 99999);
    RtRandom *rng2 = rt_random_create_with_seed(arena, 99999);

    rt_random_shuffle_long(rng1, arr1);
    rt_random_shuffle_long(rng2, arr2);

    int all_match = 1;
    for (int i = 0; i < 10; i++) {
        if (arr1[i] != arr2[i]) {
            all_match = 0;
            break;
        }
    }

    TEST_ASSERT(all_match, "Same seed must produce identical shuffle");

    printf("  Reproducibility verified: same seed = same shuffle\n");

    rt_arena_destroy(arena);
}

void test_rt_random_shuffle_null_rng()
{
    printf("Testing rt_random_shuffle with NULL rng...\n");

    RtArena *arena = rt_arena_create(NULL);
    long data[] = {1, 2, 3};
    long *arr = rt_array_create_long(arena, 3, data);

    /* Should not crash */
    rt_random_shuffle_long(NULL, arr);

    /* Array should be unchanged */
    TEST_ASSERT(arr[0] == 1 && arr[1] == 2 && arr[2] == 3, "Array unchanged with NULL rng");

    printf("  NULL rng handled gracefully\n");

    rt_arena_destroy(arena);
}

void test_rt_random_shuffle_all_types_seeded()
{
    printf("Testing all seeded shuffle types...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 54321);

    /* Double */
    double ddata[] = {1.1, 2.2, 3.3};
    double *darr = rt_array_create_double(arena, 3, ddata);
    rt_random_shuffle_double(rng, darr);

    /* String */
    const char *sdata[] = {"a", "b", "c"};
    char **sarr = rt_array_create_string(arena, 3, sdata);
    rt_random_shuffle_string(rng, sarr);

    /* Bool */
    int bdata[] = {1, 0, 1};
    int *barr = rt_array_create_bool(arena, 3, bdata);
    rt_random_shuffle_bool(rng, barr);

    /* Byte */
    unsigned char bydata[] = {1, 2, 3};
    unsigned char *byarr = rt_array_create_byte(arena, 3, bydata);
    rt_random_shuffle_byte(rng, byarr);

    printf("  All seeded shuffle types work correctly\n");

    rt_arena_destroy(arena);
}

/* Statistical test: Verify uniform permutation distribution */
void test_rt_random_shuffle_distribution()
{
    printf("Testing shuffle uniform permutation distribution...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* For a 3-element array, there are 6 possible permutations.
     * Each should occur roughly 1/6 of the time.
     * We encode permutations as: arr[0]*100 + arr[1]*10 + arr[2]
     * 123, 132, 213, 231, 312, 321
     */
    int perm_counts[6] = {0, 0, 0, 0, 0, 0};
    int iterations = 6000;

    for (int iter = 0; iter < iterations; iter++) {
        long data[] = {1, 2, 3};
        long *arr = rt_array_create_long(arena, 3, data);

        rt_random_static_shuffle_long(arr);

        int perm = (int)(arr[0] * 100 + arr[1] * 10 + arr[2]);

        /* Map permutation to index */
        int idx = -1;
        switch (perm) {
            case 123: idx = 0; break;
            case 132: idx = 1; break;
            case 213: idx = 2; break;
            case 231: idx = 3; break;
            case 312: idx = 4; break;
            case 321: idx = 5; break;
        }

        if (idx >= 0 && idx < 6) {
            perm_counts[idx]++;
        }

        /* We need to free this array - but arena will handle cleanup */
    }

    int expected = iterations / 6;  /* ~1000 */
    int tolerance = expected / 2;   /* Allow 50% deviation */

    printf("  Permutation counts: [%d, %d, %d, %d, %d, %d] (expected ~%d each)\n",
           perm_counts[0], perm_counts[1], perm_counts[2],
           perm_counts[3], perm_counts[4], perm_counts[5], expected);

    for (int i = 0; i < 6; i++) {
        int deviation = abs(perm_counts[i] - expected);
        TEST_ASSERT(deviation < tolerance, "Permutation distribution should be uniform");
    }

    rt_arena_destroy(arena);
}

void test_rt_random_shuffle_distribution_seeded()
{
    printf("Testing seeded shuffle permutation distribution...\n");

    RtArena *arena = rt_arena_create(NULL);

    int perm_counts[6] = {0, 0, 0, 0, 0, 0};
    int iterations = 6000;

    /* Use different seeds to get variety while still being deterministic */
    for (int iter = 0; iter < iterations; iter++) {
        RtRandom *rng = rt_random_create_with_seed(arena, (long)(iter * 7919));  /* Different seed each time */

        long data[] = {1, 2, 3};
        long *arr = rt_array_create_long(arena, 3, data);

        rt_random_shuffle_long(rng, arr);

        int perm = (int)(arr[0] * 100 + arr[1] * 10 + arr[2]);

        int idx = -1;
        switch (perm) {
            case 123: idx = 0; break;
            case 132: idx = 1; break;
            case 213: idx = 2; break;
            case 231: idx = 3; break;
            case 312: idx = 4; break;
            case 321: idx = 5; break;
        }

        if (idx >= 0 && idx < 6) {
            perm_counts[idx]++;
        }
    }

    int expected = iterations / 6;
    int tolerance = expected / 2;

    printf("  Seeded permutation counts: [%d, %d, %d, %d, %d, %d] (expected ~%d each)\n",
           perm_counts[0], perm_counts[1], perm_counts[2],
           perm_counts[3], perm_counts[4], perm_counts[5], expected);

    for (int i = 0; i < 6; i++) {
        int deviation = abs(perm_counts[i] - expected);
        TEST_ASSERT(deviation < tolerance, "Seeded permutation distribution should be uniform");
    }

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Sample Tests - Static (OS Entropy)
 * ============================================================================
 * Tests for Random.sample() which selects elements without replacement.
 * ============================================================================ */

void test_rt_random_static_sample_long_basic()
{
    printf("Testing rt_random_static_sample_long basic...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Create array {10, 20, 30, 40, 50} */
    long data[] = {10, 20, 30, 40, 50};
    long *arr = rt_array_create_long(arena, 5, data);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Sample 3 elements */
    long *sample = rt_random_static_sample_long(arena, arr, 3);
    TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

    /* Verify sample has correct length */
    TEST_ASSERT(rt_array_length(sample) == 3, "Sample should have 3 elements");

    /* Verify all sampled elements are from original array */
    for (int i = 0; i < 3; i++) {
        int found = 0;
        for (int j = 0; j < 5; j++) {
            if (sample[i] == data[j]) {
                found = 1;
                break;
            }
        }
        TEST_ASSERT(found, "Sampled element should be from original array");
    }

    printf("  Basic sampling works correctly\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_sample_long_no_duplicates()
{
    printf("Testing rt_random_static_sample_long no duplicates...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Create array with unique values */
    long data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    long *arr = rt_array_create_long(arena, 10, data);

    /* Sample 5 elements multiple times */
    for (int trial = 0; trial < 20; trial++) {
        long *sample = rt_random_static_sample_long(arena, arr, 5);
        TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

        /* Check for duplicates */
        for (int i = 0; i < 5; i++) {
            for (int j = i + 1; j < 5; j++) {
                TEST_ASSERT(sample[i] != sample[j], "Sample should not contain duplicates");
            }
        }
    }

    printf("  Sampling without replacement produces no duplicates\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_sample_long_full_array()
{
    printf("Testing rt_random_static_sample_long full array...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Create array */
    long data[] = {10, 20, 30, 40, 50};
    long *arr = rt_array_create_long(arena, 5, data);

    /* Sample entire array (count == length) */
    long *sample = rt_random_static_sample_long(arena, arr, 5);
    TEST_ASSERT_NOT_NULL(sample, "Sample should be created when count equals array length");
    TEST_ASSERT(rt_array_length(sample) == 5, "Sample should have all 5 elements");

    /* Verify all original elements are present */
    long original_sum = 10 + 20 + 30 + 40 + 50;
    long sample_sum = 0;
    for (int i = 0; i < 5; i++) {
        sample_sum += sample[i];
    }
    TEST_ASSERT(sample_sum == original_sum, "Full sample should contain all original elements");

    printf("  Full array sampling works correctly\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_sample_long_single_element()
{
    printf("Testing rt_random_static_sample_long single element...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Create array */
    long data[] = {100, 200, 300, 400, 500};
    long *arr = rt_array_create_long(arena, 5, data);

    /* Sample single element */
    long *sample = rt_random_static_sample_long(arena, arr, 1);
    TEST_ASSERT_NOT_NULL(sample, "Single element sample should be created");
    TEST_ASSERT(rt_array_length(sample) == 1, "Sample should have 1 element");

    /* Verify element is from original array */
    int found = 0;
    for (int j = 0; j < 5; j++) {
        if (sample[0] == data[j]) {
            found = 1;
            break;
        }
    }
    TEST_ASSERT(found, "Single sampled element should be from original array");

    printf("  Single element sampling works correctly\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_sample_long_count_exceeds_length()
{
    printf("Testing rt_random_static_sample_long count exceeds length...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Create array with 5 elements */
    long data[] = {1, 2, 3, 4, 5};
    long *arr = rt_array_create_long(arena, 5, data);

    /* Try to sample 6 elements (should return NULL) */
    long *sample = rt_random_static_sample_long(arena, arr, 6);
    TEST_ASSERT(sample == NULL, "Should return NULL when count > array length");

    /* Try to sample 10 elements (should return NULL) */
    sample = rt_random_static_sample_long(arena, arr, 10);
    TEST_ASSERT(sample == NULL, "Should return NULL when count >> array length");

    printf("  Invalid count correctly returns NULL\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_sample_long_null_handling()
{
    printf("Testing rt_random_static_sample_long null handling...\n");

    RtArena *arena = rt_arena_create(NULL);

    long data[] = {1, 2, 3};
    long *arr = rt_array_create_long(arena, 3, data);

    /* NULL arena */
    long *sample = rt_random_static_sample_long(NULL, arr, 2);
    TEST_ASSERT(sample == NULL, "Should return NULL with NULL arena");

    /* NULL array */
    sample = rt_random_static_sample_long(arena, NULL, 2);
    TEST_ASSERT(sample == NULL, "Should return NULL with NULL array");

    /* Zero count */
    sample = rt_random_static_sample_long(arena, arr, 0);
    TEST_ASSERT(sample == NULL, "Should return NULL with zero count");

    /* Negative count */
    sample = rt_random_static_sample_long(arena, arr, -1);
    TEST_ASSERT(sample == NULL, "Should return NULL with negative count");

    printf("  NULL and invalid input handling correct\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_sample_long_preserves_original()
{
    printf("Testing rt_random_static_sample_long preserves original...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Create array */
    long data[] = {100, 200, 300, 400, 500};
    long *arr = rt_array_create_long(arena, 5, data);

    /* Sample multiple times */
    for (int trial = 0; trial < 10; trial++) {
        long *sample = rt_random_static_sample_long(arena, arr, 3);
        TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

        /* Verify original array is unchanged */
        for (int i = 0; i < 5; i++) {
            TEST_ASSERT(arr[i] == data[i], "Original array should be unchanged after sampling");
        }
    }

    printf("  Original array preserved after sampling\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_sample_long_distribution()
{
    printf("Testing rt_random_static_sample_long distribution...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Create array {1, 2, 3, 4, 5} */
    long data[] = {1, 2, 3, 4, 5};
    long *arr = rt_array_create_long(arena, 5, data);

    /* Track how often each element appears in samples */
    int element_counts[5] = {0, 0, 0, 0, 0};
    int iterations = 1000;

    for (int iter = 0; iter < iterations; iter++) {
        long *sample = rt_random_static_sample_long(arena, arr, 2);
        TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 5; j++) {
                if (sample[i] == data[j]) {
                    element_counts[j]++;
                }
            }
        }
    }

    /* Each element should appear roughly (2/5) * iterations = 400 times */
    int expected = (2 * iterations) / 5;
    int tolerance = expected / 3;  /* Allow ~33% deviation */

    printf("  Element counts: [%d, %d, %d, %d, %d] (expected ~%d each)\n",
           element_counts[0], element_counts[1], element_counts[2],
           element_counts[3], element_counts[4], expected);

    for (int i = 0; i < 5; i++) {
        int deviation = abs(element_counts[i] - expected);
        TEST_ASSERT(deviation < tolerance, "Sample distribution should be roughly uniform");
    }

    printf("  Sample distribution is approximately uniform\n");

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Sample Tests - Static Double (OS Entropy)
 * ============================================================================
 * Tests for Random.sample() on double arrays.
 * ============================================================================ */

void test_rt_random_static_sample_double_basic()
{
    printf("Testing rt_random_static_sample_double basic...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Create array {1.1, 2.2, 3.3, 4.4, 5.5} */
    double data[] = {1.1, 2.2, 3.3, 4.4, 5.5};
    double *arr = rt_array_create_double(arena, 5, data);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Sample 3 elements */
    double *sample = rt_random_static_sample_double(arena, arr, 3);
    TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

    /* Verify sample has correct length */
    TEST_ASSERT(rt_array_length(sample) == 3, "Sample should have 3 elements");

    /* Verify all sampled elements are from original array */
    for (int i = 0; i < 3; i++) {
        int found = 0;
        for (int j = 0; j < 5; j++) {
            if (fabs(sample[i] - data[j]) < 0.001) {
                found = 1;
                break;
            }
        }
        TEST_ASSERT(found, "Sampled element should be from original array");
    }

    printf("  Basic sampling works correctly\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_sample_double_no_duplicates()
{
    printf("Testing rt_random_static_sample_double no duplicates...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Create array with unique values */
    double data[] = {1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9, 10.0};
    double *arr = rt_array_create_double(arena, 10, data);

    /* Sample 5 elements multiple times */
    for (int trial = 0; trial < 20; trial++) {
        double *sample = rt_random_static_sample_double(arena, arr, 5);
        TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

        /* Check for duplicates */
        for (int i = 0; i < 5; i++) {
            for (int j = i + 1; j < 5; j++) {
                TEST_ASSERT(fabs(sample[i] - sample[j]) > 0.001, "Sample should not contain duplicates");
            }
        }
    }

    printf("  Sampling without replacement produces no duplicates\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_sample_double_full_array()
{
    printf("Testing rt_random_static_sample_double full array...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Create array */
    double data[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double *arr = rt_array_create_double(arena, 5, data);

    /* Sample entire array (count == length) */
    double *sample = rt_random_static_sample_double(arena, arr, 5);
    TEST_ASSERT_NOT_NULL(sample, "Sample should be created when count equals array length");
    TEST_ASSERT(rt_array_length(sample) == 5, "Sample should have all 5 elements");

    /* Verify all original elements are present */
    double original_sum = 1.0 + 2.0 + 3.0 + 4.0 + 5.0;
    double sample_sum = 0.0;
    for (int i = 0; i < 5; i++) {
        sample_sum += sample[i];
    }
    TEST_ASSERT(fabs(sample_sum - original_sum) < 0.001, "Full sample should contain all original elements");

    printf("  Full array sampling works correctly\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_sample_double_count_exceeds_length()
{
    printf("Testing rt_random_static_sample_double count exceeds length...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Create array with 5 elements */
    double data[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double *arr = rt_array_create_double(arena, 5, data);

    /* Try to sample 6 elements (should return NULL) */
    double *sample = rt_random_static_sample_double(arena, arr, 6);
    TEST_ASSERT(sample == NULL, "Should return NULL when count > array length");

    /* Try to sample 10 elements (should return NULL) */
    sample = rt_random_static_sample_double(arena, arr, 10);
    TEST_ASSERT(sample == NULL, "Should return NULL when count >> array length");

    printf("  Invalid count correctly returns NULL\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_sample_double_null_handling()
{
    printf("Testing rt_random_static_sample_double null handling...\n");

    RtArena *arena = rt_arena_create(NULL);

    double data[] = {1.0, 2.0, 3.0};
    double *arr = rt_array_create_double(arena, 3, data);

    /* NULL arena */
    double *sample = rt_random_static_sample_double(NULL, arr, 2);
    TEST_ASSERT(sample == NULL, "Should return NULL with NULL arena");

    /* NULL array */
    sample = rt_random_static_sample_double(arena, NULL, 2);
    TEST_ASSERT(sample == NULL, "Should return NULL with NULL array");

    /* Zero count */
    sample = rt_random_static_sample_double(arena, arr, 0);
    TEST_ASSERT(sample == NULL, "Should return NULL with zero count");

    /* Negative count */
    sample = rt_random_static_sample_double(arena, arr, -1);
    TEST_ASSERT(sample == NULL, "Should return NULL with negative count");

    printf("  NULL and invalid input handling correct\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_sample_double_preserves_original()
{
    printf("Testing rt_random_static_sample_double preserves original...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Create array */
    double data[] = {10.5, 20.5, 30.5, 40.5, 50.5};
    double *arr = rt_array_create_double(arena, 5, data);

    /* Sample multiple times */
    for (int trial = 0; trial < 10; trial++) {
        double *sample = rt_random_static_sample_double(arena, arr, 3);
        TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

        /* Verify original array is unchanged */
        for (int i = 0; i < 5; i++) {
            TEST_ASSERT(fabs(arr[i] - data[i]) < 0.001, "Original array should be unchanged after sampling");
        }
    }

    printf("  Original array preserved after sampling\n");

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Sample Tests - Static String (OS Entropy)
 * ============================================================================
 * Tests for Random.sample() on string arrays.
 * ============================================================================ */

void test_rt_random_static_sample_string_basic()
{
    printf("Testing rt_random_static_sample_string basic...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Create array of strings */
    const char *data[] = {"apple", "banana", "cherry", "date", "elderberry"};
    char **arr = rt_array_create_string(arena, 5, data);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Sample 3 elements */
    char **sample = rt_random_static_sample_string(arena, arr, 3);
    TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

    /* Verify sample has correct length */
    TEST_ASSERT(rt_array_length(sample) == 3, "Sample should have 3 elements");

    /* Verify all sampled elements are from original array */
    for (int i = 0; i < 3; i++) {
        int found = 0;
        for (int j = 0; j < 5; j++) {
            if (strcmp(sample[i], data[j]) == 0) {
                found = 1;
                break;
            }
        }
        TEST_ASSERT(found, "Sampled element should be from original array");
    }

    printf("  Basic sampling works correctly\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_sample_string_no_duplicates()
{
    printf("Testing rt_random_static_sample_string no duplicates...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Create array with unique strings */
    const char *data[] = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j"};
    char **arr = rt_array_create_string(arena, 10, data);

    /* Sample 5 elements multiple times */
    for (int trial = 0; trial < 20; trial++) {
        char **sample = rt_random_static_sample_string(arena, arr, 5);
        TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

        /* Check for duplicates */
        for (int i = 0; i < 5; i++) {
            for (int j = i + 1; j < 5; j++) {
                TEST_ASSERT(strcmp(sample[i], sample[j]) != 0, "Sample should not contain duplicates");
            }
        }
    }

    printf("  Sampling without replacement produces no duplicates\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_sample_string_full_array()
{
    printf("Testing rt_random_static_sample_string full array...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Create array */
    const char *data[] = {"one", "two", "three", "four", "five"};
    char **arr = rt_array_create_string(arena, 5, data);

    /* Sample entire array (count == length) */
    char **sample = rt_random_static_sample_string(arena, arr, 5);
    TEST_ASSERT_NOT_NULL(sample, "Sample should be created when count equals array length");
    TEST_ASSERT(rt_array_length(sample) == 5, "Sample should have all 5 elements");

    /* Verify all original elements are present */
    int found[5] = {0, 0, 0, 0, 0};
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            if (strcmp(sample[i], data[j]) == 0) {
                found[j] = 1;
            }
        }
    }
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT(found[i], "All original elements should be in full sample");
    }

    printf("  Full array sampling works correctly\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_sample_string_count_exceeds_length()
{
    printf("Testing rt_random_static_sample_string count exceeds length...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Create array with 5 elements */
    const char *data[] = {"a", "b", "c", "d", "e"};
    char **arr = rt_array_create_string(arena, 5, data);

    /* Try to sample 6 elements (should return NULL) */
    char **sample = rt_random_static_sample_string(arena, arr, 6);
    TEST_ASSERT(sample == NULL, "Should return NULL when count > array length");

    /* Try to sample 10 elements (should return NULL) */
    sample = rt_random_static_sample_string(arena, arr, 10);
    TEST_ASSERT(sample == NULL, "Should return NULL when count >> array length");

    printf("  Invalid count correctly returns NULL\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_sample_string_null_handling()
{
    printf("Testing rt_random_static_sample_string null handling...\n");

    RtArena *arena = rt_arena_create(NULL);

    const char *data[] = {"x", "y", "z"};
    char **arr = rt_array_create_string(arena, 3, data);

    /* NULL arena */
    char **sample = rt_random_static_sample_string(NULL, arr, 2);
    TEST_ASSERT(sample == NULL, "Should return NULL with NULL arena");

    /* NULL array */
    sample = rt_random_static_sample_string(arena, NULL, 2);
    TEST_ASSERT(sample == NULL, "Should return NULL with NULL array");

    /* Zero count */
    sample = rt_random_static_sample_string(arena, arr, 0);
    TEST_ASSERT(sample == NULL, "Should return NULL with zero count");

    /* Negative count */
    sample = rt_random_static_sample_string(arena, arr, -1);
    TEST_ASSERT(sample == NULL, "Should return NULL with negative count");

    printf("  NULL and invalid input handling correct\n");

    rt_arena_destroy(arena);
}

void test_rt_random_static_sample_string_preserves_original()
{
    printf("Testing rt_random_static_sample_string preserves original...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Create array */
    const char *data[] = {"alpha", "beta", "gamma", "delta", "epsilon"};
    char **arr = rt_array_create_string(arena, 5, data);

    /* Sample multiple times */
    for (int trial = 0; trial < 10; trial++) {
        char **sample = rt_random_static_sample_string(arena, arr, 3);
        TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

        /* Verify original array is unchanged */
        for (int i = 0; i < 5; i++) {
            TEST_ASSERT(strcmp(arr[i], data[i]) == 0, "Original array should be unchanged after sampling");
        }
    }

    printf("  Original array preserved after sampling\n");

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Sample Tests - Instance Long (Seeded PRNG)
 * ============================================================================
 * Tests for Random.sample() instance method on long arrays.
 * ============================================================================ */

void test_rt_random_sample_long_basic()
{
    printf("Testing rt_random_sample_long basic...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    /* Create array {10, 20, 30, 40, 50} */
    long data[] = {10, 20, 30, 40, 50};
    long *arr = rt_array_create_long(arena, 5, data);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Sample 3 elements */
    long *sample = rt_random_sample_long(arena, rng, arr, 3);
    TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

    /* Verify sample has correct length */
    TEST_ASSERT(rt_array_length(sample) == 3, "Sample should have 3 elements");

    /* Verify all sampled elements are from original array */
    for (int i = 0; i < 3; i++) {
        int found = 0;
        for (int j = 0; j < 5; j++) {
            if (sample[i] == data[j]) {
                found = 1;
                break;
            }
        }
        TEST_ASSERT(found, "Sampled element should be from original array");
    }

    printf("  Basic sampling works correctly\n");

    rt_arena_destroy(arena);
}

void test_rt_random_sample_long_no_duplicates()
{
    printf("Testing rt_random_sample_long no duplicates...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 54321);

    /* Create array with unique values */
    long data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    long *arr = rt_array_create_long(arena, 10, data);

    /* Sample 5 elements multiple times */
    for (int trial = 0; trial < 20; trial++) {
        long *sample = rt_random_sample_long(arena, rng, arr, 5);
        TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

        /* Check for duplicates */
        for (int i = 0; i < 5; i++) {
            for (int j = i + 1; j < 5; j++) {
                TEST_ASSERT(sample[i] != sample[j], "Sample should not contain duplicates");
            }
        }
    }

    printf("  Sampling without replacement produces no duplicates\n");

    rt_arena_destroy(arena);
}

void test_rt_random_sample_long_reproducible()
{
    printf("Testing rt_random_sample_long reproducibility...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Create array */
    long data[] = {100, 200, 300, 400, 500};
    long *arr = rt_array_create_long(arena, 5, data);

    /* Sample with same seed twice */
    RtRandom *rng1 = rt_random_create_with_seed(arena, 99999);
    long *sample1 = rt_random_sample_long(arena, rng1, arr, 3);

    RtRandom *rng2 = rt_random_create_with_seed(arena, 99999);
    long *sample2 = rt_random_sample_long(arena, rng2, arr, 3);

    TEST_ASSERT_NOT_NULL(sample1, "First sample should be created");
    TEST_ASSERT_NOT_NULL(sample2, "Second sample should be created");

    /* Verify samples are identical */
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT(sample1[i] == sample2[i], "Samples with same seed should be identical");
    }

    printf("  Seeded sampling is reproducible\n");

    rt_arena_destroy(arena);
}

void test_rt_random_sample_long_count_exceeds_length()
{
    printf("Testing rt_random_sample_long count exceeds length...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 11111);

    /* Create array with 5 elements */
    long data[] = {1, 2, 3, 4, 5};
    long *arr = rt_array_create_long(arena, 5, data);

    /* Try to sample 6 elements (should return NULL) */
    long *sample = rt_random_sample_long(arena, rng, arr, 6);
    TEST_ASSERT(sample == NULL, "Should return NULL when count > array length");

    printf("  Invalid count correctly returns NULL\n");

    rt_arena_destroy(arena);
}

void test_rt_random_sample_long_null_handling()
{
    printf("Testing rt_random_sample_long null handling...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 22222);

    long data[] = {1, 2, 3};
    long *arr = rt_array_create_long(arena, 3, data);

    /* NULL arena */
    long *sample = rt_random_sample_long(NULL, rng, arr, 2);
    TEST_ASSERT(sample == NULL, "Should return NULL with NULL arena");

    /* NULL rng */
    sample = rt_random_sample_long(arena, NULL, arr, 2);
    TEST_ASSERT(sample == NULL, "Should return NULL with NULL rng");

    /* NULL array */
    sample = rt_random_sample_long(arena, rng, NULL, 2);
    TEST_ASSERT(sample == NULL, "Should return NULL with NULL array");

    /* Zero count */
    sample = rt_random_sample_long(arena, rng, arr, 0);
    TEST_ASSERT(sample == NULL, "Should return NULL with zero count");

    /* Negative count */
    sample = rt_random_sample_long(arena, rng, arr, -1);
    TEST_ASSERT(sample == NULL, "Should return NULL with negative count");

    printf("  NULL and invalid input handling correct\n");

    rt_arena_destroy(arena);
}

void test_rt_random_sample_long_preserves_original()
{
    printf("Testing rt_random_sample_long preserves original...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 33333);

    /* Create array */
    long data[] = {100, 200, 300, 400, 500};
    long *arr = rt_array_create_long(arena, 5, data);

    /* Sample multiple times */
    for (int trial = 0; trial < 10; trial++) {
        long *sample = rt_random_sample_long(arena, rng, arr, 3);
        TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

        /* Verify original array is unchanged */
        for (int i = 0; i < 5; i++) {
            TEST_ASSERT(arr[i] == data[i], "Original array should be unchanged after sampling");
        }
    }

    printf("  Original array preserved after sampling\n");

    rt_arena_destroy(arena);
}

void test_rt_random_sample_long_full_array()
{
    printf("Testing rt_random_sample_long full array...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 44444);

    /* Create array */
    long data[] = {10, 20, 30, 40, 50};
    long *arr = rt_array_create_long(arena, 5, data);

    /* Sample entire array (count == length) */
    long *sample = rt_random_sample_long(arena, rng, arr, 5);
    TEST_ASSERT_NOT_NULL(sample, "Sample should be created when count equals array length");
    TEST_ASSERT(rt_array_length(sample) == 5, "Sample should have all 5 elements");

    /* Verify all original elements are present (sum should match) */
    long original_sum = 10 + 20 + 30 + 40 + 50;
    long sample_sum = 0;
    for (int i = 0; i < 5; i++) {
        sample_sum += sample[i];
    }
    TEST_ASSERT(sample_sum == original_sum, "Full sample should contain all original elements");

    /* Verify no duplicates */
    for (int i = 0; i < 5; i++) {
        for (int j = i + 1; j < 5; j++) {
            TEST_ASSERT(sample[i] != sample[j], "Full array sample should have no duplicates");
        }
    }

    printf("  Full array sampling works correctly\n");

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Sample Tests - Instance Double (Seeded PRNG)
 * ============================================================================
 * Tests for Random.sample() instance method on double arrays.
 * ============================================================================ */

void test_rt_random_sample_double_basic()
{
    printf("Testing rt_random_sample_double basic...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    /* Create array {1.1, 2.2, 3.3, 4.4, 5.5} */
    double data[] = {1.1, 2.2, 3.3, 4.4, 5.5};
    double *arr = rt_array_create_double(arena, 5, data);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Sample 3 elements */
    double *sample = rt_random_sample_double(arena, rng, arr, 3);
    TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

    /* Verify sample has correct length */
    TEST_ASSERT(rt_array_length(sample) == 3, "Sample should have 3 elements");

    /* Verify all sampled elements are from original array */
    for (int i = 0; i < 3; i++) {
        int found = 0;
        for (int j = 0; j < 5; j++) {
            if (fabs(sample[i] - data[j]) < 0.001) {
                found = 1;
                break;
            }
        }
        TEST_ASSERT(found, "Sampled element should be from original array");
    }

    printf("  Basic sampling works correctly\n");

    rt_arena_destroy(arena);
}

void test_rt_random_sample_double_no_duplicates()
{
    printf("Testing rt_random_sample_double no duplicates...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 54321);

    /* Create array with unique values */
    double data[] = {1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9, 10.0};
    double *arr = rt_array_create_double(arena, 10, data);

    /* Sample 5 elements multiple times */
    for (int trial = 0; trial < 20; trial++) {
        double *sample = rt_random_sample_double(arena, rng, arr, 5);
        TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

        /* Check for duplicates */
        for (int i = 0; i < 5; i++) {
            for (int j = i + 1; j < 5; j++) {
                TEST_ASSERT(fabs(sample[i] - sample[j]) > 0.001, "Sample should not contain duplicates");
            }
        }
    }

    printf("  Sampling without replacement produces no duplicates\n");

    rt_arena_destroy(arena);
}

void test_rt_random_sample_double_reproducible()
{
    printf("Testing rt_random_sample_double reproducibility...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Create array */
    double data[] = {10.5, 20.5, 30.5, 40.5, 50.5};
    double *arr = rt_array_create_double(arena, 5, data);

    /* Sample with same seed twice */
    RtRandom *rng1 = rt_random_create_with_seed(arena, 99999);
    double *sample1 = rt_random_sample_double(arena, rng1, arr, 3);

    RtRandom *rng2 = rt_random_create_with_seed(arena, 99999);
    double *sample2 = rt_random_sample_double(arena, rng2, arr, 3);

    TEST_ASSERT_NOT_NULL(sample1, "First sample should be created");
    TEST_ASSERT_NOT_NULL(sample2, "Second sample should be created");

    /* Verify samples are identical */
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT(fabs(sample1[i] - sample2[i]) < 0.001, "Samples with same seed should be identical");
    }

    printf("  Seeded sampling is reproducible\n");

    rt_arena_destroy(arena);
}

void test_rt_random_sample_double_count_exceeds_length()
{
    printf("Testing rt_random_sample_double count exceeds length...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 11111);

    /* Create array with 5 elements */
    double data[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double *arr = rt_array_create_double(arena, 5, data);

    /* Try to sample 6 elements (should return NULL) */
    double *sample = rt_random_sample_double(arena, rng, arr, 6);
    TEST_ASSERT(sample == NULL, "Should return NULL when count > array length");

    printf("  Invalid count correctly returns NULL\n");

    rt_arena_destroy(arena);
}

void test_rt_random_sample_double_null_handling()
{
    printf("Testing rt_random_sample_double null handling...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 22222);

    double data[] = {1.0, 2.0, 3.0};
    double *arr = rt_array_create_double(arena, 3, data);

    /* NULL arena */
    double *sample = rt_random_sample_double(NULL, rng, arr, 2);
    TEST_ASSERT(sample == NULL, "Should return NULL with NULL arena");

    /* NULL rng */
    sample = rt_random_sample_double(arena, NULL, arr, 2);
    TEST_ASSERT(sample == NULL, "Should return NULL with NULL rng");

    /* NULL array */
    sample = rt_random_sample_double(arena, rng, NULL, 2);
    TEST_ASSERT(sample == NULL, "Should return NULL with NULL array");

    /* Zero count */
    sample = rt_random_sample_double(arena, rng, arr, 0);
    TEST_ASSERT(sample == NULL, "Should return NULL with zero count");

    /* Negative count */
    sample = rt_random_sample_double(arena, rng, arr, -1);
    TEST_ASSERT(sample == NULL, "Should return NULL with negative count");

    printf("  NULL and invalid input handling correct\n");

    rt_arena_destroy(arena);
}

void test_rt_random_sample_double_preserves_original()
{
    printf("Testing rt_random_sample_double preserves original...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 33333);

    /* Create array */
    double data[] = {10.5, 20.5, 30.5, 40.5, 50.5};
    double *arr = rt_array_create_double(arena, 5, data);

    /* Sample multiple times */
    for (int trial = 0; trial < 10; trial++) {
        double *sample = rt_random_sample_double(arena, rng, arr, 3);
        TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

        /* Verify original array is unchanged */
        for (int i = 0; i < 5; i++) {
            TEST_ASSERT(fabs(arr[i] - data[i]) < 0.001, "Original array should be unchanged after sampling");
        }
    }

    printf("  Original array preserved after sampling\n");

    rt_arena_destroy(arena);
}

void test_rt_random_sample_double_full_array()
{
    printf("Testing rt_random_sample_double full array...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 44444);

    /* Create array */
    double data[] = {10.5, 20.5, 30.5, 40.5, 50.5};
    double *arr = rt_array_create_double(arena, 5, data);

    /* Sample entire array (count == length) */
    double *sample = rt_random_sample_double(arena, rng, arr, 5);
    TEST_ASSERT_NOT_NULL(sample, "Sample should be created when count equals array length");
    TEST_ASSERT(rt_array_length(sample) == 5, "Sample should have all 5 elements");

    /* Verify all original elements are present (sum should match) */
    double original_sum = 10.5 + 20.5 + 30.5 + 40.5 + 50.5;
    double sample_sum = 0.0;
    for (int i = 0; i < 5; i++) {
        sample_sum += sample[i];
    }
    TEST_ASSERT(fabs(sample_sum - original_sum) < 0.001, "Full sample should contain all original elements");

    /* Verify no duplicates */
    for (int i = 0; i < 5; i++) {
        for (int j = i + 1; j < 5; j++) {
            TEST_ASSERT(fabs(sample[i] - sample[j]) > 0.001, "Full array sample should have no duplicates");
        }
    }

    printf("  Full array sampling works correctly\n");

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Sample Tests - Instance String (Seeded PRNG)
 * ============================================================================
 * Tests for Random.sample() instance method on string arrays.
 * ============================================================================ */

void test_rt_random_sample_string_basic()
{
    printf("Testing rt_random_sample_string basic...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    /* Create array of strings */
    const char *data[] = {"apple", "banana", "cherry", "date", "elderberry"};
    char **arr = rt_array_create_string(arena, 5, data);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Sample 3 elements */
    char **sample = rt_random_sample_string(arena, rng, arr, 3);
    TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

    /* Verify sample has correct length */
    TEST_ASSERT(rt_array_length(sample) == 3, "Sample should have 3 elements");

    /* Verify all sampled elements are from original array */
    for (int i = 0; i < 3; i++) {
        int found = 0;
        for (int j = 0; j < 5; j++) {
            if (strcmp(sample[i], data[j]) == 0) {
                found = 1;
                break;
            }
        }
        TEST_ASSERT(found, "Sampled element should be from original array");
    }

    printf("  Basic sampling works correctly\n");

    rt_arena_destroy(arena);
}

void test_rt_random_sample_string_no_duplicates()
{
    printf("Testing rt_random_sample_string no duplicates...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 54321);

    /* Create array with unique strings */
    const char *data[] = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j"};
    char **arr = rt_array_create_string(arena, 10, data);

    /* Sample 5 elements multiple times */
    for (int trial = 0; trial < 20; trial++) {
        char **sample = rt_random_sample_string(arena, rng, arr, 5);
        TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

        /* Check for duplicates */
        for (int i = 0; i < 5; i++) {
            for (int j = i + 1; j < 5; j++) {
                TEST_ASSERT(strcmp(sample[i], sample[j]) != 0, "Sample should not contain duplicates");
            }
        }
    }

    printf("  Sampling without replacement produces no duplicates\n");

    rt_arena_destroy(arena);
}

void test_rt_random_sample_string_reproducible()
{
    printf("Testing rt_random_sample_string reproducibility...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Create array */
    const char *data[] = {"alpha", "beta", "gamma", "delta", "epsilon"};
    char **arr = rt_array_create_string(arena, 5, data);

    /* Sample with same seed twice */
    RtRandom *rng1 = rt_random_create_with_seed(arena, 99999);
    char **sample1 = rt_random_sample_string(arena, rng1, arr, 3);

    RtRandom *rng2 = rt_random_create_with_seed(arena, 99999);
    char **sample2 = rt_random_sample_string(arena, rng2, arr, 3);

    TEST_ASSERT_NOT_NULL(sample1, "First sample should be created");
    TEST_ASSERT_NOT_NULL(sample2, "Second sample should be created");

    /* Verify samples are identical */
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT(strcmp(sample1[i], sample2[i]) == 0, "Samples with same seed should be identical");
    }

    printf("  Seeded sampling is reproducible\n");

    rt_arena_destroy(arena);
}

void test_rt_random_sample_string_count_exceeds_length()
{
    printf("Testing rt_random_sample_string count exceeds length...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 11111);

    /* Create array with 5 elements */
    const char *data[] = {"a", "b", "c", "d", "e"};
    char **arr = rt_array_create_string(arena, 5, data);

    /* Try to sample 6 elements (should return NULL) */
    char **sample = rt_random_sample_string(arena, rng, arr, 6);
    TEST_ASSERT(sample == NULL, "Should return NULL when count > array length");

    printf("  Invalid count correctly returns NULL\n");

    rt_arena_destroy(arena);
}

void test_rt_random_sample_string_null_handling()
{
    printf("Testing rt_random_sample_string null handling...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 22222);

    const char *data[] = {"x", "y", "z"};
    char **arr = rt_array_create_string(arena, 3, data);

    /* NULL arena */
    char **sample = rt_random_sample_string(NULL, rng, arr, 2);
    TEST_ASSERT(sample == NULL, "Should return NULL with NULL arena");

    /* NULL rng */
    sample = rt_random_sample_string(arena, NULL, arr, 2);
    TEST_ASSERT(sample == NULL, "Should return NULL with NULL rng");

    /* NULL array */
    sample = rt_random_sample_string(arena, rng, NULL, 2);
    TEST_ASSERT(sample == NULL, "Should return NULL with NULL array");

    /* Zero count */
    sample = rt_random_sample_string(arena, rng, arr, 0);
    TEST_ASSERT(sample == NULL, "Should return NULL with zero count");

    /* Negative count */
    sample = rt_random_sample_string(arena, rng, arr, -1);
    TEST_ASSERT(sample == NULL, "Should return NULL with negative count");

    printf("  NULL and invalid input handling correct\n");

    rt_arena_destroy(arena);
}

void test_rt_random_sample_string_preserves_original()
{
    printf("Testing rt_random_sample_string preserves original...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 33333);

    /* Create array */
    const char *data[] = {"alpha", "beta", "gamma", "delta", "epsilon"};
    char **arr = rt_array_create_string(arena, 5, data);

    /* Sample multiple times */
    for (int trial = 0; trial < 10; trial++) {
        char **sample = rt_random_sample_string(arena, rng, arr, 3);
        TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

        /* Verify original array is unchanged */
        for (int i = 0; i < 5; i++) {
            TEST_ASSERT(strcmp(arr[i], data[i]) == 0, "Original array should be unchanged after sampling");
        }
    }

    printf("  Original array preserved after sampling\n");

    rt_arena_destroy(arena);
}

void test_rt_random_sample_string_full_array()
{
    printf("Testing rt_random_sample_string full array...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 44444);

    /* Create array */
    const char *data[] = {"alpha", "beta", "gamma", "delta", "epsilon"};
    char **arr = rt_array_create_string(arena, 5, data);

    /* Sample entire array (count == length) */
    char **sample = rt_random_sample_string(arena, rng, arr, 5);
    TEST_ASSERT_NOT_NULL(sample, "Sample should be created when count equals array length");
    TEST_ASSERT(rt_array_length(sample) == 5, "Sample should have all 5 elements");

    /* Verify all original elements are present (each must be found) */
    for (int i = 0; i < 5; i++) {
        int found = 0;
        for (int j = 0; j < 5; j++) {
            if (strcmp(sample[j], data[i]) == 0) {
                found = 1;
                break;
            }
        }
        TEST_ASSERT(found, "Full sample should contain all original elements");
    }

    /* Verify no duplicates */
    for (int i = 0; i < 5; i++) {
        for (int j = i + 1; j < 5; j++) {
            TEST_ASSERT(strcmp(sample[i], sample[j]) != 0, "Full array sample should have no duplicates");
        }
    }

    printf("  Full array sampling works correctly\n");

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Comprehensive Edge Case Tests - Empty Arrays
 * ============================================================================ */

void test_rt_random_shuffle_empty_array()
{
    printf("Testing shuffle with empty arrays...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 12345);

    /* Static shuffle of empty long array - should not crash */
    long *empty_long = rt_array_alloc_long(arena, 0, 0);
    rt_random_static_shuffle_long(empty_long);
    TEST_ASSERT(rt_array_length(empty_long) == 0, "Empty long array should remain empty after shuffle");

    /* Static shuffle of empty double array */
    double *empty_double = rt_array_alloc_double(arena, 0, 0.0);
    rt_random_static_shuffle_double(empty_double);
    TEST_ASSERT(rt_array_length(empty_double) == 0, "Empty double array should remain empty after shuffle");

    /* Static shuffle of empty string array */
    char **empty_string = rt_array_alloc_string(arena, 0, NULL);
    rt_random_static_shuffle_string(empty_string);
    TEST_ASSERT(rt_array_length(empty_string) == 0, "Empty string array should remain empty after shuffle");

    /* Instance shuffle of empty arrays */
    long *empty_long2 = rt_array_alloc_long(arena, 0, 0);
    rt_random_shuffle_long(rng, empty_long2);
    TEST_ASSERT(rt_array_length(empty_long2) == 0, "Empty long array should remain empty after seeded shuffle");

    double *empty_double2 = rt_array_alloc_double(arena, 0, 0.0);
    rt_random_shuffle_double(rng, empty_double2);
    TEST_ASSERT(rt_array_length(empty_double2) == 0, "Empty double array should remain empty after seeded shuffle");

    printf("  Empty array shuffle handled correctly\n");

    rt_arena_destroy(arena);
}

void test_rt_random_sample_empty_array()
{
    printf("Testing sample with empty arrays...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 12345);

    /* Create empty arrays */
    long *empty_long = rt_array_alloc_long(arena, 0, 0);
    double *empty_double = rt_array_alloc_double(arena, 0, 0.0);
    char **empty_string = rt_array_alloc_string(arena, 0, NULL);

    /* Static sample from empty arrays - should return NULL */
    long *sample_long = rt_random_static_sample_long(arena, empty_long, 1);
    TEST_ASSERT(sample_long == NULL, "Sampling from empty long array should return NULL");

    double *sample_double = rt_random_static_sample_double(arena, empty_double, 1);
    TEST_ASSERT(sample_double == NULL, "Sampling from empty double array should return NULL");

    char **sample_string = rt_random_static_sample_string(arena, empty_string, 1);
    TEST_ASSERT(sample_string == NULL, "Sampling from empty string array should return NULL");

    /* Instance sample from empty arrays */
    sample_long = rt_random_sample_long(arena, rng, empty_long, 1);
    TEST_ASSERT(sample_long == NULL, "Seeded sampling from empty long array should return NULL");

    sample_double = rt_random_sample_double(arena, rng, empty_double, 1);
    TEST_ASSERT(sample_double == NULL, "Seeded sampling from empty double array should return NULL");

    sample_string = rt_random_sample_string(arena, rng, empty_string, 1);
    TEST_ASSERT(sample_string == NULL, "Seeded sampling from empty string array should return NULL");

    printf("  Empty array sample handled correctly\n");

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Comprehensive Edge Case Tests - Single Element
 * ============================================================================ */

void test_rt_random_sample_single_element_all_types()
{
    printf("Testing sample single element for all types...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 55555);

    /* Static sample single from long array */
    long long_data[] = {42};
    long *long_arr = rt_array_create_long(arena, 1, long_data);
    long *long_sample = rt_random_static_sample_long(arena, long_arr, 1);
    TEST_ASSERT_NOT_NULL(long_sample, "Single element long sample should succeed");
    TEST_ASSERT(rt_array_length(long_sample) == 1, "Single element sample should have length 1");
    TEST_ASSERT(long_sample[0] == 42, "Single element sample should be 42");

    /* Static sample single from double array */
    double double_data[] = {3.14};
    double *double_arr = rt_array_create_double(arena, 1, double_data);
    double *double_sample = rt_random_static_sample_double(arena, double_arr, 1);
    TEST_ASSERT_NOT_NULL(double_sample, "Single element double sample should succeed");
    TEST_ASSERT(rt_array_length(double_sample) == 1, "Single element sample should have length 1");
    TEST_ASSERT(fabs(double_sample[0] - 3.14) < 0.001, "Single element sample should be 3.14");

    /* Static sample single from string array */
    const char *string_data[] = {"hello"};
    char **string_arr = rt_array_create_string(arena, 1, string_data);
    char **string_sample = rt_random_static_sample_string(arena, string_arr, 1);
    TEST_ASSERT_NOT_NULL(string_sample, "Single element string sample should succeed");
    TEST_ASSERT(rt_array_length(string_sample) == 1, "Single element sample should have length 1");
    TEST_ASSERT(strcmp(string_sample[0], "hello") == 0, "Single element sample should be 'hello'");

    /* Instance sample single from long array */
    long_sample = rt_random_sample_long(arena, rng, long_arr, 1);
    TEST_ASSERT_NOT_NULL(long_sample, "Seeded single element long sample should succeed");
    TEST_ASSERT(long_sample[0] == 42, "Seeded single element sample should be 42");

    /* Instance sample single from double array */
    double_sample = rt_random_sample_double(arena, rng, double_arr, 1);
    TEST_ASSERT_NOT_NULL(double_sample, "Seeded single element double sample should succeed");
    TEST_ASSERT(fabs(double_sample[0] - 3.14) < 0.001, "Seeded single element sample should be 3.14");

    /* Instance sample single from string array */
    string_sample = rt_random_sample_string(arena, rng, string_arr, 1);
    TEST_ASSERT_NOT_NULL(string_sample, "Seeded single element string sample should succeed");
    TEST_ASSERT(strcmp(string_sample[0], "hello") == 0, "Seeded single element sample should be 'hello'");

    printf("  Single element sample for all types works correctly\n");

    rt_arena_destroy(arena);
}

void test_rt_random_shuffle_single_element_all_types()
{
    printf("Testing shuffle single element for all types...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 55555);

    /* Static shuffle single long */
    long long_data[] = {99};
    long *long_arr = rt_array_create_long(arena, 1, long_data);
    rt_random_static_shuffle_long(long_arr);
    TEST_ASSERT(long_arr[0] == 99, "Single element should remain unchanged after shuffle");

    /* Static shuffle single double */
    double double_data[] = {2.718};
    double *double_arr = rt_array_create_double(arena, 1, double_data);
    rt_random_static_shuffle_double(double_arr);
    TEST_ASSERT(fabs(double_arr[0] - 2.718) < 0.001, "Single double should remain unchanged after shuffle");

    /* Static shuffle single string */
    const char *string_data[] = {"world"};
    char **string_arr = rt_array_create_string(arena, 1, string_data);
    rt_random_static_shuffle_string(string_arr);
    TEST_ASSERT(strcmp(string_arr[0], "world") == 0, "Single string should remain unchanged after shuffle");

    /* Instance shuffle single long */
    long long_data2[] = {77};
    long *long_arr2 = rt_array_create_long(arena, 1, long_data2);
    rt_random_shuffle_long(rng, long_arr2);
    TEST_ASSERT(long_arr2[0] == 77, "Seeded single element should remain unchanged");

    /* Instance shuffle single double */
    double double_data2[] = {1.414};
    double *double_arr2 = rt_array_create_double(arena, 1, double_data2);
    rt_random_shuffle_double(rng, double_arr2);
    TEST_ASSERT(fabs(double_arr2[0] - 1.414) < 0.001, "Seeded single double should remain unchanged");

    /* Instance shuffle single string */
    const char *string_data2[] = {"test"};
    char **string_arr2 = rt_array_create_string(arena, 1, string_data2);
    rt_random_shuffle_string(rng, string_arr2);
    TEST_ASSERT(strcmp(string_arr2[0], "test") == 0, "Seeded single string should remain unchanged");

    printf("  Single element shuffle for all types works correctly\n");

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Reproducibility Tests for Sample Operations
 * ============================================================================ */

void test_rt_random_sample_double_reproducible_extended()
{
    printf("Testing rt_random_sample_double extended reproducibility...\n");

    RtArena *arena = rt_arena_create(NULL);

    double data[] = {1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9, 10.0};
    double *arr = rt_array_create_double(arena, 10, data);

    /* Same seed should produce same samples across multiple calls */
    RtRandom *rng1 = rt_random_create_with_seed(arena, 77777);
    RtRandom *rng2 = rt_random_create_with_seed(arena, 77777);

    for (int trial = 0; trial < 5; trial++) {
        double *sample1 = rt_random_sample_double(arena, rng1, arr, 4);
        double *sample2 = rt_random_sample_double(arena, rng2, arr, 4);

        TEST_ASSERT_NOT_NULL(sample1, "Sample 1 should succeed");
        TEST_ASSERT_NOT_NULL(sample2, "Sample 2 should succeed");

        for (int i = 0; i < 4; i++) {
            TEST_ASSERT(fabs(sample1[i] - sample2[i]) < 0.001,
                "Samples with same seed should be identical");
        }
    }

    printf("  Sample double extended reproducibility verified\n");

    rt_arena_destroy(arena);
}

void test_rt_random_sample_string_reproducible_extended()
{
    printf("Testing rt_random_sample_string extended reproducibility...\n");

    RtArena *arena = rt_arena_create(NULL);

    const char *data[] = {"one", "two", "three", "four", "five", "six", "seven", "eight"};
    char **arr = rt_array_create_string(arena, 8, data);

    /* Same seed should produce same samples across multiple calls */
    RtRandom *rng1 = rt_random_create_with_seed(arena, 88888);
    RtRandom *rng2 = rt_random_create_with_seed(arena, 88888);

    for (int trial = 0; trial < 5; trial++) {
        char **sample1 = rt_random_sample_string(arena, rng1, arr, 3);
        char **sample2 = rt_random_sample_string(arena, rng2, arr, 3);

        TEST_ASSERT_NOT_NULL(sample1, "Sample 1 should succeed");
        TEST_ASSERT_NOT_NULL(sample2, "Sample 2 should succeed");

        for (int i = 0; i < 3; i++) {
            TEST_ASSERT(strcmp(sample1[i], sample2[i]) == 0,
                "String samples with same seed should be identical");
        }
    }

    printf("  Sample string extended reproducibility verified\n");

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Statistical Distribution Tests
 * ============================================================================ */

void test_rt_random_sample_distribution()
{
    printf("Testing sample distribution uniformity...\n");

    RtArena *arena = rt_arena_create(NULL);

    long data[] = {0, 1, 2, 3, 4};
    long *arr = rt_array_create_long(arena, 5, data);

    /* Count how often each element appears in samples */
    int counts[5] = {0, 0, 0, 0, 0};
    int num_samples = 10000;

    for (int trial = 0; trial < num_samples; trial++) {
        long *sample = rt_random_static_sample_long(arena, arr, 2);
        TEST_ASSERT_NOT_NULL(sample, "Sample should succeed");
        counts[sample[0]]++;
        counts[sample[1]]++;
    }

    /* Each element should appear roughly equally (40% each with 2 samples from 5) */
    /* With 10000 samples of 2, each element expected 4000 times */
    int expected = num_samples * 2 / 5;
    double tolerance = expected * 0.15; /* 15% tolerance */

    for (int i = 0; i < 5; i++) {
        TEST_ASSERT(counts[i] > expected - tolerance && counts[i] < expected + tolerance,
            "Sample distribution should be approximately uniform");
    }

    printf("  Sample distribution: [%d, %d, %d, %d, %d] (expected ~%d each)\n",
        counts[0], counts[1], counts[2], counts[3], counts[4], expected);

    rt_arena_destroy(arena);
}

void test_rt_random_shuffle_distribution_extended()
{
    printf("Testing shuffle distribution for position uniformity...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Count how often each value appears at each position */
    int position_counts[5][5] = {{0}}; /* [value][position] */
    int num_trials = 10000;

    for (int trial = 0; trial < num_trials; trial++) {
        long data[] = {0, 1, 2, 3, 4};
        long *arr = rt_array_create_long(arena, 5, data);
        rt_random_static_shuffle_long(arr);

        for (int pos = 0; pos < 5; pos++) {
            position_counts[arr[pos]][pos]++;
        }
    }

    /* Each value should appear at each position roughly 20% of the time */
    int expected = num_trials / 5;
    double tolerance = expected * 0.15;

    int failed = 0;
    for (int val = 0; val < 5; val++) {
        for (int pos = 0; pos < 5; pos++) {
            if (position_counts[val][pos] < expected - tolerance ||
                position_counts[val][pos] > expected + tolerance) {
                failed = 1;
            }
        }
    }

    TEST_ASSERT(!failed, "Shuffle should produce uniform position distribution");

    printf("  Shuffle distribution verified (expected ~%d per position)\n", expected);

    rt_arena_destroy(arena);
}

void test_rt_random_choice_statistical_chi_squared()
{
    printf("Testing choice statistical properties (chi-squared)...\n");

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

    printf("  Chi-squared statistic: %.2f (critical value ~13.28 at p=0.01)\n", chi_squared);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Integration Tests - Combining Operations
 * ============================================================================ */

void test_integration_shuffle_then_sample()
{
    printf("Testing shuffle then sample integration...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 99999);

    /* Create array, shuffle it, then sample */
    long data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    long *arr = rt_array_create_long(arena, 10, data);

    /* Shuffle in place */
    rt_random_shuffle_long(rng, arr);

    /* Sample from shuffled array */
    long *sample = rt_random_sample_long(arena, rng, arr, 3);
    TEST_ASSERT_NOT_NULL(sample, "Sample from shuffled array should succeed");
    TEST_ASSERT(rt_array_length(sample) == 3, "Sample should have 3 elements");

    /* Verify all sampled elements are from original set */
    for (int i = 0; i < 3; i++) {
        int found = 0;
        for (int j = 0; j < 10; j++) {
            if (sample[i] == data[j]) {
                found = 1;
                break;
            }
        }
        TEST_ASSERT(found, "Sampled element should be from original array");
    }

    /* Verify no duplicates in sample */
    TEST_ASSERT(sample[0] != sample[1] && sample[1] != sample[2] && sample[0] != sample[2],
        "Sample should have no duplicates");

    printf("  Shuffle then sample integration works correctly\n");

    rt_arena_destroy(arena);
}

void test_integration_sample_then_choice()
{
    printf("Testing sample then choice integration...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 11111);

    /* Create array, sample from it, then choose from sample */
    long data[] = {100, 200, 300, 400, 500};
    long *arr = rt_array_create_long(arena, 5, data);

    /* Sample 3 elements */
    long *sample = rt_random_sample_long(arena, rng, arr, 3);
    TEST_ASSERT_NOT_NULL(sample, "Sample should succeed");

    /* Choose from the sample multiple times */
    for (int i = 0; i < 10; i++) {
        long choice = rt_random_choice_long(rng, sample, 3);

        /* Verify choice is in original data */
        int found = 0;
        for (int j = 0; j < 5; j++) {
            if (choice == data[j]) {
                found = 1;
                break;
            }
        }
        TEST_ASSERT(found, "Choice from sample should be from original array");
    }

    printf("  Sample then choice integration works correctly\n");

    rt_arena_destroy(arena);
}

void test_integration_multiple_samples_different_seeds()
{
    printf("Testing multiple samples with different seeds...\n");

    RtArena *arena = rt_arena_create(NULL);

    const char *data[] = {"apple", "banana", "cherry", "date", "elderberry"};
    char **arr = rt_array_create_string(arena, 5, data);

    /* Create multiple RNGs with different seeds */
    RtRandom *rng1 = rt_random_create_with_seed(arena, 11111);
    RtRandom *rng2 = rt_random_create_with_seed(arena, 22222);
    RtRandom *rng3 = rt_random_create_with_seed(arena, 33333);

    /* Take samples with each RNG */
    char **sample1 = rt_random_sample_string(arena, rng1, arr, 2);
    char **sample2 = rt_random_sample_string(arena, rng2, arr, 2);
    char **sample3 = rt_random_sample_string(arena, rng3, arr, 2);

    TEST_ASSERT_NOT_NULL(sample1, "Sample 1 should succeed");
    TEST_ASSERT_NOT_NULL(sample2, "Sample 2 should succeed");
    TEST_ASSERT_NOT_NULL(sample3, "Sample 3 should succeed");

    /* At least one pair should differ (statistically almost certain) */
    int all_same = (strcmp(sample1[0], sample2[0]) == 0 && strcmp(sample1[1], sample2[1]) == 0) &&
                   (strcmp(sample2[0], sample3[0]) == 0 && strcmp(sample2[1], sample3[1]) == 0);
    TEST_ASSERT(!all_same, "Different seeds should produce different samples");

    printf("  Multiple samples with different seeds work correctly\n");

    rt_arena_destroy(arena);
}

void test_integration_weighted_choice_after_shuffle()
{
    printf("Testing weighted choice after shuffle...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 44444);

    /* Create array with values and corresponding weights */
    long data[] = {1, 2, 3, 4, 5};
    long *arr = rt_array_create_long(arena, 5, data);
    double weights_data[] = {5.0, 4.0, 3.0, 2.0, 1.0};
    double *weights = rt_array_create_double(arena, 5, weights_data);

    /* Shuffle the array (weights stay matched by index) */
    /* Note: In real use, would need to shuffle weights too - this tests the API */
    rt_random_shuffle_long(rng, arr);

    /* Make weighted choices - weights still correspond to shuffled positions */
    int counts[5] = {0};
    for (int i = 0; i < 1000; i++) {
        long choice = rt_random_weighted_choice_long(rng, arr, weights);
        counts[choice - 1]++;
    }

    /* Just verify the function works without crashing */
    int total = counts[0] + counts[1] + counts[2] + counts[3] + counts[4];
    TEST_ASSERT(total == 1000, "All choices should be valid");

    printf("  Weighted choice after shuffle works correctly\n");

    rt_arena_destroy(arena);
}

void test_integration_reproducible_workflow()
{
    printf("Testing reproducible workflow with multiple operations...\n");

    RtArena *arena = rt_arena_create(NULL);

    long data[] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
    long *arr = rt_array_create_long(arena, 10, data);

    /* Run same workflow twice with same seed */
    for (int run = 0; run < 2; run++) {
        RtRandom *rng = rt_random_create_with_seed(arena, 55555);

        /* 1. Make some random choices */
        long choice1 = rt_random_choice_long(rng, arr, 10);
        long choice2 = rt_random_choice_long(rng, arr, 10);

        /* 2. Sample from array */
        long *sample = rt_random_sample_long(arena, rng, arr, 3);

        /* 3. Shuffle a copy */
        long copy_data[] = {1, 2, 3, 4, 5};
        long *copy = rt_array_create_long(arena, 5, copy_data);
        rt_random_shuffle_long(rng, copy);

        /* Store results from first run */
        static long first_choice1, first_choice2;
        static long first_sample[3];
        static long first_shuffled[5];

        if (run == 0) {
            first_choice1 = choice1;
            first_choice2 = choice2;
            for (int i = 0; i < 3; i++) first_sample[i] = sample[i];
            for (int i = 0; i < 5; i++) first_shuffled[i] = copy[i];
        } else {
            /* Compare with first run */
            TEST_ASSERT(choice1 == first_choice1, "Choice 1 should be reproducible");
            TEST_ASSERT(choice2 == first_choice2, "Choice 2 should be reproducible");
            for (int i = 0; i < 3; i++) {
                TEST_ASSERT(sample[i] == first_sample[i], "Sample should be reproducible");
            }
            for (int i = 0; i < 5; i++) {
                TEST_ASSERT(copy[i] == first_shuffled[i], "Shuffle should be reproducible");
            }
        }
    }

    printf("  Reproducible workflow verified\n");

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

void test_rt_random_main()
{
    printf("\n");
    printf("================================================\n");
    printf(" Runtime Random Tests\n");
    printf("================================================\n");

    /* Core entropy function tests */
    test_rt_random_fill_entropy_basic();
    test_rt_random_fill_entropy_different_calls();
    test_rt_random_fill_entropy_small_buffer();
    test_rt_random_fill_entropy_large_buffer();
    test_rt_random_fill_entropy_null_buffer();
    test_rt_random_fill_entropy_zero_length();
    test_rt_random_fill_entropy_statistical_distribution();

    /* xoshiro256** PRNG tests */
    test_rt_random_create_with_seed_basic();
    test_rt_random_create_with_seed_deterministic();
    test_rt_random_create_with_seed_different_seeds();
    test_rt_random_create_with_seed_not_all_zeros();
    test_rt_random_create_with_seed_state_advances();
    test_rt_random_create_with_seed_statistical();
    test_rt_random_create_os_entropy();

    /* Factory method edge case tests */
    test_rt_random_create_null_arena();
    test_rt_random_create_with_seed_null_arena();

    /* Static value generation tests */
    test_rt_random_static_int_range();
    test_rt_random_static_int_distribution();
    test_rt_random_static_int_power_of_two_range();
    test_rt_random_static_int_large_range();
    test_rt_random_static_long_range();
    test_rt_random_static_long_power_of_two_range();
    test_rt_random_static_long_large_range();
    test_rt_random_static_double_range();
    test_rt_random_static_double_small_range();
    test_rt_random_static_double_large_range();
    test_rt_random_static_bool();
    test_rt_random_static_byte();
    test_rt_random_static_bytes();
    test_rt_random_static_gaussian();
    test_rt_random_static_gaussian_zero_stddev();
    test_rt_random_static_gaussian_extreme_stddev();

    /* Instance value generation tests */
    test_rt_random_int_range();
    test_rt_random_int_power_of_two_range();
    test_rt_random_int_large_range();
    test_rt_random_long_range();
    test_rt_random_long_power_of_two_range();
    test_rt_random_long_large_range();
    test_rt_random_double_range();
    test_rt_random_double_small_range();
    test_rt_random_bool_instance();
    test_rt_random_byte_instance();
    test_rt_random_bytes_instance();
    test_rt_random_gaussian_instance();
    test_rt_random_gaussian_extreme_stddev();

    /* Reproducibility tests */
    test_rt_random_seeded_reproducibility();
    test_rt_random_seeded_different_types_reproducibility();
    test_rt_random_seeded_bytes_reproducibility();
    test_rt_random_seeded_gaussian_reproducibility();

    /* Static batch generation tests */
    test_rt_random_static_int_many_count_and_range();
    test_rt_random_static_int_many_null_arena();
    test_rt_random_static_int_many_zero_count();
    test_rt_random_static_long_many_count_and_range();
    test_rt_random_static_long_many_null_arena();
    test_rt_random_static_double_many_count_and_range();
    test_rt_random_static_double_many_null_arena();
    test_rt_random_static_bool_many_count();
    test_rt_random_static_bool_many_null_arena();
    test_rt_random_static_gaussian_many_count_and_distribution();
    test_rt_random_static_gaussian_many_null_arena();

    /* Instance batch generation tests */
    test_rt_random_int_many_count_and_range();
    test_rt_random_int_many_null_args();
    test_rt_random_int_many_reproducibility();
    test_rt_random_long_many_count_and_range();
    test_rt_random_long_many_null_args();
    test_rt_random_long_many_reproducibility();
    test_rt_random_double_many_count_and_range();
    test_rt_random_double_many_null_args();
    test_rt_random_double_many_reproducibility();
    test_rt_random_bool_many_count();
    test_rt_random_bool_many_null_args();
    test_rt_random_bool_many_reproducibility();
    test_rt_random_gaussian_many_count_and_distribution();
    test_rt_random_gaussian_many_null_args();
    test_rt_random_gaussian_many_reproducibility();

    /* Large batch performance test */
    test_rt_random_batch_large_count();

    /* Static choice tests */
    test_rt_random_static_choice_long_basic();
    test_rt_random_static_choice_long_single_element();
    test_rt_random_static_choice_long_null_empty();
    test_rt_random_static_choice_long_distribution();
    test_rt_random_static_choice_double_basic();
    test_rt_random_static_choice_double_null_empty();
    test_rt_random_static_choice_string_basic();
    test_rt_random_static_choice_string_null_empty();
    test_rt_random_static_choice_bool_basic();
    test_rt_random_static_choice_bool_null_empty();
    test_rt_random_static_choice_byte_basic();
    test_rt_random_static_choice_byte_null_empty();

    /* Instance choice tests */
    test_rt_random_choice_long_basic();
    test_rt_random_choice_long_reproducibility();
    test_rt_random_choice_long_null_args();
    test_rt_random_choice_long_distribution();
    test_rt_random_choice_double_basic();
    test_rt_random_choice_double_null_args();
    test_rt_random_choice_string_basic();
    test_rt_random_choice_string_null_args();
    test_rt_random_choice_bool_basic();
    test_rt_random_choice_bool_null_args();
    test_rt_random_choice_byte_basic();
    test_rt_random_choice_byte_null_args();

    /* Statistical distribution tests for choice */
    test_rt_random_static_choice_double_distribution();
    test_rt_random_static_choice_string_distribution();
    test_rt_random_static_choice_byte_distribution();
    test_rt_random_choice_double_distribution();
    test_rt_random_choice_string_distribution();
    test_rt_random_choice_byte_distribution();

    /* Weight validation helper tests */
    test_rt_random_validate_weights_valid();
    test_rt_random_validate_weights_negative();
    test_rt_random_validate_weights_zero();
    test_rt_random_validate_weights_empty();
    test_rt_random_validate_weights_null();

    /* Cumulative distribution helper tests */
    test_rt_random_build_cumulative_basic();
    test_rt_random_build_cumulative_normalization();
    test_rt_random_build_cumulative_single_element();
    test_rt_random_build_cumulative_two_elements();
    test_rt_random_build_cumulative_null_arena();
    test_rt_random_build_cumulative_null_weights();
    test_rt_random_build_cumulative_empty_array();
    test_rt_random_build_cumulative_large_weights();

    /* Weighted index selection helper tests */
    test_rt_random_select_weighted_index_basic();
    test_rt_random_select_weighted_index_edge_zero();
    test_rt_random_select_weighted_index_edge_near_one();
    test_rt_random_select_weighted_index_single_element();
    test_rt_random_select_weighted_index_two_elements();
    test_rt_random_select_weighted_index_boundary_values();
    test_rt_random_select_weighted_index_null();
    test_rt_random_select_weighted_index_invalid_len();
    test_rt_random_select_weighted_index_large_array();

    /* Static weighted choice tests */
    test_rt_random_static_weighted_choice_long_basic();
    test_rt_random_static_weighted_choice_long_single_element();
    test_rt_random_static_weighted_choice_long_null_arr();
    test_rt_random_static_weighted_choice_long_null_weights();
    test_rt_random_static_weighted_choice_long_invalid_weights();
    test_rt_random_static_weighted_choice_long_distribution();

    /* Static weighted choice double tests */
    test_rt_random_static_weighted_choice_double_basic();
    test_rt_random_static_weighted_choice_double_single_element();
    test_rt_random_static_weighted_choice_double_null_arr();
    test_rt_random_static_weighted_choice_double_null_weights();
    test_rt_random_static_weighted_choice_double_invalid_weights();
    test_rt_random_static_weighted_choice_double_distribution();

    /* Static weighted choice string tests */
    test_rt_random_static_weighted_choice_string_basic();
    test_rt_random_static_weighted_choice_string_single_element();
    test_rt_random_static_weighted_choice_string_null_arr();
    test_rt_random_static_weighted_choice_string_null_weights();
    test_rt_random_static_weighted_choice_string_invalid_weights();
    test_rt_random_static_weighted_choice_string_distribution();

    /* Instance weighted choice long tests */
    test_rt_random_weighted_choice_long_basic();
    test_rt_random_weighted_choice_long_single_element();
    test_rt_random_weighted_choice_long_null_rng();
    test_rt_random_weighted_choice_long_null_arr();
    test_rt_random_weighted_choice_long_null_weights();
    test_rt_random_weighted_choice_long_invalid_weights();
    test_rt_random_weighted_choice_long_reproducible();
    test_rt_random_weighted_choice_long_distribution();

    /* Instance weighted choice double tests */
    test_rt_random_weighted_choice_double_basic();
    test_rt_random_weighted_choice_double_single_element();
    test_rt_random_weighted_choice_double_null_rng();
    test_rt_random_weighted_choice_double_null_arr();
    test_rt_random_weighted_choice_double_null_weights();
    test_rt_random_weighted_choice_double_invalid_weights();
    test_rt_random_weighted_choice_double_reproducible();
    test_rt_random_weighted_choice_double_distribution();

    /* Instance weighted choice string tests */
    test_rt_random_weighted_choice_string_basic();
    test_rt_random_weighted_choice_string_single_element();
    test_rt_random_weighted_choice_string_null_rng();
    test_rt_random_weighted_choice_string_null_arr();
    test_rt_random_weighted_choice_string_null_weights();
    test_rt_random_weighted_choice_string_invalid_weights();
    test_rt_random_weighted_choice_string_reproducible();
    test_rt_random_weighted_choice_string_distribution();

    /* Weighted selection probability distribution tests */
    test_weighted_distribution_equal_weights_uniform();
    test_weighted_distribution_extreme_ratio();
    test_weighted_distribution_single_element();
    test_weighted_distribution_large_sample_accuracy();
    test_weighted_distribution_seeded_prng_reproducible();
    test_weighted_distribution_os_entropy_varies();
    test_weighted_distribution_static_vs_instance();

    /* Integration test: Weighted loot drop scenario */
    test_integration_weighted_loot_drop_static();
    test_integration_weighted_loot_drop_seeded();
    test_integration_weighted_loot_drop_all_tiers();

    /* Shuffle tests - Static (OS Entropy) */
    test_rt_random_static_shuffle_long_basic();
    test_rt_random_static_shuffle_double_basic();
    test_rt_random_static_shuffle_string_basic();
    test_rt_random_static_shuffle_bool_basic();
    test_rt_random_static_shuffle_byte_basic();
    test_rt_random_static_shuffle_null_handling();
    test_rt_random_static_shuffle_single_element();

    /* Shuffle tests - Instance (Seeded PRNG) */
    test_rt_random_shuffle_long_basic();
    test_rt_random_shuffle_reproducible();
    test_rt_random_shuffle_null_rng();
    test_rt_random_shuffle_all_types_seeded();
    test_rt_random_shuffle_distribution();
    test_rt_random_shuffle_distribution_seeded();

    /* Sample tests - Static Long (OS Entropy) */
    test_rt_random_static_sample_long_basic();
    test_rt_random_static_sample_long_no_duplicates();
    test_rt_random_static_sample_long_full_array();
    test_rt_random_static_sample_long_single_element();
    test_rt_random_static_sample_long_count_exceeds_length();
    test_rt_random_static_sample_long_null_handling();
    test_rt_random_static_sample_long_preserves_original();
    test_rt_random_static_sample_long_distribution();

    /* Sample tests - Static Double (OS Entropy) */
    test_rt_random_static_sample_double_basic();
    test_rt_random_static_sample_double_no_duplicates();
    test_rt_random_static_sample_double_full_array();
    test_rt_random_static_sample_double_count_exceeds_length();
    test_rt_random_static_sample_double_null_handling();
    test_rt_random_static_sample_double_preserves_original();

    /* Sample tests - Static String (OS Entropy) */
    test_rt_random_static_sample_string_basic();
    test_rt_random_static_sample_string_no_duplicates();
    test_rt_random_static_sample_string_full_array();
    test_rt_random_static_sample_string_count_exceeds_length();
    test_rt_random_static_sample_string_null_handling();
    test_rt_random_static_sample_string_preserves_original();

    /* Sample tests - Instance Long (Seeded PRNG) */
    test_rt_random_sample_long_basic();
    test_rt_random_sample_long_no_duplicates();
    test_rt_random_sample_long_reproducible();
    test_rt_random_sample_long_count_exceeds_length();
    test_rt_random_sample_long_null_handling();
    test_rt_random_sample_long_preserves_original();
    test_rt_random_sample_long_full_array();

    /* Sample tests - Instance Double (Seeded PRNG) */
    test_rt_random_sample_double_basic();
    test_rt_random_sample_double_no_duplicates();
    test_rt_random_sample_double_reproducible();
    test_rt_random_sample_double_count_exceeds_length();
    test_rt_random_sample_double_null_handling();
    test_rt_random_sample_double_preserves_original();
    test_rt_random_sample_double_full_array();

    /* Sample tests - Instance String (Seeded PRNG) */
    test_rt_random_sample_string_basic();
    test_rt_random_sample_string_no_duplicates();
    test_rt_random_sample_string_reproducible();
    test_rt_random_sample_string_count_exceeds_length();
    test_rt_random_sample_string_null_handling();
    test_rt_random_sample_string_preserves_original();
    test_rt_random_sample_string_full_array();

    /* Comprehensive edge case tests - empty arrays */
    test_rt_random_shuffle_empty_array();
    test_rt_random_sample_empty_array();

    /* Comprehensive edge case tests - single element */
    test_rt_random_sample_single_element_all_types();
    test_rt_random_shuffle_single_element_all_types();

    /* Reproducibility tests for sample operations */
    test_rt_random_sample_double_reproducible_extended();
    test_rt_random_sample_string_reproducible_extended();

    /* Statistical distribution tests */
    test_rt_random_sample_distribution();
    test_rt_random_shuffle_distribution_extended();
    test_rt_random_choice_statistical_chi_squared();

    /* Integration tests - combining operations */
    test_integration_shuffle_then_sample();
    test_integration_sample_then_choice();
    test_integration_multiple_samples_different_seeds();
    test_integration_weighted_choice_after_shuffle();
    test_integration_reproducible_workflow();

    printf("------------------------------------------------\n");
    printf(" All runtime random tests passed!\n");
    printf("================================================\n");
}
