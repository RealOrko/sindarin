#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include "runtime_random.h"
#include "runtime_array.h"

/* Platform-specific includes for entropy sources */
#if defined(__linux__)
#include <sys/random.h>
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#include <stdlib.h>  /* arc4random_buf on BSD/macOS */
#elif defined(_WIN32)
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#endif

/* ============================================================================
 * Core Entropy Function
 * ============================================================================
 * Platform-specific implementation for filling a buffer with random bytes
 * from the operating system's entropy source.
 * ============================================================================ */

void rt_random_fill_entropy(uint8_t *buf, size_t len) {
    if (buf == NULL || len == 0) {
        return;
    }

#if defined(__linux__)
    /*
     * Linux: Use getrandom() syscall
     * - Blocks until entropy pool is initialized (default behavior)
     * - Returns bytes written, may be less than requested
     * - EAGAIN only with GRND_NONBLOCK flag (we don't use it)
     * - EINTR can occur if signal received before any bytes read
     */
    size_t remaining = len;
    uint8_t *ptr = buf;

    while (remaining > 0) {
        ssize_t ret = getrandom(ptr, remaining, 0);

        if (ret < 0) {
            if (errno == EINTR) {
                /* Interrupted by signal before any bytes read, retry */
                continue;
            }
            if (errno == EAGAIN) {
                /* Should not happen without GRND_NONBLOCK, but handle gracefully */
                /* In practice this means entropy pool not ready - very early boot */
                continue;
            }
            /* Fatal error - shouldn't happen in normal operation */
            fprintf(stderr, "rt_random_fill_entropy: getrandom() failed: %s\n",
                    strerror(errno));
            exit(1);
        }

        ptr += ret;
        remaining -= (size_t)ret;
    }

#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    /*
     * BSD/macOS: Use arc4random_buf()
     * - Never fails
     * - Never blocks
     * - Uses kernel entropy source (ChaCha20 on modern systems)
     * - Automatically reseeds as needed
     */
    arc4random_buf(buf, len);

#elif defined(_WIN32)
    /*
     * Windows: Use BCryptGenRandom()
     * - BCRYPT_USE_SYSTEM_PREFERRED_RNG: use default system RNG
     * - First parameter NULL when using system preferred RNG
     */
    NTSTATUS status = BCryptGenRandom(
        NULL,
        buf,
        (ULONG)len,
        BCRYPT_USE_SYSTEM_PREFERRED_RNG
    );

    if (!BCRYPT_SUCCESS(status)) {
        fprintf(stderr, "rt_random_fill_entropy: BCryptGenRandom() failed: 0x%lx\n",
                (unsigned long)status);
        exit(1);
    }

#else
    /*
     * Fallback: Use /dev/urandom
     * - Works on most Unix-like systems
     * - May block on some systems if entropy pool not ready
     */
    FILE *urandom = fopen("/dev/urandom", "rb");
    if (urandom == NULL) {
        fprintf(stderr, "rt_random_fill_entropy: failed to open /dev/urandom: %s\n",
                strerror(errno));
        exit(1);
    }

    size_t bytes_read = fread(buf, 1, len, urandom);
    if (bytes_read != len) {
        fprintf(stderr, "rt_random_fill_entropy: failed to read from /dev/urandom\n");
        fclose(urandom);
        exit(1);
    }

    fclose(urandom);
#endif
}

/* ============================================================================
 * xoshiro256** PRNG Algorithm
 * ============================================================================
 * A fast, high-quality pseudorandom number generator.
 * Reference: https://prng.di.unimi.it/
 * - Period: 2^256 - 1
 * - Passes BigCrush statistical tests
 * - Used by Rust's rand crate, among others
 * ============================================================================ */

/* Rotate left helper for xoshiro256** */
static inline uint64_t rotl(const uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

/*
 * xoshiro256** next function
 * Generates the next 64-bit value and advances the state.
 *
 * This is the "starstar" variant which has better statistical properties
 * for the lower bits than the basic xoshiro256.
 */
static uint64_t xoshiro256_next(uint64_t *state) {
    const uint64_t result = rotl(state[1] * 5, 7) * 9;

    const uint64_t t = state[1] << 17;

    state[2] ^= state[0];
    state[3] ^= state[1];
    state[1] ^= state[2];
    state[0] ^= state[3];

    state[2] ^= t;

    state[3] = rotl(state[3], 45);

    return result;
}

/* ============================================================================
 * SplitMix64 Seed Initialization
 * ============================================================================
 * Converts a single 64-bit seed into the 4-word state required by xoshiro256**.
 * SplitMix64 is recommended by the xoshiro authors for seeding.
 * Reference: https://prng.di.unimi.it/splitmix64.c
 * ============================================================================ */

/*
 * SplitMix64 single step
 * Used to expand a single seed into multiple words of state.
 */
static uint64_t splitmix64_next(uint64_t *x) {
    uint64_t z = (*x += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

/*
 * Initialize xoshiro256** state from a single seed
 * Uses SplitMix64 to expand the seed into 4 words.
 */
static void xoshiro256_seed(uint64_t *state, uint64_t seed) {
    uint64_t x = seed;

    state[0] = splitmix64_next(&x);
    state[1] = splitmix64_next(&x);
    state[2] = splitmix64_next(&x);
    state[3] = splitmix64_next(&x);

    /* Ensure state is not all zeros (would be a fixed point) */
    if (state[0] == 0 && state[1] == 0 && state[2] == 0 && state[3] == 0) {
        /* This is extremely unlikely with SplitMix64, but handle it anyway */
        state[0] = 1;
    }
}

/* ============================================================================
 * Factory Methods
 * ============================================================================ */

/* Create an OS-entropy backed random instance */
RtRandom *rt_random_create(RtArena *arena) {
    if (arena == NULL) {
        return NULL;
    }

    RtRandom *rng = rt_arena_alloc(arena, sizeof(RtRandom));
    rng->is_seeded = 0;  /* Uses OS entropy */

    /* Initialize state from OS entropy for non-seeded generator */
    rt_random_fill_entropy((uint8_t *)rng->state, sizeof(rng->state));

    return rng;
}

/* Create a seeded PRNG instance for reproducible sequences */
RtRandom *rt_random_create_with_seed(RtArena *arena, long seed) {
    if (arena == NULL) {
        return NULL;
    }

    RtRandom *rng = rt_arena_alloc(arena, sizeof(RtRandom));
    rng->is_seeded = 1;  /* Uses seeded PRNG */

    /* Initialize state from seed using SplitMix64 */
    xoshiro256_seed(rng->state, (uint64_t)seed);

    return rng;
}

/* ============================================================================
 * Internal Helper: Get next 64-bit random value from generator
 * ============================================================================ */

/*
 * Get next 64-bit random value from the generator.
 * For seeded generators, uses xoshiro256**.
 * For OS-entropy generators, fetches fresh entropy.
 */
static uint64_t rt_random_next_u64(RtRandom *rng) {
    if (rng->is_seeded) {
        return xoshiro256_next(rng->state);
    } else {
        /* For OS-entropy mode, get fresh random bytes */
        uint64_t result;
        rt_random_fill_entropy((uint8_t *)&result, sizeof(result));
        return result;
    }
}

/* ============================================================================
 * Instance Value Generation Methods
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
 * Internal helper: Get a 64-bit random value from OS entropy.
 */
static uint64_t rt_random_static_next_u64(void) {
    uint64_t result;
    rt_random_fill_entropy((uint8_t *)&result, sizeof(result));
    return result;
}

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

/* ============================================================================
 * Static Batch Generation Methods (OS Entropy)
 * ============================================================================
 * Generate multiple values in one call for performance.
 * Uses arena allocation for efficient array creation.
 * ============================================================================ */

/*
 * Static: Array of random integers in range [min, max] inclusive.
 */
long *rt_random_static_int_many(RtArena *arena, long min, long max, long count) {
    if (arena == NULL || count <= 0) {
        return NULL;
    }

    long *result = rt_arena_alloc(arena, (size_t)count * sizeof(long));

    for (long i = 0; i < count; i++) {
        result[i] = rt_random_static_int(min, max);
    }

    return result;
}

/*
 * Static: Array of random longs in range [min, max] inclusive.
 */
long long *rt_random_static_long_many(RtArena *arena, long long min, long long max, long count) {
    if (arena == NULL || count <= 0) {
        return NULL;
    }

    long long *result = rt_arena_alloc(arena, (size_t)count * sizeof(long long));

    for (long i = 0; i < count; i++) {
        result[i] = rt_random_static_long(min, max);
    }

    return result;
}

/*
 * Static: Array of random doubles in range [min, max).
 */
double *rt_random_static_double_many(RtArena *arena, double min, double max, long count) {
    if (arena == NULL || count <= 0) {
        return NULL;
    }

    double *result = rt_arena_alloc(arena, (size_t)count * sizeof(double));

    for (long i = 0; i < count; i++) {
        result[i] = rt_random_static_double(min, max);
    }

    return result;
}

/*
 * Static: Array of random booleans.
 */
int *rt_random_static_bool_many(RtArena *arena, long count) {
    if (arena == NULL || count <= 0) {
        return NULL;
    }

    int *result = rt_arena_alloc(arena, (size_t)count * sizeof(int));

    for (long i = 0; i < count; i++) {
        result[i] = rt_random_static_bool();
    }

    return result;
}

/*
 * Static: Array of gaussian samples.
 */
double *rt_random_static_gaussian_many(RtArena *arena, double mean, double stddev, long count) {
    if (arena == NULL || count <= 0) {
        return NULL;
    }

    double *result = rt_arena_alloc(arena, (size_t)count * sizeof(double));

    for (long i = 0; i < count; i++) {
        result[i] = rt_random_static_gaussian(mean, stddev);
    }

    return result;
}

/* ============================================================================
 * Instance Batch Generation Methods (Seeded PRNG)
 * ============================================================================
 * Generate multiple values in one call for performance.
 * Uses arena allocation for efficient array creation.
 * ============================================================================ */

/*
 * Instance: Array of random integers in range [min, max] inclusive.
 */
long *rt_random_int_many(RtArena *arena, RtRandom *rng, long min, long max, long count) {
    if (arena == NULL || rng == NULL || count <= 0) {
        return NULL;
    }

    long *result = rt_arena_alloc(arena, (size_t)count * sizeof(long));

    for (long i = 0; i < count; i++) {
        result[i] = rt_random_int(rng, min, max);
    }

    return result;
}

/*
 * Instance: Array of random longs in range [min, max] inclusive.
 */
long long *rt_random_long_many(RtArena *arena, RtRandom *rng, long long min, long long max, long count) {
    if (arena == NULL || rng == NULL || count <= 0) {
        return NULL;
    }

    long long *result = rt_arena_alloc(arena, (size_t)count * sizeof(long long));

    for (long i = 0; i < count; i++) {
        result[i] = rt_random_long(rng, min, max);
    }

    return result;
}

/*
 * Instance: Array of random doubles in range [min, max).
 */
double *rt_random_double_many(RtArena *arena, RtRandom *rng, double min, double max, long count) {
    if (arena == NULL || rng == NULL || count <= 0) {
        return NULL;
    }

    double *result = rt_arena_alloc(arena, (size_t)count * sizeof(double));

    for (long i = 0; i < count; i++) {
        result[i] = rt_random_double(rng, min, max);
    }

    return result;
}

/*
 * Instance: Array of random booleans.
 */
int *rt_random_bool_many(RtArena *arena, RtRandom *rng, long count) {
    if (arena == NULL || rng == NULL || count <= 0) {
        return NULL;
    }

    int *result = rt_arena_alloc(arena, (size_t)count * sizeof(int));

    for (long i = 0; i < count; i++) {
        result[i] = rt_random_bool(rng);
    }

    return result;
}

/*
 * Instance: Array of gaussian samples.
 */
double *rt_random_gaussian_many(RtArena *arena, RtRandom *rng, double mean, double stddev, long count) {
    if (arena == NULL || rng == NULL || count <= 0) {
        return NULL;
    }

    double *result = rt_arena_alloc(arena, (size_t)count * sizeof(double));

    for (long i = 0; i < count; i++) {
        result[i] = rt_random_gaussian(rng, mean, stddev);
    }

    return result;
}

/* ============================================================================
 * Static Collection Operations (OS Entropy)
 * ============================================================================
 * Random selection from arrays using OS entropy.
 * ============================================================================ */

/*
 * Static: Random element from long array.
 * Returns 0 if array is NULL or empty.
 */
long rt_random_static_choice_long(long *arr, long len) {
    if (arr == NULL || len <= 0) {
        return 0;
    }

    long index = rt_random_static_int(0, len - 1);
    return arr[index];
}

/*
 * Static: Random element from double array.
 * Returns 0.0 if array is NULL or empty.
 */
double rt_random_static_choice_double(double *arr, long len) {
    if (arr == NULL || len <= 0) {
        return 0.0;
    }

    long index = rt_random_static_int(0, len - 1);
    return arr[index];
}

/*
 * Static: Random element from string array.
 * Returns NULL if array is NULL or empty.
 */
char *rt_random_static_choice_string(char **arr, long len) {
    if (arr == NULL || len <= 0) {
        return NULL;
    }

    long index = rt_random_static_int(0, len - 1);
    return arr[index];
}

/*
 * Static: Random element from bool (int) array.
 * Returns 0 if array is NULL or empty.
 */
int rt_random_static_choice_bool(int *arr, long len) {
    if (arr == NULL || len <= 0) {
        return 0;
    }

    long index = rt_random_static_int(0, len - 1);
    return arr[index];
}

/*
 * Static: Random element from byte array.
 * Returns 0 if array is NULL or empty.
 */
unsigned char rt_random_static_choice_byte(unsigned char *arr, long len) {
    if (arr == NULL || len <= 0) {
        return 0;
    }

    long index = rt_random_static_int(0, len - 1);
    return arr[index];
}

/* ============================================================================
 * Instance Collection Operations (Seeded PRNG)
 * ============================================================================
 * Random selection from arrays using the instance's PRNG state.
 * ============================================================================ */

/*
 * Instance: Random element from long array.
 * Returns 0 if array is NULL or empty.
 */
long rt_random_choice_long(RtRandom *rng, long *arr, long len) {
    if (rng == NULL || arr == NULL || len <= 0) {
        return 0;
    }

    long index = rt_random_int(rng, 0, len - 1);
    return arr[index];
}

/*
 * Instance: Random element from double array.
 * Returns 0.0 if array is NULL or empty.
 */
double rt_random_choice_double(RtRandom *rng, double *arr, long len) {
    if (rng == NULL || arr == NULL || len <= 0) {
        return 0.0;
    }

    long index = rt_random_int(rng, 0, len - 1);
    return arr[index];
}

/*
 * Instance: Random element from string array.
 * Returns NULL if array is NULL or empty.
 */
char *rt_random_choice_string(RtRandom *rng, char **arr, long len) {
    if (rng == NULL || arr == NULL || len <= 0) {
        return NULL;
    }

    long index = rt_random_int(rng, 0, len - 1);
    return arr[index];
}

/*
 * Instance: Random element from bool (int) array.
 * Returns 0 if array is NULL or empty.
 */
int rt_random_choice_bool(RtRandom *rng, int *arr, long len) {
    if (rng == NULL || arr == NULL || len <= 0) {
        return 0;
    }

    long index = rt_random_int(rng, 0, len - 1);
    return arr[index];
}

/*
 * Instance: Random element from byte array.
 * Returns 0 if array is NULL or empty.
 */
unsigned char rt_random_choice_byte(RtRandom *rng, unsigned char *arr, long len) {
    if (rng == NULL || arr == NULL || len <= 0) {
        return 0;
    }

    long index = rt_random_int(rng, 0, len - 1);
    return arr[index];
}

/* ============================================================================
 * Weight Validation Helper
 * ============================================================================
 * Validates that a weights array is suitable for weighted random selection.
 * ============================================================================ */

/*
 * Validate weights array for weighted random selection.
 * Requirements:
 *   - weights must not be NULL
 *   - len must be > 0 (non-empty array)
 *   - All weights must be positive (> 0)
 *   - Sum of weights must be non-zero
 *
 * Returns: 1 if valid, 0 if invalid
 */
int rt_random_validate_weights(double *weights, long len) {
    /* Check for NULL pointer */
    if (weights == NULL) {
        return 0;
    }

    /* Check for empty array */
    if (len <= 0) {
        return 0;
    }

    /* Check all weights are positive and calculate sum */
    double sum = 0.0;
    for (long i = 0; i < len; i++) {
        if (weights[i] <= 0.0) {
            /* Weight must be strictly positive */
            return 0;
        }
        sum += weights[i];
    }

    /* Check sum is non-zero (technically covered by positive check, but explicit) */
    if (sum <= 0.0) {
        return 0;
    }

    return 1;
}

/*
 * Build cumulative distribution array from weights.
 * Normalizes weights to sum to 1.0 and creates cumulative distribution.
 *
 * For weights [0.7, 0.25, 0.05], produces cumulative [0.7, 0.95, 1.0].
 * For single-element weight [1.0], produces cumulative [1.0].
 *
 * Returns: Arena-allocated array of cumulative probabilities, or NULL on error.
 */
double *rt_random_build_cumulative(RtArena *arena, double *weights, long len) {
    /* Validate inputs */
    if (arena == NULL || weights == NULL || len <= 0) {
        return NULL;
    }

    /* Calculate sum of all weights */
    double sum = 0.0;
    for (long i = 0; i < len; i++) {
        sum += weights[i];
    }

    /* Avoid division by zero */
    if (sum <= 0.0) {
        return NULL;
    }

    /* Allocate cumulative array using arena */
    double *cumulative = rt_arena_alloc(arena, (size_t)len * sizeof(double));

    /* Build cumulative distribution with normalization */
    double running_sum = 0.0;
    for (long i = 0; i < len; i++) {
        running_sum += weights[i] / sum;  /* Normalize each weight */
        cumulative[i] = running_sum;
    }

    /* Ensure last element is exactly 1.0 to handle floating point errors */
    cumulative[len - 1] = 1.0;

    return cumulative;
}

/*
 * Select index from cumulative distribution using a random value.
 * Uses binary search to find the first index where cumulative[index] > random_val.
 *
 * For cumulative [0.7, 0.95, 1.0]:
 *   - random_val in [0, 0.7) returns 0
 *   - random_val in [0.7, 0.95) returns 1
 *   - random_val in [0.95, 1.0) returns 2
 *
 * Returns: Index into the original array (0 to len-1)
 */
long rt_random_select_weighted_index(double random_val, double *cumulative, long len) {
    /* Handle invalid inputs */
    if (cumulative == NULL || len <= 0) {
        return 0;
    }

    /* Handle single element case - always return 0 */
    if (len == 1) {
        return 0;
    }

    /* Handle edge case: random_val >= 1.0 (shouldn't happen but be safe) */
    if (random_val >= 1.0) {
        return len - 1;
    }

    /* Handle edge case: random_val <= 0.0 */
    if (random_val <= 0.0) {
        return 0;
    }

    /* Binary search for the first index where cumulative[index] > random_val */
    long left = 0;
    long right = len - 1;

    while (left < right) {
        long mid = left + (right - left) / 2;

        if (cumulative[mid] > random_val) {
            /* Found a valid index, but there might be a smaller one */
            right = mid;
        } else {
            /* cumulative[mid] <= random_val, need to look right */
            left = mid + 1;
        }
    }

    return left;
}

/* ============================================================================
 * Static Weighted Choice Functions (OS Entropy)
 * ============================================================================
 * Weighted random selection from arrays using OS entropy.
 * Uses the helper functions: validate_weights, build_cumulative, select_weighted_index.
 * ============================================================================ */

/*
 * Static: Weighted random choice from long array.
 * Returns a random element with probability proportional to its weight.
 * Returns 0 if inputs are invalid (NULL arrays, invalid weights).
 */
long rt_random_static_weighted_choice_long(long *arr, double *weights) {
    /* Handle NULL inputs */
    if (arr == NULL || weights == NULL) {
        return 0;
    }

    /* Get array length */
    long len = (long)rt_array_length(arr);
    if (len <= 0) {
        return 0;
    }

    /* Validate weights using helper */
    if (!rt_random_validate_weights(weights, len)) {
        return 0;
    }

    /* Create temporary arena for cumulative distribution */
    RtArena *temp_arena = rt_arena_create(NULL);
    if (temp_arena == NULL) {
        return 0;
    }

    /* Build cumulative distribution using helper */
    double *cumulative = rt_random_build_cumulative(temp_arena, weights, len);
    if (cumulative == NULL) {
        rt_arena_destroy(temp_arena);
        return 0;
    }

    /* Generate random value in [0, 1) using OS entropy */
    double random_val = rt_random_static_double(0.0, 1.0);

    /* Select index using helper */
    long index = rt_random_select_weighted_index(random_val, cumulative, len);

    /* Get the selected value */
    long result = arr[index];

    /* Clean up temporary arena */
    rt_arena_destroy(temp_arena);

    return result;
}

/*
 * Static: Weighted random choice from double array.
 * Returns a random element with probability proportional to its weight.
 * Returns 0.0 if inputs are invalid (NULL arrays, invalid weights).
 */
double rt_random_static_weighted_choice_double(double *arr, double *weights) {
    /* Handle NULL inputs */
    if (arr == NULL || weights == NULL) {
        return 0.0;
    }

    /* Get array length */
    long len = (long)rt_array_length(arr);
    if (len <= 0) {
        return 0.0;
    }

    /* Validate weights using helper */
    if (!rt_random_validate_weights(weights, len)) {
        return 0.0;
    }

    /* Create temporary arena for cumulative distribution */
    RtArena *temp_arena = rt_arena_create(NULL);
    if (temp_arena == NULL) {
        return 0.0;
    }

    /* Build cumulative distribution using helper */
    double *cumulative = rt_random_build_cumulative(temp_arena, weights, len);
    if (cumulative == NULL) {
        rt_arena_destroy(temp_arena);
        return 0.0;
    }

    /* Generate random value in [0, 1) using OS entropy */
    double random_val = rt_random_static_double(0.0, 1.0);

    /* Select index using helper */
    long index = rt_random_select_weighted_index(random_val, cumulative, len);

    /* Get the selected value */
    double result = arr[index];

    /* Clean up temporary arena */
    rt_arena_destroy(temp_arena);

    return result;
}

/*
 * Static: Weighted random choice from string array.
 * Returns a random element with probability proportional to its weight.
 * Returns NULL if inputs are invalid (NULL arrays, invalid weights).
 */
char *rt_random_static_weighted_choice_string(char **arr, double *weights) {
    /* Handle NULL inputs */
    if (arr == NULL || weights == NULL) {
        return NULL;
    }

    /* Get array length */
    long len = (long)rt_array_length(arr);
    if (len <= 0) {
        return NULL;
    }

    /* Validate weights using helper */
    if (!rt_random_validate_weights(weights, len)) {
        return NULL;
    }

    /* Create temporary arena for cumulative distribution */
    RtArena *temp_arena = rt_arena_create(NULL);
    if (temp_arena == NULL) {
        return NULL;
    }

    /* Build cumulative distribution using helper */
    double *cumulative = rt_random_build_cumulative(temp_arena, weights, len);
    if (cumulative == NULL) {
        rt_arena_destroy(temp_arena);
        return NULL;
    }

    /* Generate random value in [0, 1) using OS entropy */
    double random_val = rt_random_static_double(0.0, 1.0);

    /* Select index using helper */
    long index = rt_random_select_weighted_index(random_val, cumulative, len);

    /* Get the selected value */
    char *result = arr[index];

    /* Clean up temporary arena */
    rt_arena_destroy(temp_arena);

    return result;
}

/* ============================================================================
 * Instance Weighted Choice Functions (Seeded PRNG)
 * ============================================================================
 * Weighted random selection from arrays using seeded PRNG.
 * Uses the helper functions: validate_weights, build_cumulative, select_weighted_index.
 * ============================================================================ */

/*
 * Instance: Weighted random choice from long array.
 * Returns a random element with probability proportional to its weight.
 * Returns 0 if inputs are invalid (NULL rng, NULL arrays, invalid weights).
 */
long rt_random_weighted_choice_long(RtRandom *rng, long *arr, double *weights) {
    /* Handle NULL inputs */
    if (rng == NULL || arr == NULL || weights == NULL) {
        return 0;
    }

    /* Get array length */
    long len = (long)rt_array_length(arr);
    if (len <= 0) {
        return 0;
    }

    /* Validate weights using helper */
    if (!rt_random_validate_weights(weights, len)) {
        return 0;
    }

    /* Create temporary arena for cumulative distribution */
    RtArena *temp_arena = rt_arena_create(NULL);
    if (temp_arena == NULL) {
        return 0;
    }

    /* Build cumulative distribution using helper */
    double *cumulative = rt_random_build_cumulative(temp_arena, weights, len);
    if (cumulative == NULL) {
        rt_arena_destroy(temp_arena);
        return 0;
    }

    /* Generate random value in [0, 1) using seeded PRNG */
    double random_val = rt_random_double(rng, 0.0, 1.0);

    /* Select index using helper */
    long index = rt_random_select_weighted_index(random_val, cumulative, len);

    /* Get the selected value */
    long result = arr[index];

    /* Clean up temporary arena */
    rt_arena_destroy(temp_arena);

    return result;
}

/*
 * Instance: Weighted random choice from double array.
 * Returns a random element with probability proportional to its weight.
 * Returns 0.0 if inputs are invalid (NULL rng, NULL arrays, invalid weights).
 */
double rt_random_weighted_choice_double(RtRandom *rng, double *arr, double *weights) {
    /* Handle NULL inputs */
    if (rng == NULL || arr == NULL || weights == NULL) {
        return 0.0;
    }

    /* Get array length */
    long len = (long)rt_array_length(arr);
    if (len <= 0) {
        return 0.0;
    }

    /* Validate weights using helper */
    if (!rt_random_validate_weights(weights, len)) {
        return 0.0;
    }

    /* Create temporary arena for cumulative distribution */
    RtArena *temp_arena = rt_arena_create(NULL);
    if (temp_arena == NULL) {
        return 0.0;
    }

    /* Build cumulative distribution using helper */
    double *cumulative = rt_random_build_cumulative(temp_arena, weights, len);
    if (cumulative == NULL) {
        rt_arena_destroy(temp_arena);
        return 0.0;
    }

    /* Generate random value in [0, 1) using seeded PRNG */
    double random_val = rt_random_double(rng, 0.0, 1.0);

    /* Select index using helper */
    long index = rt_random_select_weighted_index(random_val, cumulative, len);

    /* Get the selected value */
    double result = arr[index];

    /* Clean up temporary arena */
    rt_arena_destroy(temp_arena);

    return result;
}

/*
 * Instance: Weighted random choice from string array.
 * Returns a random element with probability proportional to its weight.
 * Returns NULL if inputs are invalid (NULL rng, NULL arrays, invalid weights).
 */
char *rt_random_weighted_choice_string(RtRandom *rng, char **arr, double *weights) {
    /* Handle NULL inputs */
    if (rng == NULL || arr == NULL || weights == NULL) {
        return NULL;
    }

    /* Get array length */
    long len = (long)rt_array_length(arr);
    if (len <= 0) {
        return NULL;
    }

    /* Validate weights using helper */
    if (!rt_random_validate_weights(weights, len)) {
        return NULL;
    }

    /* Create temporary arena for cumulative distribution */
    RtArena *temp_arena = rt_arena_create(NULL);
    if (temp_arena == NULL) {
        return NULL;
    }

    /* Build cumulative distribution using helper */
    double *cumulative = rt_random_build_cumulative(temp_arena, weights, len);
    if (cumulative == NULL) {
        rt_arena_destroy(temp_arena);
        return NULL;
    }

    /* Generate random value in [0, 1) using seeded PRNG */
    double random_val = rt_random_double(rng, 0.0, 1.0);

    /* Select index using helper */
    long index = rt_random_select_weighted_index(random_val, cumulative, len);

    /* Get the selected value */
    char *result = arr[index];

    /* Clean up temporary arena */
    rt_arena_destroy(temp_arena);

    return result;
}

/* ============================================================================
 * Static Shuffle Functions (OS Entropy)
 * ============================================================================
 * Fisher-Yates (modern/Knuth) shuffle algorithm.
 * Shuffles array in-place with no allocation.
 * Uses OS entropy for true randomness.
 * ============================================================================ */

/*
 * Static: Shuffle long array in-place using Fisher-Yates algorithm.
 * For i from n-1 down to 1:
 *   j = random index in [0, i]
 *   swap arr[i] and arr[j]
 */
void rt_random_static_shuffle_long(long *arr) {
    if (arr == NULL) {
        return;
    }

    size_t n = rt_array_length(arr);
    if (n <= 1) {
        return;
    }

    /* Fisher-Yates shuffle (modern variant) */
    for (size_t i = n - 1; i > 0; i--) {
        /* Generate random index j in [0, i] inclusive */
        size_t j = (size_t)rt_random_static_int(0, (long)i);

        /* Swap arr[i] and arr[j] */
        long temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

/*
 * Static: Shuffle double array in-place using Fisher-Yates algorithm.
 */
void rt_random_static_shuffle_double(double *arr) {
    if (arr == NULL) {
        return;
    }

    size_t n = rt_array_length(arr);
    if (n <= 1) {
        return;
    }

    for (size_t i = n - 1; i > 0; i--) {
        size_t j = (size_t)rt_random_static_int(0, (long)i);

        double temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

/*
 * Static: Shuffle string array in-place using Fisher-Yates algorithm.
 */
void rt_random_static_shuffle_string(char **arr) {
    if (arr == NULL) {
        return;
    }

    size_t n = rt_array_length(arr);
    if (n <= 1) {
        return;
    }

    for (size_t i = n - 1; i > 0; i--) {
        size_t j = (size_t)rt_random_static_int(0, (long)i);

        char *temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

/*
 * Static: Shuffle bool array in-place using Fisher-Yates algorithm.
 */
void rt_random_static_shuffle_bool(int *arr) {
    if (arr == NULL) {
        return;
    }

    size_t n = rt_array_length(arr);
    if (n <= 1) {
        return;
    }

    for (size_t i = n - 1; i > 0; i--) {
        size_t j = (size_t)rt_random_static_int(0, (long)i);

        int temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

/*
 * Static: Shuffle byte array in-place using Fisher-Yates algorithm.
 */
void rt_random_static_shuffle_byte(unsigned char *arr) {
    if (arr == NULL) {
        return;
    }

    size_t n = rt_array_length(arr);
    if (n <= 1) {
        return;
    }

    for (size_t i = n - 1; i > 0; i--) {
        size_t j = (size_t)rt_random_static_int(0, (long)i);

        unsigned char temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

/* ============================================================================
 * Instance Shuffle Functions (Seeded PRNG)
 * ============================================================================
 * Fisher-Yates (modern/Knuth) shuffle algorithm.
 * Shuffles array in-place with no allocation.
 * Uses seeded PRNG for reproducible shuffles.
 * ============================================================================ */

/*
 * Instance: Shuffle long array in-place using Fisher-Yates algorithm.
 */
void rt_random_shuffle_long(RtRandom *rng, long *arr) {
    if (rng == NULL || arr == NULL) {
        return;
    }

    size_t n = rt_array_length(arr);
    if (n <= 1) {
        return;
    }

    for (size_t i = n - 1; i > 0; i--) {
        size_t j = (size_t)rt_random_int(rng, 0, (long)i);

        long temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

/*
 * Instance: Shuffle double array in-place using Fisher-Yates algorithm.
 */
void rt_random_shuffle_double(RtRandom *rng, double *arr) {
    if (rng == NULL || arr == NULL) {
        return;
    }

    size_t n = rt_array_length(arr);
    if (n <= 1) {
        return;
    }

    for (size_t i = n - 1; i > 0; i--) {
        size_t j = (size_t)rt_random_int(rng, 0, (long)i);

        double temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

/*
 * Instance: Shuffle string array in-place using Fisher-Yates algorithm.
 */
void rt_random_shuffle_string(RtRandom *rng, char **arr) {
    if (rng == NULL || arr == NULL) {
        return;
    }

    size_t n = rt_array_length(arr);
    if (n <= 1) {
        return;
    }

    for (size_t i = n - 1; i > 0; i--) {
        size_t j = (size_t)rt_random_int(rng, 0, (long)i);

        char *temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

/*
 * Instance: Shuffle bool array in-place using Fisher-Yates algorithm.
 */
void rt_random_shuffle_bool(RtRandom *rng, int *arr) {
    if (rng == NULL || arr == NULL) {
        return;
    }

    size_t n = rt_array_length(arr);
    if (n <= 1) {
        return;
    }

    for (size_t i = n - 1; i > 0; i--) {
        size_t j = (size_t)rt_random_int(rng, 0, (long)i);

        int temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

/*
 * Instance: Shuffle byte array in-place using Fisher-Yates algorithm.
 */
void rt_random_shuffle_byte(RtRandom *rng, unsigned char *arr) {
    if (rng == NULL || arr == NULL) {
        return;
    }

    size_t n = rt_array_length(arr);
    if (n <= 1) {
        return;
    }

    for (size_t i = n - 1; i > 0; i--) {
        size_t j = (size_t)rt_random_int(rng, 0, (long)i);

        unsigned char temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

/* ============================================================================
 * Static Sample Functions (OS Entropy)
 * ============================================================================
 * Random sampling without replacement using partial Fisher-Yates shuffle.
 * Returns a new array with `count` randomly selected elements from the input.
 * ============================================================================ */

/*
 * Static: Random sample without replacement from long array.
 * Uses partial Fisher-Yates shuffle for efficiency.
 * Returns NULL if:
 *   - arena is NULL
 *   - arr is NULL
 *   - count <= 0
 *   - count > array length (invalid sample size)
 *
 * Returns: Arena-allocated array of `count` randomly selected elements.
 */
long *rt_random_static_sample_long(RtArena *arena, long *arr, long count) {
    /* Validate arena and array */
    if (arena == NULL || arr == NULL) {
        return NULL;
    }

    /* Validate count */
    if (count <= 0) {
        return NULL;
    }

    /* Get array length */
    size_t n = rt_array_length(arr);

    /* Validate count <= array length */
    if (count > (long)n) {
        return NULL;
    }

    /* Create temporary copy of the array for shuffling */
    RtArena *temp_arena = rt_arena_create(NULL);
    if (temp_arena == NULL) {
        return NULL;
    }

    long *temp = rt_arena_alloc(temp_arena, n * sizeof(long));
    memcpy(temp, arr, n * sizeof(long));

    /* Allocate result array using arena */
    long *result = rt_array_create_long(arena, count, NULL);
    if (result == NULL) {
        rt_arena_destroy(temp_arena);
        return NULL;
    }

    /* Partial Fisher-Yates shuffle: only shuffle the first `count` elements */
    for (long i = 0; i < count; i++) {
        /* Generate random index j in [i, n-1] inclusive */
        size_t j = (size_t)rt_random_static_int((long)i, (long)(n - 1));

        /* Swap temp[i] and temp[j] */
        long swap = temp[i];
        temp[i] = temp[j];
        temp[j] = swap;

        /* Copy selected element to result */
        result[i] = temp[i];
    }

    rt_arena_destroy(temp_arena);

    return result;
}

/*
 * Static: Random sample without replacement from double array.
 * Uses partial Fisher-Yates shuffle for efficiency.
 * Returns NULL if:
 *   - arena is NULL
 *   - arr is NULL
 *   - count <= 0
 *   - count > array length (invalid sample size)
 *
 * Returns: Arena-allocated array of `count` randomly selected elements.
 */
double *rt_random_static_sample_double(RtArena *arena, double *arr, long count) {
    /* Validate arena and array */
    if (arena == NULL || arr == NULL) {
        return NULL;
    }

    /* Validate count */
    if (count <= 0) {
        return NULL;
    }

    /* Get array length */
    size_t n = rt_array_length(arr);

    /* Validate count <= array length */
    if (count > (long)n) {
        return NULL;
    }

    /* Create temporary copy of the array for shuffling */
    RtArena *temp_arena = rt_arena_create(NULL);
    if (temp_arena == NULL) {
        return NULL;
    }

    double *temp = rt_arena_alloc(temp_arena, n * sizeof(double));
    memcpy(temp, arr, n * sizeof(double));

    /* Allocate result array using arena */
    double *result = rt_array_create_double(arena, count, NULL);
    if (result == NULL) {
        rt_arena_destroy(temp_arena);
        return NULL;
    }

    /* Partial Fisher-Yates shuffle: only shuffle the first `count` elements */
    for (long i = 0; i < count; i++) {
        /* Generate random index j in [i, n-1] inclusive */
        size_t j = (size_t)rt_random_static_int((long)i, (long)(n - 1));

        /* Swap temp[i] and temp[j] */
        double swap = temp[i];
        temp[i] = temp[j];
        temp[j] = swap;

        /* Copy selected element to result */
        result[i] = temp[i];
    }

    rt_arena_destroy(temp_arena);

    return result;
}

/*
 * Static: Random sample without replacement from string array.
 * Uses partial Fisher-Yates shuffle for efficiency.
 * Returns NULL if:
 *   - arena is NULL
 *   - arr is NULL
 *   - count <= 0
 *   - count > array length (invalid sample size)
 *
 * Returns: Arena-allocated array of `count` randomly selected string pointers.
 */
char **rt_random_static_sample_string(RtArena *arena, char **arr, long count) {
    /* Validate arena and array */
    if (arena == NULL || arr == NULL) {
        return NULL;
    }

    /* Validate count */
    if (count <= 0) {
        return NULL;
    }

    /* Get array length */
    size_t n = rt_array_length(arr);

    /* Validate count <= array length */
    if (count > (long)n) {
        return NULL;
    }

    /* Create temporary copy of the array for shuffling */
    RtArena *temp_arena = rt_arena_create(NULL);
    if (temp_arena == NULL) {
        return NULL;
    }

    char **temp = rt_arena_alloc(temp_arena, n * sizeof(char *));
    memcpy(temp, arr, n * sizeof(char *));

    /* Allocate result array using arena */
    char **result = rt_array_create_string(arena, count, NULL);
    if (result == NULL) {
        rt_arena_destroy(temp_arena);
        return NULL;
    }

    /* Partial Fisher-Yates shuffle: only shuffle the first `count` elements */
    for (long i = 0; i < count; i++) {
        /* Generate random index j in [i, n-1] inclusive */
        size_t j = (size_t)rt_random_static_int((long)i, (long)(n - 1));

        /* Swap temp[i] and temp[j] */
        char *swap = temp[i];
        temp[i] = temp[j];
        temp[j] = swap;

        /* Copy selected element to result */
        result[i] = temp[i];
    }

    rt_arena_destroy(temp_arena);

    return result;
}

/* ============================================================================
 * Instance Sample Functions (Seeded PRNG)
 * ============================================================================
 * Random sampling without replacement using seeded PRNG for reproducibility.
 * Returns a new array with `count` randomly selected elements from the input.
 * ============================================================================ */

/*
 * Instance: Random sample without replacement from long array.
 * Uses seeded PRNG for reproducible sampling.
 * Returns NULL if:
 *   - arena is NULL
 *   - rng is NULL
 *   - arr is NULL
 *   - count <= 0
 *   - count > array length (invalid sample size)
 *
 * Returns: Arena-allocated array of `count` randomly selected elements.
 */
long *rt_random_sample_long(RtArena *arena, RtRandom *rng, long *arr, long count) {
    /* Validate inputs */
    if (arena == NULL || rng == NULL || arr == NULL) {
        return NULL;
    }

    /* Validate count */
    if (count <= 0) {
        return NULL;
    }

    /* Get array length */
    size_t n = rt_array_length(arr);

    /* Validate count <= array length */
    if (count > (long)n) {
        return NULL;
    }

    /* Create temporary copy of the array for shuffling */
    RtArena *temp_arena = rt_arena_create(NULL);
    if (temp_arena == NULL) {
        return NULL;
    }

    long *temp = rt_arena_alloc(temp_arena, n * sizeof(long));
    memcpy(temp, arr, n * sizeof(long));

    /* Allocate result array using arena */
    long *result = rt_array_create_long(arena, count, NULL);
    if (result == NULL) {
        rt_arena_destroy(temp_arena);
        return NULL;
    }

    /* Partial Fisher-Yates shuffle: only shuffle the first `count` elements */
    for (long i = 0; i < count; i++) {
        /* Generate random index j in [i, n-1] inclusive using seeded PRNG */
        size_t j = (size_t)rt_random_int(rng, (long)i, (long)(n - 1));

        /* Swap temp[i] and temp[j] */
        long swap = temp[i];
        temp[i] = temp[j];
        temp[j] = swap;

        /* Copy selected element to result */
        result[i] = temp[i];
    }

    rt_arena_destroy(temp_arena);

    return result;
}

/*
 * Instance: Random sample without replacement from double array.
 * Uses seeded PRNG for reproducible sampling.
 * Returns NULL if:
 *   - arena is NULL
 *   - rng is NULL
 *   - arr is NULL
 *   - count <= 0
 *   - count > array length (invalid sample size)
 *
 * Returns: Arena-allocated array of `count` randomly selected elements.
 */
double *rt_random_sample_double(RtArena *arena, RtRandom *rng, double *arr, long count) {
    /* Validate inputs */
    if (arena == NULL || rng == NULL || arr == NULL) {
        return NULL;
    }

    /* Validate count */
    if (count <= 0) {
        return NULL;
    }

    /* Get array length */
    size_t n = rt_array_length(arr);

    /* Validate count <= array length */
    if (count > (long)n) {
        return NULL;
    }

    /* Create temporary copy of the array for shuffling */
    RtArena *temp_arena = rt_arena_create(NULL);
    if (temp_arena == NULL) {
        return NULL;
    }

    double *temp = rt_arena_alloc(temp_arena, n * sizeof(double));
    memcpy(temp, arr, n * sizeof(double));

    /* Allocate result array using arena */
    double *result = rt_array_create_double(arena, count, NULL);
    if (result == NULL) {
        rt_arena_destroy(temp_arena);
        return NULL;
    }

    /* Partial Fisher-Yates shuffle: only shuffle the first `count` elements */
    for (long i = 0; i < count; i++) {
        /* Generate random index j in [i, n-1] inclusive using seeded PRNG */
        size_t j = (size_t)rt_random_int(rng, (long)i, (long)(n - 1));

        /* Swap temp[i] and temp[j] */
        double swap = temp[i];
        temp[i] = temp[j];
        temp[j] = swap;

        /* Copy selected element to result */
        result[i] = temp[i];
    }

    rt_arena_destroy(temp_arena);

    return result;
}

/*
 * Instance: Random sample without replacement from string array.
 * Uses seeded PRNG for reproducible sampling.
 * Returns NULL if:
 *   - arena is NULL
 *   - rng is NULL
 *   - arr is NULL
 *   - count <= 0
 *   - count > array length (invalid sample size)
 *
 * Returns: Arena-allocated array of `count` randomly selected string pointers.
 */
char **rt_random_sample_string(RtArena *arena, RtRandom *rng, char **arr, long count) {
    /* Validate inputs */
    if (arena == NULL || rng == NULL || arr == NULL) {
        return NULL;
    }

    /* Validate count */
    if (count <= 0) {
        return NULL;
    }

    /* Get array length */
    size_t n = rt_array_length(arr);

    /* Validate count <= array length */
    if (count > (long)n) {
        return NULL;
    }

    /* Create temporary copy of the array for shuffling */
    RtArena *temp_arena = rt_arena_create(NULL);
    if (temp_arena == NULL) {
        return NULL;
    }

    char **temp = rt_arena_alloc(temp_arena, n * sizeof(char *));
    memcpy(temp, arr, n * sizeof(char *));

    /* Allocate result array using arena */
    char **result = rt_array_create_string(arena, count, NULL);
    if (result == NULL) {
        rt_arena_destroy(temp_arena);
        return NULL;
    }

    /* Partial Fisher-Yates shuffle: only shuffle the first `count` elements */
    for (long i = 0; i < count; i++) {
        /* Generate random index j in [i, n-1] inclusive using seeded PRNG */
        size_t j = (size_t)rt_random_int(rng, (long)i, (long)(n - 1));

        /* Swap temp[i] and temp[j] */
        char *swap = temp[i];
        temp[i] = temp[j];
        temp[j] = swap;

        /* Copy selected element to result */
        result[i] = temp[i];
    }

    rt_arena_destroy(temp_arena);

    return result;
}
