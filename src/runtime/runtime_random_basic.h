#ifndef RUNTIME_RANDOM_BASIC_H
#define RUNTIME_RANDOM_BASIC_H

#include "runtime_random_core.h"

/* ============================================================================
 * Basic Random Value Generation
 * ============================================================================
 * This module contains basic value generation functions for both static
 * (OS entropy) and instance (seeded PRNG) modes.
 *
 * Static methods use OS entropy directly - no seeding required.
 * Instance methods use the RtRandom's internal PRNG state.
 * ============================================================================ */

/* ============================================================================
 * Instance Value Generation Methods (Seeded PRNG)
 * ============================================================================ */

/* Random integer in range [min, max] inclusive */
long rt_random_int(RtRandom *rng, long min, long max);

/* Random long in range [min, max] inclusive */
long long rt_random_long(RtRandom *rng, long long min, long long max);

/* Random double in range [min, max) */
double rt_random_double(RtRandom *rng, double min, double max);

/* Random boolean (50/50) */
int rt_random_bool(RtRandom *rng);

/* Random byte (0-255) */
unsigned char rt_random_byte(RtRandom *rng);

/* Array of random bytes */
unsigned char *rt_random_bytes(RtArena *arena, RtRandom *rng, long count);

/* Sample from normal distribution using Box-Muller transform */
double rt_random_gaussian(RtRandom *rng, double mean, double stddev);

/* ============================================================================
 * Static Value Generation Methods (OS Entropy)
 * ============================================================================ */

/* Random integer in range [min, max] inclusive */
long rt_random_static_int(long min, long max);

/* Random long in range [min, max] inclusive */
long long rt_random_static_long(long long min, long long max);

/* Random double in range [min, max) */
double rt_random_static_double(double min, double max);

/* Random boolean (50/50) */
int rt_random_static_bool(void);

/* Random byte (0-255) */
unsigned char rt_random_static_byte(void);

/* Array of random bytes */
unsigned char *rt_random_static_bytes(RtArena *arena, long count);

/* Sample from normal distribution using Box-Muller transform */
double rt_random_static_gaussian(double mean, double stddev);

#endif /* RUNTIME_RANDOM_BASIC_H */
