#ifndef RUNTIME_RANDOM_H
#define RUNTIME_RANDOM_H

#include "runtime_random_core.h"
#include "runtime_random_basic.h"
#include "runtime_random_static.h"
#include "runtime_random_choice.h"
#include "runtime_random_collection.h"

/* ============================================================================
 * Random Number Generator
 * ============================================================================
 * Sindarin provides the Random type for generating random values. The default
 * static methods use OS entropy (secure by default). For reproducible sequences,
 * create an instance with createWithSeed().
 *
 * Two modes of operation:
 * 1. Static methods - Use OS entropy (cryptographically secure)
 * 2. Instance methods - Use seeded PRNG (reproducible sequences)
 *
 * Core types and factory functions are defined in runtime_random_core.h:
 * - RtRandom type
 * - rt_random_create(), rt_random_create_with_seed()
 * - rt_random_fill_entropy()
 *
 * Basic value generation functions are defined in runtime_random_basic.h:
 * - rt_random_int(), rt_random_long(), rt_random_double(), etc.
 * - rt_random_static_int(), rt_random_static_long(), etc.
 *
 * Collection operations are defined in runtime_random_collection.h:
 * - rt_random_*_many() batch generation functions
 * - rt_random_shuffle_*() shuffle functions
 * - rt_random_sample_*() sample functions
 * ============================================================================ */

#endif /* RUNTIME_RANDOM_H */
