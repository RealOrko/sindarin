#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "runtime_random_core.h"

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
 * Internal Helper Functions (exported for other runtime_random_* modules)
 * ============================================================================ */

/*
 * Get next 64-bit random value from the generator.
 * For seeded generators, uses xoshiro256**.
 * For OS-entropy generators, fetches fresh entropy.
 */
uint64_t rt_random_next_u64(RtRandom *rng) {
    if (rng->is_seeded) {
        return xoshiro256_next(rng->state);
    } else {
        /* For OS-entropy mode, get fresh random bytes */
        uint64_t result;
        rt_random_fill_entropy((uint8_t *)&result, sizeof(result));
        return result;
    }
}

/*
 * Get next 64-bit random value from OS entropy (stateless).
 * Used by static methods that don't have an RtRandom instance.
 */
uint64_t rt_random_static_next_u64(void) {
    uint64_t result;
    rt_random_fill_entropy((uint8_t *)&result, sizeof(result));
    return result;
}
