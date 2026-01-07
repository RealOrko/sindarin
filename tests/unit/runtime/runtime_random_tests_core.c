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

/* ============================================================================
 * Reproducibility Tests
 * ============================================================================
 * These tests verify that seeded generators produce identical sequences.
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
 * Main Test Runner
 * ============================================================================ */

void test_rt_random_core_main()
{
    printf("\n");
    printf("================================================\n");
    printf(" Runtime Random Core Tests\n");
    printf("================================================\n");

    /* rt_random_fill_entropy() tests */
    test_rt_random_fill_entropy_basic();
    test_rt_random_fill_entropy_different_calls();
    test_rt_random_fill_entropy_small_buffer();
    test_rt_random_fill_entropy_large_buffer();
    test_rt_random_fill_entropy_null_buffer();
    test_rt_random_fill_entropy_zero_length();
    test_rt_random_fill_entropy_statistical_distribution();

    /* Factory method tests */
    test_rt_random_create_with_seed_basic();
    test_rt_random_create_with_seed_deterministic();
    test_rt_random_create_with_seed_different_seeds();
    test_rt_random_create_with_seed_not_all_zeros();
    test_rt_random_create_with_seed_state_advances();
    test_rt_random_create_with_seed_statistical();
    test_rt_random_create_os_entropy();
    test_rt_random_create_null_arena();
    test_rt_random_create_with_seed_null_arena();

    /* Reproducibility tests */
    test_rt_random_seeded_reproducibility();
    test_rt_random_seeded_different_types_reproducibility();
    test_rt_random_seeded_bytes_reproducibility();
    test_rt_random_seeded_gaussian_reproducibility();

    printf("------------------------------------------------\n");
    printf(" All runtime random core tests passed!\n");
    printf("================================================\n");
}
