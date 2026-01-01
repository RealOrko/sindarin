// tests/unit/runtime/runtime_byte_tests.c
// Tests for runtime byte array operations (encoding/decoding)

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../runtime.h"

/* ============================================================================
 * Byte Array to String Tests
 * ============================================================================ */

void test_rt_byte_array_to_string()
{
    printf("Testing rt_byte_array_to_string...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Basic ASCII string */
    unsigned char *bytes = rt_array_alloc_byte(arena, 5, 0);
    bytes[0] = 'h';
    bytes[1] = 'e';
    bytes[2] = 'l';
    bytes[3] = 'l';
    bytes[4] = 'o';

    char *result = rt_byte_array_to_string(arena, bytes);
    assert(strcmp(result, "hello") == 0);

    /* Empty array */
    bytes = rt_array_alloc_byte(arena, 0, 0);
    result = rt_byte_array_to_string(arena, bytes);
    assert(strcmp(result, "") == 0);

    /* NULL array */
    result = rt_byte_array_to_string(arena, NULL);
    assert(strcmp(result, "") == 0);

    /* Bytes with high values */
    bytes = rt_array_alloc_byte(arena, 3, 0);
    bytes[0] = 0x00;
    bytes[1] = 0x7F;
    bytes[2] = 0xFF;
    result = rt_byte_array_to_string(arena, bytes);
    assert(result[0] == '\0');  /* String ends at first null byte when using strlen */

    rt_arena_destroy(arena);
}

void test_rt_byte_array_to_string_latin1()
{
    printf("Testing rt_byte_array_to_string_latin1...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* ASCII range (0x00-0x7F) - single byte UTF-8 */
    unsigned char *bytes = rt_array_alloc_byte(arena, 3, 0);
    bytes[0] = 'A';
    bytes[1] = 'B';
    bytes[2] = 'C';

    char *result = rt_byte_array_to_string_latin1(arena, bytes);
    assert(strcmp(result, "ABC") == 0);

    /* Extended Latin-1 (0x80-0xFF) - becomes 2-byte UTF-8 */
    bytes = rt_array_alloc_byte(arena, 2, 0);
    bytes[0] = 0xC0;  /* Latin-1: Capital A with grave */
    bytes[1] = 0xE9;  /* Latin-1: Small e with acute */

    result = rt_byte_array_to_string_latin1(arena, bytes);
    /* 0xC0 -> UTF-8: 0xC3 0x80 */
    /* 0xE9 -> UTF-8: 0xC3 0xA9 */
    assert((unsigned char)result[0] == 0xC3);
    assert((unsigned char)result[1] == 0x80);
    assert((unsigned char)result[2] == 0xC3);
    assert((unsigned char)result[3] == 0xA9);

    /* Empty array */
    bytes = rt_array_alloc_byte(arena, 0, 0);
    result = rt_byte_array_to_string_latin1(arena, bytes);
    assert(strcmp(result, "") == 0);

    /* NULL array */
    result = rt_byte_array_to_string_latin1(arena, NULL);
    assert(strcmp(result, "") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Byte Array to Hex Tests
 * ============================================================================ */

void test_rt_byte_array_to_hex()
{
    printf("Testing rt_byte_array_to_hex...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Basic conversion */
    unsigned char *bytes = rt_array_alloc_byte(arena, 3, 0);
    bytes[0] = 0xDE;
    bytes[1] = 0xAD;
    bytes[2] = 0xBE;

    char *result = rt_byte_array_to_hex(arena, bytes);
    assert(strcmp(result, "deadbe") == 0);

    /* All zeros */
    bytes = rt_array_alloc_byte(arena, 2, 0);
    bytes[0] = 0x00;
    bytes[1] = 0x00;

    result = rt_byte_array_to_hex(arena, bytes);
    assert(strcmp(result, "0000") == 0);

    /* All 0xFF */
    bytes = rt_array_alloc_byte(arena, 2, 0xFF);

    result = rt_byte_array_to_hex(arena, bytes);
    assert(strcmp(result, "ffff") == 0);

    /* Single byte */
    bytes = rt_array_alloc_byte(arena, 1, 0);
    bytes[0] = 0xAB;

    result = rt_byte_array_to_hex(arena, bytes);
    assert(strcmp(result, "ab") == 0);

    /* Empty array */
    bytes = rt_array_alloc_byte(arena, 0, 0);
    result = rt_byte_array_to_hex(arena, bytes);
    assert(strcmp(result, "") == 0);

    /* NULL array */
    result = rt_byte_array_to_hex(arena, NULL);
    assert(strcmp(result, "") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Byte Array to Base64 Tests
 * ============================================================================ */

void test_rt_byte_array_to_base64()
{
    printf("Testing rt_byte_array_to_base64...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Test vector: "Man" -> "TWFu" */
    unsigned char *bytes = rt_array_alloc_byte(arena, 3, 0);
    bytes[0] = 'M';
    bytes[1] = 'a';
    bytes[2] = 'n';

    char *result = rt_byte_array_to_base64(arena, bytes);
    assert(strcmp(result, "TWFu") == 0);

    /* Test vector: "Ma" -> "TWE=" (1 padding) */
    bytes = rt_array_alloc_byte(arena, 2, 0);
    bytes[0] = 'M';
    bytes[1] = 'a';

    result = rt_byte_array_to_base64(arena, bytes);
    assert(strcmp(result, "TWE=") == 0);

    /* Test vector: "M" -> "TQ==" (2 padding) */
    bytes = rt_array_alloc_byte(arena, 1, 0);
    bytes[0] = 'M';

    result = rt_byte_array_to_base64(arena, bytes);
    assert(strcmp(result, "TQ==") == 0);

    /* Test vector: "Hello" -> "SGVsbG8=" */
    bytes = rt_array_alloc_byte(arena, 5, 0);
    bytes[0] = 'H';
    bytes[1] = 'e';
    bytes[2] = 'l';
    bytes[3] = 'l';
    bytes[4] = 'o';

    result = rt_byte_array_to_base64(arena, bytes);
    assert(strcmp(result, "SGVsbG8=") == 0);

    /* Empty array */
    bytes = rt_array_alloc_byte(arena, 0, 0);
    result = rt_byte_array_to_base64(arena, bytes);
    assert(strcmp(result, "") == 0);

    /* NULL array */
    result = rt_byte_array_to_base64(arena, NULL);
    assert(strcmp(result, "") == 0);

    /* Binary data */
    bytes = rt_array_alloc_byte(arena, 4, 0);
    bytes[0] = 0x00;
    bytes[1] = 0xFF;
    bytes[2] = 0x00;
    bytes[3] = 0xFF;

    result = rt_byte_array_to_base64(arena, bytes);
    assert(strcmp(result, "AP8A/w==") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * String to Bytes Tests
 * ============================================================================ */

void test_rt_string_to_bytes()
{
    printf("Testing rt_string_to_bytes...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Basic string */
    unsigned char *bytes = rt_string_to_bytes(arena, "hello");
    assert(rt_array_length(bytes) == 5);
    assert(bytes[0] == 'h');
    assert(bytes[1] == 'e');
    assert(bytes[2] == 'l');
    assert(bytes[3] == 'l');
    assert(bytes[4] == 'o');

    /* Empty string */
    bytes = rt_string_to_bytes(arena, "");
    assert(rt_array_length(bytes) == 0);

    /* NULL string */
    bytes = rt_string_to_bytes(arena, NULL);
    assert(rt_array_length(bytes) == 0);

    /* String with special characters */
    bytes = rt_string_to_bytes(arena, "\t\n");
    assert(rt_array_length(bytes) == 2);
    assert(bytes[0] == '\t');
    assert(bytes[1] == '\n');

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Bytes from Hex Tests
 * ============================================================================ */

void test_rt_bytes_from_hex()
{
    printf("Testing rt_bytes_from_hex...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Lowercase hex */
    unsigned char *bytes = rt_bytes_from_hex(arena, "deadbeef");
    assert(rt_array_length(bytes) == 4);
    assert(bytes[0] == 0xDE);
    assert(bytes[1] == 0xAD);
    assert(bytes[2] == 0xBE);
    assert(bytes[3] == 0xEF);

    /* Uppercase hex */
    bytes = rt_bytes_from_hex(arena, "DEADBEEF");
    assert(rt_array_length(bytes) == 4);
    assert(bytes[0] == 0xDE);
    assert(bytes[1] == 0xAD);
    assert(bytes[2] == 0xBE);
    assert(bytes[3] == 0xEF);

    /* Mixed case */
    bytes = rt_bytes_from_hex(arena, "DeAdBeEf");
    assert(rt_array_length(bytes) == 4);
    assert(bytes[0] == 0xDE);
    assert(bytes[1] == 0xAD);
    assert(bytes[2] == 0xBE);
    assert(bytes[3] == 0xEF);

    /* All zeros */
    bytes = rt_bytes_from_hex(arena, "0000");
    assert(rt_array_length(bytes) == 2);
    assert(bytes[0] == 0x00);
    assert(bytes[1] == 0x00);

    /* All 0xFF */
    bytes = rt_bytes_from_hex(arena, "ffff");
    assert(rt_array_length(bytes) == 2);
    assert(bytes[0] == 0xFF);
    assert(bytes[1] == 0xFF);

    /* Empty string */
    bytes = rt_bytes_from_hex(arena, "");
    assert(rt_array_length(bytes) == 0);

    /* NULL string */
    bytes = rt_bytes_from_hex(arena, NULL);
    assert(rt_array_length(bytes) == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Bytes from Base64 Tests
 * ============================================================================ */

void test_rt_bytes_from_base64()
{
    printf("Testing rt_bytes_from_base64...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Test vector: "TWFu" -> "Man" */
    unsigned char *bytes = rt_bytes_from_base64(arena, "TWFu");
    assert(rt_array_length(bytes) == 3);
    assert(bytes[0] == 'M');
    assert(bytes[1] == 'a');
    assert(bytes[2] == 'n');

    /* Test vector: "TWE=" -> "Ma" (1 padding) */
    bytes = rt_bytes_from_base64(arena, "TWE=");
    assert(rt_array_length(bytes) == 2);
    assert(bytes[0] == 'M');
    assert(bytes[1] == 'a');

    /* Test vector: "TQ==" -> "M" (2 padding) */
    bytes = rt_bytes_from_base64(arena, "TQ==");
    assert(rt_array_length(bytes) == 1);
    assert(bytes[0] == 'M');

    /* Test vector: "SGVsbG8=" -> "Hello" */
    bytes = rt_bytes_from_base64(arena, "SGVsbG8=");
    assert(rt_array_length(bytes) == 5);
    assert(bytes[0] == 'H');
    assert(bytes[1] == 'e');
    assert(bytes[2] == 'l');
    assert(bytes[3] == 'l');
    assert(bytes[4] == 'o');

    /* Empty string */
    bytes = rt_bytes_from_base64(arena, "");
    assert(rt_array_length(bytes) == 0);

    /* NULL string */
    bytes = rt_bytes_from_base64(arena, NULL);
    assert(rt_array_length(bytes) == 0);

    /* Binary data: "AP8A/w==" -> {0x00, 0xFF, 0x00, 0xFF} */
    bytes = rt_bytes_from_base64(arena, "AP8A/w==");
    assert(rt_array_length(bytes) == 4);
    assert(bytes[0] == 0x00);
    assert(bytes[1] == 0xFF);
    assert(bytes[2] == 0x00);
    assert(bytes[3] == 0xFF);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Roundtrip Tests (encode then decode)
 * ============================================================================ */

void test_hex_roundtrip()
{
    printf("Testing hex encode/decode roundtrip...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Create original byte array */
    unsigned char *original = rt_array_alloc_byte(arena, 10, 0);
    for (int i = 0; i < 10; i++) {
        original[i] = (unsigned char)(i * 25);  /* 0, 25, 50, 75, ... */
    }

    /* Encode to hex */
    char *hex = rt_byte_array_to_hex(arena, original);

    /* Decode back */
    unsigned char *decoded = rt_bytes_from_hex(arena, hex);

    /* Verify */
    assert(rt_array_length(decoded) == rt_array_length(original));
    for (size_t i = 0; i < rt_array_length(original); i++) {
        assert(decoded[i] == original[i]);
    }

    rt_arena_destroy(arena);
}

void test_base64_roundtrip()
{
    printf("Testing base64 encode/decode roundtrip...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Test with various lengths (to test padding cases) */
    for (int len = 1; len <= 10; len++) {
        unsigned char *original = rt_array_alloc_byte(arena, len, 0);
        for (int i = 0; i < len; i++) {
            original[i] = (unsigned char)(i * 17 + 33);
        }

        /* Encode to base64 */
        char *b64 = rt_byte_array_to_base64(arena, original);

        /* Decode back */
        unsigned char *decoded = rt_bytes_from_base64(arena, b64);

        /* Verify */
        assert(rt_array_length(decoded) == (size_t)len);
        for (int i = 0; i < len; i++) {
            assert(decoded[i] == original[i]);
        }
    }

    /* Test with all byte values */
    unsigned char *all_bytes = rt_array_alloc_byte(arena, 256, 0);
    for (int i = 0; i < 256; i++) {
        all_bytes[i] = (unsigned char)i;
    }

    char *b64 = rt_byte_array_to_base64(arena, all_bytes);
    unsigned char *decoded = rt_bytes_from_base64(arena, b64);

    assert(rt_array_length(decoded) == 256);
    for (int i = 0; i < 256; i++) {
        assert(decoded[i] == (unsigned char)i);
    }

    rt_arena_destroy(arena);
}

void test_string_bytes_roundtrip()
{
    printf("Testing string to bytes roundtrip...\n");

    RtArena *arena = rt_arena_create(NULL);

    const char *original = "Hello, World!";

    /* Convert to bytes */
    unsigned char *bytes = rt_string_to_bytes(arena, original);

    /* Convert back to string */
    char *result = rt_byte_array_to_string(arena, bytes);

    /* Verify */
    assert(strcmp(result, original) == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

void test_rt_byte_main()
{
    /* Byte array to string */
    test_rt_byte_array_to_string();
    test_rt_byte_array_to_string_latin1();

    /* Byte array to hex/base64 */
    test_rt_byte_array_to_hex();
    test_rt_byte_array_to_base64();

    /* String/hex/base64 to bytes */
    test_rt_string_to_bytes();
    test_rt_bytes_from_hex();
    test_rt_bytes_from_base64();

    /* Roundtrip tests */
    test_hex_roundtrip();
    test_base64_roundtrip();
    test_string_bytes_roundtrip();
}
