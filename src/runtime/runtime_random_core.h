#ifndef RUNTIME_RANDOM_CORE_H
#define RUNTIME_RANDOM_CORE_H

#include "runtime_arena.h"
#include <stdint.h>

/* ============================================================================
 * Random Number Generator Core Types and Functions
 * ============================================================================
 * This module contains the core infrastructure for random number generation:
 * - The RtRandom type definition
 * - Platform-specific entropy source access
 * - PRNG algorithm implementation (xoshiro256**)
 * - Factory methods for creating Random instances
 * ============================================================================ */

/* Random generator handle */
typedef struct RtRandom {
    int is_seeded;          /* 0 = OS entropy, 1 = seeded PRNG */
    uint64_t state[4];      /* PRNG state (xoshiro256** algorithm) */
} RtRandom;

/* ============================================================================
 * Core Entropy Function
 * ============================================================================ */

/* Fill buffer with random bytes from OS entropy source
 * Uses the best available entropy source for the platform:
 * - Linux: getrandom()
 * - BSD/macOS: arc4random_buf()
 * - Windows: BCryptGenRandom()
 * - Fallback: /dev/urandom
 */
void rt_random_fill_entropy(uint8_t *buf, size_t len);

/* ============================================================================
 * Factory Methods
 * ============================================================================ */

/* Create an OS-entropy backed random instance
 * Each call to methods on this instance fetches fresh entropy.
 */
RtRandom *rt_random_create(RtArena *arena);

/* Create a seeded PRNG instance for reproducible sequences
 * Uses xoshiro256** algorithm with SplitMix64 seeding.
 */
RtRandom *rt_random_create_with_seed(RtArena *arena, long seed);

/* ============================================================================
 * Internal Helper (for use by other runtime_random_* modules)
 * ============================================================================ */

/* Get next 64-bit random value from the generator
 * For seeded generators, uses xoshiro256**.
 * For OS-entropy generators, fetches fresh entropy.
 */
uint64_t rt_random_next_u64(RtRandom *rng);

/* Get next 64-bit random value from OS entropy (stateless) */
uint64_t rt_random_static_next_u64(void);

#endif /* RUNTIME_RANDOM_CORE_H */
