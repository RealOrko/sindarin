#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "runtime_byte.h"
#include "runtime_array.h"

/* ============================================================================
 * Byte Array Conversion Functions
 * ============================================================================
 * Implementation of byte array to string and string to byte array conversions.
 * ============================================================================ */

/* ============================================================================
 * Byte Array to String Conversions
 * ============================================================================ */

/* Base64 encoding table */
static const char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* Convert byte array to string using UTF-8 decoding
 * Note: This is a simple passthrough since our strings are already UTF-8.
 * Invalid UTF-8 sequences are passed through as-is. */
char *rt_byte_array_to_string(RtArena *arena, unsigned char *bytes) {
    if (bytes == NULL) {
        char *result = rt_arena_alloc(arena, 1);
        result[0] = '\0';
        return result;
    }

    size_t len = rt_array_length(bytes);
    char *result = rt_arena_alloc(arena, len + 1);

    for (size_t i = 0; i < len; i++) {
        result[i] = (char)bytes[i];
    }
    result[len] = '\0';

    return result;
}

/* Convert byte array to string using Latin-1/ISO-8859-1 decoding
 * Each byte directly maps to its Unicode code point (0x00-0xFF).
 * This requires UTF-8 encoding for values 0x80-0xFF. */
char *rt_byte_array_to_string_latin1(RtArena *arena, unsigned char *bytes) {
    if (bytes == NULL) {
        char *result = rt_arena_alloc(arena, 1);
        result[0] = '\0';
        return result;
    }

    size_t len = rt_array_length(bytes);

    /* Calculate output size: bytes 0x00-0x7F = 1 byte, 0x80-0xFF = 2 bytes in UTF-8 */
    size_t out_len = 0;
    for (size_t i = 0; i < len; i++) {
        if (bytes[i] < 0x80) {
            out_len += 1;
        } else {
            out_len += 2;  /* UTF-8 encoding for 0x80-0xFF */
        }
    }

    char *result = rt_arena_alloc(arena, out_len + 1);
    size_t out_idx = 0;

    for (size_t i = 0; i < len; i++) {
        if (bytes[i] < 0x80) {
            result[out_idx++] = (char)bytes[i];
        } else {
            /* UTF-8 encoding for code points 0x80-0xFF: 110xxxxx 10xxxxxx */
            result[out_idx++] = (char)(0xC0 | (bytes[i] >> 6));
            result[out_idx++] = (char)(0x80 | (bytes[i] & 0x3F));
        }
    }
    result[out_idx] = '\0';

    return result;
}

/* Convert byte array to hexadecimal string */
char *rt_byte_array_to_hex(RtArena *arena, unsigned char *bytes) {
    static const char hex_chars[] = "0123456789abcdef";

    if (bytes == NULL) {
        char *result = rt_arena_alloc(arena, 1);
        result[0] = '\0';
        return result;
    }

    size_t len = rt_array_length(bytes);
    char *result = rt_arena_alloc(arena, len * 2 + 1);

    for (size_t i = 0; i < len; i++) {
        result[i * 2] = hex_chars[(bytes[i] >> 4) & 0xF];
        result[i * 2 + 1] = hex_chars[bytes[i] & 0xF];
    }
    result[len * 2] = '\0';

    return result;
}

/* Convert byte array to Base64 string */
char *rt_byte_array_to_base64(RtArena *arena, unsigned char *bytes) {
    if (bytes == NULL) {
        char *result = rt_arena_alloc(arena, 1);
        result[0] = '\0';
        return result;
    }

    size_t len = rt_array_length(bytes);

    /* Calculate output size: 4 output chars for every 3 input bytes, rounded up */
    size_t out_len = ((len + 2) / 3) * 4;
    char *result = rt_arena_alloc(arena, out_len + 1);

    size_t i = 0;
    size_t out_idx = 0;

    while (i + 2 < len) {
        /* Process 3 bytes at a time */
        unsigned int val = ((unsigned int)bytes[i] << 16) |
                          ((unsigned int)bytes[i + 1] << 8) |
                          ((unsigned int)bytes[i + 2]);

        result[out_idx++] = base64_chars[(val >> 18) & 0x3F];
        result[out_idx++] = base64_chars[(val >> 12) & 0x3F];
        result[out_idx++] = base64_chars[(val >> 6) & 0x3F];
        result[out_idx++] = base64_chars[val & 0x3F];

        i += 3;
    }

    /* Handle remaining bytes */
    if (i < len) {
        unsigned int val = (unsigned int)bytes[i] << 16;
        if (i + 1 < len) {
            val |= (unsigned int)bytes[i + 1] << 8;
        }

        result[out_idx++] = base64_chars[(val >> 18) & 0x3F];
        result[out_idx++] = base64_chars[(val >> 12) & 0x3F];

        if (i + 1 < len) {
            result[out_idx++] = base64_chars[(val >> 6) & 0x3F];
        } else {
            result[out_idx++] = '=';
        }
        result[out_idx++] = '=';
    }

    result[out_idx] = '\0';

    return result;
}

/* ============================================================================
 * String to Byte Array Conversions
 * ============================================================================ */

/* Convert string to UTF-8 byte array
 * Since our strings are already UTF-8, this is a simple copy. */
unsigned char *rt_string_to_bytes(RtArena *arena, const char *str) {
    if (str == NULL) {
        /* Return empty byte array */
        return rt_array_create_byte_uninit(arena, 0);
    }

    size_t len = strlen(str);
    unsigned char *bytes = rt_array_create_byte_uninit(arena, len);

    for (size_t i = 0; i < len; i++) {
        bytes[i] = (unsigned char)str[i];
    }

    return bytes;
}

/* Decode hexadecimal string to byte array */
unsigned char *rt_bytes_from_hex(RtArena *arena, const char *hex) {
    if (hex == NULL) {
        return rt_array_create_byte_uninit(arena, 0);
    }

    size_t hex_len = strlen(hex);

    /* Hex string must have even length */
    if (hex_len % 2 != 0) {
        fprintf(stderr, "Error: Hex string must have even length\n");
        exit(1);
    }

    size_t byte_len = hex_len / 2;
    unsigned char *bytes = rt_array_create_byte_uninit(arena, byte_len);

    for (size_t i = 0; i < byte_len; i++) {
        unsigned char hi = hex[i * 2];
        unsigned char lo = hex[i * 2 + 1];

        int hi_val, lo_val;

        /* Parse high nibble */
        if (hi >= '0' && hi <= '9') {
            hi_val = hi - '0';
        } else if (hi >= 'a' && hi <= 'f') {
            hi_val = hi - 'a' + 10;
        } else if (hi >= 'A' && hi <= 'F') {
            hi_val = hi - 'A' + 10;
        } else {
            fprintf(stderr, "Error: Invalid hex character '%c'\n", hi);
            exit(1);
        }

        /* Parse low nibble */
        if (lo >= '0' && lo <= '9') {
            lo_val = lo - '0';
        } else if (lo >= 'a' && lo <= 'f') {
            lo_val = lo - 'a' + 10;
        } else if (lo >= 'A' && lo <= 'F') {
            lo_val = lo - 'A' + 10;
        } else {
            fprintf(stderr, "Error: Invalid hex character '%c'\n", lo);
            exit(1);
        }

        bytes[i] = (unsigned char)((hi_val << 4) | lo_val);
    }

    return bytes;
}

/* Base64 decoding lookup table */
static const signed char base64_decode_table[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

/* Decode Base64 string to byte array */
unsigned char *rt_bytes_from_base64(RtArena *arena, const char *b64) {
    if (b64 == NULL) {
        return rt_array_create_byte_uninit(arena, 0);
    }

    size_t len = strlen(b64);
    if (len == 0) {
        return rt_array_create_byte_uninit(arena, 0);
    }

    /* Count padding characters */
    size_t padding = 0;
    if (len >= 1 && b64[len - 1] == '=') padding++;
    if (len >= 2 && b64[len - 2] == '=') padding++;

    /* Calculate output size: 3 output bytes for every 4 input chars */
    size_t out_len = (len / 4) * 3 - padding;
    unsigned char *bytes = rt_array_create_byte_uninit(arena, out_len);

    size_t i = 0;
    size_t out_idx = 0;

    while (i < len) {
        /* Skip whitespace */
        while (i < len && (b64[i] == ' ' || b64[i] == '\n' || b64[i] == '\r' || b64[i] == '\t')) {
            i++;
        }
        if (i >= len) break;

        /* Read 4 characters (some may be padding) */
        unsigned int vals[4] = {0, 0, 0, 0};
        int valid_chars = 0;

        for (int j = 0; j < 4 && i < len; j++, i++) {
            if (b64[i] == '=') {
                vals[j] = 0;
            } else {
                signed char val = base64_decode_table[(unsigned char)b64[i]];
                if (val < 0) {
                    fprintf(stderr, "Error: Invalid Base64 character '%c'\n", b64[i]);
                    exit(1);
                }
                vals[j] = (unsigned int)val;
                valid_chars++;
            }
        }

        /* Decode: combine 4 6-bit values into 3 8-bit values */
        unsigned int combined = (vals[0] << 18) | (vals[1] << 12) | (vals[2] << 6) | vals[3];

        if (out_idx < out_len) {
            bytes[out_idx++] = (unsigned char)((combined >> 16) & 0xFF);
        }
        if (out_idx < out_len && valid_chars >= 3) {
            bytes[out_idx++] = (unsigned char)((combined >> 8) & 0xFF);
        }
        if (out_idx < out_len && valid_chars >= 4) {
            bytes[out_idx++] = (unsigned char)(combined & 0xFF);
        }
    }

    return bytes;
}
