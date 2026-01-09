// tests/unit/runtime_random_tests_core.c
// Tests for runtime random core functionality: entropy, RNG creation, seeding

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
#include "../test_harness.h"

/* ============================================================================
 * rt_random_fill_entropy() Tests
 * ============================================================================
 * Tests for the core entropy function that uses OS-provided randomness.
 * ============================================================================ */

static void test_rt_random_fill_entropy_basic(void)
{
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
}

static void test_rt_random_fill_entropy_different_calls(void)
{
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
}

static void test_rt_random_fill_entropy_small_buffer(void)
{
    uint8_t buf[1];
    buf[0] = 0;

    /* This should work without errors */
    rt_random_fill_entropy(buf, sizeof(buf));
}

static void test_rt_random_fill_entropy_large_buffer(void)
{
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

    free(buf);
}

static void test_rt_random_fill_entropy_null_buffer(void)
{
    /* Should handle NULL gracefully without crashing */
    rt_random_fill_entropy(NULL, 32);
}

static void test_rt_random_fill_entropy_zero_length(void)
{
    uint8_t buf[4] = {0xAA, 0xBB, 0xCC, 0xDD};

    /* Should handle zero length without modifying buffer */
    rt_random_fill_entropy(buf, 0);

    /* Buffer should be unchanged */
    TEST_ASSERT(buf[0] == 0xAA, "Buffer should be unchanged with zero length");
    TEST_ASSERT(buf[1] == 0xBB, "Buffer should be unchanged with zero length");
    TEST_ASSERT(buf[2] == 0xCC, "Buffer should be unchanged with zero length");
    TEST_ASSERT(buf[3] == 0xDD, "Buffer should be unchanged with zero length");
}

static void test_rt_random_fill_entropy_statistical_distribution(void)
{
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

    /* Each quarter should have roughly 1/4 of the bytes (25% +/- some variance) */
    int expected = (int)size / 4;
    int tolerance = expected / 4;  /* Allow 25% deviation */

    for (int q = 0; q < 4; q++) {
        int deviation = abs(quarters[q] - expected);
        TEST_ASSERT(deviation < tolerance, "Distribution should be roughly uniform");
    }

    free(buf);
}

/* ============================================================================
 * Factory Method Tests
 * ============================================================================
 * Tests for rt_random_create() and rt_random_create_with_seed()
 * ============================================================================ */

static void test_rt_random_create_with_seed_basic(void)
{
    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "Random generator should be created");
    TEST_ASSERT(rng->is_seeded == 1, "Should be marked as seeded");

    /* Verify state is non-zero after seeding */
    int has_nonzero = (rng->state[0] != 0 || rng->state[1] != 0 ||
                       rng->state[2] != 0 || rng->state[3] != 0);
    TEST_ASSERT(has_nonzero, "State should be initialized (not all zeros)");

    rt_arena_destroy(arena);
}

static void test_rt_random_create_with_seed_deterministic(void)
{
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

    rt_arena_destroy(arena);
}

static void test_rt_random_create_with_seed_different_seeds(void)
{
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

    rt_arena_destroy(arena);
}

static void test_rt_random_create_with_seed_not_all_zeros(void)
{
    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Even a zero seed should produce non-zero state */
    RtRandom *rng = rt_random_create_with_seed(arena, 0);
    TEST_ASSERT_NOT_NULL(rng, "Generator should be created");

    int has_nonzero = (rng->state[0] != 0 || rng->state[1] != 0 ||
                       rng->state[2] != 0 || rng->state[3] != 0);
    TEST_ASSERT(has_nonzero, "Zero seed should still produce non-zero state");

    rt_arena_destroy(arena);
}

static void test_rt_random_create_with_seed_state_advances(void)
{
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

    rt_arena_destroy(arena);
}

static void test_rt_random_create_with_seed_statistical(void)
{
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
        }
    }
    TEST_ASSERT(all_within_tolerance, "Distribution should be roughly uniform");

    rt_arena_destroy(arena);
}

static void test_rt_random_create_os_entropy(void)
{
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

    rt_arena_destroy(arena);
}

static void test_rt_random_create_null_arena(void)
{
    /* rt_random_create should return NULL when arena is NULL */
    RtRandom *rng = rt_random_create(NULL);
    TEST_ASSERT(rng == NULL, "rt_random_create(NULL) should return NULL");
}

static void test_rt_random_create_with_seed_null_arena(void)
{
    /* rt_random_create_with_seed should return NULL when arena is NULL */
    RtRandom *rng = rt_random_create_with_seed(NULL, 12345);
    TEST_ASSERT(rng == NULL, "rt_random_create_with_seed(NULL, seed) should return NULL");
}

/* ============================================================================
 * Reproducibility Tests
 * ============================================================================
 * These tests verify that seeded generators produce identical sequences.
 * ============================================================================ */

static void test_rt_random_seeded_reproducibility(void)
{
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

    rt_arena_destroy(arena);
}

static void test_rt_random_seeded_different_types_reproducibility(void)
{
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

    rt_arena_destroy(arena);
}

static void test_rt_random_seeded_bytes_reproducibility(void)
{
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

    rt_arena_destroy(arena);
}

static void test_rt_random_seeded_gaussian_reproducibility(void)
{
    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng1 = rt_random_create_with_seed(arena, 42);
    RtRandom *rng2 = rt_random_create_with_seed(arena, 42);

    for (int i = 0; i < 100; i++) {
        double v1 = rt_random_gaussian(rng1, 0.0, 1.0);
        double v2 = rt_random_gaussian(rng2, 0.0, 1.0);
        TEST_ASSERT(v1 == v2, "Gaussian should match for same seed");
    }

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

void test_rt_random_core_main(void)
{
    TEST_SECTION("Runtime Random Core");

    /* rt_random_fill_entropy() tests */
    TEST_RUN("fill_entropy_basic", test_rt_random_fill_entropy_basic);
    TEST_RUN("fill_entropy_different_calls", test_rt_random_fill_entropy_different_calls);
    TEST_RUN("fill_entropy_small_buffer", test_rt_random_fill_entropy_small_buffer);
    TEST_RUN("fill_entropy_large_buffer", test_rt_random_fill_entropy_large_buffer);
    TEST_RUN("fill_entropy_null_buffer", test_rt_random_fill_entropy_null_buffer);
    TEST_RUN("fill_entropy_zero_length", test_rt_random_fill_entropy_zero_length);
    TEST_RUN("fill_entropy_statistical_distribution", test_rt_random_fill_entropy_statistical_distribution);

    /* Factory method tests */
    TEST_RUN("create_with_seed_basic", test_rt_random_create_with_seed_basic);
    TEST_RUN("create_with_seed_deterministic", test_rt_random_create_with_seed_deterministic);
    TEST_RUN("create_with_seed_different_seeds", test_rt_random_create_with_seed_different_seeds);
    TEST_RUN("create_with_seed_not_all_zeros", test_rt_random_create_with_seed_not_all_zeros);
    TEST_RUN("create_with_seed_state_advances", test_rt_random_create_with_seed_state_advances);
    TEST_RUN("create_with_seed_statistical", test_rt_random_create_with_seed_statistical);
    TEST_RUN("create_os_entropy", test_rt_random_create_os_entropy);
    TEST_RUN("create_null_arena", test_rt_random_create_null_arena);
    TEST_RUN("create_with_seed_null_arena", test_rt_random_create_with_seed_null_arena);

    /* Reproducibility tests */
    TEST_RUN("seeded_reproducibility", test_rt_random_seeded_reproducibility);
    TEST_RUN("seeded_different_types_reproducibility", test_rt_random_seeded_different_types_reproducibility);
    TEST_RUN("seeded_bytes_reproducibility", test_rt_random_seeded_bytes_reproducibility);
    TEST_RUN("seeded_gaussian_reproducibility", test_rt_random_seeded_gaussian_reproducibility);
}
