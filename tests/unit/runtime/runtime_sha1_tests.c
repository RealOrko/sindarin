// tests/unit/runtime/runtime_sha1_tests.c
// Tests for the SHA-1 hash algorithm implementation

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "../../src/runtime/runtime_sha1.h"
#include "../test_utils.h"
#include "../debug.h"

/* ============================================================================
 * sha1_init() Tests
 * ============================================================================
 * Tests for SHA-1 context initialization with RFC 3174 constants.
 * ============================================================================ */

void test_sha1_init_sets_h0_h4_constants()
{
    printf("Testing sha1_init sets H0-H4 constants...\n");

    SHA1_Context ctx;
    sha1_init(&ctx);

    /* Verify initial hash values (RFC 3174 Section 6.1) */
    TEST_ASSERT_EQ(ctx.H[0], 0x67452301, "H0 = 0x67452301");
    TEST_ASSERT_EQ(ctx.H[1], 0xEFCDAB89, "H1 = 0xEFCDAB89");
    TEST_ASSERT_EQ(ctx.H[2], 0x98BADCFE, "H2 = 0x98BADCFE");
    TEST_ASSERT_EQ(ctx.H[3], 0x10325476, "H3 = 0x10325476");
    TEST_ASSERT_EQ(ctx.H[4], 0xC3D2E1F0, "H4 = 0xC3D2E1F0");

    /* Verify buffer and length are initialized */
    TEST_ASSERT_EQ(ctx.buffer_len, (size_t)0, "buffer_len = 0");
    TEST_ASSERT_EQ(ctx.total_len, (uint64_t)0, "total_len = 0");

    printf("  SHA-1 context initialized correctly\n");
}

void test_sha1_init_null_context()
{
    printf("Testing sha1_init with NULL context...\n");

    /* Should not crash with NULL */
    sha1_init(NULL);

    printf("  NULL context handled gracefully\n");
}

/* ============================================================================
 * sha1_pad_message() Tests
 * ============================================================================
 * Tests for SHA-1 message padding according to RFC 3174.
 * ============================================================================ */

void test_sha1_pad_empty_message()
{
    printf("Testing sha1_pad_message with empty message...\n");

    uint8_t block[128];
    int block_count;

    /* Empty message: total_len = 0, data_len = 0 */
    int result = sha1_pad_message(block, &block_count, NULL, 0, 0);
    TEST_ASSERT_EQ(result, 0, "Padding should succeed");
    TEST_ASSERT_EQ(block_count, 1, "Empty message needs 1 block");

    /* Verify '1' bit (0x80) at position 0 */
    TEST_ASSERT_EQ(block[0], 0x80, "First byte should be 0x80");

    /* Verify zeros from position 1 to 55 */
    for (int i = 1; i < 56; i++) {
        TEST_ASSERT_EQ(block[i], 0x00, "Padding should be zeros");
    }

    /* Verify length encoding (0 bits = 0x0000000000000000) */
    for (int i = 56; i < 64; i++) {
        TEST_ASSERT_EQ(block[i], 0x00, "Length should be 0");
    }

    printf("  Empty message padding correct\n");
}

void test_sha1_pad_short_message()
{
    printf("Testing sha1_pad_message with short message...\n");

    uint8_t block[128];
    int block_count;
    uint8_t data[] = "abc";  /* 3 bytes */

    int result = sha1_pad_message(block, &block_count, data, 3, 3);
    TEST_ASSERT_EQ(result, 0, "Padding should succeed");
    TEST_ASSERT_EQ(block_count, 1, "Short message needs 1 block");

    /* Verify data copied */
    TEST_ASSERT_EQ(block[0], 'a', "Data byte 0");
    TEST_ASSERT_EQ(block[1], 'b', "Data byte 1");
    TEST_ASSERT_EQ(block[2], 'c', "Data byte 2");

    /* Verify '1' bit (0x80) at position 3 */
    TEST_ASSERT_EQ(block[3], 0x80, "0x80 after data");

    /* Verify zeros from position 4 to 55 */
    for (int i = 4; i < 56; i++) {
        TEST_ASSERT_EQ(block[i], 0x00, "Padding should be zeros");
    }

    /* Verify length encoding (24 bits = 0x0000000000000018) */
    for (int i = 56; i < 63; i++) {
        TEST_ASSERT_EQ(block[i], 0x00, "Upper length bytes should be 0");
    }
    TEST_ASSERT_EQ(block[63], 0x18, "Length should be 24 bits (0x18)");

    printf("  Short message padding correct\n");
}

void test_sha1_pad_55_byte_message()
{
    printf("Testing sha1_pad_message with 55-byte message...\n");

    uint8_t block[128];
    int block_count;
    uint8_t data[55];
    memset(data, 'A', 55);

    int result = sha1_pad_message(block, &block_count, data, 55, 55);
    TEST_ASSERT_EQ(result, 0, "Padding should succeed");
    TEST_ASSERT_EQ(block_count, 1, "55-byte message fits in 1 block");

    /* Verify data copied */
    for (int i = 0; i < 55; i++) {
        TEST_ASSERT_EQ(block[i], 'A', "Data byte preserved");
    }

    /* Verify '1' bit at position 55 */
    TEST_ASSERT_EQ(block[55], 0x80, "0x80 after data");

    /* Verify length encoding (55 * 8 = 440 = 0x1B8) */
    for (int i = 56; i < 62; i++) {
        TEST_ASSERT_EQ(block[i], 0x00, "Upper length bytes should be 0");
    }
    TEST_ASSERT_EQ(block[62], 0x01, "Length high byte");
    TEST_ASSERT_EQ(block[63], 0xB8, "Length low byte (440 = 0x1B8)");

    printf("  55-byte message padding correct (fits in 1 block)\n");
}

void test_sha1_pad_56_byte_message()
{
    printf("Testing sha1_pad_message with 56-byte message...\n");

    uint8_t block[128];
    int block_count;
    uint8_t data[56];
    memset(data, 'B', 56);

    int result = sha1_pad_message(block, &block_count, data, 56, 56);
    TEST_ASSERT_EQ(result, 0, "Padding should succeed");
    TEST_ASSERT_EQ(block_count, 2, "56-byte message needs 2 blocks");

    /* Verify data copied to first block */
    for (int i = 0; i < 56; i++) {
        TEST_ASSERT_EQ(block[i], 'B', "Data byte preserved");
    }

    /* Verify '1' bit at position 56 */
    TEST_ASSERT_EQ(block[56], 0x80, "0x80 after data");

    /* Verify rest of first block is zeros */
    for (int i = 57; i < 64; i++) {
        TEST_ASSERT_EQ(block[i], 0x00, "First block padding zeros");
    }

    /* Verify second block is zeros until length */
    for (int i = 64; i < 120; i++) {
        TEST_ASSERT_EQ(block[i], 0x00, "Second block zeros");
    }

    /* Verify length encoding (56 * 8 = 448 = 0x1C0) at bytes 120-127 */
    for (int i = 120; i < 126; i++) {
        TEST_ASSERT_EQ(block[i], 0x00, "Upper length bytes should be 0");
    }
    TEST_ASSERT_EQ(block[126], 0x01, "Length high byte");
    TEST_ASSERT_EQ(block[127], 0xC0, "Length low byte (448 = 0x1C0)");

    printf("  56-byte message padding correct (needs 2 blocks)\n");
}

void test_sha1_pad_63_byte_message()
{
    printf("Testing sha1_pad_message with 63-byte message...\n");

    uint8_t block[128];
    int block_count;
    uint8_t data[63];
    memset(data, 'C', 63);

    int result = sha1_pad_message(block, &block_count, data, 63, 63);
    TEST_ASSERT_EQ(result, 0, "Padding should succeed");
    TEST_ASSERT_EQ(block_count, 2, "63-byte message needs 2 blocks");

    /* Verify data copied to first block */
    for (int i = 0; i < 63; i++) {
        TEST_ASSERT_EQ(block[i], 'C', "Data byte preserved");
    }

    /* Verify '1' bit at position 63 */
    TEST_ASSERT_EQ(block[63], 0x80, "0x80 after data");

    /* Verify second block is zeros until length */
    for (int i = 64; i < 120; i++) {
        TEST_ASSERT_EQ(block[i], 0x00, "Second block zeros");
    }

    /* Verify length encoding (63 * 8 = 504 = 0x1F8) */
    for (int i = 120; i < 126; i++) {
        TEST_ASSERT_EQ(block[i], 0x00, "Upper length bytes should be 0");
    }
    TEST_ASSERT_EQ(block[126], 0x01, "Length high byte");
    TEST_ASSERT_EQ(block[127], 0xF8, "Length low byte (504 = 0x1F8)");

    printf("  63-byte message padding correct (needs 2 blocks)\n");
}

void test_sha1_pad_length_encoding()
{
    printf("Testing sha1_pad_message length encoding (big-endian)...\n");

    uint8_t block[128];
    int block_count;

    /* Test with larger total_len to verify big-endian encoding */
    /* total_len = 0x123456789ABC (about 20TB - just for testing encoding) */
    uint64_t total_len = 0x123456789ABCULL;
    uint64_t bit_len = total_len * 8;  /* 0x91A2B3C4D5E0 */

    int result = sha1_pad_message(block, &block_count, NULL, 0, total_len);
    TEST_ASSERT_EQ(result, 0, "Padding should succeed");

    /* Verify big-endian encoding of bit length at bytes 56-63 */
    TEST_ASSERT_EQ(block[56], (uint8_t)(bit_len >> 56), "Length byte 0");
    TEST_ASSERT_EQ(block[57], (uint8_t)(bit_len >> 48), "Length byte 1");
    TEST_ASSERT_EQ(block[58], (uint8_t)(bit_len >> 40), "Length byte 2");
    TEST_ASSERT_EQ(block[59], (uint8_t)(bit_len >> 32), "Length byte 3");
    TEST_ASSERT_EQ(block[60], (uint8_t)(bit_len >> 24), "Length byte 4");
    TEST_ASSERT_EQ(block[61], (uint8_t)(bit_len >> 16), "Length byte 5");
    TEST_ASSERT_EQ(block[62], (uint8_t)(bit_len >> 8), "Length byte 6");
    TEST_ASSERT_EQ(block[63], (uint8_t)(bit_len), "Length byte 7");

    printf("  Big-endian length encoding correct\n");
}

void test_sha1_pad_null_inputs()
{
    printf("Testing sha1_pad_message with NULL inputs...\n");

    uint8_t block[128];
    int block_count;

    /* NULL block should fail */
    int result = sha1_pad_message(NULL, &block_count, NULL, 0, 0);
    TEST_ASSERT_EQ(result, -1, "NULL block should fail");

    /* NULL block_count should fail */
    result = sha1_pad_message(block, NULL, NULL, 0, 0);
    TEST_ASSERT_EQ(result, -1, "NULL block_count should fail");

    /* data_len >= 64 should fail */
    result = sha1_pad_message(block, &block_count, NULL, 64, 64);
    TEST_ASSERT_EQ(result, -1, "data_len >= 64 should fail");

    printf("  NULL input handling correct\n");
}

void test_sha1_pad_output_is_512_bit_aligned()
{
    printf("Testing sha1_pad_message output is 512-bit aligned...\n");

    uint8_t block[128];
    int block_count;

    /* Test various message lengths */
    size_t test_lengths[] = {0, 1, 10, 55, 56, 57, 63};
    int expected_blocks[] = {1, 1, 1, 1, 2, 2, 2};

    for (size_t i = 0; i < sizeof(test_lengths) / sizeof(test_lengths[0]); i++) {
        size_t len = test_lengths[i];
        uint8_t data[64];
        memset(data, 'X', len);

        int result = sha1_pad_message(block, &block_count, data, len, len);
        TEST_ASSERT_EQ(result, 0, "Padding should succeed");
        TEST_ASSERT_EQ(block_count, expected_blocks[i], "Correct block count");

        /* Total output is block_count * 64 bytes = block_count * 512 bits */
        size_t total_bits = (size_t)block_count * 512;
        TEST_ASSERT(total_bits % 512 == 0, "Output should be 512-bit aligned");
    }

    printf("  All outputs are 512-bit aligned\n");
}

/* ============================================================================
 * sha1_hash() Tests - RFC 3174 Test Vectors
 * ============================================================================
 * These tests verify the complete SHA-1 implementation using official test
 * vectors. The hash results depend on all round functions (f0, f1, f2, f3)
 * working correctly, so passing these tests validates the logical functions.
 * ============================================================================ */

/* Helper to print digest as hex for debugging */
static void digest_to_hex(const uint8_t digest[SHA1_DIGEST_SIZE], char hex[41])
{
    const char *hex_chars = "0123456789abcdef";
    for (int i = 0; i < SHA1_DIGEST_SIZE; i++) {
        hex[i * 2]     = hex_chars[(digest[i] >> 4) & 0xf];
        hex[i * 2 + 1] = hex_chars[digest[i] & 0xf];
    }
    hex[40] = '\0';
}

void test_sha1_hash_abc()
{
    printf("Testing sha1_hash with \"abc\" (RFC 3174 test vector)...\n");

    /*
     * RFC 3174 Appendix A - Test vector 1:
     * Input: "abc" (3 bytes)
     * Expected: a9993e36 4706816a ba3e2571 7850c26c 9cd0d89d
     *
     * This tests all 80 rounds including f0 (rounds 0-19), f1 (rounds 20-39),
     * f2 (rounds 40-59), and f3 (rounds 60-79).
     */
    uint8_t data[] = "abc";
    uint8_t digest[SHA1_DIGEST_SIZE];

    sha1_hash(data, 3, digest);

    /* Expected digest bytes */
    uint8_t expected[SHA1_DIGEST_SIZE] = {
        0xa9, 0x99, 0x3e, 0x36, 0x47, 0x06, 0x81, 0x6a, 0xba, 0x3e,
        0x25, 0x71, 0x78, 0x50, 0xc2, 0x6c, 0x9c, 0xd0, 0xd8, 0x9d
    };

    for (int i = 0; i < SHA1_DIGEST_SIZE; i++) {
        TEST_ASSERT_EQ(digest[i], expected[i], "SHA-1(abc) byte match");
    }

    char hex[41];
    digest_to_hex(digest, hex);
    printf("  SHA-1(\"abc\") = %s\n", hex);
}

void test_sha1_hash_empty()
{
    printf("Testing sha1_hash with empty message...\n");

    /*
     * SHA-1 of empty string:
     * Expected: da39a3ee 5e6b4b0d 3255bfef 95601890 afd80709
     */
    uint8_t digest[SHA1_DIGEST_SIZE];

    sha1_hash(NULL, 0, digest);

    uint8_t expected[SHA1_DIGEST_SIZE] = {
        0xda, 0x39, 0xa3, 0xee, 0x5e, 0x6b, 0x4b, 0x0d, 0x32, 0x55,
        0xbf, 0xef, 0x95, 0x60, 0x18, 0x90, 0xaf, 0xd8, 0x07, 0x09
    };

    for (int i = 0; i < SHA1_DIGEST_SIZE; i++) {
        TEST_ASSERT_EQ(digest[i], expected[i], "SHA-1(empty) byte match");
    }

    char hex[41];
    digest_to_hex(digest, hex);
    printf("  SHA-1(\"\") = %s\n", hex);
}

void test_sha1_hash_448_bits()
{
    printf("Testing sha1_hash with 448-bit message (56 chars)...\n");

    /*
     * RFC 3174 Appendix A - Test vector 2:
     * Input: "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
     * (56 bytes = 448 bits, exactly fitting in padding boundary)
     * Expected: 84983e44 1c3bd26e baae4aa1 f95129e5 e54670f1
     */
    uint8_t data[] = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    uint8_t digest[SHA1_DIGEST_SIZE];

    sha1_hash(data, 56, digest);

    uint8_t expected[SHA1_DIGEST_SIZE] = {
        0x84, 0x98, 0x3e, 0x44, 0x1c, 0x3b, 0xd2, 0x6e, 0xba, 0xae,
        0x4a, 0xa1, 0xf9, 0x51, 0x29, 0xe5, 0xe5, 0x46, 0x70, 0xf1
    };

    for (int i = 0; i < SHA1_DIGEST_SIZE; i++) {
        TEST_ASSERT_EQ(digest[i], expected[i], "SHA-1(448-bit) byte match");
    }

    char hex[41];
    digest_to_hex(digest, hex);
    printf("  SHA-1(448-bit message) = %s\n", hex);
}

void test_sha1_process_block_verifies_logical_functions()
{
    printf("Testing sha1_process_block verifies logical functions...\n");

    /*
     * This test specifically validates that sha1_process_block uses the
     * correct logical functions in each round range:
     *   - f0(b,c,d) = (b & c) | ((~b) & d) for rounds 0-19
     *   - f1(b,c,d) = b ^ c ^ d for rounds 20-39
     *   - f2(b,c,d) = (b & c) | (b & d) | (c & d) for rounds 40-59
     *   - f3(b,c,d) = b ^ c ^ d for rounds 60-79
     *
     * We test by hashing a known message and checking the result matches
     * the expected value. If any logical function is wrong, the hash
     * will not match.
     */

    /* Test with "The quick brown fox jumps over the lazy dog" */
    uint8_t data[] = "The quick brown fox jumps over the lazy dog";
    uint8_t digest[SHA1_DIGEST_SIZE];

    sha1_hash(data, 43, digest);

    /* Known SHA-1: 2fd4e1c6 7a2d28fc ed849ee1 bb76e739 1b93eb12 */
    uint8_t expected[SHA1_DIGEST_SIZE] = {
        0x2f, 0xd4, 0xe1, 0xc6, 0x7a, 0x2d, 0x28, 0xfc, 0xed, 0x84,
        0x9e, 0xe1, 0xbb, 0x76, 0xe7, 0x39, 0x1b, 0x93, 0xeb, 0x12
    };

    for (int i = 0; i < SHA1_DIGEST_SIZE; i++) {
        TEST_ASSERT_EQ(digest[i], expected[i], "SHA-1 logical functions verified");
    }

    printf("  All 4 logical functions (f0, f1, f2, f3) verified via hash test\n");
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

void test_rt_sha1_main()
{
    printf("\n=== Runtime SHA-1 Tests ===\n\n");

    /* sha1_init() tests */
    test_sha1_init_sets_h0_h4_constants();
    test_sha1_init_null_context();

    /* sha1_pad_message() tests */
    test_sha1_pad_empty_message();
    test_sha1_pad_short_message();
    test_sha1_pad_55_byte_message();
    test_sha1_pad_56_byte_message();
    test_sha1_pad_63_byte_message();
    test_sha1_pad_length_encoding();
    test_sha1_pad_null_inputs();
    test_sha1_pad_output_is_512_bit_aligned();

    /* sha1_hash() tests - verifies logical functions f0, f1, f2, f3 */
    test_sha1_hash_abc();
    test_sha1_hash_empty();
    test_sha1_hash_448_bits();
    test_sha1_process_block_verifies_logical_functions();

    printf("\n=== All Runtime SHA-1 Tests Passed ===\n\n");
}
