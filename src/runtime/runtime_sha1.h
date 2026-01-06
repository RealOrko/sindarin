#ifndef RUNTIME_SHA1_H
#define RUNTIME_SHA1_H

#include <stdint.h>
#include <stddef.h>

/* ============================================================================
 * SHA-1 Hash Algorithm (RFC 3174)
 * ============================================================================
 * This implementation provides SHA-1 hashing for UUIDv5 generation.
 * SHA-1 produces a 160-bit (20-byte) message digest.
 *
 * Note: SHA-1 is considered cryptographically weak for security applications
 * but is still required for UUIDv5 per RFC 9562.
 * ============================================================================ */

/* SHA-1 produces a 20-byte (160-bit) digest */
#define SHA1_DIGEST_SIZE 20

/* SHA-1 block size is 64 bytes (512 bits) */
#define SHA1_BLOCK_SIZE 64

/* SHA-1 context for incremental hashing */
typedef struct {
    uint32_t H[5];              /* Hash state (H0-H4) */
    uint8_t buffer[SHA1_BLOCK_SIZE];  /* Block buffer */
    size_t buffer_len;          /* Bytes in buffer */
    uint64_t total_len;         /* Total message length in bytes */
} SHA1_Context;

/* ============================================================================
 * SHA-1 API
 * ============================================================================ */

/* Initialize SHA-1 context with initial hash values */
void sha1_init(SHA1_Context *ctx);

/* Update SHA-1 context with additional data */
void sha1_update(SHA1_Context *ctx, const uint8_t *data, size_t len);

/* Finalize SHA-1 computation and output digest */
void sha1_final(SHA1_Context *ctx, uint8_t digest[SHA1_DIGEST_SIZE]);

/* One-shot SHA-1 hash computation */
void sha1_hash(const uint8_t *data, size_t len, uint8_t digest[SHA1_DIGEST_SIZE]);

/* ============================================================================
 * Internal Functions (exposed for testing)
 * ============================================================================ */

/* Pad a message block for SHA-1 processing
 *
 * Parameters:
 *   block       - Output buffer for padded block (must be 64 or 128 bytes)
 *   block_count - Output: number of blocks produced (1 or 2)
 *   data        - Input data to pad (can be partial block)
 *   data_len    - Length of input data (0 to 63 bytes)
 *   total_len   - Total message length in bytes (for length encoding)
 *
 * Returns: 0 on success, -1 on error
 *
 * SHA-1 padding rules (RFC 3174):
 *   1. Append '1' bit (0x80 byte)
 *   2. Append zero bits until length ≡ 448 (mod 512)
 *   3. Append original length as 64-bit big-endian integer
 */
int sha1_pad_message(uint8_t *block, int *block_count,
                     const uint8_t *data, size_t data_len, uint64_t total_len);

/* Process a single 512-bit (64-byte) block */
void sha1_process_block(uint32_t H[5], const uint8_t *block);

#endif /* RUNTIME_SHA1_H */
