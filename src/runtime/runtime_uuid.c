// src/runtime/runtime_uuid.c
// UUID generation and manipulation for Sindarin runtime

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
    #if defined(__MINGW32__) || defined(__MINGW64__)
    /* MinGW is POSIX-compatible */
    #include <sys/time.h>
    #else
    #include "../platform/compat_windows.h"
    #include "../platform/compat_time.h"
    #endif
#else
#include <sys/time.h>
#endif

#include "runtime_uuid.h"
#include "runtime_random.h"
#include "runtime_arena.h"
#include "runtime_time.h"
#include "runtime_sha1.h"

/* ============================================================================
 * Namespace Constants (RFC 9562)
 * ============================================================================
 * Standard namespaces for generating v5 deterministic UUIDs.
 * ============================================================================ */

/* DNS namespace: 6ba7b810-9dad-11d1-80b4-00c04fd430c8 */
const RtUuid RT_UUID_NAMESPACE_DNS = {
    .high = 0x6ba7b8109dad11d1ULL,
    .low  = 0x80b400c04fd430c8ULL
};

/* URL namespace: 6ba7b811-9dad-11d1-80b4-00c04fd430c8 */
const RtUuid RT_UUID_NAMESPACE_URL = {
    .high = 0x6ba7b8119dad11d1ULL,
    .low  = 0x80b400c04fd430c8ULL
};

/* OID namespace: 6ba7b812-9dad-11d1-80b4-00c04fd430c8 */
const RtUuid RT_UUID_NAMESPACE_OID = {
    .high = 0x6ba7b8129dad11d1ULL,
    .low  = 0x80b400c04fd430c8ULL
};

/* X.500 namespace: 6ba7b814-9dad-11d1-80b4-00c04fd430c8 */
const RtUuid RT_UUID_NAMESPACE_X500 = {
    .high = 0x6ba7b8149dad11d1ULL,
    .low  = 0x80b400c04fd430c8ULL
};

/* ============================================================================
 * UUIDv4 Generation
 * ============================================================================
 * Generate a random UUID following RFC 9562 (version 4).
 *
 * UUIDv4 Structure (128 bits total):
 *   - Bits 0-31:   Random
 *   - Bits 32-47:  Random
 *   - Bits 48-51:  Version (0100 = 4)
 *   - Bits 52-63:  Random
 *   - Bits 64-65:  Variant (10)
 *   - Bits 66-127: Random
 *
 * The version and variant bits are set according to RFC 9562:
 *   - Version 4: bits 48-51 of high word = 0100 (binary)
 *   - Variant 1: bits 64-65 of low word = 10 (binary)
 * ============================================================================ */

RtUuid *rt_uuid_v4(RtArena *arena) {
    if (arena == NULL) {
        return NULL;
    }

    /* Allocate UUID structure in arena */
    RtUuid *uuid = rt_arena_alloc(arena, sizeof(RtUuid));
    if (uuid == NULL) {
        return NULL;
    }

    /* Fill with 16 bytes of random data from OS entropy */
    uint8_t bytes[16];
    rt_random_fill_entropy(bytes, sizeof(bytes));

    /* Construct high 64 bits from first 8 bytes */
    uuid->high = ((uint64_t)bytes[0] << 56) |
                 ((uint64_t)bytes[1] << 48) |
                 ((uint64_t)bytes[2] << 40) |
                 ((uint64_t)bytes[3] << 32) |
                 ((uint64_t)bytes[4] << 24) |
                 ((uint64_t)bytes[5] << 16) |
                 ((uint64_t)bytes[6] << 8)  |
                 ((uint64_t)bytes[7]);

    /* Construct low 64 bits from last 8 bytes */
    uuid->low = ((uint64_t)bytes[8] << 56)  |
                ((uint64_t)bytes[9] << 48)  |
                ((uint64_t)bytes[10] << 40) |
                ((uint64_t)bytes[11] << 32) |
                ((uint64_t)bytes[12] << 24) |
                ((uint64_t)bytes[13] << 16) |
                ((uint64_t)bytes[14] << 8)  |
                ((uint64_t)bytes[15]);

    /*
     * Set version bits (bits 48-51 in high word)
     * Version 4 = 0100 (binary)
     *
     * Layout of uuid->high (bits 63-0):
     *   bits 63-48: bytes[0-1] (random)
     *   bits 47-32: bytes[2-3] (random)
     *   bits 31-16: bytes[4-5] (random)
     *   bits 15-0:  bytes[6-7] (contains version at bits 15-12)
     *
     * The version is in bits 15-12 of uuid->high, which corresponds to
     * the upper nibble of byte 6 in the standard UUID byte layout.
     */
    uuid->high = (uuid->high & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;

    /*
     * Set variant bits (bits 63-62 in low word)
     * Variant 1 (RFC 9562) = 10 (binary)
     *
     * Layout of uuid->low (bits 63-0):
     *   bits 63-56: byte[8] (contains variant at bits 63-62)
     *   bits 55-0:  bytes[9-15] (random)
     *
     * The variant is in the two MSBs of uuid->low.
     * Mask off bits 63-62 (AND with 0x3FFF...) then set bit 63 (OR with 0x8000...).
     */
    uuid->low = (uuid->low & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

    return uuid;
}

/* ============================================================================
 * UUIDv5 Generation
 * ============================================================================
 * Generate a deterministic UUID from namespace + name using SHA-1 hash.
 *
 * UUIDv5 Algorithm (RFC 9562):
 *   1. Concatenate namespace UUID (16 bytes) + name
 *   2. Compute SHA-1 hash (20 bytes)
 *   3. Take first 16 bytes of hash
 *   4. Set version bits (5) and variant bits
 *
 * UUIDv5 Structure (128 bits total):
 *   - Bits 0-31:   Hash bytes 0-3
 *   - Bits 32-47:  Hash bytes 4-5
 *   - Bits 48-51:  Version (0101 = 5)
 *   - Bits 52-63:  Hash bytes 6-7 (lower 12 bits)
 *   - Bits 64-65:  Variant (10)
 *   - Bits 66-127: Hash bytes 8-15
 *
 * Key property: Same namespace + name always produces the same UUID.
 * ============================================================================ */

RtUuid *rt_uuid_v5(RtArena *arena, RtUuid *namespace_uuid, const char *name) {
    if (arena == NULL || namespace_uuid == NULL || name == NULL) {
        return NULL;
    }

    /* Allocate UUID structure in arena */
    RtUuid *uuid = rt_arena_alloc(arena, sizeof(RtUuid));
    if (uuid == NULL) {
        return NULL;
    }

    /* Get namespace UUID as 16 bytes */
    uint8_t namespace_bytes[16];
    namespace_bytes[0]  = (uint8_t)(namespace_uuid->high >> 56);
    namespace_bytes[1]  = (uint8_t)(namespace_uuid->high >> 48);
    namespace_bytes[2]  = (uint8_t)(namespace_uuid->high >> 40);
    namespace_bytes[3]  = (uint8_t)(namespace_uuid->high >> 32);
    namespace_bytes[4]  = (uint8_t)(namespace_uuid->high >> 24);
    namespace_bytes[5]  = (uint8_t)(namespace_uuid->high >> 16);
    namespace_bytes[6]  = (uint8_t)(namespace_uuid->high >> 8);
    namespace_bytes[7]  = (uint8_t)(namespace_uuid->high);
    namespace_bytes[8]  = (uint8_t)(namespace_uuid->low >> 56);
    namespace_bytes[9]  = (uint8_t)(namespace_uuid->low >> 48);
    namespace_bytes[10] = (uint8_t)(namespace_uuid->low >> 40);
    namespace_bytes[11] = (uint8_t)(namespace_uuid->low >> 32);
    namespace_bytes[12] = (uint8_t)(namespace_uuid->low >> 24);
    namespace_bytes[13] = (uint8_t)(namespace_uuid->low >> 16);
    namespace_bytes[14] = (uint8_t)(namespace_uuid->low >> 8);
    namespace_bytes[15] = (uint8_t)(namespace_uuid->low);

    /* Compute SHA-1 hash of namespace + name */
    size_t name_len = strlen(name);
    uint8_t digest[SHA1_DIGEST_SIZE];

    SHA1_Context ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, namespace_bytes, 16);
    sha1_update(&ctx, (const uint8_t *)name, name_len);
    sha1_final(&ctx, digest);

    /*
     * Build UUID from first 16 bytes of SHA-1 hash.
     * Bytes are in big-endian order.
     */
    uuid->high = ((uint64_t)digest[0] << 56) |
                 ((uint64_t)digest[1] << 48) |
                 ((uint64_t)digest[2] << 40) |
                 ((uint64_t)digest[3] << 32) |
                 ((uint64_t)digest[4] << 24) |
                 ((uint64_t)digest[5] << 16) |
                 ((uint64_t)digest[6] << 8)  |
                 ((uint64_t)digest[7]);

    uuid->low = ((uint64_t)digest[8] << 56)  |
                ((uint64_t)digest[9] << 48)  |
                ((uint64_t)digest[10] << 40) |
                ((uint64_t)digest[11] << 32) |
                ((uint64_t)digest[12] << 24) |
                ((uint64_t)digest[13] << 16) |
                ((uint64_t)digest[14] << 8)  |
                ((uint64_t)digest[15]);

    /*
     * Set version bits (bits 15-12 in high word)
     * Version 5 = 0101 (binary)
     */
    uuid->high = (uuid->high & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000005000ULL;

    /*
     * Set variant bits (bits 63-62 in low word)
     * Variant 1 (RFC 9562) = 10 (binary)
     */
    uuid->low = (uuid->low & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

    return uuid;
}

/* ============================================================================
 * Property Getters
 * ============================================================================ */

/*
 * Get UUID version (1-8).
 * The version is stored in bits 15-12 of the high word.
 */
long rt_uuid_get_version(RtUuid *uuid) {
    if (uuid == NULL) {
        return 0;
    }
    /* Extract bits 15-12 from high word */
    return (long)((uuid->high >> 12) & 0x0F);
}

/*
 * Get UUID variant.
 * The variant is determined by the most significant bits of byte 8.
 *
 * Variant encoding (bits 63-62 of low word):
 *   0x = 0 (NCS backward compatibility)
 *   10 = 1 (RFC 9562 / RFC 4122)
 *   110 = 2 (Microsoft backward compatibility)
 *   111 = 3 (Reserved for future)
 */
long rt_uuid_get_variant(RtUuid *uuid) {
    if (uuid == NULL) {
        return 0;
    }

    /* Get the two MSBs of the low word (bits 63-62) */
    uint64_t variant_bits = (uuid->low >> 62) & 0x03;

    if ((variant_bits & 0x02) == 0) {
        /* 0x = variant 0 (NCS) */
        return 0;
    } else if ((variant_bits & 0x03) == 0x02) {
        /* 10 = variant 1 (RFC 9562) */
        return 1;
    } else if ((variant_bits & 0x03) == 0x03) {
        /* Check bit 61 to distinguish variant 2 from future */
        uint64_t bit61 = (uuid->low >> 61) & 0x01;
        if (bit61 == 0) {
            /* 110 = variant 2 (Microsoft) */
            return 2;
        } else {
            /* 111 = variant 3 (reserved) */
            return 3;
        }
    }

    return 1; /* Default to RFC variant */
}

/*
 * Check if UUID is nil (all zeros).
 */
int rt_uuid_is_nil(RtUuid *uuid) {
    if (uuid == NULL) {
        return 0;
    }
    return (uuid->high == 0 && uuid->low == 0) ? 1 : 0;
}

/* ============================================================================
 * Conversion Methods
 * ============================================================================ */

/*
 * Format UUID as standard 36-char string.
 * Format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
 */
char *rt_uuid_to_string(RtArena *arena, RtUuid *uuid) {
    if (arena == NULL || uuid == NULL) {
        return NULL;
    }

    /* UUID string is 36 chars + null terminator */
    char *str = rt_arena_alloc(arena, 37);
    if (str == NULL) {
        return NULL;
    }

    /*
     * UUID byte layout (16 bytes):
     *   time_low:          bytes 0-3  (32 bits) - from high bits 63-32
     *   time_mid:          bytes 4-5  (16 bits) - from high bits 31-16
     *   time_hi_version:   bytes 6-7  (16 bits) - from high bits 15-0
     *   clock_seq_hi_var:  byte 8     (8 bits)  - from low bits 63-56
     *   clock_seq_low:     byte 9     (8 bits)  - from low bits 55-48
     *   node:              bytes 10-15 (48 bits) - from low bits 47-0
     */
    uint32_t time_low = (uint32_t)(uuid->high >> 32);
    uint16_t time_mid = (uint16_t)((uuid->high >> 16) & 0xFFFF);
    uint16_t time_hi_version = (uint16_t)(uuid->high & 0xFFFF);
    uint16_t clock_seq = (uint16_t)((uuid->low >> 48) & 0xFFFF);
    uint64_t node = uuid->low & 0xFFFFFFFFFFFFULL;

    snprintf(str, 37, "%08x-%04x-%04x-%04x-%012llx",
             time_low, time_mid, time_hi_version, clock_seq,
             (unsigned long long)node);

    return str;
}

/*
 * Format UUID as 32-char hex string (no dashes).
 */
char *rt_uuid_to_hex(RtArena *arena, RtUuid *uuid) {
    if (arena == NULL || uuid == NULL) {
        return NULL;
    }

    /* UUID hex string is 32 chars + null terminator */
    char *str = rt_arena_alloc(arena, 33);
    if (str == NULL) {
        return NULL;
    }

    snprintf(str, 33, "%016llx%016llx",
             (unsigned long long)uuid->high,
             (unsigned long long)uuid->low);

    return str;
}

/*
 * Convert UUID to 16-byte array.
 */
unsigned char *rt_uuid_to_bytes(RtArena *arena, RtUuid *uuid) {
    if (arena == NULL || uuid == NULL) {
        return NULL;
    }

    unsigned char *bytes = rt_arena_alloc(arena, 16);
    if (bytes == NULL) {
        return NULL;
    }

    /* High 64 bits -> bytes 0-7 (big-endian) */
    bytes[0] = (unsigned char)(uuid->high >> 56);
    bytes[1] = (unsigned char)(uuid->high >> 48);
    bytes[2] = (unsigned char)(uuid->high >> 40);
    bytes[3] = (unsigned char)(uuid->high >> 32);
    bytes[4] = (unsigned char)(uuid->high >> 24);
    bytes[5] = (unsigned char)(uuid->high >> 16);
    bytes[6] = (unsigned char)(uuid->high >> 8);
    bytes[7] = (unsigned char)(uuid->high);

    /* Low 64 bits -> bytes 8-15 (big-endian) */
    bytes[8]  = (unsigned char)(uuid->low >> 56);
    bytes[9]  = (unsigned char)(uuid->low >> 48);
    bytes[10] = (unsigned char)(uuid->low >> 40);
    bytes[11] = (unsigned char)(uuid->low >> 32);
    bytes[12] = (unsigned char)(uuid->low >> 24);
    bytes[13] = (unsigned char)(uuid->low >> 16);
    bytes[14] = (unsigned char)(uuid->low >> 8);
    bytes[15] = (unsigned char)(uuid->low);

    return bytes;
}

/* URL-safe base64 alphabet (RFC 4648 §5) */
static const char BASE64_URL_ALPHABET[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

/*
 * Format UUID as 22-char URL-safe base64 string.
 * Uses RFC 4648 §5 URL-safe alphabet (- and _ instead of + and /).
 * No padding is used since UUID is always exactly 16 bytes (128 bits).
 */
char *rt_uuid_to_base64(RtArena *arena, RtUuid *uuid) {
    if (arena == NULL || uuid == NULL) {
        return NULL;
    }

    /* Get UUID as 16 bytes */
    unsigned char bytes[16];
    bytes[0]  = (unsigned char)(uuid->high >> 56);
    bytes[1]  = (unsigned char)(uuid->high >> 48);
    bytes[2]  = (unsigned char)(uuid->high >> 40);
    bytes[3]  = (unsigned char)(uuid->high >> 32);
    bytes[4]  = (unsigned char)(uuid->high >> 24);
    bytes[5]  = (unsigned char)(uuid->high >> 16);
    bytes[6]  = (unsigned char)(uuid->high >> 8);
    bytes[7]  = (unsigned char)(uuid->high);
    bytes[8]  = (unsigned char)(uuid->low >> 56);
    bytes[9]  = (unsigned char)(uuid->low >> 48);
    bytes[10] = (unsigned char)(uuid->low >> 40);
    bytes[11] = (unsigned char)(uuid->low >> 32);
    bytes[12] = (unsigned char)(uuid->low >> 24);
    bytes[13] = (unsigned char)(uuid->low >> 16);
    bytes[14] = (unsigned char)(uuid->low >> 8);
    bytes[15] = (unsigned char)(uuid->low);

    /* 16 bytes -> 22 base64 characters (16 * 8 = 128 bits, 128 / 6 = 21.33, rounds to 22) */
    char *str = rt_arena_alloc(arena, 23);
    if (str == NULL) {
        return NULL;
    }

    /*
     * Encode 16 bytes into 22 base64 characters.
     * Each group of 3 bytes becomes 4 base64 characters.
     * 16 bytes = 5 groups of 3 + 1 remaining byte.
     * 5 * 4 = 20 chars, plus 2 chars for the last byte = 22 chars.
     */
    int out_idx = 0;
    int i;

    /* Process 5 complete groups of 3 bytes (15 bytes) */
    for (i = 0; i < 15; i += 3) {
        uint32_t triplet = ((uint32_t)bytes[i] << 16) |
                           ((uint32_t)bytes[i + 1] << 8) |
                           ((uint32_t)bytes[i + 2]);
        str[out_idx++] = BASE64_URL_ALPHABET[(triplet >> 18) & 0x3F];
        str[out_idx++] = BASE64_URL_ALPHABET[(triplet >> 12) & 0x3F];
        str[out_idx++] = BASE64_URL_ALPHABET[(triplet >> 6) & 0x3F];
        str[out_idx++] = BASE64_URL_ALPHABET[triplet & 0x3F];
    }

    /* Process the last byte (byte 15) - produces 2 base64 chars */
    /* We have 1 byte = 8 bits. In base64, we need 2 characters (12 bits) with 4 bits of padding. */
    str[out_idx++] = BASE64_URL_ALPHABET[(bytes[15] >> 2) & 0x3F];
    str[out_idx++] = BASE64_URL_ALPHABET[(bytes[15] << 4) & 0x3F];

    str[out_idx] = '\0';

    return str;
}

/* ============================================================================
 * Comparison Methods
 * ============================================================================ */

/*
 * Check if two UUIDs are equal.
 */
int rt_uuid_equals(RtUuid *uuid, RtUuid *other) {
    if (uuid == NULL || other == NULL) {
        return (uuid == other) ? 1 : 0;
    }
    return (uuid->high == other->high && uuid->low == other->low) ? 1 : 0;
}

/*
 * Compare two UUIDs.
 * Returns -1 if uuid < other, 0 if equal, 1 if uuid > other.
 */
int rt_uuid_compare(RtUuid *uuid, RtUuid *other) {
    if (uuid == NULL && other == NULL) return 0;
    if (uuid == NULL) return -1;
    if (other == NULL) return 1;

    if (uuid->high < other->high) return -1;
    if (uuid->high > other->high) return 1;
    if (uuid->low < other->low) return -1;
    if (uuid->low > other->low) return 1;
    return 0;
}

/*
 * Check if uuid < other.
 */
int rt_uuid_is_less_than(RtUuid *uuid, RtUuid *other) {
    return rt_uuid_compare(uuid, other) < 0 ? 1 : 0;
}

/*
 * Check if uuid > other.
 */
int rt_uuid_is_greater_than(RtUuid *uuid, RtUuid *other) {
    return rt_uuid_compare(uuid, other) > 0 ? 1 : 0;
}

/* ============================================================================
 * Special UUIDs
 * ============================================================================ */

/*
 * Get nil UUID (all zeros).
 */
RtUuid *rt_uuid_nil(RtArena *arena) {
    if (arena == NULL) {
        return NULL;
    }

    RtUuid *uuid = rt_arena_alloc(arena, sizeof(RtUuid));
    if (uuid == NULL) {
        return NULL;
    }

    uuid->high = 0;
    uuid->low = 0;
    return uuid;
}

/*
 * Get max UUID (all ones).
 */
RtUuid *rt_uuid_max(RtArena *arena) {
    if (arena == NULL) {
        return NULL;
    }

    RtUuid *uuid = rt_arena_alloc(arena, sizeof(RtUuid));
    if (uuid == NULL) {
        return NULL;
    }

    uuid->high = 0xFFFFFFFFFFFFFFFFULL;
    uuid->low = 0xFFFFFFFFFFFFFFFFULL;
    return uuid;
}

/* ============================================================================
 * Namespace Accessors
 * ============================================================================ */

RtUuid *rt_uuid_namespace_dns(RtArena *arena) {
    if (arena == NULL) {
        return NULL;
    }

    RtUuid *uuid = rt_arena_alloc(arena, sizeof(RtUuid));
    if (uuid == NULL) {
        return NULL;
    }

    *uuid = RT_UUID_NAMESPACE_DNS;
    return uuid;
}

RtUuid *rt_uuid_namespace_url(RtArena *arena) {
    if (arena == NULL) {
        return NULL;
    }

    RtUuid *uuid = rt_arena_alloc(arena, sizeof(RtUuid));
    if (uuid == NULL) {
        return NULL;
    }

    *uuid = RT_UUID_NAMESPACE_URL;
    return uuid;
}

RtUuid *rt_uuid_namespace_oid(RtArena *arena) {
    if (arena == NULL) {
        return NULL;
    }

    RtUuid *uuid = rt_arena_alloc(arena, sizeof(RtUuid));
    if (uuid == NULL) {
        return NULL;
    }

    *uuid = RT_UUID_NAMESPACE_OID;
    return uuid;
}

RtUuid *rt_uuid_namespace_x500(RtArena *arena) {
    if (arena == NULL) {
        return NULL;
    }

    RtUuid *uuid = rt_arena_alloc(arena, sizeof(RtUuid));
    if (uuid == NULL) {
        return NULL;
    }

    *uuid = RT_UUID_NAMESPACE_X500;
    return uuid;
}

/* ============================================================================
 * UUIDv7 Generation
 * ============================================================================
 * Generate a time-ordered UUID following RFC 9562 (version 7).
 *
 * UUIDv7 Structure (128 bits total):
 *   - Bits 0-47:   Unix timestamp in milliseconds (48 bits)
 *   - Bits 48-51:  Version (0111 = 7)
 *   - Bits 52-63:  Random (12 bits)
 *   - Bits 64-65:  Variant (10)
 *   - Bits 66-127: Random (62 bits)
 *
 * The version and variant bits are set according to RFC 9562:
 *   - Version 7: bits 48-51 = 0111 (binary)
 *   - Variant 1: bits 64-65 = 10 (binary)
 *
 * UUIDv7 provides:
 *   - Time-ordered UUIDs (good for database indexing)
 *   - Millisecond precision timestamp
 *   - Sufficient randomness for uniqueness
 * ============================================================================ */

RtUuid *rt_uuid_v7(RtArena *arena) {
    if (arena == NULL) {
        return NULL;
    }

    /* Allocate UUID structure in arena */
    RtUuid *uuid = rt_arena_alloc(arena, sizeof(RtUuid));
    if (uuid == NULL) {
        return NULL;
    }

    /* Get current Unix timestamp in milliseconds */
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t timestamp_ms = (uint64_t)tv.tv_sec * 1000ULL + (uint64_t)tv.tv_usec / 1000ULL;

    /* Get 10 bytes of random data for the random portions */
    uint8_t random_bytes[10];
    rt_random_fill_entropy(random_bytes, sizeof(random_bytes));

    /*
     * Build the high 64 bits:
     *   - Bits 63-16: Unix timestamp in milliseconds (48 bits)
     *   - Bits 15-12: Version (0111 = 7)
     *   - Bits 11-0:  Random (12 bits from random_bytes[0-1])
     *
     * High word layout:
     *   [timestamp 48 bits][version 4 bits][random 12 bits]
     */
    uuid->high = (timestamp_ms << 16) |           /* Timestamp in upper 48 bits (shifted to bits 63-16) */
                 0x7000ULL |                       /* Version 7 in bits 15-12 */
                 ((uint64_t)(random_bytes[0] & 0x0F) << 8) |  /* Random high nibble (4 bits) */
                 (uint64_t)random_bytes[1];                   /* Random low byte (8 bits) */

    /*
     * Build the low 64 bits:
     *   - Bits 63-62: Variant (10)
     *   - Bits 61-0:  Random (62 bits from random_bytes[2-9])
     */
    uuid->low = ((uint64_t)random_bytes[2] << 56) |
                ((uint64_t)random_bytes[3] << 48) |
                ((uint64_t)random_bytes[4] << 40) |
                ((uint64_t)random_bytes[5] << 32) |
                ((uint64_t)random_bytes[6] << 24) |
                ((uint64_t)random_bytes[7] << 16) |
                ((uint64_t)random_bytes[8] << 8)  |
                ((uint64_t)random_bytes[9]);

    /* Set variant bits (bits 63-62 = 10) */
    uuid->low = (uuid->low & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

    return uuid;
}

/*
 * Create a UUID using the recommended default (v7).
 * v7 is recommended because it provides time-ordered UUIDs which are
 * better for database index performance.
 */
RtUuid *rt_uuid_create(RtArena *arena) {
    return rt_uuid_v7(arena);
}

/* ============================================================================
 * Parsing Helpers
 * ============================================================================ */

/*
 * Convert a hex character to its numeric value (0-15).
 * Returns -1 for invalid characters.
 * Handles both uppercase and lowercase.
 */
static int hex_char_to_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* ============================================================================
 * Parsing Methods
 * ============================================================================ */

/*
 * Parse UUID from standard 36-char format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
 * Returns NULL for invalid input.
 */
RtUuid *rt_uuid_from_string(RtArena *arena, const char *str) {
    if (arena == NULL || str == NULL) {
        return NULL;
    }

    /* Verify length is exactly 36 characters */
    size_t len = strlen(str);
    if (len != 36) {
        return NULL;
    }

    /* Verify dashes are at positions 8, 13, 18, 23 */
    if (str[8] != '-' || str[13] != '-' || str[18] != '-' || str[23] != '-') {
        return NULL;
    }

    /* Parse hex digits into bytes */
    uint8_t bytes[16];
    int byte_idx = 0;
    int hex_positions[] = {0, 2, 4, 6,      /* time_low: 4 bytes */
                          9, 11,             /* time_mid: 2 bytes */
                          14, 16,            /* time_hi_version: 2 bytes */
                          19, 21,            /* clock_seq: 2 bytes */
                          24, 26, 28, 30, 32, 34}; /* node: 6 bytes */

    for (int i = 0; i < 16; i++) {
        int pos = hex_positions[i];
        int high_nibble = hex_char_to_int(str[pos]);
        int low_nibble = hex_char_to_int(str[pos + 1]);

        if (high_nibble < 0 || low_nibble < 0) {
            return NULL; /* Invalid hex character */
        }

        bytes[byte_idx++] = (uint8_t)((high_nibble << 4) | low_nibble);
    }

    /* Allocate UUID structure */
    RtUuid *uuid = rt_arena_alloc(arena, sizeof(RtUuid));
    if (uuid == NULL) {
        return NULL;
    }

    /* Construct high 64 bits from first 8 bytes */
    uuid->high = ((uint64_t)bytes[0] << 56) |
                 ((uint64_t)bytes[1] << 48) |
                 ((uint64_t)bytes[2] << 40) |
                 ((uint64_t)bytes[3] << 32) |
                 ((uint64_t)bytes[4] << 24) |
                 ((uint64_t)bytes[5] << 16) |
                 ((uint64_t)bytes[6] << 8)  |
                 ((uint64_t)bytes[7]);

    /* Construct low 64 bits from last 8 bytes */
    uuid->low = ((uint64_t)bytes[8] << 56)  |
                ((uint64_t)bytes[9] << 48)  |
                ((uint64_t)bytes[10] << 40) |
                ((uint64_t)bytes[11] << 32) |
                ((uint64_t)bytes[12] << 24) |
                ((uint64_t)bytes[13] << 16) |
                ((uint64_t)bytes[14] << 8)  |
                ((uint64_t)bytes[15]);

    return uuid;
}

/*
 * Parse UUID from 32-char hex format (no dashes): xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
 * Returns NULL for invalid input.
 */
RtUuid *rt_uuid_from_hex(RtArena *arena, const char *str) {
    if (arena == NULL || str == NULL) {
        return NULL;
    }

    /* Verify length is exactly 32 characters */
    size_t len = strlen(str);
    if (len != 32) {
        return NULL;
    }

    /* Parse hex digits into bytes */
    uint8_t bytes[16];
    for (int i = 0; i < 16; i++) {
        int high_nibble = hex_char_to_int(str[i * 2]);
        int low_nibble = hex_char_to_int(str[i * 2 + 1]);

        if (high_nibble < 0 || low_nibble < 0) {
            return NULL; /* Invalid hex character */
        }

        bytes[i] = (uint8_t)((high_nibble << 4) | low_nibble);
    }

    /* Allocate UUID structure */
    RtUuid *uuid = rt_arena_alloc(arena, sizeof(RtUuid));
    if (uuid == NULL) {
        return NULL;
    }

    /* Construct high 64 bits from first 8 bytes */
    uuid->high = ((uint64_t)bytes[0] << 56) |
                 ((uint64_t)bytes[1] << 48) |
                 ((uint64_t)bytes[2] << 40) |
                 ((uint64_t)bytes[3] << 32) |
                 ((uint64_t)bytes[4] << 24) |
                 ((uint64_t)bytes[5] << 16) |
                 ((uint64_t)bytes[6] << 8)  |
                 ((uint64_t)bytes[7]);

    /* Construct low 64 bits from last 8 bytes */
    uuid->low = ((uint64_t)bytes[8] << 56)  |
                ((uint64_t)bytes[9] << 48)  |
                ((uint64_t)bytes[10] << 40) |
                ((uint64_t)bytes[11] << 32) |
                ((uint64_t)bytes[12] << 24) |
                ((uint64_t)bytes[13] << 16) |
                ((uint64_t)bytes[14] << 8)  |
                ((uint64_t)bytes[15]);

    return uuid;
}

/*
 * Create UUID from 16-byte array.
 */
RtUuid *rt_uuid_from_bytes(RtArena *arena, const unsigned char *bytes) {
    if (arena == NULL || bytes == NULL) {
        return NULL;
    }

    /* Allocate UUID structure */
    RtUuid *uuid = rt_arena_alloc(arena, sizeof(RtUuid));
    if (uuid == NULL) {
        return NULL;
    }

    /* Construct high 64 bits from first 8 bytes (big-endian) */
    uuid->high = ((uint64_t)bytes[0] << 56) |
                 ((uint64_t)bytes[1] << 48) |
                 ((uint64_t)bytes[2] << 40) |
                 ((uint64_t)bytes[3] << 32) |
                 ((uint64_t)bytes[4] << 24) |
                 ((uint64_t)bytes[5] << 16) |
                 ((uint64_t)bytes[6] << 8)  |
                 ((uint64_t)bytes[7]);

    /* Construct low 64 bits from last 8 bytes (big-endian) */
    uuid->low = ((uint64_t)bytes[8] << 56)  |
                ((uint64_t)bytes[9] << 48)  |
                ((uint64_t)bytes[10] << 40) |
                ((uint64_t)bytes[11] << 32) |
                ((uint64_t)bytes[12] << 24) |
                ((uint64_t)bytes[13] << 16) |
                ((uint64_t)bytes[14] << 8)  |
                ((uint64_t)bytes[15]);

    return uuid;
}

/*
 * Decode a single base64 URL-safe character to its 6-bit value.
 * Returns -1 for invalid characters.
 */
static int base64_url_char_to_int(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '-') return 62;
    if (c == '_') return 63;
    return -1;
}

/*
 * Parse UUID from 22-char URL-safe base64 format.
 * Uses RFC 4648 §5 URL-safe alphabet (- and _ instead of + and /).
 * Returns NULL for invalid input.
 */
RtUuid *rt_uuid_from_base64(RtArena *arena, const char *str) {
    if (arena == NULL || str == NULL) {
        return NULL;
    }

    /* Verify length is exactly 22 characters */
    size_t len = strlen(str);
    if (len != 22) {
        return NULL;
    }

    /* Decode base64 to 16 bytes */
    uint8_t bytes[16];
    int byte_idx = 0;

    /*
     * 22 base64 characters encode 132 bits (22 * 6).
     * We need 128 bits (16 bytes), so the last 4 bits are padding zeros.
     *
     * Process 5 groups of 4 characters (20 chars = 15 bytes),
     * then 2 remaining characters (for the last byte).
     */

    /* Process 5 complete groups of 4 base64 chars -> 3 bytes each */
    for (int i = 0; i < 20; i += 4) {
        int v0 = base64_url_char_to_int(str[i]);
        int v1 = base64_url_char_to_int(str[i + 1]);
        int v2 = base64_url_char_to_int(str[i + 2]);
        int v3 = base64_url_char_to_int(str[i + 3]);

        if (v0 < 0 || v1 < 0 || v2 < 0 || v3 < 0) {
            return NULL; /* Invalid character */
        }

        uint32_t triplet = ((uint32_t)v0 << 18) | ((uint32_t)v1 << 12) |
                           ((uint32_t)v2 << 6) | (uint32_t)v3;

        bytes[byte_idx++] = (uint8_t)((triplet >> 16) & 0xFF);
        bytes[byte_idx++] = (uint8_t)((triplet >> 8) & 0xFF);
        bytes[byte_idx++] = (uint8_t)(triplet & 0xFF);
    }

    /* Process last 2 characters -> 1 byte (with 4 bits of padding) */
    int v0 = base64_url_char_to_int(str[20]);
    int v1 = base64_url_char_to_int(str[21]);

    if (v0 < 0 || v1 < 0) {
        return NULL; /* Invalid character */
    }

    /* Verify last 4 bits are zero (padding) */
    if ((v1 & 0x0F) != 0) {
        return NULL; /* Invalid padding */
    }

    bytes[byte_idx] = (uint8_t)((v0 << 2) | (v1 >> 4));

    /* Create UUID from bytes */
    return rt_uuid_from_bytes(arena, bytes);
}

/* ============================================================================
 * Time Extraction (v7 only)
 * ============================================================================ */

/*
 * Get Unix timestamp in milliseconds from a v7 UUID.
 * The timestamp is stored in the upper 48 bits of the high word.
 *
 * Returns -1 if the UUID is not version 7.
 */
long long rt_uuid_get_timestamp(RtUuid *uuid) {
    if (uuid == NULL) {
        fprintf(stderr, "rt_uuid_get_timestamp: NULL UUID\n");
        exit(1);
    }

    /* Check version is 7 */
    long version = rt_uuid_get_version(uuid);
    if (version != 7) {
        fprintf(stderr, "rt_uuid_get_timestamp: UUID is not version 7 (version=%ld)\n", version);
        exit(1);
    }

    /* Extract the 48-bit timestamp from the high word (bits 63-16) */
    return (long long)(uuid->high >> 16);
}

/*
 * Get Time when UUID was created (v7 only).
 * This creates an RtTime from the embedded timestamp.
 */
RtTime *rt_uuid_get_time(RtArena *arena, RtUuid *uuid) {
    if (arena == NULL) {
        fprintf(stderr, "rt_uuid_get_time: NULL arena\n");
        return NULL;
    }
    if (uuid == NULL) {
        fprintf(stderr, "rt_uuid_get_time: NULL UUID\n");
        exit(1);
    }

    /* Get timestamp (this also validates version) */
    long long timestamp_ms = rt_uuid_get_timestamp(uuid);

    /* Create RtTime from milliseconds */
    return rt_time_from_millis(arena, timestamp_ms);
}
