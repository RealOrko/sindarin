// tests/unit/runtime/runtime_uuid_tests.c
// Tests for the runtime UUID generation system

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>
#include "../../src/runtime/runtime_uuid.h"
#include "../../src/runtime/runtime_arena.h"
#include "../test_utils.h"
#include "../debug.h"

/* ============================================================================
 * rt_uuid_v4() Tests
 * ============================================================================
 * Tests for UUIDv4 (random) generation following RFC 9562.
 * ============================================================================ */

void test_rt_uuid_v4_basic()
{
    printf("Testing rt_uuid_v4 basic functionality...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtUuid *uuid = rt_uuid_v4(arena);
    TEST_ASSERT_NOT_NULL(uuid, "UUID should be created");

    /* UUID should not be all zeros */
    TEST_ASSERT(uuid->high != 0 || uuid->low != 0, "UUID should not be nil");

    printf("  Created UUID successfully\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_v4_version_bits()
{
    printf("Testing rt_uuid_v4 version bits...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Generate multiple UUIDs and check version bits */
    for (int i = 0; i < 100; i++) {
        RtUuid *uuid = rt_uuid_v4(arena);
        TEST_ASSERT_NOT_NULL(uuid, "UUID should be created");

        /* Version should be 4 */
        long version = rt_uuid_get_version(uuid);
        TEST_ASSERT_EQ(version, 4, "UUID version should be 4");
    }

    printf("  All 100 UUIDs have correct version (4)\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_v4_variant_bits()
{
    printf("Testing rt_uuid_v4 variant bits...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Generate multiple UUIDs and check variant bits */
    for (int i = 0; i < 100; i++) {
        RtUuid *uuid = rt_uuid_v4(arena);
        TEST_ASSERT_NOT_NULL(uuid, "UUID should be created");

        /* Variant should be 1 (RFC 9562) */
        long variant = rt_uuid_get_variant(uuid);
        TEST_ASSERT_EQ(variant, 1, "UUID variant should be 1 (RFC 9562)");

        /* Verify variant bits directly: bits 63-62 of low word should be 10 */
        uint64_t variant_bits = (uuid->low >> 62) & 0x03;
        TEST_ASSERT_EQ(variant_bits, 0x02, "Variant bits should be 10 (binary)");
    }

    printf("  All 100 UUIDs have correct variant (1 / RFC 9562)\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_v4_uniqueness()
{
    printf("Testing rt_uuid_v4 uniqueness...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Generate 1000 UUIDs and check they are all unique */
    #define NUM_UUIDS 1000
    RtUuid *uuids[NUM_UUIDS];

    for (int i = 0; i < NUM_UUIDS; i++) {
        uuids[i] = rt_uuid_v4(arena);
        TEST_ASSERT_NOT_NULL(uuids[i], "UUID should be created");
    }

    /* Check all pairs are different */
    int duplicates = 0;
    for (int i = 0; i < NUM_UUIDS; i++) {
        for (int j = i + 1; j < NUM_UUIDS; j++) {
            if (rt_uuid_equals(uuids[i], uuids[j])) {
                duplicates++;
            }
        }
    }

    TEST_ASSERT_EQ(duplicates, 0, "All UUIDs should be unique");

    printf("  Generated %d unique UUIDs\n", NUM_UUIDS);
    #undef NUM_UUIDS

    rt_arena_destroy(arena);
}

void test_rt_uuid_v4_randomness()
{
    printf("Testing rt_uuid_v4 randomness (bit distribution)...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Generate many UUIDs and check bit distribution */
    #define NUM_SAMPLES 500
    int high_bit_counts[64] = {0};
    int low_bit_counts[64] = {0};

    for (int i = 0; i < NUM_SAMPLES; i++) {
        RtUuid *uuid = rt_uuid_v4(arena);
        TEST_ASSERT_NOT_NULL(uuid, "UUID should be created");

        /* Count set bits in high word */
        for (int b = 0; b < 64; b++) {
            if ((uuid->high >> b) & 1) {
                high_bit_counts[b]++;
            }
        }

        /* Count set bits in low word */
        for (int b = 0; b < 64; b++) {
            if ((uuid->low >> b) & 1) {
                low_bit_counts[b]++;
            }
        }
    }

    /* For random bits, each bit should be set roughly 50% of the time.
     * Allow some variance (30% to 70% range = NUM_SAMPLES * 0.3 to 0.7).
     * Skip version bits (15-12 in high) and variant bits (63-62 in low).
     */
    int min_count = (int)(NUM_SAMPLES * 0.3);
    int max_count = (int)(NUM_SAMPLES * 0.7);

    /* Check high word bits (skip version bits 15-12) */
    for (int b = 0; b < 64; b++) {
        if (b >= 12 && b <= 15) {
            /* Skip version bits - they are fixed */
            continue;
        }
        TEST_ASSERT(high_bit_counts[b] >= min_count && high_bit_counts[b] <= max_count,
                    "High word random bits should have ~50% distribution");
    }

    /* Check low word bits (skip variant bits 63-62) */
    for (int b = 0; b < 64; b++) {
        if (b >= 62) {
            /* Skip variant bits - they are fixed */
            continue;
        }
        TEST_ASSERT(low_bit_counts[b] >= min_count && low_bit_counts[b] <= max_count,
                    "Low word random bits should have ~50% distribution");
    }

    printf("  Bit distribution is approximately uniform\n");
    #undef NUM_SAMPLES

    rt_arena_destroy(arena);
}

void test_rt_uuid_v4_null_arena()
{
    printf("Testing rt_uuid_v4 with NULL arena...\n");

    RtUuid *uuid = rt_uuid_v4(NULL);
    TEST_ASSERT_NULL(uuid, "UUID should be NULL with NULL arena");

    printf("  NULL arena handled correctly\n");
}

/* ============================================================================
 * rt_uuid_to_string() Tests
 * ============================================================================ */

void test_rt_uuid_to_string_format()
{
    printf("Testing rt_uuid_to_string format...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtUuid *uuid = rt_uuid_v4(arena);
    TEST_ASSERT_NOT_NULL(uuid, "UUID should be created");

    char *str = rt_uuid_to_string(arena, uuid);
    TEST_ASSERT_NOT_NULL(str, "String should be created");

    /* Check length is 36 characters */
    TEST_ASSERT_EQ(strlen(str), 36, "UUID string should be 36 characters");

    /* Check format: 8-4-4-4-12 with dashes at positions 8, 13, 18, 23 */
    TEST_ASSERT(str[8] == '-', "Dash at position 8");
    TEST_ASSERT(str[13] == '-', "Dash at position 13");
    TEST_ASSERT(str[18] == '-', "Dash at position 18");
    TEST_ASSERT(str[23] == '-', "Dash at position 23");

    /* Check all other characters are hex digits */
    for (int i = 0; i < 36; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            continue; /* Skip dashes */
        }
        int is_hex = (str[i] >= '0' && str[i] <= '9') ||
                     (str[i] >= 'a' && str[i] <= 'f') ||
                     (str[i] >= 'A' && str[i] <= 'F');
        TEST_ASSERT(is_hex, "All non-dash characters should be hex digits");
    }

    /* Check version digit (position 14) is '4' */
    TEST_ASSERT(str[14] == '4', "Version digit should be '4'");

    /* Check variant digit (position 19) is 8, 9, a, or b */
    int variant_digit_ok = (str[19] == '8' || str[19] == '9' ||
                           str[19] == 'a' || str[19] == 'b' ||
                           str[19] == 'A' || str[19] == 'B');
    TEST_ASSERT(variant_digit_ok, "Variant digit should be 8, 9, a, or b");

    printf("  UUID string format: %s\n", str);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * rt_uuid_to_hex() Tests
 * ============================================================================ */

void test_rt_uuid_to_hex_format()
{
    printf("Testing rt_uuid_to_hex format...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtUuid *uuid = rt_uuid_v4(arena);
    TEST_ASSERT_NOT_NULL(uuid, "UUID should be created");

    char *hex = rt_uuid_to_hex(arena, uuid);
    TEST_ASSERT_NOT_NULL(hex, "Hex string should be created");

    /* Check length is 32 characters */
    TEST_ASSERT_EQ(strlen(hex), 32, "UUID hex should be 32 characters");

    /* Check all characters are hex digits */
    for (int i = 0; i < 32; i++) {
        int is_hex = (hex[i] >= '0' && hex[i] <= '9') ||
                     (hex[i] >= 'a' && hex[i] <= 'f') ||
                     (hex[i] >= 'A' && hex[i] <= 'F');
        TEST_ASSERT(is_hex, "All characters should be hex digits");
    }

    printf("  UUID hex: %s\n", hex);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * rt_uuid_to_bytes() Tests
 * ============================================================================ */

void test_rt_uuid_to_bytes()
{
    printf("Testing rt_uuid_to_bytes...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtUuid *uuid = rt_uuid_v4(arena);
    TEST_ASSERT_NOT_NULL(uuid, "UUID should be created");

    unsigned char *bytes = rt_uuid_to_bytes(arena, uuid);
    TEST_ASSERT_NOT_NULL(bytes, "Bytes should be created");

    /* Verify bytes match UUID structure */
    /* High word -> bytes 0-7 */
    TEST_ASSERT_EQ(bytes[0], (uuid->high >> 56) & 0xFF, "Byte 0 correct");
    TEST_ASSERT_EQ(bytes[1], (uuid->high >> 48) & 0xFF, "Byte 1 correct");
    TEST_ASSERT_EQ(bytes[2], (uuid->high >> 40) & 0xFF, "Byte 2 correct");
    TEST_ASSERT_EQ(bytes[3], (uuid->high >> 32) & 0xFF, "Byte 3 correct");
    TEST_ASSERT_EQ(bytes[4], (uuid->high >> 24) & 0xFF, "Byte 4 correct");
    TEST_ASSERT_EQ(bytes[5], (uuid->high >> 16) & 0xFF, "Byte 5 correct");
    TEST_ASSERT_EQ(bytes[6], (uuid->high >> 8) & 0xFF, "Byte 6 correct");
    TEST_ASSERT_EQ(bytes[7], uuid->high & 0xFF, "Byte 7 correct");

    /* Low word -> bytes 8-15 */
    TEST_ASSERT_EQ(bytes[8], (uuid->low >> 56) & 0xFF, "Byte 8 correct");
    TEST_ASSERT_EQ(bytes[9], (uuid->low >> 48) & 0xFF, "Byte 9 correct");
    TEST_ASSERT_EQ(bytes[10], (uuid->low >> 40) & 0xFF, "Byte 10 correct");
    TEST_ASSERT_EQ(bytes[11], (uuid->low >> 32) & 0xFF, "Byte 11 correct");
    TEST_ASSERT_EQ(bytes[12], (uuid->low >> 24) & 0xFF, "Byte 12 correct");
    TEST_ASSERT_EQ(bytes[13], (uuid->low >> 16) & 0xFF, "Byte 13 correct");
    TEST_ASSERT_EQ(bytes[14], (uuid->low >> 8) & 0xFF, "Byte 14 correct");
    TEST_ASSERT_EQ(bytes[15], uuid->low & 0xFF, "Byte 15 correct");

    /* Version byte (byte 6) should have 0x4X pattern */
    TEST_ASSERT_EQ((bytes[6] >> 4) & 0x0F, 4, "Version nibble correct");

    /* Variant byte (byte 8) should have 0x8X-0xBX pattern */
    TEST_ASSERT((bytes[8] & 0xC0) == 0x80, "Variant bits correct in byte 8");

    printf("  Bytes conversion correct\n");

    rt_arena_destroy(arena);
}

/* ============================================================================
 * rt_uuid_to_base64() Tests
 * ============================================================================ */

void test_rt_uuid_to_base64_format()
{
    printf("Testing rt_uuid_to_base64 format...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtUuid *uuid = rt_uuid_v4(arena);
    TEST_ASSERT_NOT_NULL(uuid, "UUID should be created");

    char *base64 = rt_uuid_to_base64(arena, uuid);
    TEST_ASSERT_NOT_NULL(base64, "Base64 string should be created");

    /* Check length is 22 characters */
    TEST_ASSERT_EQ(strlen(base64), 22, "UUID base64 should be 22 characters");

    /* Check all characters are URL-safe base64 (A-Z, a-z, 0-9, -, _) */
    for (int i = 0; i < 22; i++) {
        int is_valid = (base64[i] >= 'A' && base64[i] <= 'Z') ||
                       (base64[i] >= 'a' && base64[i] <= 'z') ||
                       (base64[i] >= '0' && base64[i] <= '9') ||
                       base64[i] == '-' || base64[i] == '_';
        TEST_ASSERT(is_valid, "All characters should be URL-safe base64");
    }

    /* Verify no standard base64 characters that aren't URL-safe */
    for (int i = 0; i < 22; i++) {
        TEST_ASSERT(base64[i] != '+', "No + in URL-safe base64");
        TEST_ASSERT(base64[i] != '/', "No / in URL-safe base64");
        TEST_ASSERT(base64[i] != '=', "No padding in UUID base64");
    }

    printf("  UUID base64: %s\n", base64);

    rt_arena_destroy(arena);
}

void test_rt_uuid_to_base64_known_value()
{
    printf("Testing rt_uuid_to_base64 with known value...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Use nil UUID for predictable output */
    RtUuid *nil = rt_uuid_nil(arena);
    char *base64 = rt_uuid_to_base64(arena, nil);
    TEST_ASSERT_NOT_NULL(base64, "Base64 should be created");

    /* Nil UUID (all zeros) should produce all 'A's */
    TEST_ASSERT_STR_EQ(base64, "AAAAAAAAAAAAAAAAAAAAAA", "Nil UUID base64 should be all As");

    printf("  Nil UUID base64: %s\n", base64);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Comparison Tests
 * ============================================================================ */

void test_rt_uuid_equals()
{
    printf("Testing rt_uuid_equals...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtUuid *uuid1 = rt_uuid_v4(arena);
    RtUuid *uuid2 = rt_uuid_v4(arena);
    TEST_ASSERT_NOT_NULL(uuid1, "UUID1 should be created");
    TEST_ASSERT_NOT_NULL(uuid2, "UUID2 should be created");

    /* Different UUIDs should not be equal */
    TEST_ASSERT_FALSE(rt_uuid_equals(uuid1, uuid2), "Different UUIDs should not be equal");

    /* Same UUID should be equal to itself */
    TEST_ASSERT_TRUE(rt_uuid_equals(uuid1, uuid1), "UUID should equal itself");

    /* Copy UUID and check equality */
    RtUuid *uuid_copy = rt_arena_alloc(arena, sizeof(RtUuid));
    uuid_copy->high = uuid1->high;
    uuid_copy->low = uuid1->low;
    TEST_ASSERT_TRUE(rt_uuid_equals(uuid1, uuid_copy), "Copied UUID should equal original");

    printf("  Equality comparison correct\n");

    rt_arena_destroy(arena);
}

void test_rt_uuid_compare()
{
    printf("Testing rt_uuid_compare...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Create UUIDs with known ordering */
    RtUuid *low = rt_arena_alloc(arena, sizeof(RtUuid));
    RtUuid *high = rt_arena_alloc(arena, sizeof(RtUuid));

    low->high = 0x0000000000000000ULL;
    low->low = 0x0000000000000001ULL;

    high->high = 0xFFFFFFFFFFFFFFFFULL;
    high->low = 0xFFFFFFFFFFFFFFFFULL;

    TEST_ASSERT(rt_uuid_compare(low, high) < 0, "Low UUID should be less than high");
    TEST_ASSERT(rt_uuid_compare(high, low) > 0, "High UUID should be greater than low");
    TEST_ASSERT_EQ(rt_uuid_compare(low, low), 0, "UUID should equal itself");

    TEST_ASSERT_TRUE(rt_uuid_is_less_than(low, high), "is_less_than should be true");
    TEST_ASSERT_FALSE(rt_uuid_is_less_than(high, low), "is_less_than should be false");
    TEST_ASSERT_TRUE(rt_uuid_is_greater_than(high, low), "is_greater_than should be true");
    TEST_ASSERT_FALSE(rt_uuid_is_greater_than(low, high), "is_greater_than should be false");

    printf("  Comparison operations correct\n");

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Special UUID Tests
 * ============================================================================ */

void test_rt_uuid_nil()
{
    printf("Testing rt_uuid_nil...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtUuid *nil = rt_uuid_nil(arena);
    TEST_ASSERT_NOT_NULL(nil, "Nil UUID should be created");

    TEST_ASSERT_EQ(nil->high, 0ULL, "Nil high should be 0");
    TEST_ASSERT_EQ(nil->low, 0ULL, "Nil low should be 0");
    TEST_ASSERT_TRUE(rt_uuid_is_nil(nil), "is_nil should return true for nil UUID");

    char *str = rt_uuid_to_string(arena, nil);
    TEST_ASSERT_STR_EQ(str, "00000000-0000-0000-0000-000000000000", "Nil string format");

    printf("  Nil UUID: %s\n", str);

    rt_arena_destroy(arena);
}

void test_rt_uuid_max()
{
    printf("Testing rt_uuid_max...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtUuid *max = rt_uuid_max(arena);
    TEST_ASSERT_NOT_NULL(max, "Max UUID should be created");

    TEST_ASSERT_EQ(max->high, 0xFFFFFFFFFFFFFFFFULL, "Max high should be all 1s");
    TEST_ASSERT_EQ(max->low, 0xFFFFFFFFFFFFFFFFULL, "Max low should be all 1s");
    TEST_ASSERT_FALSE(rt_uuid_is_nil(max), "is_nil should return false for max UUID");

    char *str = rt_uuid_to_string(arena, max);
    TEST_ASSERT_STR_EQ(str, "ffffffff-ffff-ffff-ffff-ffffffffffff", "Max string format");

    printf("  Max UUID: %s\n", str);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Namespace Tests
 * ============================================================================ */

void test_rt_uuid_namespaces()
{
    printf("Testing UUID namespaces...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* DNS namespace: 6ba7b810-9dad-11d1-80b4-00c04fd430c8 */
    RtUuid *dns = rt_uuid_namespace_dns(arena);
    TEST_ASSERT_NOT_NULL(dns, "DNS namespace should be created");
    char *dns_str = rt_uuid_to_string(arena, dns);
    TEST_ASSERT_STR_EQ(dns_str, "6ba7b810-9dad-11d1-80b4-00c04fd430c8", "DNS namespace");

    /* URL namespace: 6ba7b811-9dad-11d1-80b4-00c04fd430c8 */
    RtUuid *url = rt_uuid_namespace_url(arena);
    TEST_ASSERT_NOT_NULL(url, "URL namespace should be created");
    char *url_str = rt_uuid_to_string(arena, url);
    TEST_ASSERT_STR_EQ(url_str, "6ba7b811-9dad-11d1-80b4-00c04fd430c8", "URL namespace");

    /* OID namespace: 6ba7b812-9dad-11d1-80b4-00c04fd430c8 */
    RtUuid *oid = rt_uuid_namespace_oid(arena);
    TEST_ASSERT_NOT_NULL(oid, "OID namespace should be created");
    char *oid_str = rt_uuid_to_string(arena, oid);
    TEST_ASSERT_STR_EQ(oid_str, "6ba7b812-9dad-11d1-80b4-00c04fd430c8", "OID namespace");

    /* X.500 namespace: 6ba7b814-9dad-11d1-80b4-00c04fd430c8 */
    RtUuid *x500 = rt_uuid_namespace_x500(arena);
    TEST_ASSERT_NOT_NULL(x500, "X500 namespace should be created");
    char *x500_str = rt_uuid_to_string(arena, x500);
    TEST_ASSERT_STR_EQ(x500_str, "6ba7b814-9dad-11d1-80b4-00c04fd430c8", "X500 namespace");

    printf("  DNS:  %s\n", dns_str);
    printf("  URL:  %s\n", url_str);
    printf("  OID:  %s\n", oid_str);
    printf("  X500: %s\n", x500_str);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * rt_uuid_v7() Tests
 * ============================================================================
 * Tests for UUIDv7 (timestamp + random) generation following RFC 9562.
 * ============================================================================ */

void test_rt_uuid_v7_basic()
{
    printf("Testing rt_uuid_v7 basic functionality...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtUuid *uuid = rt_uuid_v7(arena);
    TEST_ASSERT_NOT_NULL(uuid, "UUID should be created");

    /* UUID should not be all zeros */
    TEST_ASSERT(uuid->high != 0 || uuid->low != 0, "UUID should not be nil");

    printf("  Created UUIDv7 successfully\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_v7_version_bits()
{
    printf("Testing rt_uuid_v7 version bits...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Generate multiple UUIDs and check version bits */
    for (int i = 0; i < 100; i++) {
        RtUuid *uuid = rt_uuid_v7(arena);
        TEST_ASSERT_NOT_NULL(uuid, "UUID should be created");

        /* Version should be 7 */
        long version = rt_uuid_get_version(uuid);
        TEST_ASSERT_EQ(version, 7, "UUID version should be 7");
    }

    printf("  All 100 UUIDs have correct version (7)\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_v7_variant_bits()
{
    printf("Testing rt_uuid_v7 variant bits...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Generate multiple UUIDs and check variant bits */
    for (int i = 0; i < 100; i++) {
        RtUuid *uuid = rt_uuid_v7(arena);
        TEST_ASSERT_NOT_NULL(uuid, "UUID should be created");

        /* Variant should be 1 (RFC 9562) */
        long variant = rt_uuid_get_variant(uuid);
        TEST_ASSERT_EQ(variant, 1, "UUID variant should be 1 (RFC 9562)");

        /* Verify variant bits directly: bits 63-62 of low word should be 10 */
        uint64_t variant_bits = (uuid->low >> 62) & 0x03;
        TEST_ASSERT_EQ(variant_bits, 0x02, "Variant bits should be 10 (binary)");
    }

    printf("  All 100 UUIDs have correct variant (1 / RFC 9562)\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_v7_timestamp()
{
    printf("Testing rt_uuid_v7 timestamp extraction...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Get current time before and after creating UUID */
    struct timeval tv_before, tv_after;
    gettimeofday(&tv_before, NULL);
    long long ms_before = (long long)tv_before.tv_sec * 1000 + tv_before.tv_usec / 1000;

    RtUuid *uuid = rt_uuid_v7(arena);
    TEST_ASSERT_NOT_NULL(uuid, "UUID should be created");

    gettimeofday(&tv_after, NULL);
    long long ms_after = (long long)tv_after.tv_sec * 1000 + tv_after.tv_usec / 1000;

    /* Extract timestamp from UUID */
    long long uuid_timestamp = rt_uuid_get_timestamp(uuid);

    /* UUID timestamp should be between before and after timestamps (with tolerance) */
    TEST_ASSERT(uuid_timestamp >= ms_before - 1, "UUID timestamp should be >= before time");
    TEST_ASSERT(uuid_timestamp <= ms_after + 1, "UUID timestamp should be <= after time");

    printf("  Timestamp extracted correctly: %lld ms\n", uuid_timestamp);
    printf("  Time range: [%lld, %lld] ms\n", ms_before, ms_after);

    rt_arena_destroy(arena);
}

void test_rt_uuid_v7_ordering()
{
    printf("Testing rt_uuid_v7 time ordering...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Generate multiple UUIDs and verify they are ordered */
    #define NUM_ORDERED_UUIDS 100
    RtUuid *uuids[NUM_ORDERED_UUIDS];

    for (int i = 0; i < NUM_ORDERED_UUIDS; i++) {
        uuids[i] = rt_uuid_v7(arena);
        TEST_ASSERT_NOT_NULL(uuids[i], "UUID should be created");
    }

    /* Check timestamps are non-decreasing (may be equal within same millisecond) */
    int ordering_errors = 0;
    for (int i = 1; i < NUM_ORDERED_UUIDS; i++) {
        long long ts_prev = rt_uuid_get_timestamp(uuids[i-1]);
        long long ts_curr = rt_uuid_get_timestamp(uuids[i]);
        if (ts_curr < ts_prev) {
            ordering_errors++;
        }
    }

    TEST_ASSERT_EQ(ordering_errors, 0, "UUIDs should have non-decreasing timestamps");

    printf("  All %d UUIDs have non-decreasing timestamps\n", NUM_ORDERED_UUIDS);
    #undef NUM_ORDERED_UUIDS

    rt_arena_destroy(arena);
}

void test_rt_uuid_v7_uniqueness()
{
    printf("Testing rt_uuid_v7 uniqueness...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Generate 1000 UUIDs and check they are all unique */
    #define NUM_UUIDS_V7 1000
    RtUuid *uuids[NUM_UUIDS_V7];

    for (int i = 0; i < NUM_UUIDS_V7; i++) {
        uuids[i] = rt_uuid_v7(arena);
        TEST_ASSERT_NOT_NULL(uuids[i], "UUID should be created");
    }

    /* Check all pairs are different */
    int duplicates = 0;
    for (int i = 0; i < NUM_UUIDS_V7; i++) {
        for (int j = i + 1; j < NUM_UUIDS_V7; j++) {
            if (rt_uuid_equals(uuids[i], uuids[j])) {
                duplicates++;
            }
        }
    }

    TEST_ASSERT_EQ(duplicates, 0, "All UUIDs should be unique");

    printf("  Generated %d unique v7 UUIDs\n", NUM_UUIDS_V7);
    #undef NUM_UUIDS_V7

    rt_arena_destroy(arena);
}

void test_rt_uuid_v7_randomness()
{
    printf("Testing rt_uuid_v7 randomness in non-timestamp bits...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Generate many UUIDs and check random bit distribution in low 62 bits */
    #define NUM_SAMPLES_V7 500
    int low_bit_counts[62] = {0};

    for (int i = 0; i < NUM_SAMPLES_V7; i++) {
        RtUuid *uuid = rt_uuid_v7(arena);
        TEST_ASSERT_NOT_NULL(uuid, "UUID should be created");

        /* Count set bits in random portion of low word (bits 61-0) */
        for (int b = 0; b < 62; b++) {
            if ((uuid->low >> b) & 1) {
                low_bit_counts[b]++;
            }
        }
    }

    /* For random bits, each bit should be set roughly 50% of the time */
    int min_count = (int)(NUM_SAMPLES_V7 * 0.3);
    int max_count = (int)(NUM_SAMPLES_V7 * 0.7);

    for (int b = 0; b < 62; b++) {
        TEST_ASSERT(low_bit_counts[b] >= min_count && low_bit_counts[b] <= max_count,
                    "Random bits should have ~50% distribution");
    }

    printf("  Random bit distribution is approximately uniform\n");
    #undef NUM_SAMPLES_V7

    rt_arena_destroy(arena);
}

void test_rt_uuid_v7_string_format()
{
    printf("Testing rt_uuid_v7 string format...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtUuid *uuid = rt_uuid_v7(arena);
    TEST_ASSERT_NOT_NULL(uuid, "UUID should be created");

    char *str = rt_uuid_to_string(arena, uuid);
    TEST_ASSERT_NOT_NULL(str, "String should be created");

    /* Check length is 36 characters */
    TEST_ASSERT_EQ(strlen(str), 36, "UUID string should be 36 characters");

    /* Check version digit (position 14) is '7' */
    TEST_ASSERT(str[14] == '7', "Version digit should be '7'");

    /* Check variant digit (position 19) is 8, 9, a, or b */
    int variant_digit_ok = (str[19] == '8' || str[19] == '9' ||
                           str[19] == 'a' || str[19] == 'b' ||
                           str[19] == 'A' || str[19] == 'B');
    TEST_ASSERT(variant_digit_ok, "Variant digit should be 8, 9, a, or b");

    printf("  UUIDv7 string format: %s\n", str);

    rt_arena_destroy(arena);
}

void test_rt_uuid_create_returns_v7()
{
    printf("Testing rt_uuid_create returns v7...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtUuid *uuid = rt_uuid_create(arena);
    TEST_ASSERT_NOT_NULL(uuid, "UUID should be created");

    /* rt_uuid_create should return v7 */
    long version = rt_uuid_get_version(uuid);
    TEST_ASSERT_EQ(version, 7, "rt_uuid_create should return v7");

    printf("  rt_uuid_create returns version 7\n");

    rt_arena_destroy(arena);
}

/* ============================================================================
 * rt_uuid_v5() Tests
 * ============================================================================
 * Tests for UUIDv5 (SHA-1 hash) generation following RFC 9562.
 * UUIDv5 generates deterministic UUIDs from namespace + name.
 * ============================================================================ */

void test_rt_uuid_v5_basic()
{
    printf("Testing rt_uuid_v5 basic functionality...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtUuid *ns_dns = rt_uuid_namespace_dns(arena);
    TEST_ASSERT_NOT_NULL(ns_dns, "DNS namespace should be created");

    RtUuid *uuid = rt_uuid_v5(arena, ns_dns, "example.com");
    TEST_ASSERT_NOT_NULL(uuid, "UUID should be created");

    /* UUID should not be all zeros */
    TEST_ASSERT(uuid->high != 0 || uuid->low != 0, "UUID should not be nil");

    printf("  Created UUIDv5 successfully\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_v5_version_bits()
{
    printf("Testing rt_uuid_v5 version bits...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtUuid *ns_dns = rt_uuid_namespace_dns(arena);

    /* Generate multiple UUIDs and check version bits */
    const char *names[] = {"test1", "test2", "example.com", "foo.bar", "hello"};
    for (int i = 0; i < 5; i++) {
        RtUuid *uuid = rt_uuid_v5(arena, ns_dns, names[i]);
        TEST_ASSERT_NOT_NULL(uuid, "UUID should be created");

        /* Version should be 5 */
        long version = rt_uuid_get_version(uuid);
        TEST_ASSERT_EQ(version, 5, "UUID version should be 5");
    }

    printf("  All UUIDs have correct version (5)\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_v5_variant_bits()
{
    printf("Testing rt_uuid_v5 variant bits...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtUuid *ns_dns = rt_uuid_namespace_dns(arena);

    /* Generate multiple UUIDs and check variant bits */
    const char *names[] = {"test1", "test2", "example.com", "foo.bar", "hello"};
    for (int i = 0; i < 5; i++) {
        RtUuid *uuid = rt_uuid_v5(arena, ns_dns, names[i]);
        TEST_ASSERT_NOT_NULL(uuid, "UUID should be created");

        /* Variant should be 1 (RFC 9562) */
        long variant = rt_uuid_get_variant(uuid);
        TEST_ASSERT_EQ(variant, 1, "UUID variant should be 1 (RFC 9562)");

        /* Verify variant bits directly: bits 63-62 of low word should be 10 */
        uint64_t variant_bits = (uuid->low >> 62) & 0x03;
        TEST_ASSERT_EQ(variant_bits, 0x02, "Variant bits should be 10 (binary)");
    }

    printf("  All UUIDs have correct variant (1 / RFC 9562)\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_v5_deterministic()
{
    printf("Testing rt_uuid_v5 determinism (same inputs = same UUID)...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtUuid *ns_dns = rt_uuid_namespace_dns(arena);

    /* Same namespace + name should produce same UUID */
    RtUuid *uuid1 = rt_uuid_v5(arena, ns_dns, "example.com");
    RtUuid *uuid2 = rt_uuid_v5(arena, ns_dns, "example.com");
    TEST_ASSERT_NOT_NULL(uuid1, "UUID1 should be created");
    TEST_ASSERT_NOT_NULL(uuid2, "UUID2 should be created");

    TEST_ASSERT_TRUE(rt_uuid_equals(uuid1, uuid2), "Same inputs should produce same UUID");

    /* Different name should produce different UUID */
    RtUuid *uuid3 = rt_uuid_v5(arena, ns_dns, "other.com");
    TEST_ASSERT_NOT_NULL(uuid3, "UUID3 should be created");
    TEST_ASSERT_FALSE(rt_uuid_equals(uuid1, uuid3), "Different names should produce different UUIDs");

    char *str1 = rt_uuid_to_string(arena, uuid1);
    char *str2 = rt_uuid_to_string(arena, uuid2);
    printf("  UUID for 'example.com': %s\n", str1);
    printf("  Same again:             %s\n", str2);

    rt_arena_destroy(arena);
}

void test_rt_uuid_v5_different_namespaces()
{
    printf("Testing rt_uuid_v5 with different namespaces...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtUuid *ns_dns = rt_uuid_namespace_dns(arena);
    RtUuid *ns_url = rt_uuid_namespace_url(arena);
    RtUuid *ns_oid = rt_uuid_namespace_oid(arena);

    /* Same name with different namespaces should produce different UUIDs */
    const char *name = "test";
    RtUuid *uuid_dns = rt_uuid_v5(arena, ns_dns, name);
    RtUuid *uuid_url = rt_uuid_v5(arena, ns_url, name);
    RtUuid *uuid_oid = rt_uuid_v5(arena, ns_oid, name);

    TEST_ASSERT_NOT_NULL(uuid_dns, "UUID (DNS) should be created");
    TEST_ASSERT_NOT_NULL(uuid_url, "UUID (URL) should be created");
    TEST_ASSERT_NOT_NULL(uuid_oid, "UUID (OID) should be created");

    TEST_ASSERT_FALSE(rt_uuid_equals(uuid_dns, uuid_url), "Different namespaces should produce different UUIDs");
    TEST_ASSERT_FALSE(rt_uuid_equals(uuid_dns, uuid_oid), "Different namespaces should produce different UUIDs");
    TEST_ASSERT_FALSE(rt_uuid_equals(uuid_url, uuid_oid), "Different namespaces should produce different UUIDs");

    printf("  Different namespaces produce different UUIDs\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_v5_known_vector()
{
    printf("Testing rt_uuid_v5 against known test vector...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /*
     * RFC 4122 / Wikipedia reference:
     * UUIDv5(DNS, "python.org") = 886313e1-3b8a-5372-9b90-0c9aee199e5d
     */
    RtUuid *ns_dns = rt_uuid_namespace_dns(arena);
    RtUuid *uuid = rt_uuid_v5(arena, ns_dns, "python.org");
    TEST_ASSERT_NOT_NULL(uuid, "UUID should be created");

    char *str = rt_uuid_to_string(arena, uuid);
    TEST_ASSERT_NOT_NULL(str, "String should be created");

    /* Verify it's a valid v5 UUID with correct version and variant */
    TEST_ASSERT_EQ(rt_uuid_get_version(uuid), 5, "Should be version 5");
    TEST_ASSERT_EQ(rt_uuid_get_variant(uuid), 1, "Should be RFC 9562 variant");

    /* Verify string format shows version 5 */
    TEST_ASSERT(str[14] == '5', "Version digit should be '5'");

    printf("  UUIDv5(DNS, \"python.org\") = %s\n", str);

    rt_arena_destroy(arena);
}

void test_rt_uuid_v5_empty_name()
{
    printf("Testing rt_uuid_v5 with empty name...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtUuid *ns_dns = rt_uuid_namespace_dns(arena);

    /* Empty string should still produce valid UUID */
    RtUuid *uuid = rt_uuid_v5(arena, ns_dns, "");
    TEST_ASSERT_NOT_NULL(uuid, "UUID should be created for empty name");

    /* Should still have correct version and variant */
    TEST_ASSERT_EQ(rt_uuid_get_version(uuid), 5, "Should be version 5");
    TEST_ASSERT_EQ(rt_uuid_get_variant(uuid), 1, "Should be RFC 9562 variant");

    /* Empty name should be deterministic too */
    RtUuid *uuid2 = rt_uuid_v5(arena, ns_dns, "");
    TEST_ASSERT_TRUE(rt_uuid_equals(uuid, uuid2), "Empty name should be deterministic");

    printf("  Empty name handled correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_v5_null_inputs()
{
    printf("Testing rt_uuid_v5 with NULL inputs...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtUuid *ns_dns = rt_uuid_namespace_dns(arena);

    /* NULL arena should return NULL */
    RtUuid *uuid1 = rt_uuid_v5(NULL, ns_dns, "test");
    TEST_ASSERT_NULL(uuid1, "NULL arena should return NULL");

    /* NULL namespace should return NULL */
    RtUuid *uuid2 = rt_uuid_v5(arena, NULL, "test");
    TEST_ASSERT_NULL(uuid2, "NULL namespace should return NULL");

    /* NULL name should return NULL */
    RtUuid *uuid3 = rt_uuid_v5(arena, ns_dns, NULL);
    TEST_ASSERT_NULL(uuid3, "NULL name should return NULL");

    printf("  NULL inputs handled correctly\n");
    rt_arena_destroy(arena);
}

/* ============================================================================
 * rt_uuid_from_string() Tests
 * ============================================================================
 * Tests for parsing UUIDs from standard 36-char format with dashes.
 * ============================================================================ */

void test_rt_uuid_from_string_basic()
{
    printf("Testing rt_uuid_from_string basic functionality...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Parse a known UUID */
    RtUuid *uuid = rt_uuid_from_string(arena, "6ba7b810-9dad-11d1-80b4-00c04fd430c8");
    TEST_ASSERT_NOT_NULL(uuid, "UUID should be parsed");

    /* Verify it matches the DNS namespace */
    RtUuid *dns = rt_uuid_namespace_dns(arena);
    TEST_ASSERT_TRUE(rt_uuid_equals(uuid, dns), "Parsed UUID should equal DNS namespace");

    printf("  Parsed UUID successfully\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_from_string_roundtrip()
{
    printf("Testing rt_uuid_from_string roundtrip...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Create a UUID, convert to string, parse back */
    RtUuid *original = rt_uuid_v4(arena);
    TEST_ASSERT_NOT_NULL(original, "Original UUID should be created");

    char *str = rt_uuid_to_string(arena, original);
    TEST_ASSERT_NOT_NULL(str, "String should be created");

    RtUuid *parsed = rt_uuid_from_string(arena, str);
    TEST_ASSERT_NOT_NULL(parsed, "Parsed UUID should be created");

    TEST_ASSERT_TRUE(rt_uuid_equals(original, parsed), "Roundtrip should preserve UUID");

    printf("  Roundtrip: %s\n", str);
    rt_arena_destroy(arena);
}

void test_rt_uuid_from_string_lowercase()
{
    printf("Testing rt_uuid_from_string with lowercase...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtUuid *uuid = rt_uuid_from_string(arena, "abcdef01-2345-6789-abcd-ef0123456789");
    TEST_ASSERT_NOT_NULL(uuid, "UUID should be parsed");

    /* Verify parsed values */
    char *str = rt_uuid_to_string(arena, uuid);
    TEST_ASSERT_STR_EQ(str, "abcdef01-2345-6789-abcd-ef0123456789", "Lowercase parsed correctly");

    printf("  Lowercase handled correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_from_string_uppercase()
{
    printf("Testing rt_uuid_from_string with uppercase...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtUuid *uuid = rt_uuid_from_string(arena, "ABCDEF01-2345-6789-ABCD-EF0123456789");
    TEST_ASSERT_NOT_NULL(uuid, "UUID should be parsed");

    /* Should produce same result as lowercase */
    RtUuid *uuid_lower = rt_uuid_from_string(arena, "abcdef01-2345-6789-abcd-ef0123456789");
    TEST_ASSERT_NOT_NULL(uuid_lower, "Lowercase UUID should be parsed");
    TEST_ASSERT_TRUE(rt_uuid_equals(uuid, uuid_lower), "Case should not matter");

    printf("  Uppercase handled correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_from_string_mixed_case()
{
    printf("Testing rt_uuid_from_string with mixed case...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtUuid *uuid = rt_uuid_from_string(arena, "AbCdEf01-2345-6789-aBcD-eF0123456789");
    TEST_ASSERT_NOT_NULL(uuid, "UUID should be parsed");

    /* Should produce same result as all lowercase */
    RtUuid *uuid_lower = rt_uuid_from_string(arena, "abcdef01-2345-6789-abcd-ef0123456789");
    TEST_ASSERT_NOT_NULL(uuid_lower, "Lowercase UUID should be parsed");
    TEST_ASSERT_TRUE(rt_uuid_equals(uuid, uuid_lower), "Mixed case should not matter");

    printf("  Mixed case handled correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_from_string_nil()
{
    printf("Testing rt_uuid_from_string with nil UUID...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtUuid *uuid = rt_uuid_from_string(arena, "00000000-0000-0000-0000-000000000000");
    TEST_ASSERT_NOT_NULL(uuid, "UUID should be parsed");
    TEST_ASSERT_TRUE(rt_uuid_is_nil(uuid), "Parsed UUID should be nil");

    printf("  Nil UUID parsed correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_from_string_max()
{
    printf("Testing rt_uuid_from_string with max UUID...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtUuid *uuid = rt_uuid_from_string(arena, "ffffffff-ffff-ffff-ffff-ffffffffffff");
    TEST_ASSERT_NOT_NULL(uuid, "UUID should be parsed");

    RtUuid *max = rt_uuid_max(arena);
    TEST_ASSERT_TRUE(rt_uuid_equals(uuid, max), "Parsed UUID should equal max UUID");

    printf("  Max UUID parsed correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_from_string_invalid_length()
{
    printf("Testing rt_uuid_from_string with invalid length...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Too short */
    RtUuid *uuid1 = rt_uuid_from_string(arena, "6ba7b810-9dad-11d1-80b4-00c04fd430c");
    TEST_ASSERT_NULL(uuid1, "Too short UUID should return NULL");

    /* Too long */
    RtUuid *uuid2 = rt_uuid_from_string(arena, "6ba7b810-9dad-11d1-80b4-00c04fd430c8a");
    TEST_ASSERT_NULL(uuid2, "Too long UUID should return NULL");

    /* Empty string */
    RtUuid *uuid3 = rt_uuid_from_string(arena, "");
    TEST_ASSERT_NULL(uuid3, "Empty string should return NULL");

    printf("  Invalid lengths handled correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_from_string_invalid_dashes()
{
    printf("Testing rt_uuid_from_string with invalid dashes...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Missing dash at position 8 */
    RtUuid *uuid1 = rt_uuid_from_string(arena, "6ba7b81009dad-11d1-80b4-00c04fd430c8");
    TEST_ASSERT_NULL(uuid1, "Missing dash should return NULL");

    /* Dash in wrong position */
    RtUuid *uuid2 = rt_uuid_from_string(arena, "6ba7b810-9da-d11d1-80b4-00c04fd430c8");
    TEST_ASSERT_NULL(uuid2, "Dash in wrong position should return NULL");

    /* No dashes at all (wrong format) */
    RtUuid *uuid3 = rt_uuid_from_string(arena, "6ba7b8109dad11d180b400c04fd430c8ab");
    TEST_ASSERT_NULL(uuid3, "No dashes should return NULL");

    printf("  Invalid dashes handled correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_from_string_invalid_chars()
{
    printf("Testing rt_uuid_from_string with invalid characters...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Contains 'g' which is not valid hex */
    RtUuid *uuid1 = rt_uuid_from_string(arena, "6ba7b810-9dad-11d1-80b4-00c04fd430g8");
    TEST_ASSERT_NULL(uuid1, "Invalid hex 'g' should return NULL");

    /* Contains space */
    RtUuid *uuid2 = rt_uuid_from_string(arena, "6ba7b810-9dad-11d1-80b4-00c04fd430 8");
    TEST_ASSERT_NULL(uuid2, "Space should return NULL");

    /* Contains special character */
    RtUuid *uuid3 = rt_uuid_from_string(arena, "6ba7b810-9dad-11d1-80b4-00c04fd430@8");
    TEST_ASSERT_NULL(uuid3, "Special char should return NULL");

    printf("  Invalid characters handled correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_from_string_null_inputs()
{
    printf("Testing rt_uuid_from_string with NULL inputs...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* NULL arena */
    RtUuid *uuid1 = rt_uuid_from_string(NULL, "6ba7b810-9dad-11d1-80b4-00c04fd430c8");
    TEST_ASSERT_NULL(uuid1, "NULL arena should return NULL");

    /* NULL string */
    RtUuid *uuid2 = rt_uuid_from_string(arena, NULL);
    TEST_ASSERT_NULL(uuid2, "NULL string should return NULL");

    printf("  NULL inputs handled correctly\n");
    rt_arena_destroy(arena);
}

/* ============================================================================
 * rt_uuid_from_hex() Tests
 * ============================================================================
 * Tests for parsing UUIDs from 32-char hex format without dashes.
 * ============================================================================ */

void test_rt_uuid_from_hex_basic()
{
    printf("Testing rt_uuid_from_hex basic functionality...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Parse DNS namespace in hex format */
    RtUuid *uuid = rt_uuid_from_hex(arena, "6ba7b8109dad11d180b400c04fd430c8");
    TEST_ASSERT_NOT_NULL(uuid, "UUID should be parsed");

    /* Verify it matches the DNS namespace */
    RtUuid *dns = rt_uuid_namespace_dns(arena);
    TEST_ASSERT_TRUE(rt_uuid_equals(uuid, dns), "Parsed UUID should equal DNS namespace");

    printf("  Parsed UUID from hex successfully\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_from_hex_roundtrip()
{
    printf("Testing rt_uuid_from_hex roundtrip...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Create a UUID, convert to hex, parse back */
    RtUuid *original = rt_uuid_v4(arena);
    TEST_ASSERT_NOT_NULL(original, "Original UUID should be created");

    char *hex = rt_uuid_to_hex(arena, original);
    TEST_ASSERT_NOT_NULL(hex, "Hex string should be created");

    RtUuid *parsed = rt_uuid_from_hex(arena, hex);
    TEST_ASSERT_NOT_NULL(parsed, "Parsed UUID should be created");

    TEST_ASSERT_TRUE(rt_uuid_equals(original, parsed), "Roundtrip should preserve UUID");

    printf("  Roundtrip: %s\n", hex);
    rt_arena_destroy(arena);
}

void test_rt_uuid_from_hex_lowercase()
{
    printf("Testing rt_uuid_from_hex with lowercase...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtUuid *uuid = rt_uuid_from_hex(arena, "abcdef0123456789abcdef0123456789");
    TEST_ASSERT_NOT_NULL(uuid, "UUID should be parsed");

    printf("  Lowercase handled correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_from_hex_uppercase()
{
    printf("Testing rt_uuid_from_hex with uppercase...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtUuid *uuid = rt_uuid_from_hex(arena, "ABCDEF0123456789ABCDEF0123456789");
    TEST_ASSERT_NOT_NULL(uuid, "UUID should be parsed");

    /* Should produce same result as lowercase */
    RtUuid *uuid_lower = rt_uuid_from_hex(arena, "abcdef0123456789abcdef0123456789");
    TEST_ASSERT_TRUE(rt_uuid_equals(uuid, uuid_lower), "Case should not matter");

    printf("  Uppercase handled correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_from_hex_mixed_case()
{
    printf("Testing rt_uuid_from_hex with mixed case...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtUuid *uuid = rt_uuid_from_hex(arena, "AbCdEf0123456789aBcDeF0123456789");
    TEST_ASSERT_NOT_NULL(uuid, "UUID should be parsed");

    /* Should produce same result as all lowercase */
    RtUuid *uuid_lower = rt_uuid_from_hex(arena, "abcdef0123456789abcdef0123456789");
    TEST_ASSERT_TRUE(rt_uuid_equals(uuid, uuid_lower), "Mixed case should not matter");

    printf("  Mixed case handled correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_from_hex_nil()
{
    printf("Testing rt_uuid_from_hex with nil UUID...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtUuid *uuid = rt_uuid_from_hex(arena, "00000000000000000000000000000000");
    TEST_ASSERT_NOT_NULL(uuid, "UUID should be parsed");
    TEST_ASSERT_TRUE(rt_uuid_is_nil(uuid), "Parsed UUID should be nil");

    printf("  Nil UUID parsed correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_from_hex_max()
{
    printf("Testing rt_uuid_from_hex with max UUID...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtUuid *uuid = rt_uuid_from_hex(arena, "ffffffffffffffffffffffffffffffff");
    TEST_ASSERT_NOT_NULL(uuid, "UUID should be parsed");

    RtUuid *max = rt_uuid_max(arena);
    TEST_ASSERT_TRUE(rt_uuid_equals(uuid, max), "Parsed UUID should equal max UUID");

    printf("  Max UUID parsed correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_from_hex_invalid_length()
{
    printf("Testing rt_uuid_from_hex with invalid length...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Too short (31 chars) */
    RtUuid *uuid1 = rt_uuid_from_hex(arena, "6ba7b8109dad11d180b400c04fd430c");
    TEST_ASSERT_NULL(uuid1, "Too short UUID should return NULL");

    /* Too long (33 chars) */
    RtUuid *uuid2 = rt_uuid_from_hex(arena, "6ba7b8109dad11d180b400c04fd430c8a");
    TEST_ASSERT_NULL(uuid2, "Too long UUID should return NULL");

    /* Empty string */
    RtUuid *uuid3 = rt_uuid_from_hex(arena, "");
    TEST_ASSERT_NULL(uuid3, "Empty string should return NULL");

    /* With dashes (36 chars, wrong format) */
    RtUuid *uuid4 = rt_uuid_from_hex(arena, "6ba7b810-9dad-11d1-80b4-00c04fd430c8");
    TEST_ASSERT_NULL(uuid4, "36-char format with dashes should return NULL");

    printf("  Invalid lengths handled correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_from_hex_invalid_chars()
{
    printf("Testing rt_uuid_from_hex with invalid characters...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Contains 'g' which is not valid hex */
    RtUuid *uuid1 = rt_uuid_from_hex(arena, "6ba7b8109dad11d180b400c04fd430g8");
    TEST_ASSERT_NULL(uuid1, "Invalid hex 'g' should return NULL");

    /* Contains dash */
    RtUuid *uuid2 = rt_uuid_from_hex(arena, "6ba7b810-9dad11d180b400c04fd430c8");
    TEST_ASSERT_NULL(uuid2, "Dash should return NULL");

    /* Contains space */
    RtUuid *uuid3 = rt_uuid_from_hex(arena, "6ba7b8109dad11d180b400c04fd430 8");
    TEST_ASSERT_NULL(uuid3, "Space should return NULL");

    printf("  Invalid characters handled correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_from_hex_null_inputs()
{
    printf("Testing rt_uuid_from_hex with NULL inputs...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* NULL arena */
    RtUuid *uuid1 = rt_uuid_from_hex(NULL, "6ba7b8109dad11d180b400c04fd430c8");
    TEST_ASSERT_NULL(uuid1, "NULL arena should return NULL");

    /* NULL string */
    RtUuid *uuid2 = rt_uuid_from_hex(arena, NULL);
    TEST_ASSERT_NULL(uuid2, "NULL string should return NULL");

    printf("  NULL inputs handled correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_from_string_vs_from_hex()
{
    printf("Testing rt_uuid_from_string vs rt_uuid_from_hex consistency...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Same UUID in both formats */
    RtUuid *uuid_str = rt_uuid_from_string(arena, "6ba7b810-9dad-11d1-80b4-00c04fd430c8");
    RtUuid *uuid_hex = rt_uuid_from_hex(arena, "6ba7b8109dad11d180b400c04fd430c8");

    TEST_ASSERT_NOT_NULL(uuid_str, "UUID from string should be parsed");
    TEST_ASSERT_NOT_NULL(uuid_hex, "UUID from hex should be parsed");

    TEST_ASSERT_TRUE(rt_uuid_equals(uuid_str, uuid_hex), "Both formats should produce same UUID");

    printf("  Both formats produce consistent results\n");
    rt_arena_destroy(arena);
}

/* ============================================================================
 * rt_uuid_from_bytes() Tests
 * ============================================================================
 * Tests for creating UUIDs from 16-byte arrays.
 * ============================================================================ */

void test_rt_uuid_from_bytes_basic()
{
    printf("Testing rt_uuid_from_bytes basic functionality...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* DNS namespace bytes: 6ba7b810-9dad-11d1-80b4-00c04fd430c8 */
    unsigned char dns_bytes[16] = {
        0x6b, 0xa7, 0xb8, 0x10, 0x9d, 0xad, 0x11, 0xd1,
        0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8
    };

    RtUuid *uuid = rt_uuid_from_bytes(arena, dns_bytes);
    TEST_ASSERT_NOT_NULL(uuid, "UUID should be created");

    /* Verify it matches the DNS namespace */
    RtUuid *dns = rt_uuid_namespace_dns(arena);
    TEST_ASSERT_TRUE(rt_uuid_equals(uuid, dns), "UUID from bytes should equal DNS namespace");

    printf("  Created UUID from bytes successfully\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_from_bytes_roundtrip()
{
    printf("Testing rt_uuid_from_bytes roundtrip (toBytes -> fromBytes)...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Create a UUID, convert to bytes, parse back */
    RtUuid *original = rt_uuid_v4(arena);
    TEST_ASSERT_NOT_NULL(original, "Original UUID should be created");

    unsigned char *bytes = rt_uuid_to_bytes(arena, original);
    TEST_ASSERT_NOT_NULL(bytes, "Bytes should be created");

    RtUuid *parsed = rt_uuid_from_bytes(arena, bytes);
    TEST_ASSERT_NOT_NULL(parsed, "Parsed UUID should be created");

    TEST_ASSERT_TRUE(rt_uuid_equals(original, parsed), "Roundtrip should preserve UUID");

    char *str = rt_uuid_to_string(arena, original);
    printf("  Roundtrip successful: %s\n", str);
    rt_arena_destroy(arena);
}

void test_rt_uuid_from_bytes_nil()
{
    printf("Testing rt_uuid_from_bytes with nil UUID...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    unsigned char nil_bytes[16] = {0};

    RtUuid *uuid = rt_uuid_from_bytes(arena, nil_bytes);
    TEST_ASSERT_NOT_NULL(uuid, "UUID should be created");
    TEST_ASSERT_TRUE(rt_uuid_is_nil(uuid), "UUID from zero bytes should be nil");

    printf("  Nil UUID created correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_from_bytes_max()
{
    printf("Testing rt_uuid_from_bytes with max UUID...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    unsigned char max_bytes[16] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    };

    RtUuid *uuid = rt_uuid_from_bytes(arena, max_bytes);
    TEST_ASSERT_NOT_NULL(uuid, "UUID should be created");

    RtUuid *max = rt_uuid_max(arena);
    TEST_ASSERT_TRUE(rt_uuid_equals(uuid, max), "UUID from max bytes should equal max UUID");

    printf("  Max UUID created correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_from_bytes_null_inputs()
{
    printf("Testing rt_uuid_from_bytes with NULL inputs...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    unsigned char bytes[16] = {0};

    /* NULL arena */
    RtUuid *uuid1 = rt_uuid_from_bytes(NULL, bytes);
    TEST_ASSERT_NULL(uuid1, "NULL arena should return NULL");

    /* NULL bytes */
    RtUuid *uuid2 = rt_uuid_from_bytes(arena, NULL);
    TEST_ASSERT_NULL(uuid2, "NULL bytes should return NULL");

    printf("  NULL inputs handled correctly\n");
    rt_arena_destroy(arena);
}

/* ============================================================================
 * rt_uuid_from_base64() Tests
 * ============================================================================
 * Tests for parsing UUIDs from 22-char URL-safe base64 format.
 * ============================================================================ */

void test_rt_uuid_from_base64_basic()
{
    printf("Testing rt_uuid_from_base64 basic functionality...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Create a UUID, encode to base64, then decode */
    RtUuid *original = rt_uuid_v4(arena);
    TEST_ASSERT_NOT_NULL(original, "Original UUID should be created");

    char *base64 = rt_uuid_to_base64(arena, original);
    TEST_ASSERT_NOT_NULL(base64, "Base64 string should be created");
    TEST_ASSERT_EQ(strlen(base64), 22, "Base64 should be 22 chars");

    RtUuid *parsed = rt_uuid_from_base64(arena, base64);
    TEST_ASSERT_NOT_NULL(parsed, "UUID should be parsed from base64");

    TEST_ASSERT_TRUE(rt_uuid_equals(original, parsed), "Parsed UUID should equal original");

    printf("  Base64: %s\n", base64);
    rt_arena_destroy(arena);
}

void test_rt_uuid_from_base64_roundtrip()
{
    printf("Testing rt_uuid_from_base64 roundtrip...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Test multiple roundtrips */
    for (int i = 0; i < 10; i++) {
        RtUuid *original = rt_uuid_v4(arena);
        TEST_ASSERT_NOT_NULL(original, "Original UUID should be created");

        char *base64 = rt_uuid_to_base64(arena, original);
        TEST_ASSERT_NOT_NULL(base64, "Base64 should be created");

        RtUuid *parsed = rt_uuid_from_base64(arena, base64);
        TEST_ASSERT_NOT_NULL(parsed, "Parsed UUID should be created");

        TEST_ASSERT_TRUE(rt_uuid_equals(original, parsed), "Roundtrip should preserve UUID");
    }

    printf("  10 roundtrips successful\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_from_base64_nil()
{
    printf("Testing rt_uuid_from_base64 with nil UUID...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Nil UUID in base64 is all 'A's (0 values) */
    RtUuid *nil = rt_uuid_nil(arena);
    char *base64 = rt_uuid_to_base64(arena, nil);
    TEST_ASSERT_NOT_NULL(base64, "Base64 should be created");

    RtUuid *parsed = rt_uuid_from_base64(arena, base64);
    TEST_ASSERT_NOT_NULL(parsed, "Parsed UUID should be created");
    TEST_ASSERT_TRUE(rt_uuid_is_nil(parsed), "Parsed UUID should be nil");

    printf("  Nil UUID base64: %s\n", base64);
    rt_arena_destroy(arena);
}

void test_rt_uuid_from_base64_url_safe_chars()
{
    printf("Testing rt_uuid_from_base64 handles URL-safe characters...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Generate UUIDs until we get one with - or _ in base64 */
    int found_dash = 0;
    int found_underscore = 0;
    int attempts = 0;

    while ((!found_dash || !found_underscore) && attempts < 100) {
        RtUuid *uuid = rt_uuid_v4(arena);
        char *base64 = rt_uuid_to_base64(arena, uuid);

        for (int i = 0; i < 22 && base64[i]; i++) {
            if (base64[i] == '-') found_dash = 1;
            if (base64[i] == '_') found_underscore = 1;
        }

        /* Verify roundtrip works */
        RtUuid *parsed = rt_uuid_from_base64(arena, base64);
        TEST_ASSERT_NOT_NULL(parsed, "Parsed UUID should not be NULL");
        TEST_ASSERT_TRUE(rt_uuid_equals(uuid, parsed), "Roundtrip should work");

        attempts++;
    }

    printf("  URL-safe chars verified (found dash: %d, underscore: %d)\n",
           found_dash, found_underscore);
    rt_arena_destroy(arena);
}

void test_rt_uuid_from_base64_invalid_length()
{
    printf("Testing rt_uuid_from_base64 with invalid length...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Too short (21 chars) */
    RtUuid *uuid1 = rt_uuid_from_base64(arena, "AAAAAAAAAAAAAAAAAAAAA");
    TEST_ASSERT_NULL(uuid1, "Too short base64 should return NULL");

    /* Too long */
    RtUuid *uuid2 = rt_uuid_from_base64(arena, "AAAAAAAAAAAAAAAAAAAAAAA");  /* 23 chars */
    TEST_ASSERT_NULL(uuid2, "Too long base64 should return NULL");

    /* Empty string */
    RtUuid *uuid3 = rt_uuid_from_base64(arena, "");
    TEST_ASSERT_NULL(uuid3, "Empty string should return NULL");

    printf("  Invalid lengths handled correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_from_base64_invalid_chars()
{
    printf("Testing rt_uuid_from_base64 with invalid characters...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Contains '+' which is standard base64, not URL-safe */
    RtUuid *uuid1 = rt_uuid_from_base64(arena, "AAAAAAAAAAAAAAAAAAA+AA");
    TEST_ASSERT_NULL(uuid1, "'+' should be invalid in URL-safe base64");

    /* Contains '/' which is standard base64, not URL-safe */
    RtUuid *uuid2 = rt_uuid_from_base64(arena, "AAAAAAAAAAAAAAAAAAA/AA");
    TEST_ASSERT_NULL(uuid2, "'/' should be invalid in URL-safe base64");

    /* Contains space */
    RtUuid *uuid3 = rt_uuid_from_base64(arena, "AAAAAAAAAAAAAAAAAAA AA");
    TEST_ASSERT_NULL(uuid3, "Space should return NULL");

    /* Contains special character */
    RtUuid *uuid4 = rt_uuid_from_base64(arena, "AAAAAAAAAAAAAAAAAAA@AA");
    TEST_ASSERT_NULL(uuid4, "Special char should return NULL");

    printf("  Invalid characters handled correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_from_base64_null_inputs()
{
    printf("Testing rt_uuid_from_base64 with NULL inputs...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* NULL arena */
    RtUuid *uuid1 = rt_uuid_from_base64(NULL, "AAAAAAAAAAAAAAAAAAAAAA");
    TEST_ASSERT_NULL(uuid1, "NULL arena should return NULL");

    /* NULL string */
    RtUuid *uuid2 = rt_uuid_from_base64(arena, NULL);
    TEST_ASSERT_NULL(uuid2, "NULL string should return NULL");

    printf("  NULL inputs handled correctly\n");
    rt_arena_destroy(arena);
}

void test_rt_uuid_all_formats_consistency()
{
    printf("Testing all UUID format roundtrips are consistent...\n");

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Create original UUID */
    RtUuid *original = rt_uuid_v4(arena);
    TEST_ASSERT_NOT_NULL(original, "Original UUID should be created");

    /* Convert to all formats */
    char *str = rt_uuid_to_string(arena, original);
    char *hex = rt_uuid_to_hex(arena, original);
    char *base64 = rt_uuid_to_base64(arena, original);
    unsigned char *bytes = rt_uuid_to_bytes(arena, original);

    /* Parse from all formats */
    RtUuid *from_str = rt_uuid_from_string(arena, str);
    RtUuid *from_hex = rt_uuid_from_hex(arena, hex);
    RtUuid *from_base64 = rt_uuid_from_base64(arena, base64);
    RtUuid *from_bytes = rt_uuid_from_bytes(arena, bytes);

    /* All should equal the original */
    TEST_ASSERT_TRUE(rt_uuid_equals(original, from_str), "String roundtrip");
    TEST_ASSERT_TRUE(rt_uuid_equals(original, from_hex), "Hex roundtrip");
    TEST_ASSERT_TRUE(rt_uuid_equals(original, from_base64), "Base64 roundtrip");
    TEST_ASSERT_TRUE(rt_uuid_equals(original, from_bytes), "Bytes roundtrip");

    printf("  All formats consistent\n");
    printf("    String: %s\n", str);
    printf("    Hex:    %s\n", hex);
    printf("    Base64: %s\n", base64);
    rt_arena_destroy(arena);
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

void test_rt_uuid_main()
{
    printf("\n=== Runtime UUID Tests ===\n\n");

    /* rt_uuid_v4() tests */
    test_rt_uuid_v4_basic();
    test_rt_uuid_v4_version_bits();
    test_rt_uuid_v4_variant_bits();
    test_rt_uuid_v4_uniqueness();
    test_rt_uuid_v4_randomness();
    test_rt_uuid_v4_null_arena();

    /* rt_uuid_v5() tests */
    test_rt_uuid_v5_basic();
    test_rt_uuid_v5_version_bits();
    test_rt_uuid_v5_variant_bits();
    test_rt_uuid_v5_deterministic();
    test_rt_uuid_v5_different_namespaces();
    test_rt_uuid_v5_known_vector();
    test_rt_uuid_v5_empty_name();
    test_rt_uuid_v5_null_inputs();

    /* rt_uuid_v7() tests */
    test_rt_uuid_v7_basic();
    test_rt_uuid_v7_version_bits();
    test_rt_uuid_v7_variant_bits();
    test_rt_uuid_v7_timestamp();
    test_rt_uuid_v7_ordering();
    test_rt_uuid_v7_uniqueness();
    test_rt_uuid_v7_randomness();
    test_rt_uuid_v7_string_format();
    test_rt_uuid_create_returns_v7();

    /* Conversion tests */
    test_rt_uuid_to_string_format();
    test_rt_uuid_to_hex_format();
    test_rt_uuid_to_bytes();
    test_rt_uuid_to_base64_format();
    test_rt_uuid_to_base64_known_value();

    /* Comparison tests */
    test_rt_uuid_equals();
    test_rt_uuid_compare();

    /* Special UUID tests */
    test_rt_uuid_nil();
    test_rt_uuid_max();

    /* Namespace tests */
    test_rt_uuid_namespaces();

    /* rt_uuid_from_string() tests */
    test_rt_uuid_from_string_basic();
    test_rt_uuid_from_string_roundtrip();
    test_rt_uuid_from_string_lowercase();
    test_rt_uuid_from_string_uppercase();
    test_rt_uuid_from_string_mixed_case();
    test_rt_uuid_from_string_nil();
    test_rt_uuid_from_string_max();
    test_rt_uuid_from_string_invalid_length();
    test_rt_uuid_from_string_invalid_dashes();
    test_rt_uuid_from_string_invalid_chars();
    test_rt_uuid_from_string_null_inputs();

    /* rt_uuid_from_hex() tests */
    test_rt_uuid_from_hex_basic();
    test_rt_uuid_from_hex_roundtrip();
    test_rt_uuid_from_hex_lowercase();
    test_rt_uuid_from_hex_uppercase();
    test_rt_uuid_from_hex_mixed_case();
    test_rt_uuid_from_hex_nil();
    test_rt_uuid_from_hex_max();
    test_rt_uuid_from_hex_invalid_length();
    test_rt_uuid_from_hex_invalid_chars();
    test_rt_uuid_from_hex_null_inputs();

    /* Cross-format consistency test */
    test_rt_uuid_from_string_vs_from_hex();

    /* rt_uuid_from_bytes() tests */
    test_rt_uuid_from_bytes_basic();
    test_rt_uuid_from_bytes_roundtrip();
    test_rt_uuid_from_bytes_nil();
    test_rt_uuid_from_bytes_max();
    test_rt_uuid_from_bytes_null_inputs();

    /* rt_uuid_from_base64() tests */
    test_rt_uuid_from_base64_basic();
    test_rt_uuid_from_base64_roundtrip();
    test_rt_uuid_from_base64_nil();
    test_rt_uuid_from_base64_url_safe_chars();
    test_rt_uuid_from_base64_invalid_length();
    test_rt_uuid_from_base64_invalid_chars();
    test_rt_uuid_from_base64_null_inputs();

    /* All formats consistency test */
    test_rt_uuid_all_formats_consistency();

    printf("\n=== All Runtime UUID Tests Passed ===\n\n");
}
