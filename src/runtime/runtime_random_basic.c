#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "runtime_random_basic.h"

/* ============================================================================
 * Instance Value Generation Methods (Seeded PRNG)
 * ============================================================================
 * These methods use the instance's internal PRNG state.
 * Reproducible when created with createWithSeed().
 * ============================================================================ */

/*
 * Generate random integer in range [min, max] inclusive.
 * Uses rejection sampling to avoid modulo bias.
 */
long rt_random_int(RtRandom *rng, long min, long max) {
    if (min > max) {
        /* Swap if inverted */
        long tmp = min;
        min = max;
        max = tmp;
    }
    if (min == max) {
        return min;
    }

    /* Calculate range as unsigned to handle full long range */
    uint64_t range = (uint64_t)(max - min) + 1;

    /* For power-of-two ranges, simple masking works */
    if ((range & (range - 1)) == 0) {
        uint64_t val = rt_random_next_u64(rng);
        return min + (long)(val & (range - 1));
    }

    /* Rejection sampling to avoid modulo bias */
    uint64_t threshold = (uint64_t)(-(int64_t)range) % range;
    uint64_t val;

    do {
        val = rt_random_next_u64(rng);
    } while (val < threshold);

    return min + (long)(val % range);
}

/*
 * Generate random long in range [min, max] inclusive.
 */
long long rt_random_long(RtRandom *rng, long long min, long long max) {
    if (min > max) {
        long long tmp = min;
        min = max;
        max = tmp;
    }
    if (min == max) {
        return min;
    }

    uint64_t range = (uint64_t)(max - min) + 1;

    if ((range & (range - 1)) == 0) {
        uint64_t val = rt_random_next_u64(rng);
        return min + (long long)(val & (range - 1));
    }

    uint64_t threshold = (uint64_t)(-(int64_t)range) % range;
    uint64_t val;

    do {
        val = rt_random_next_u64(rng);
    } while (val < threshold);

    return min + (long long)(val % range);
}

/*
 * Generate random double in range [min, max).
 * Uses 53 bits of randomness for full double precision.
 */
double rt_random_double(RtRandom *rng, double min, double max) {
    if (min > max) {
        double tmp = min;
        min = max;
        max = tmp;
    }
    if (min == max) {
        return min;
    }

    /* Use 53 bits for full double precision mantissa */
    uint64_t val = rt_random_next_u64(rng) >> 11;
    double normalized = (double)val / (double)(1ULL << 53);

    return min + normalized * (max - min);
}

/*
 * Generate random boolean (50/50).
 */
int rt_random_bool(RtRandom *rng) {
    return (rt_random_next_u64(rng) & 1) ? 1 : 0;
}

/*
 * Generate random byte (0-255).
 */
unsigned char rt_random_byte(RtRandom *rng) {
    return (unsigned char)(rt_random_next_u64(rng) & 0xFF);
}

/*
 * Generate array of random bytes.
 */
unsigned char *rt_random_bytes(RtArena *arena, RtRandom *rng, long count) {
    if (arena == NULL || count <= 0) {
        return NULL;
    }

    unsigned char *buf = rt_arena_alloc(arena, (size_t)count);

    if (rng->is_seeded) {
        /* Generate from PRNG state */
        for (long i = 0; i < count; i++) {
            buf[i] = rt_random_byte(rng);
        }
    } else {
        /* Get fresh entropy */
        rt_random_fill_entropy(buf, (size_t)count);
    }

    return buf;
}

/*
 * Sample from normal distribution using Box-Muller transform.
 */
double rt_random_gaussian(RtRandom *rng, double mean, double stddev) {
    /* Box-Muller transform generates two values at once.
     * For simplicity, we generate both but only use one.
     * A more optimized version could cache the second value. */
    double u1, u2;

    do {
        u1 = rt_random_double(rng, 0.0, 1.0);
    } while (u1 == 0.0);  /* u1 must be > 0 for log() */

    u2 = rt_random_double(rng, 0.0, 1.0);

    /* Box-Muller transform */
    double mag = stddev * sqrt(-2.0 * log(u1));
    double z0 = mag * cos(2.0 * 3.14159265358979323846 * u2);

    return mean + z0;
}

/* ============================================================================
 * Static Value Generation Methods (OS Entropy)
 * ============================================================================
 * These methods use OS entropy directly without a persistent state.
 * They provide a convenient API for one-off random value generation.
 * ============================================================================ */

/*
 * Static: Random integer in range [min, max] inclusive.
 */
long rt_random_static_int(long min, long max) {
    if (min > max) {
        long tmp = min;
        min = max;
        max = tmp;
    }
    if (min == max) {
        return min;
    }

    uint64_t range = (uint64_t)(max - min) + 1;

    if ((range & (range - 1)) == 0) {
        uint64_t val = rt_random_static_next_u64();
        return min + (long)(val & (range - 1));
    }

    uint64_t threshold = (uint64_t)(-(int64_t)range) % range;
    uint64_t val;

    do {
        val = rt_random_static_next_u64();
    } while (val < threshold);

    return min + (long)(val % range);
}

/*
 * Static: Random long in range [min, max] inclusive.
 */
long long rt_random_static_long(long long min, long long max) {
    if (min > max) {
        long long tmp = min;
        min = max;
        max = tmp;
    }
    if (min == max) {
        return min;
    }

    uint64_t range = (uint64_t)(max - min) + 1;

    if ((range & (range - 1)) == 0) {
        uint64_t val = rt_random_static_next_u64();
        return min + (long long)(val & (range - 1));
    }

    uint64_t threshold = (uint64_t)(-(int64_t)range) % range;
    uint64_t val;

    do {
        val = rt_random_static_next_u64();
    } while (val < threshold);

    return min + (long long)(val % range);
}

/*
 * Static: Random double in range [min, max).
 */
double rt_random_static_double(double min, double max) {
    if (min > max) {
        double tmp = min;
        min = max;
        max = tmp;
    }
    if (min == max) {
        return min;
    }

    uint64_t val = rt_random_static_next_u64() >> 11;
    double normalized = (double)val / (double)(1ULL << 53);

    return min + normalized * (max - min);
}

/*
 * Static: Random boolean (50/50).
 */
int rt_random_static_bool(void) {
    return (rt_random_static_next_u64() & 1) ? 1 : 0;
}

/*
 * Static: Random byte (0-255).
 */
unsigned char rt_random_static_byte(void) {
    unsigned char result;
    rt_random_fill_entropy(&result, 1);
    return result;
}

/*
 * Static: Array of random bytes.
 */
unsigned char *rt_random_static_bytes(RtArena *arena, long count) {
    if (arena == NULL || count <= 0) {
        return NULL;
    }

    unsigned char *buf = rt_arena_alloc(arena, (size_t)count);
    rt_random_fill_entropy(buf, (size_t)count);
    return buf;
}

/*
 * Static: Sample from normal distribution using Box-Muller transform.
 */
double rt_random_static_gaussian(double mean, double stddev) {
    double u1, u2;

    do {
        u1 = rt_random_static_double(0.0, 1.0);
    } while (u1 == 0.0);

    u2 = rt_random_static_double(0.0, 1.0);

    double mag = stddev * sqrt(-2.0 * log(u1));
    double z0 = mag * cos(2.0 * 3.14159265358979323846 * u2);

    return mean + z0;
}
