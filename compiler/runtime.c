#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include "runtime.h"

static const char *null_str = "(null)";

/* ============================================================================
 * Arena Memory Management Implementation
 * ============================================================================ */

/* Allocate a new arena block */
static RtArenaBlock *rt_arena_new_block(size_t size)
{
    RtArenaBlock *block = malloc(sizeof(RtArenaBlock) + size);
    if (block == NULL) {
        fprintf(stderr, "rt_arena_new_block: allocation failed\n");
        exit(1);
    }
    block->next = NULL;
    block->size = size;
    block->used = 0;
    return block;
}

/* Create a new arena with custom block size */
RtArena *rt_arena_create_sized(RtArena *parent, size_t block_size)
{
    if (block_size == 0) {
        block_size = RT_ARENA_DEFAULT_BLOCK_SIZE;
    }

    RtArena *arena = malloc(sizeof(RtArena));
    if (arena == NULL) {
        fprintf(stderr, "rt_arena_create_sized: allocation failed\n");
        exit(1);
    }

    arena->parent = parent;
    arena->default_block_size = block_size;
    arena->total_allocated = 0;
    arena->open_files = NULL;  /* Initialize file handle list to empty */

    /* Create initial block */
    arena->first = rt_arena_new_block(block_size);
    arena->current = arena->first;
    arena->total_allocated += sizeof(RtArenaBlock) + block_size;

    return arena;
}

/* Create a new arena with default block size */
RtArena *rt_arena_create(RtArena *parent)
{
    return rt_arena_create_sized(parent, RT_ARENA_DEFAULT_BLOCK_SIZE);
}

/* Align a pointer up to the given alignment */
static inline size_t rt_align_up(size_t value, size_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

/* Allocate aligned memory from arena */
void *rt_arena_alloc_aligned(RtArena *arena, size_t size, size_t alignment)
{
    if (arena == NULL || size == 0) {
        return NULL;
    }

    /* Ensure alignment is at least sizeof(void*) and a power of 2 */
    if (alignment < sizeof(void *)) {
        alignment = sizeof(void *);
    }

    RtArenaBlock *block = arena->current;

    /* Calculate the actual address where we'd allocate next */
    char *current_ptr = block->data + block->used;

    /* Calculate alignment padding needed for the actual memory address */
    size_t addr = (size_t)current_ptr;
    size_t aligned_addr = rt_align_up(addr, alignment);
    size_t padding = aligned_addr - addr;

    /* Calculate total space needed */
    size_t end_offset = block->used + padding + size;

    /* Check if allocation fits in current block */
    if (end_offset <= block->size) {
        block->used = end_offset;
        return (char *)aligned_addr;
    }

    /* Need a new block - calculate size needed */
    size_t needed = size + alignment;  /* Extra for alignment */
    size_t new_block_size = arena->default_block_size;
    if (needed > new_block_size) {
        new_block_size = needed;
    }

    /* Allocate new block */
    RtArenaBlock *new_block = rt_arena_new_block(new_block_size);
    arena->total_allocated += sizeof(RtArenaBlock) + new_block_size;

    /* Link new block */
    block->next = new_block;
    arena->current = new_block;

    /* Allocate from new block with proper alignment */
    current_ptr = new_block->data;
    addr = (size_t)current_ptr;
    aligned_addr = rt_align_up(addr, alignment);
    padding = aligned_addr - addr;

    new_block->used = padding + size;
    return (char *)aligned_addr;
}

/* Allocate memory from arena (uninitialized) */
void *rt_arena_alloc(RtArena *arena, size_t size)
{
    return rt_arena_alloc_aligned(arena, size, sizeof(void *));
}

/* Allocate zeroed memory from arena */
void *rt_arena_calloc(RtArena *arena, size_t count, size_t size)
{
    size_t total = count * size;
    void *ptr = rt_arena_alloc(arena, total);
    if (ptr != NULL) {
        memset(ptr, 0, total);
    }
    return ptr;
}

/* Duplicate a string into arena.
 * IMMUTABLE STRING: Creates a simple arena-allocated copy with NO metadata.
 * This function can remain unchanged - it creates immutable strings that are
 * compatible with all existing code. Use rt_string_from() if you need a
 * mutable string with metadata that can be efficiently appended to. */
char *rt_arena_strdup(RtArena *arena, const char *str)
{
    if (str == NULL) {
        return NULL;
    }
    size_t len = strlen(str);
    char *copy = rt_arena_alloc(arena, len + 1);
    if (copy != NULL) {
        memcpy(copy, str, len + 1);
    }
    return copy;
}

/* Duplicate n bytes of a string into arena */
char *rt_arena_strndup(RtArena *arena, const char *str, size_t n)
{
    if (str == NULL) {
        return NULL;
    }
    size_t len = strlen(str);
    if (n < len) {
        len = n;
    }
    char *copy = rt_arena_alloc(arena, len + 1);
    if (copy != NULL) {
        memcpy(copy, str, len);
        copy[len] = '\0';
    }
    return copy;
}

/* Destroy arena and free all memory */
void rt_arena_destroy(RtArena *arena)
{
    if (arena == NULL) {
        return;
    }

    /* Close all tracked file handles first */
    RtFileHandle *fh = arena->open_files;
    while (fh != NULL) {
        if (fh->is_open && fh->fp != NULL) {
            fclose((FILE *)fh->fp);
            fh->is_open = false;
        }
        fh = fh->next;
    }

    /* Free all blocks */
    RtArenaBlock *block = arena->first;
    while (block != NULL) {
        RtArenaBlock *next = block->next;
        free(block);
        block = next;
    }

    free(arena);
}

/* Reset arena for reuse (keeps first block, frees rest) */
void rt_arena_reset(RtArena *arena)
{
    if (arena == NULL) {
        return;
    }

    /* Close all tracked file handles first */
    RtFileHandle *fh = arena->open_files;
    while (fh != NULL) {
        if (fh->is_open && fh->fp != NULL) {
            fclose((FILE *)fh->fp);
            fh->is_open = false;
        }
        fh = fh->next;
    }
    arena->open_files = NULL;

    /* Free all blocks except the first */
    RtArenaBlock *block = arena->first->next;
    while (block != NULL) {
        RtArenaBlock *next = block->next;
        free(block);
        block = next;
    }

    /* Reset first block */
    arena->first->next = NULL;
    arena->first->used = 0;
    arena->current = arena->first;
    arena->total_allocated = sizeof(RtArenaBlock) + arena->first->size;
}

/* Copy data from one arena to another (for promotion) */
void *rt_arena_promote(RtArena *dest, const void *src, size_t size)
{
    if (dest == NULL || src == NULL || size == 0) {
        return NULL;
    }
    void *copy = rt_arena_alloc(dest, size);
    if (copy != NULL) {
        memcpy(copy, src, size);
    }
    return copy;
}

/* Copy string from one arena to another (for promotion) */
char *rt_arena_promote_string(RtArena *dest, const char *src)
{
    return rt_arena_strdup(dest, src);
}

/* Get total bytes allocated by arena */
size_t rt_arena_total_allocated(RtArena *arena)
{
    if (arena == NULL) {
        return 0;
    }
    return arena->total_allocated;
}

/* ============================================================================
 * File Handle Tracking Implementation
 * ============================================================================ */

/* Track a file handle in an arena */
RtFileHandle *rt_arena_track_file(RtArena *arena, void *fp, const char *path, bool is_text)
{
    if (arena == NULL || fp == NULL) {
        return NULL;
    }

    /* Allocate handle from arena */
    RtFileHandle *handle = rt_arena_alloc(arena, sizeof(RtFileHandle));
    if (handle == NULL) {
        return NULL;
    }

    handle->fp = fp;
    handle->path = rt_arena_strdup(arena, path);
    handle->is_open = true;
    handle->is_text = is_text;

    /* Add to front of arena's file list */
    handle->next = arena->open_files;
    arena->open_files = handle;

    return handle;
}

/* Untrack a file handle from an arena (removes from list but doesn't close) */
void rt_arena_untrack_file(RtArena *arena, RtFileHandle *handle)
{
    if (arena == NULL || handle == NULL) {
        return;
    }

    /* Find and remove from list */
    RtFileHandle **curr = &arena->open_files;
    while (*curr != NULL) {
        if (*curr == handle) {
            *curr = handle->next;
            handle->next = NULL;
            return;
        }
        curr = &(*curr)->next;
    }
}

/* Promote a text file handle to a destination arena */
RtTextFile *rt_text_file_promote(RtArena *dest, RtArena *src_arena, RtTextFile *src)
{
    if (dest == NULL || src == NULL || !src->is_open) {
        return NULL;
    }

    /* Allocate new handle in destination arena */
    RtTextFile *promoted = rt_arena_alloc(dest, sizeof(RtTextFile));
    if (promoted == NULL) {
        return NULL;
    }

    /* Copy file state */
    promoted->fp = src->fp;
    promoted->path = rt_arena_promote_string(dest, src->path);
    promoted->is_open = src->is_open;

    /* Track in destination arena */
    promoted->handle = rt_arena_track_file(dest, src->fp, promoted->path, true);

    /* Untrack from source arena (file is no longer owned by source) */
    if (src->handle != NULL) {
        rt_arena_untrack_file(src_arena, src->handle);
    }

    /* Mark source as closed (ownership transferred) */
    src->is_open = false;
    src->fp = NULL;

    return promoted;
}

/* Promote a binary file handle to a destination arena */
RtBinaryFile *rt_binary_file_promote(RtArena *dest, RtArena *src_arena, RtBinaryFile *src)
{
    if (dest == NULL || src == NULL || !src->is_open) {
        return NULL;
    }

    /* Allocate new handle in destination arena */
    RtBinaryFile *promoted = rt_arena_alloc(dest, sizeof(RtBinaryFile));
    if (promoted == NULL) {
        return NULL;
    }

    /* Copy file state */
    promoted->fp = src->fp;
    promoted->path = rt_arena_promote_string(dest, src->path);
    promoted->is_open = src->is_open;

    /* Track in destination arena */
    promoted->handle = rt_arena_track_file(dest, src->fp, promoted->path, false);

    /* Untrack from source arena (file is no longer owned by source) */
    if (src->handle != NULL) {
        rt_arena_untrack_file(src_arena, src->handle);
    }

    /* Mark source as closed (ownership transferred) */
    src->is_open = false;
    src->fp = NULL;

    return promoted;
}

/* Concatenate two strings into a new string.
 * IMMUTABLE STRING: Creates a new arena-allocated string with NO metadata.
 * This function can remain unchanged - it creates immutable strings and
 * always allocates a new string for the result (no in-place modification).
 * This is the appropriate behavior for functional-style string operations.
 *
 * TODO: Future optimization - if left string is mutable and has enough
 * capacity, could append in-place using rt_string_append() instead. */
char *rt_str_concat(RtArena *arena, const char *left, const char *right) {
    const char *l = left ? left : "";
    const char *r = right ? right : "";
    size_t left_len = strlen(l);
    size_t right_len = strlen(r);
    size_t new_len = left_len + right_len;
    if (new_len > (1UL << 30) - 1) {
        return NULL;
    }
    char *new_str = rt_arena_alloc(arena, new_len + 1);
    if (new_str == NULL) {
        return NULL;
    }
    memcpy(new_str, l, left_len);
    memcpy(new_str + left_len, r, right_len + 1);
    return new_str;
}

/* ============================================================================
 * MUTABLE String Functions
 *
 * These functions create and manipulate strings WITH RtStringMeta, enabling
 * efficient append operations and O(1) length queries. See runtime.h for
 * detailed documentation on mutable vs immutable strings.
 * ============================================================================ */

/* Create a mutable string with specified capacity.
 * Allocates RtStringMeta + capacity + 1 bytes, initializes metadata,
 * and returns pointer to the string data (after metadata).
 * The string is initialized as empty (length=0, str[0]='\0'). */
char *rt_string_with_capacity(RtArena *arena, size_t capacity) {
    /* Validate arena */
    if (arena == NULL) {
        fprintf(stderr, "rt_string_with_capacity: arena is NULL\n");
        exit(1);
    }

    /* Validate capacity to prevent overflow (limit to 1GB) */
    if (capacity > (1UL << 30)) {
        fprintf(stderr, "rt_string_with_capacity: capacity too large (%zu)\n", capacity);
        exit(1);
    }

    size_t total = sizeof(RtStringMeta) + capacity + 1;
    RtStringMeta *meta = rt_arena_alloc(arena, total);
    if (meta == NULL) {
        fprintf(stderr, "rt_string_with_capacity: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->length = 0;
    meta->capacity = capacity;
    char *str = (char*)(meta + 1);
    str[0] = '\0';
    return str;
}

/* Create a mutable string from an immutable source string.
 * Copies the content into a new mutable string with metadata. */
char *rt_string_from(RtArena *arena, const char *src) {
    if (arena == NULL) {
        fprintf(stderr, "rt_string_from: arena is NULL\n");
        exit(1);
    }

    size_t len = src ? strlen(src) : 0;
    /* Allocate with some extra capacity to allow appending */
    size_t capacity = len < 16 ? 32 : len * 2;

    char *str = rt_string_with_capacity(arena, capacity);
    if (src && len > 0) {
        memcpy(str, src, len);
        str[len] = '\0';
        RT_STR_META(str)->length = len;
    }
    return str;
}

/* Ensure a string is mutable. If it's already mutable (has valid metadata),
 * returns it unchanged. If it's immutable, converts it to a mutable string.
 * This is used before append operations on strings that may be immutable.
 *
 * SAFETY: We use a magic number approach to identify mutable strings.
 * Mutable strings have a specific pattern in their metadata that immutable
 * strings (from arena_strdup or string literals) cannot have.
 */
char *rt_string_ensure_mutable(RtArena *arena, char *str) {
    if (str == NULL) {
        /* NULL becomes an empty mutable string */
        return rt_string_with_capacity(arena, 32);
    }

    /* Check if this pointer points into our arena's memory.
     * Mutable strings created by rt_string_with_capacity have their
     * metadata stored immediately before the string data.
     *
     * We check if the arena pointer in metadata matches AND is non-NULL.
     * For safety, we also verify the capacity is reasonable.
     *
     * For strings NOT created by rt_string_with_capacity (e.g., rt_arena_strdup
     * or string literals), the bytes before them are NOT our metadata.
     *
     * To avoid accessing invalid memory, we take a conservative approach:
     * We only check for strings that were previously returned from
     * rt_string_ensure_mutable or rt_string_with_capacity - which we identify
     * by checking if the alleged metadata has a valid-looking arena pointer
     * and reasonable capacity value.
     */
    RtStringMeta *meta = RT_STR_META(str);

    /* Validate that this looks like a valid mutable string:
     * 1. Arena pointer matches our arena
     * 2. Capacity is reasonable (not garbage)
     * 3. Length is <= capacity
     */
    if (meta->arena == arena &&
        meta->capacity > 0 &&
        meta->capacity < (1UL << 30) &&
        meta->length <= meta->capacity) {
        return str;
    }

    /* Otherwise, convert to mutable */
    return rt_string_from(arena, str);
}

/* Append a string to a mutable string (in-place if capacity allows).
 * Returns dest pointer - may be different from input if reallocation occurred.
 * Uses 2x growth strategy when capacity is exceeded. */
char *rt_string_append(char *dest, const char *src) {
    /* Validate inputs */
    if (dest == NULL) {
        fprintf(stderr, "rt_string_append: dest is NULL\n");
        exit(1);
    }
    if (src == NULL) {
        return dest;  /* Appending NULL is a no-op */
    }

    /* Get metadata and validate it's a mutable string */
    RtStringMeta *meta = RT_STR_META(dest);
    if (meta->arena == NULL) {
        fprintf(stderr, "rt_string_append: dest is not a mutable string (arena is NULL)\n");
        exit(1);
    }

    /* Calculate lengths */
    size_t src_len = strlen(src);
    size_t new_len = meta->length + src_len;

    /* Check for length overflow */
    if (new_len < meta->length) {
        fprintf(stderr, "rt_string_append: string length overflow\n");
        exit(1);
    }

    /* Save current length before potential reallocation */
    size_t old_len = meta->length;

    /* Check if we need to grow the buffer */
    if (new_len + 1 > meta->capacity) {
        /* Grow by 2x to amortize allocation cost */
        size_t new_cap = (new_len + 1) * 2;

        /* Check for capacity overflow */
        if (new_cap < new_len + 1 || new_cap > (1UL << 30)) {
            fprintf(stderr, "rt_string_append: capacity overflow (%zu)\n", new_cap);
            exit(1);
        }

        char *new_str = rt_string_with_capacity(meta->arena, new_cap);

        /* Copy existing content to new buffer */
        memcpy(new_str, dest, old_len);

        /* Update dest and meta to point to new buffer */
        dest = new_str;
        meta = RT_STR_META(dest);
    }

    /* Append the source string (including null terminator) */
    memcpy(dest + old_len, src, src_len + 1);
    meta->length = new_len;

    return dest;
}

/* ============================================================================
 * Type Conversion Functions (rt_to_string_*)
 *
 * IMMUTABLE STRINGS: All rt_to_string_* functions create immutable strings
 * via rt_arena_strdup(). These functions can remain unchanged because:
 * 1. They create short, fixed-size strings that don't need appending
 * 2. The results are typically used in interpolation/concatenation
 * 3. No benefit to adding metadata for these small strings
 *
 * TODO: If profiling shows these are bottlenecks in hot loops, consider
 * caching common values (e.g., "true"/"false", small integers 0-99).
 * ============================================================================ */

char *rt_to_string_long(RtArena *arena, long val)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", val);
    return rt_arena_strdup(arena, buf);
}

char *rt_to_string_double(RtArena *arena, double val)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%.5f", val);
    return rt_arena_strdup(arena, buf);
}

char *rt_to_string_char(RtArena *arena, char val)
{
    char buf[2] = {val, '\0'};
    return rt_arena_strdup(arena, buf);
}

char *rt_to_string_bool(RtArena *arena, int val)
{
    return rt_arena_strdup(arena, val ? "true" : "false");
}

char *rt_to_string_byte(RtArena *arena, unsigned char val)
{
    char buf[8];
    snprintf(buf, sizeof(buf), "0x%02X", val);
    return rt_arena_strdup(arena, buf);
}

char *rt_to_string_string(RtArena *arena, const char *val)
{
    if (val == NULL) {
        return (char *)null_str;
    }
    return rt_arena_strdup(arena, val);
}

char *rt_to_string_void(RtArena *arena)
{
    return rt_arena_strdup(arena, "void");
}

char *rt_to_string_pointer(RtArena *arena, void *p)
{
    if (p == NULL) {
        return rt_arena_strdup(arena, "nil");
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "%p", p);
    return rt_arena_strdup(arena, buf);
}

/* Format specifier support for string interpolation */

char *rt_format_long(RtArena *arena, long val, const char *fmt)
{
    char buf[128];
    char format_str[64];

    if (fmt == NULL || fmt[0] == '\0') {
        snprintf(buf, sizeof(buf), "%ld", val);
        return rt_arena_strdup(arena, buf);
    }

    /* Parse format specifier: [width][type] where type is d, x, X, o, b */
    int width = 0;
    int zero_pad = 0;
    const char *p = fmt;

    /* Check for zero padding */
    if (*p == '0') {
        zero_pad = 1;
        p++;
    }

    /* Parse width */
    while (*p >= '0' && *p <= '9') {
        width = width * 10 + (*p - '0');
        p++;
    }

    /* Determine type */
    char type = *p ? *p : 'd';

    switch (type) {
        case 'd':
            if (zero_pad && width > 0) {
                snprintf(format_str, sizeof(format_str), "%%0%dld", width);
            } else if (width > 0) {
                snprintf(format_str, sizeof(format_str), "%%%dld", width);
            } else {
                snprintf(format_str, sizeof(format_str), "%%ld");
            }
            snprintf(buf, sizeof(buf), format_str, val);
            break;
        case 'x':
            if (zero_pad && width > 0) {
                snprintf(format_str, sizeof(format_str), "%%0%dlx", width);
            } else if (width > 0) {
                snprintf(format_str, sizeof(format_str), "%%%dlx", width);
            } else {
                snprintf(format_str, sizeof(format_str), "%%lx");
            }
            snprintf(buf, sizeof(buf), format_str, val);
            break;
        case 'X':
            if (zero_pad && width > 0) {
                snprintf(format_str, sizeof(format_str), "%%0%dlX", width);
            } else if (width > 0) {
                snprintf(format_str, sizeof(format_str), "%%%dlX", width);
            } else {
                snprintf(format_str, sizeof(format_str), "%%lX");
            }
            snprintf(buf, sizeof(buf), format_str, val);
            break;
        case 'o':
            if (zero_pad && width > 0) {
                snprintf(format_str, sizeof(format_str), "%%0%dlo", width);
            } else if (width > 0) {
                snprintf(format_str, sizeof(format_str), "%%%dlo", width);
            } else {
                snprintf(format_str, sizeof(format_str), "%%lo");
            }
            snprintf(buf, sizeof(buf), format_str, val);
            break;
        case 'b': {
            /* Binary format - custom implementation */
            char binbuf[65];
            int len = 0;
            unsigned long uval = (unsigned long)val;
            if (uval == 0) {
                binbuf[len++] = '0';
            } else {
                while (uval > 0) {
                    binbuf[len++] = (uval & 1) ? '1' : '0';
                    uval >>= 1;
                }
            }
            /* Reverse the string */
            for (int i = 0; i < len / 2; i++) {
                char tmp = binbuf[i];
                binbuf[i] = binbuf[len - 1 - i];
                binbuf[len - 1 - i] = tmp;
            }
            /* Pad if needed */
            if (width > len) {
                int pad = width - len;
                memmove(binbuf + pad, binbuf, len);
                for (int i = 0; i < pad; i++) {
                    binbuf[i] = zero_pad ? '0' : ' ';
                }
                len = width;
            }
            binbuf[len] = '\0';
            return rt_arena_strdup(arena, binbuf);
        }
        default:
            snprintf(buf, sizeof(buf), "%ld", val);
            break;
    }

    return rt_arena_strdup(arena, buf);
}

char *rt_format_double(RtArena *arena, double val, const char *fmt)
{
    char buf[128];
    char format_str[64];

    if (fmt == NULL || fmt[0] == '\0') {
        snprintf(buf, sizeof(buf), "%g", val);
        return rt_arena_strdup(arena, buf);
    }

    /* Parse format specifier: [width][.precision][type] where type is f, e, E, g, G, % */
    int width = 0;
    int precision = -1;
    int zero_pad = 0;
    const char *p = fmt;

    /* Check for zero padding */
    if (*p == '0') {
        zero_pad = 1;
        p++;
    }

    /* Parse width */
    while (*p >= '0' && *p <= '9') {
        width = width * 10 + (*p - '0');
        p++;
    }

    /* Parse precision */
    if (*p == '.') {
        p++;
        precision = 0;
        while (*p >= '0' && *p <= '9') {
            precision = precision * 10 + (*p - '0');
            p++;
        }
    }

    /* Determine type */
    char type = *p ? *p : 'f';

    /* Handle percentage format */
    if (type == '%') {
        val *= 100.0;
        if (precision >= 0) {
            snprintf(format_str, sizeof(format_str), "%%.%df%%%%", precision);
        } else {
            snprintf(format_str, sizeof(format_str), "%%f%%%%");
        }
        snprintf(buf, sizeof(buf), format_str, val);
        return rt_arena_strdup(arena, buf);
    }

    /* Build format string */
    int pos = 0;
    format_str[pos++] = '%';
    if (zero_pad) format_str[pos++] = '0';
    if (width > 0) {
        pos += snprintf(format_str + pos, sizeof(format_str) - pos, "%d", width);
    }
    if (precision >= 0) {
        pos += snprintf(format_str + pos, sizeof(format_str) - pos, ".%d", precision);
    }

    switch (type) {
        case 'f':
            format_str[pos++] = 'f';
            break;
        case 'e':
            format_str[pos++] = 'e';
            break;
        case 'E':
            format_str[pos++] = 'E';
            break;
        case 'g':
            format_str[pos++] = 'g';
            break;
        case 'G':
            format_str[pos++] = 'G';
            break;
        default:
            format_str[pos++] = 'f';
            break;
    }
    format_str[pos] = '\0';

    snprintf(buf, sizeof(buf), format_str, val);
    return rt_arena_strdup(arena, buf);
}

char *rt_format_string(RtArena *arena, const char *val, const char *fmt)
{
    if (val == NULL) {
        val = "nil";
    }

    if (fmt == NULL || fmt[0] == '\0') {
        return rt_arena_strdup(arena, val);
    }

    /* Parse format specifier: [width][.maxlen]s */
    int width = 0;
    int maxlen = -1;
    int left_align = 0;
    const char *p = fmt;

    /* Check for left alignment */
    if (*p == '-') {
        left_align = 1;
        p++;
    }

    /* Parse width */
    while (*p >= '0' && *p <= '9') {
        width = width * 10 + (*p - '0');
        p++;
    }

    /* Parse max length */
    if (*p == '.') {
        p++;
        maxlen = 0;
        while (*p >= '0' && *p <= '9') {
            maxlen = maxlen * 10 + (*p - '0');
            p++;
        }
    }

    int len = strlen(val);

    /* Apply max length truncation */
    if (maxlen >= 0 && len > maxlen) {
        len = maxlen;
    }

    /* Calculate output size */
    int out_len = len;
    if (width > len) {
        out_len = width;
    }

    char *result = rt_arena_alloc(arena, out_len + 1);
    if (result == NULL) return NULL;

    if (width > len) {
        if (left_align) {
            memcpy(result, val, len);
            memset(result + len, ' ', width - len);
        } else {
            memset(result, ' ', width - len);
            memcpy(result + (width - len), val, len);
        }
        result[width] = '\0';
    } else {
        memcpy(result, val, len);
        result[len] = '\0';
    }

    return result;
}

void rt_print_long(long val)
{
    printf("%ld", val);
}

void rt_print_double(double val)
{
    if (isnan(val))
    {
        printf("NaN");
    }
    else if (isinf(val))
    {
        if (val > 0)
        {
            printf("Inf");
        }
        else
        {
            printf("-Inf");
        }
    }
    else
    {
        printf("%.5f", val);
    }
}

void rt_print_char(long c)
{
    if (c < 0 || c > 255)
    {
        fprintf(stderr, "rt_print_char: invalid char value %ld (must be 0-255)\n", c);
        printf("?");
    }
    else
    {
        printf("%c", (int)c);
    }
}

void rt_print_string(const char *s)
{
    if (s == NULL)
    {
        printf("(null)");
    }
    else
    {
        printf("%s", s);
    }
}

void rt_print_bool(long b)
{
    printf("%s", b ? "true" : "false");
}

void rt_print_byte(unsigned char b)
{
    printf("0x%02X", b);
}

long rt_add_long(long a, long b)
{
    if ((b > 0 && a > LONG_MAX - b) || (b < 0 && a < LONG_MIN - b))
    {
        fprintf(stderr, "rt_add_long: overflow detected\n");
        exit(1);
    }
    return a + b;
}

long rt_sub_long(long a, long b)
{
    if ((b > 0 && a < LONG_MIN + b) || (b < 0 && a > LONG_MAX + b))
    {
        fprintf(stderr, "rt_sub_long: overflow detected\n");
        exit(1);
    }
    return a - b;
}

long rt_mul_long(long a, long b)
{
    if (a != 0 && b != 0 && ((a > LONG_MAX / b) || (a < LONG_MIN / b)))
    {
        fprintf(stderr, "rt_mul_long: overflow detected\n");
        exit(1);
    }
    return a * b;
}

long rt_div_long(long a, long b)
{
    if (b == 0)
    {
        fprintf(stderr, "Division by zero\n");
        exit(1);
    }
    if (a == LONG_MIN && b == -1)
    {
        fprintf(stderr, "rt_div_long: overflow detected (LONG_MIN / -1)\n");
        exit(1);
    }
    return a / b;
}

long rt_mod_long(long a, long b)
{
    if (b == 0)
    {
        fprintf(stderr, "Modulo by zero\n");
        exit(1);
    }
    if (a == LONG_MIN && b == -1)
    {
        fprintf(stderr, "rt_mod_long: overflow detected (LONG_MIN %% -1)\n");
        exit(1);
    }
    return a % b;
}

/* rt_eq_long, rt_ne_long, rt_lt_long, rt_le_long, rt_gt_long, rt_ge_long
 * are defined as static inline in runtime.h for inlining optimization */

double rt_add_double(double a, double b)
{
    double result = a + b;
    if (isinf(result) && !isinf(a) && !isinf(b))
    {
        fprintf(stderr, "rt_add_double: overflow to infinity\n");
        exit(1);
    }
    return result;
}

double rt_sub_double(double a, double b)
{
    double result = a - b;
    if (isinf(result) && !isinf(a) && !isinf(b))
    {
        fprintf(stderr, "rt_sub_double: overflow to infinity\n");
        exit(1);
    }
    return result;
}

double rt_mul_double(double a, double b)
{
    double result = a * b;
    if (isinf(result) && !isinf(a) && !isinf(b))
    {
        fprintf(stderr, "rt_mul_double: overflow to infinity\n");
        exit(1);
    }
    return result;
}

double rt_div_double(double a, double b)
{
    if (b == 0.0)
    {
        fprintf(stderr, "Division by zero\n");
        exit(1);
    }
    double result = a / b;
    if (isinf(result) && !isinf(a) && b != 0.0)
    {
        fprintf(stderr, "rt_div_double: overflow to infinity\n");
        exit(1);
    }
    return result;
}

/* rt_eq_double, rt_ne_double, rt_lt_double, rt_le_double, rt_gt_double, rt_ge_double
 * are defined as static inline in runtime.h for inlining optimization */

long rt_neg_long(long a)
{
    if (a == LONG_MIN)
    {
        fprintf(stderr, "rt_neg_long: overflow detected (-LONG_MIN)\n");
        exit(1);
    }
    return -a;
}

double rt_neg_double(double a) { return -a; }

/* rt_not_bool is defined as static inline in runtime.h */

long rt_post_inc_long(long *p)
{
    if (p == NULL)
    {
        fprintf(stderr, "rt_post_inc_long: NULL pointer\n");
        exit(1);
    }
    if (*p == LONG_MAX)
    {
        fprintf(stderr, "rt_post_inc_long: overflow detected\n");
        exit(1);
    }
    return (*p)++;
}

long rt_post_dec_long(long *p)
{
    if (p == NULL)
    {
        fprintf(stderr, "rt_post_dec_long: NULL pointer\n");
        exit(1);
    }
    if (*p == LONG_MIN)
    {
        fprintf(stderr, "rt_post_dec_long: overflow detected\n");
        exit(1);
    }
    return (*p)--;
}

/* rt_eq_string, rt_ne_string, rt_lt_string, rt_le_string, rt_gt_string, rt_ge_string
 * are defined as static inline in runtime.h for inlining optimization */

/* ArrayMetadata is now defined in runtime.h as RtArrayMetadata for inlining.
 * We use a local typedef for compatibility with existing code. */
typedef RtArrayMetadata ArrayMetadata;

/*
 * Macro to generate type-safe array push functions.
 * Uses arena allocation - when capacity is exceeded, allocates new array and copies.
 * The arena parameter is used only for NEW arrays; existing arrays use their stored arena.
 */
#define DEFINE_ARRAY_PUSH(suffix, elem_type, assign_expr)                      \
elem_type *rt_array_push_##suffix(RtArena *arena, elem_type *arr, elem_type element) { \
    ArrayMetadata *meta;                                                       \
    elem_type *new_arr;                                                        \
    size_t new_capacity;                                                       \
    RtArena *alloc_arena;                                                      \
                                                                               \
    if (arr == NULL) {                                                         \
        meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + 4 * sizeof(elem_type)); \
        if (meta == NULL) {                                                    \
            fprintf(stderr, "rt_array_push_" #suffix ": allocation failed\n"); \
            exit(1);                                                           \
        }                                                                      \
        meta->arena = arena;                                                   \
        meta->size = 1;                                                        \
        meta->capacity = 4;                                                    \
        new_arr = (elem_type *)(meta + 1);                                     \
        new_arr[0] = assign_expr;                                              \
        return new_arr;                                                        \
    }                                                                          \
                                                                               \
    meta = ((ArrayMetadata *)arr) - 1;                                         \
    alloc_arena = meta->arena ? meta->arena : arena;                           \
                                                                               \
    if (meta->size >= meta->capacity) {                                        \
        new_capacity = meta->capacity * 2;                                     \
        if (new_capacity < meta->capacity) {                                   \
            fprintf(stderr, "rt_array_push_" #suffix ": capacity overflow\n"); \
            exit(1);                                                           \
        }                                                                      \
        ArrayMetadata *new_meta = rt_arena_alloc(alloc_arena, sizeof(ArrayMetadata) + new_capacity * sizeof(elem_type)); \
        if (new_meta == NULL) {                                                \
            fprintf(stderr, "rt_array_push_" #suffix ": allocation failed\n"); \
            exit(1);                                                           \
        }                                                                      \
        new_meta->arena = alloc_arena;                                         \
        new_meta->size = meta->size;                                           \
        new_meta->capacity = new_capacity;                                     \
        new_arr = (elem_type *)(new_meta + 1);                                 \
        memcpy(new_arr, arr, meta->size * sizeof(elem_type));                  \
        meta = new_meta;                                                       \
    } else {                                                                   \
        new_arr = arr;                                                         \
    }                                                                          \
                                                                               \
    new_arr[meta->size] = assign_expr;                                         \
    meta->size++;                                                              \
    return new_arr;                                                            \
}

/* Generate array push functions for each type */
DEFINE_ARRAY_PUSH(long, long, element)
DEFINE_ARRAY_PUSH(double, double, element)
DEFINE_ARRAY_PUSH(char, char, element)
DEFINE_ARRAY_PUSH(bool, int, element)
DEFINE_ARRAY_PUSH(byte, unsigned char, element)
DEFINE_ARRAY_PUSH(ptr, void *, element)  /* For closures/function pointers and other pointer types */

/* String arrays need special handling for strdup */
char **rt_array_push_string(RtArena *arena, char **arr, const char *element) {
    ArrayMetadata *meta;
    char **new_arr;
    size_t new_capacity;
    RtArena *alloc_arena;

    if (arr == NULL) {
        meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + 4 * sizeof(char *));
        if (meta == NULL) {
            fprintf(stderr, "rt_array_push_string: allocation failed\n");
            exit(1);
        }
        meta->arena = arena;
        meta->size = 1;
        meta->capacity = 4;
        new_arr = (char **)(meta + 1);
        new_arr[0] = element ? rt_arena_strdup(arena, element) : NULL;
        return new_arr;
    }

    meta = ((ArrayMetadata *)arr) - 1;
    alloc_arena = meta->arena ? meta->arena : arena;

    if (meta->size >= meta->capacity) {
        new_capacity = meta->capacity * 2;
        if (new_capacity < meta->capacity) {
            fprintf(stderr, "rt_array_push_string: capacity overflow\n");
            exit(1);
        }
        ArrayMetadata *new_meta = rt_arena_alloc(alloc_arena, sizeof(ArrayMetadata) + new_capacity * sizeof(char *));
        if (new_meta == NULL) {
            fprintf(stderr, "rt_array_push_string: allocation failed\n");
            exit(1);
        }
        new_meta->arena = alloc_arena;
        new_meta->size = meta->size;
        new_meta->capacity = new_capacity;
        new_arr = (char **)(new_meta + 1);
        memcpy(new_arr, arr, meta->size * sizeof(char *));
        meta = new_meta;
    } else {
        new_arr = arr;
    }

    new_arr[meta->size] = element ? rt_arena_strdup(alloc_arena, element) : NULL;
    meta->size++;
    return new_arr;
}

/* rt_array_length is defined as static inline in runtime.h */

/* Print array functions */
void rt_print_array_long(long *arr) {
    printf("[");
    if (arr != NULL) {
        size_t len = rt_array_length(arr);
        for (size_t i = 0; i < len; i++) {
            if (i > 0) printf(", ");
            printf("%ld", arr[i]);
        }
    }
    printf("]");
}

void rt_print_array_double(double *arr) {
    printf("[");
    if (arr != NULL) {
        size_t len = rt_array_length(arr);
        for (size_t i = 0; i < len; i++) {
            if (i > 0) printf(", ");
            printf("%.5f", arr[i]);
        }
    }
    printf("]");
}

void rt_print_array_char(char *arr) {
    printf("[");
    if (arr != NULL) {
        size_t len = rt_array_length(arr);
        for (size_t i = 0; i < len; i++) {
            if (i > 0) printf(", ");
            printf("'%c'", arr[i]);
        }
    }
    printf("]");
}

void rt_print_array_bool(int *arr) {
    printf("[");
    if (arr != NULL) {
        size_t len = rt_array_length(arr);
        for (size_t i = 0; i < len; i++) {
            if (i > 0) printf(", ");
            printf("%s", arr[i] ? "true" : "false");
        }
    }
    printf("]");
}

void rt_print_array_byte(unsigned char *arr) {
    printf("[");
    if (arr != NULL) {
        size_t len = rt_array_length(arr);
        for (size_t i = 0; i < len; i++) {
            if (i > 0) printf(", ");
            printf("0x%02X", arr[i]);
        }
    }
    printf("]");
}

void rt_print_array_string(char **arr) {
    printf("[");
    if (arr != NULL) {
        size_t len = rt_array_length(arr);
        for (size_t i = 0; i < len; i++) {
            if (i > 0) printf(", ");
            printf("\"%s\"", arr[i] ? arr[i] : "(null)");
        }
    }
    printf("]");
}

/* Clear array - sets size to 0 but keeps capacity */
void rt_array_clear(void *arr) {
    if (arr == NULL) {
        return;
    }
    ArrayMetadata *meta = ((ArrayMetadata *)arr) - 1;
    meta->size = 0;
}

/* Pop functions - remove and return last element */
#define DEFINE_ARRAY_POP(suffix, elem_type, default_val)                       \
elem_type rt_array_pop_##suffix(elem_type *arr) {                              \
    if (arr == NULL) {                                                         \
        fprintf(stderr, "rt_array_pop_" #suffix ": NULL array\n");             \
        exit(1);                                                               \
    }                                                                          \
    ArrayMetadata *meta = ((ArrayMetadata *)arr) - 1;                          \
    if (meta->size == 0) {                                                     \
        fprintf(stderr, "rt_array_pop_" #suffix ": empty array\n");            \
        exit(1);                                                               \
    }                                                                          \
    meta->size--;                                                              \
    return arr[meta->size];                                                    \
}

DEFINE_ARRAY_POP(long, long, 0)
DEFINE_ARRAY_POP(double, double, 0.0)
DEFINE_ARRAY_POP(char, char, '\0')
DEFINE_ARRAY_POP(bool, int, 0)
DEFINE_ARRAY_POP(byte, unsigned char, 0)
DEFINE_ARRAY_POP(ptr, void *, NULL)  /* For closures/function pointers and other pointer types */

char *rt_array_pop_string(char **arr) {
    if (arr == NULL) {
        fprintf(stderr, "rt_array_pop_string: NULL array\n");
        exit(1);
    }
    ArrayMetadata *meta = ((ArrayMetadata *)arr) - 1;
    if (meta->size == 0) {
        fprintf(stderr, "rt_array_pop_string: empty array\n");
        exit(1);
    }
    meta->size--;
    return arr[meta->size];
}

/* Concat functions - return a NEW array containing elements from both arrays (non-mutating) */
#define DEFINE_ARRAY_CONCAT(suffix, elem_type)                                 \
elem_type *rt_array_concat_##suffix(RtArena *arena, elem_type *arr1, elem_type *arr2) { \
    size_t len1 = arr1 ? rt_array_length(arr1) : 0;                            \
    size_t len2 = arr2 ? rt_array_length(arr2) : 0;                            \
    size_t total = len1 + len2;                                                \
    size_t capacity = total > 4 ? total : 4;                                   \
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(elem_type)); \
    if (meta == NULL) {                                                        \
        fprintf(stderr, "rt_array_concat_" #suffix ": allocation failed\n");   \
        exit(1);                                                               \
    }                                                                          \
    meta->arena = arena;                                                       \
    meta->size = total;                                                        \
    meta->capacity = capacity;                                                 \
    elem_type *result = (elem_type *)(meta + 1);                               \
    for (size_t i = 0; i < len1; i++) {                                        \
        result[i] = arr1[i];                                                   \
    }                                                                          \
    for (size_t i = 0; i < len2; i++) {                                        \
        result[len1 + i] = arr2[i];                                            \
    }                                                                          \
    return result;                                                             \
}

DEFINE_ARRAY_CONCAT(long, long)
DEFINE_ARRAY_CONCAT(double, double)
DEFINE_ARRAY_CONCAT(char, char)
DEFINE_ARRAY_CONCAT(bool, int)
DEFINE_ARRAY_CONCAT(byte, unsigned char)
DEFINE_ARRAY_CONCAT(ptr, void *)  /* For closures/function pointers and other pointer types */

char **rt_array_concat_string(RtArena *arena, char **arr1, char **arr2) {
    size_t len1 = arr1 ? rt_array_length(arr1) : 0;
    size_t len2 = arr2 ? rt_array_length(arr2) : 0;
    size_t total = len1 + len2;
    size_t capacity = total > 4 ? total : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_concat_string: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = total;
    meta->capacity = capacity;
    char **result = (char **)(meta + 1);
    for (size_t i = 0; i < len1; i++) {
        result[i] = arr1[i] ? rt_arena_strdup(arena, arr1[i]) : NULL;
    }
    for (size_t i = 0; i < len2; i++) {
        result[len1 + i] = arr2[i] ? rt_arena_strdup(arena, arr2[i]) : NULL;
    }
    return result;
}

/* Array slice functions - create a new array from a portion of the source
 * start: starting index (inclusive), use LONG_MIN for beginning
 * end: ending index (exclusive), use LONG_MIN for end of array
 * step: step size, use LONG_MIN for default step of 1
 */
#define DEFINE_ARRAY_SLICE(suffix, elem_type)                                   \
elem_type *rt_array_slice_##suffix(RtArena *arena, elem_type *arr, long start, long end, long step) { \
    if (arr == NULL) {                                                          \
        return NULL;                                                            \
    }                                                                           \
    size_t len = rt_array_length(arr);                                          \
    /* Handle step: LONG_MIN means step of 1 */                                 \
    long actual_step = (step == LONG_MIN) ? 1 : step;                           \
    if (actual_step <= 0) {                                                     \
        fprintf(stderr, "rt_array_slice_" #suffix ": step must be positive\n"); \
        return NULL;                                                            \
    }                                                                           \
    /* Handle start: LONG_MIN means "from beginning", negative means from end */\
    long actual_start;                                                          \
    if (start == LONG_MIN) {                                                    \
        actual_start = 0;                                                       \
    } else if (start < 0) {                                                     \
        actual_start = (long)len + start;                                       \
        if (actual_start < 0) actual_start = 0;                                 \
    } else {                                                                    \
        actual_start = start;                                                   \
    }                                                                           \
    /* Handle end: LONG_MIN means "to end", negative means from end */          \
    long actual_end;                                                            \
    if (end == LONG_MIN) {                                                      \
        actual_end = (long)len;                                                 \
    } else if (end < 0) {                                                       \
        actual_end = (long)len + end;                                           \
        if (actual_end < 0) actual_end = 0;                                     \
    } else {                                                                    \
        actual_end = end;                                                       \
    }                                                                           \
    if (actual_start > (long)len) actual_start = (long)len;                     \
    if (actual_end > (long)len) actual_end = (long)len;                         \
    if (actual_start >= actual_end) {                                           \
        return NULL;                                                            \
    }                                                                           \
    /* Calculate slice length with step */                                      \
    size_t range = (size_t)(actual_end - actual_start);                         \
    size_t slice_len = (range + (size_t)actual_step - 1) / (size_t)actual_step; \
    size_t capacity = slice_len > 4 ? slice_len : 4;                            \
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(elem_type)); \
    if (meta == NULL) {                                                         \
        fprintf(stderr, "rt_array_slice_" #suffix ": allocation failed\n");     \
        exit(1);                                                                \
    }                                                                           \
    meta->arena = arena;                                                        \
    meta->size = slice_len;                                                     \
    meta->capacity = capacity;                                                  \
    elem_type *new_arr = (elem_type *)(meta + 1);                               \
    for (size_t i = 0; i < slice_len; i++) {                                    \
        new_arr[i] = arr[actual_start + i * (size_t)actual_step];               \
    }                                                                           \
    return new_arr;                                                             \
}

DEFINE_ARRAY_SLICE(long, long)
DEFINE_ARRAY_SLICE(double, double)
DEFINE_ARRAY_SLICE(char, char)
DEFINE_ARRAY_SLICE(bool, int)
DEFINE_ARRAY_SLICE(byte, unsigned char)

/* String slice needs special handling to strdup elements */
char **rt_array_slice_string(RtArena *arena, char **arr, long start, long end, long step) {
    if (arr == NULL) {
        return NULL;
    }
    size_t len = rt_array_length(arr);
    /* Handle step: LONG_MIN means step of 1 */
    long actual_step = (step == LONG_MIN) ? 1 : step;
    if (actual_step <= 0) {
        fprintf(stderr, "rt_array_slice_string: step must be positive\n");
        return NULL;
    }
    /* Handle start: LONG_MIN means "from beginning", negative means from end */
    long actual_start;
    if (start == LONG_MIN) {
        actual_start = 0;
    } else if (start < 0) {
        actual_start = (long)len + start;
        if (actual_start < 0) actual_start = 0;
    } else {
        actual_start = start;
    }
    /* Handle end: LONG_MIN means "to end", negative means from end */
    long actual_end;
    if (end == LONG_MIN) {
        actual_end = (long)len;
    } else if (end < 0) {
        actual_end = (long)len + end;
        if (actual_end < 0) actual_end = 0;
    } else {
        actual_end = end;
    }
    if (actual_start > (long)len) actual_start = (long)len;
    if (actual_end > (long)len) actual_end = (long)len;
    if (actual_start >= actual_end) {
        return NULL;
    }
    /* Calculate slice length with step */
    size_t range = (size_t)(actual_end - actual_start);
    size_t slice_len = (range + (size_t)actual_step - 1) / (size_t)actual_step;
    size_t capacity = slice_len > 4 ? slice_len : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_slice_string: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = slice_len;
    meta->capacity = capacity;
    char **new_arr = (char **)(meta + 1);
    for (size_t i = 0; i < slice_len; i++) {
        size_t src_idx = actual_start + i * (size_t)actual_step;
        new_arr[i] = arr[src_idx] ? rt_arena_strdup(arena, arr[src_idx]) : NULL;
    }
    return new_arr;
}

/* Array reverse functions - return a new reversed array */
#define DEFINE_ARRAY_REV(suffix, elem_type)                                     \
elem_type *rt_array_rev_##suffix(RtArena *arena, elem_type *arr) {              \
    if (arr == NULL) {                                                          \
        return NULL;                                                            \
    }                                                                           \
    size_t len = rt_array_length(arr);                                          \
    if (len == 0) {                                                             \
        return NULL;                                                            \
    }                                                                           \
    size_t capacity = len > 4 ? len : 4;                                        \
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(elem_type)); \
    if (meta == NULL) {                                                         \
        fprintf(stderr, "rt_array_rev_" #suffix ": allocation failed\n");       \
        exit(1);                                                                \
    }                                                                           \
    meta->arena = arena;                                                        \
    meta->size = len;                                                           \
    meta->capacity = capacity;                                                  \
    elem_type *new_arr = (elem_type *)(meta + 1);                               \
    for (size_t i = 0; i < len; i++) {                                          \
        new_arr[i] = arr[len - 1 - i];                                          \
    }                                                                           \
    return new_arr;                                                             \
}

DEFINE_ARRAY_REV(long, long)
DEFINE_ARRAY_REV(double, double)
DEFINE_ARRAY_REV(char, char)
DEFINE_ARRAY_REV(bool, int)
DEFINE_ARRAY_REV(byte, unsigned char)

/* String reverse needs special handling to strdup elements */
char **rt_array_rev_string(RtArena *arena, char **arr) {
    if (arr == NULL) {
        return NULL;
    }
    size_t len = rt_array_length(arr);
    if (len == 0) {
        return NULL;
    }
    size_t capacity = len > 4 ? len : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_rev_string: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = len;
    meta->capacity = capacity;
    char **new_arr = (char **)(meta + 1);
    for (size_t i = 0; i < len; i++) {
        new_arr[i] = arr[len - 1 - i] ? rt_arena_strdup(arena, arr[len - 1 - i]) : NULL;
    }
    return new_arr;
}

/* Array remove at index functions - return a new array without the element */
#define DEFINE_ARRAY_REM(suffix, elem_type)                                     \
elem_type *rt_array_rem_##suffix(RtArena *arena, elem_type *arr, long index) {  \
    if (arr == NULL) {                                                          \
        return NULL;                                                            \
    }                                                                           \
    size_t len = rt_array_length(arr);                                          \
    if (index < 0 || (size_t)index >= len) {                                    \
        fprintf(stderr, "rt_array_rem_" #suffix ": index out of bounds\n");     \
        exit(1);                                                                \
    }                                                                           \
    if (len == 1) {                                                             \
        return NULL;                                                            \
    }                                                                           \
    size_t new_len = len - 1;                                                   \
    size_t capacity = new_len > 4 ? new_len : 4;                                \
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(elem_type)); \
    if (meta == NULL) {                                                         \
        fprintf(stderr, "rt_array_rem_" #suffix ": allocation failed\n");       \
        exit(1);                                                                \
    }                                                                           \
    meta->arena = arena;                                                        \
    meta->size = new_len;                                                       \
    meta->capacity = capacity;                                                  \
    elem_type *new_arr = (elem_type *)(meta + 1);                               \
    for (size_t i = 0; i < (size_t)index; i++) {                                \
        new_arr[i] = arr[i];                                                    \
    }                                                                           \
    for (size_t i = (size_t)index; i < new_len; i++) {                          \
        new_arr[i] = arr[i + 1];                                                \
    }                                                                           \
    return new_arr;                                                             \
}

DEFINE_ARRAY_REM(long, long)
DEFINE_ARRAY_REM(double, double)
DEFINE_ARRAY_REM(char, char)
DEFINE_ARRAY_REM(bool, int)
DEFINE_ARRAY_REM(byte, unsigned char)

/* String remove needs special handling to strdup elements */
char **rt_array_rem_string(RtArena *arena, char **arr, long index) {
    if (arr == NULL) {
        return NULL;
    }
    size_t len = rt_array_length(arr);
    if (index < 0 || (size_t)index >= len) {
        fprintf(stderr, "rt_array_rem_string: index out of bounds\n");
        exit(1);
    }
    if (len == 1) {
        return NULL;
    }
    size_t new_len = len - 1;
    size_t capacity = new_len > 4 ? new_len : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_rem_string: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = new_len;
    meta->capacity = capacity;
    char **new_arr = (char **)(meta + 1);
    for (size_t i = 0; i < (size_t)index; i++) {
        new_arr[i] = arr[i] ? rt_arena_strdup(arena, arr[i]) : NULL;
    }
    for (size_t i = (size_t)index; i < new_len; i++) {
        new_arr[i] = arr[i + 1] ? rt_arena_strdup(arena, arr[i + 1]) : NULL;
    }
    return new_arr;
}

/* Array insert at index functions - return a new array with the element inserted */
#define DEFINE_ARRAY_INS(suffix, elem_type)                                     \
elem_type *rt_array_ins_##suffix(RtArena *arena, elem_type *arr, elem_type elem, long index) { \
    size_t len = arr ? rt_array_length(arr) : 0;                                \
    if (index < 0) index = 0;                                                   \
    if ((size_t)index > len) index = (long)len;                                 \
    size_t new_len = len + 1;                                                   \
    size_t capacity = new_len > 4 ? new_len : 4;                                \
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(elem_type)); \
    if (meta == NULL) {                                                         \
        fprintf(stderr, "rt_array_ins_" #suffix ": allocation failed\n");       \
        exit(1);                                                                \
    }                                                                           \
    meta->arena = arena;                                                        \
    meta->size = new_len;                                                       \
    meta->capacity = capacity;                                                  \
    elem_type *new_arr = (elem_type *)(meta + 1);                               \
    for (size_t i = 0; i < (size_t)index; i++) {                                \
        new_arr[i] = arr[i];                                                    \
    }                                                                           \
    new_arr[index] = elem;                                                      \
    for (size_t i = (size_t)index + 1; i < new_len; i++) {                      \
        new_arr[i] = arr[i - 1];                                                \
    }                                                                           \
    return new_arr;                                                             \
}

DEFINE_ARRAY_INS(long, long)
DEFINE_ARRAY_INS(double, double)
DEFINE_ARRAY_INS(char, char)
DEFINE_ARRAY_INS(bool, int)
DEFINE_ARRAY_INS(byte, unsigned char)

/* String insert needs special handling to strdup elements */
char **rt_array_ins_string(RtArena *arena, char **arr, const char *elem, long index) {
    size_t len = arr ? rt_array_length(arr) : 0;
    if (index < 0) index = 0;
    if ((size_t)index > len) index = (long)len;
    size_t new_len = len + 1;
    size_t capacity = new_len > 4 ? new_len : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_ins_string: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = new_len;
    meta->capacity = capacity;
    char **new_arr = (char **)(meta + 1);
    for (size_t i = 0; i < (size_t)index; i++) {
        new_arr[i] = arr[i] ? rt_arena_strdup(arena, arr[i]) : NULL;
    }
    new_arr[index] = elem ? rt_arena_strdup(arena, elem) : NULL;
    for (size_t i = (size_t)index + 1; i < new_len; i++) {
        new_arr[i] = arr[i - 1] ? rt_arena_strdup(arena, arr[i - 1]) : NULL;
    }
    return new_arr;
}

/* Array push (copy) functions - create a NEW array with element appended */
#define DEFINE_ARRAY_PUSH_COPY(suffix, elem_type)                               \
elem_type *rt_array_push_copy_##suffix(RtArena *arena, elem_type *arr, elem_type elem) { \
    size_t len = arr ? rt_array_length(arr) : 0;                                \
    size_t new_len = len + 1;                                                   \
    size_t capacity = new_len > 4 ? new_len : 4;                                \
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(elem_type)); \
    if (meta == NULL) {                                                         \
        fprintf(stderr, "rt_array_push_copy_" #suffix ": allocation failed\n"); \
        exit(1);                                                                \
    }                                                                           \
    meta->arena = arena;                                                        \
    meta->size = new_len;                                                       \
    meta->capacity = capacity;                                                  \
    elem_type *new_arr = (elem_type *)(meta + 1);                               \
    for (size_t i = 0; i < len; i++) {                                          \
        new_arr[i] = arr[i];                                                    \
    }                                                                           \
    new_arr[len] = elem;                                                        \
    return new_arr;                                                             \
}

DEFINE_ARRAY_PUSH_COPY(long, long)
DEFINE_ARRAY_PUSH_COPY(double, double)
DEFINE_ARRAY_PUSH_COPY(char, char)
DEFINE_ARRAY_PUSH_COPY(bool, int)
DEFINE_ARRAY_PUSH_COPY(byte, unsigned char)

/* String push copy needs special handling to strdup elements */
char **rt_array_push_copy_string(RtArena *arena, char **arr, const char *elem) {
    size_t len = arr ? rt_array_length(arr) : 0;
    size_t new_len = len + 1;
    size_t capacity = new_len > 4 ? new_len : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_push_copy_string: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = new_len;
    meta->capacity = capacity;
    char **new_arr = (char **)(meta + 1);
    for (size_t i = 0; i < len; i++) {
        new_arr[i] = arr[i] ? rt_arena_strdup(arena, arr[i]) : NULL;
    }
    new_arr[len] = elem ? rt_arena_strdup(arena, elem) : NULL;
    return new_arr;
}

/* Array indexOf functions - find first index of element, returns -1 if not found */
#define DEFINE_ARRAY_INDEXOF(suffix, elem_type, compare_expr)                   \
long rt_array_indexOf_##suffix(elem_type *arr, elem_type elem) {                \
    if (arr == NULL) {                                                          \
        return -1L;                                                             \
    }                                                                           \
    size_t len = rt_array_length(arr);                                          \
    for (size_t i = 0; i < len; i++) {                                          \
        if (compare_expr) {                                                     \
            return (long)i;                                                     \
        }                                                                       \
    }                                                                           \
    return -1L;                                                                 \
}

DEFINE_ARRAY_INDEXOF(long, long, arr[i] == elem)
DEFINE_ARRAY_INDEXOF(double, double, arr[i] == elem)
DEFINE_ARRAY_INDEXOF(char, char, arr[i] == elem)
DEFINE_ARRAY_INDEXOF(bool, int, arr[i] == elem)
DEFINE_ARRAY_INDEXOF(byte, unsigned char, arr[i] == elem)

/* String indexOf needs special comparison */
long rt_array_indexOf_string(char **arr, const char *elem) {
    if (arr == NULL) {
        return -1L;
    }
    size_t len = rt_array_length(arr);
    for (size_t i = 0; i < len; i++) {
        if (arr[i] == NULL && elem == NULL) {
            return (long)i;
        }
        if (arr[i] != NULL && elem != NULL && strcmp(arr[i], elem) == 0) {
            return (long)i;
        }
    }
    return -1L;
}

/* Array contains functions - check if element exists */
#define DEFINE_ARRAY_CONTAINS(suffix, elem_type)                                \
int rt_array_contains_##suffix(elem_type *arr, elem_type elem) {                \
    return rt_array_indexOf_##suffix(arr, elem) >= 0;                           \
}

DEFINE_ARRAY_CONTAINS(long, long)
DEFINE_ARRAY_CONTAINS(double, double)
DEFINE_ARRAY_CONTAINS(char, char)
DEFINE_ARRAY_CONTAINS(bool, int)
DEFINE_ARRAY_CONTAINS(byte, unsigned char)

int rt_array_contains_string(char **arr, const char *elem) {
    return rt_array_indexOf_string(arr, elem) >= 0;
}

/* Array clone functions - create a deep copy of the array */
#define DEFINE_ARRAY_CLONE(suffix, elem_type)                                   \
elem_type *rt_array_clone_##suffix(RtArena *arena, elem_type *arr) {            \
    if (arr == NULL) {                                                          \
        return NULL;                                                            \
    }                                                                           \
    size_t len = rt_array_length(arr);                                          \
    if (len == 0) {                                                             \
        return NULL;                                                            \
    }                                                                           \
    size_t capacity = len > 4 ? len : 4;                                        \
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(elem_type)); \
    if (meta == NULL) {                                                         \
        fprintf(stderr, "rt_array_clone_" #suffix ": allocation failed\n");     \
        exit(1);                                                                \
    }                                                                           \
    meta->arena = arena;                                                        \
    meta->size = len;                                                           \
    meta->capacity = capacity;                                                  \
    elem_type *new_arr = (elem_type *)(meta + 1);                               \
    memcpy(new_arr, arr, len * sizeof(elem_type));                              \
    return new_arr;                                                             \
}

DEFINE_ARRAY_CLONE(long, long)
DEFINE_ARRAY_CLONE(double, double)
DEFINE_ARRAY_CLONE(char, char)
DEFINE_ARRAY_CLONE(bool, int)
DEFINE_ARRAY_CLONE(byte, unsigned char)

/* String clone needs special handling to strdup elements */
char **rt_array_clone_string(RtArena *arena, char **arr) {
    if (arr == NULL) {
        return NULL;
    }
    size_t len = rt_array_length(arr);
    if (len == 0) {
        return NULL;
    }
    size_t capacity = len > 4 ? len : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_clone_string: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = len;
    meta->capacity = capacity;
    char **new_arr = (char **)(meta + 1);
    for (size_t i = 0; i < len; i++) {
        new_arr[i] = arr[i] ? rt_arena_strdup(arena, arr[i]) : NULL;
    }
    return new_arr;
}

/* Array join function - join elements into a string with separator */
char *rt_array_join_long(RtArena *arena, long *arr, const char *separator) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "");
    }
    size_t len = rt_array_length(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    /* Estimate buffer size: each long can be up to 20 chars + separators */
    size_t buf_size = len * 24 + (len - 1) * sep_len + 1;
    char *result = rt_arena_alloc(arena, buf_size);
    if (result == NULL) {
        fprintf(stderr, "rt_array_join_long: allocation failed\n");
        exit(1);
    }

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        ptr += sprintf(ptr, "%ld", arr[i]);
    }
    return result;
}

char *rt_array_join_double(RtArena *arena, double *arr, const char *separator) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "");
    }
    size_t len = rt_array_length(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    size_t buf_size = len * 32 + (len - 1) * sep_len + 1;
    char *result = rt_arena_alloc(arena, buf_size);
    if (result == NULL) {
        fprintf(stderr, "rt_array_join_double: allocation failed\n");
        exit(1);
    }

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        ptr += sprintf(ptr, "%.5f", arr[i]);
    }
    return result;
}

char *rt_array_join_char(RtArena *arena, char *arr, const char *separator) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "");
    }
    size_t len = rt_array_length(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    size_t buf_size = len + (len - 1) * sep_len + 1;
    char *result = rt_arena_alloc(arena, buf_size);
    if (result == NULL) {
        fprintf(stderr, "rt_array_join_char: allocation failed\n");
        exit(1);
    }

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        *ptr++ = arr[i];
    }
    *ptr = '\0';
    return result;
}

char *rt_array_join_bool(RtArena *arena, int *arr, const char *separator) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "");
    }
    size_t len = rt_array_length(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    /* "true" or "false" + separators */
    size_t buf_size = len * 6 + (len - 1) * sep_len + 1;
    char *result = rt_arena_alloc(arena, buf_size);
    if (result == NULL) {
        fprintf(stderr, "rt_array_join_bool: allocation failed\n");
        exit(1);
    }

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        ptr += sprintf(ptr, "%s", arr[i] ? "true" : "false");
    }
    return result;
}

char *rt_array_join_byte(RtArena *arena, unsigned char *arr, const char *separator) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "");
    }
    size_t len = rt_array_length(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    /* "0xXX" (4 chars) + separators */
    size_t buf_size = len * 4 + (len - 1) * sep_len + 1;
    char *result = rt_arena_alloc(arena, buf_size);
    if (result == NULL) {
        fprintf(stderr, "rt_array_join_byte: allocation failed\n");
        exit(1);
    }

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        ptr += sprintf(ptr, "0x%02X", arr[i]);
    }
    return result;
}

char *rt_array_join_string(RtArena *arena, char **arr, const char *separator) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "");
    }
    size_t len = rt_array_length(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    /* Calculate total length */
    size_t total_len = 0;
    for (size_t i = 0; i < len; i++) {
        if (arr[i]) {
            total_len += strlen(arr[i]);
        }
    }
    total_len += (len - 1) * sep_len + 1;

    char *result = rt_arena_alloc(arena, total_len);
    if (result == NULL) {
        fprintf(stderr, "rt_array_join_string: allocation failed\n");
        exit(1);
    }

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            size_t l = strlen(separator);
            memcpy(ptr, separator, l);
            ptr += l;
        }
        if (arr[i]) {
            size_t l = strlen(arr[i]);
            memcpy(ptr, arr[i], l);
            ptr += l;
        }
    }
    *ptr = '\0';
    return result;
}

/* Array create functions - create runtime array from static C array */
#define DEFINE_ARRAY_CREATE(suffix, elem_type)                                  \
elem_type *rt_array_create_##suffix(RtArena *arena, size_t count, const elem_type *data) { \
    /* Always create array with metadata to track arena, even for empty arrays */ \
    size_t capacity = count > 4 ? count : 4;                                    \
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(elem_type)); \
    if (meta == NULL) {                                                         \
        fprintf(stderr, "rt_array_create_" #suffix ": allocation failed\n");    \
        exit(1);                                                                \
    }                                                                           \
    meta->arena = arena;                                                        \
    meta->size = count;                                                         \
    meta->capacity = capacity;                                                  \
    elem_type *arr = (elem_type *)(meta + 1);                                   \
    for (size_t i = 0; i < count; i++) {                                        \
        if (data != NULL) arr[i] = data[i];                                     \
    }                                                                           \
    return arr;                                                                 \
}

DEFINE_ARRAY_CREATE(long, long)
DEFINE_ARRAY_CREATE(double, double)
DEFINE_ARRAY_CREATE(char, char)
DEFINE_ARRAY_CREATE(bool, int)
DEFINE_ARRAY_CREATE(byte, unsigned char)

/* Create an uninitialized byte array for filling in later (e.g., file reads) */
unsigned char *rt_array_create_byte_uninit(RtArena *arena, size_t count) {
    size_t capacity = count > 4 ? count : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(unsigned char));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_create_byte_uninit: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = count;
    meta->capacity = capacity;
    unsigned char *arr = (unsigned char *)(meta + 1);
    /* Zero-initialize for safety */
    memset(arr, 0, count);
    return arr;
}

/* String array create needs special handling for strdup */
char **rt_array_create_string(RtArena *arena, size_t count, const char **data) {
    /* Always create array with metadata to track arena, even for empty arrays */
    size_t capacity = count > 4 ? count : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_create_string: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = count;
    meta->capacity = capacity;
    char **arr = (char **)(meta + 1);
    for (size_t i = 0; i < count; i++) {
        arr[i] = (data && data[i]) ? rt_arena_strdup(arena, data[i]) : NULL;
    }
    return arr;
}

/* Array alloc with default value - creates array of count elements filled with default_value */
long *rt_array_alloc_long(RtArena *arena, size_t count, long default_value) {
    size_t data_size = count * sizeof(long);
    size_t total = sizeof(ArrayMetadata) + data_size;

    ArrayMetadata *meta = rt_arena_alloc(arena, total);
    if (meta == NULL) {
        fprintf(stderr, "rt_array_alloc_long: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;

    long *data = (long *)(meta + 1);
    if (default_value == 0) {
        memset(data, 0, data_size);
    } else {
        for (size_t i = 0; i < count; i++) data[i] = default_value;
    }
    return data;
}

double *rt_array_alloc_double(RtArena *arena, size_t count, double default_value) {
    size_t data_size = count * sizeof(double);
    size_t total = sizeof(ArrayMetadata) + data_size;

    ArrayMetadata *meta = rt_arena_alloc(arena, total);
    if (meta == NULL) {
        fprintf(stderr, "rt_array_alloc_double: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;

    double *data = (double *)(meta + 1);
    if (default_value == 0.0) {
        memset(data, 0, data_size);
    } else {
        for (size_t i = 0; i < count; i++) data[i] = default_value;
    }
    return data;
}

char *rt_array_alloc_char(RtArena *arena, size_t count, char default_value) {
    size_t data_size = count * sizeof(char);
    size_t total = sizeof(ArrayMetadata) + data_size;

    ArrayMetadata *meta = rt_arena_alloc(arena, total);
    if (meta == NULL) {
        fprintf(stderr, "rt_array_alloc_char: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;

    char *data = (char *)(meta + 1);
    if (default_value == 0) {
        memset(data, 0, data_size);
    } else {
        for (size_t i = 0; i < count; i++) data[i] = default_value;
    }
    return data;
}

int *rt_array_alloc_bool(RtArena *arena, size_t count, int default_value) {
    size_t data_size = count * sizeof(int);
    size_t total = sizeof(ArrayMetadata) + data_size;

    ArrayMetadata *meta = rt_arena_alloc(arena, total);
    if (meta == NULL) {
        fprintf(stderr, "rt_array_alloc_bool: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;

    int *data = (int *)(meta + 1);
    if (default_value == 0) {
        memset(data, 0, data_size);
    } else {
        for (size_t i = 0; i < count; i++) data[i] = default_value;
    }
    return data;
}

unsigned char *rt_array_alloc_byte(RtArena *arena, size_t count, unsigned char default_value) {
    size_t data_size = count * sizeof(unsigned char);
    size_t total = sizeof(ArrayMetadata) + data_size;

    ArrayMetadata *meta = rt_arena_alloc(arena, total);
    if (meta == NULL) {
        fprintf(stderr, "rt_array_alloc_byte: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;

    unsigned char *data = (unsigned char *)(meta + 1);
    if (default_value == 0) {
        memset(data, 0, data_size);
    } else {
        memset(data, default_value, data_size);  /* memset works perfectly for bytes */
    }
    return data;
}

char **rt_array_alloc_string(RtArena *arena, size_t count, const char *default_value) {
    size_t data_size = count * sizeof(char *);
    size_t total = sizeof(ArrayMetadata) + data_size;

    ArrayMetadata *meta = rt_arena_alloc(arena, total);
    if (meta == NULL) {
        fprintf(stderr, "rt_array_alloc_string: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;

    char **data = (char **)(meta + 1);
    if (default_value == NULL) {
        memset(data, 0, data_size);  /* Set all pointers to NULL */
    } else {
        for (size_t i = 0; i < count; i++) {
            data[i] = rt_arena_strdup(arena, default_value);
        }
    }
    return data;
}

/* Array equality functions - compare arrays element by element */
#define DEFINE_ARRAY_EQ(suffix, elem_type, compare_expr)                        \
int rt_array_eq_##suffix(elem_type *a, elem_type *b) {                          \
    /* Both NULL means equal */                                                 \
    if (a == NULL && b == NULL) return 1;                                       \
    /* One NULL, one not means not equal */                                     \
    if (a == NULL || b == NULL) return 0;                                       \
    size_t len_a = rt_array_length(a);                                          \
    size_t len_b = rt_array_length(b);                                          \
    /* Different lengths means not equal */                                     \
    if (len_a != len_b) return 0;                                               \
    /* Compare element by element */                                            \
    for (size_t i = 0; i < len_a; i++) {                                        \
        if (!(compare_expr)) return 0;                                          \
    }                                                                           \
    return 1;                                                                   \
}

DEFINE_ARRAY_EQ(long, long, a[i] == b[i])
DEFINE_ARRAY_EQ(double, double, a[i] == b[i])
DEFINE_ARRAY_EQ(char, char, a[i] == b[i])
DEFINE_ARRAY_EQ(bool, int, a[i] == b[i])
DEFINE_ARRAY_EQ(byte, unsigned char, a[i] == b[i])

/* String array equality needs strcmp */
int rt_array_eq_string(char **a, char **b) {
    /* Both NULL means equal */
    if (a == NULL && b == NULL) return 1;
    /* One NULL, one not means not equal */
    if (a == NULL || b == NULL) return 0;
    size_t len_a = rt_array_length(a);
    size_t len_b = rt_array_length(b);
    /* Different lengths means not equal */
    if (len_a != len_b) return 0;
    /* Compare element by element */
    for (size_t i = 0; i < len_a; i++) {
        /* Handle NULL strings */
        if (a[i] == NULL && b[i] == NULL) continue;
        if (a[i] == NULL || b[i] == NULL) return 0;
        if (strcmp(a[i], b[i]) != 0) return 0;
    }
    return 1;
}

/* Range creation - creates int[] from start to end (exclusive) */
long *rt_array_range(RtArena *arena, long start, long end) {
    /* Calculate count, handle both ascending and descending ranges */
    long count;
    if (end >= start) {
        count = end - start;
    } else {
        /* Descending range: empty for now (future: could support negative step) */
        count = 0;
    }

    if (count <= 0) {
        /* Empty range */
        size_t capacity = 4;
        ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(long));
        if (meta == NULL) {
            fprintf(stderr, "rt_array_range: allocation failed\n");
            exit(1);
        }
        meta->arena = arena;
        meta->size = 0;
        meta->capacity = capacity;
        return (long *)(meta + 1);
    }

    size_t capacity = (size_t)count > 4 ? (size_t)count : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(long));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_range: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = (size_t)count;
    meta->capacity = capacity;
    long *arr = (long *)(meta + 1);

    for (long i = 0; i < count; i++) {
        arr[i] = start + i;
    }

    return arr;
}

/* ============================================================
   String Manipulation Functions
   ============================================================ */

/* Get length of string */
long rt_str_length(const char *str) {
    if (str == NULL) return 0;
    return (long)strlen(str);
}

/* Get substring from start to end (exclusive) */
char *rt_str_substring(RtArena *arena, const char *str, long start, long end) {
    if (str == NULL) return rt_arena_strdup(arena, "");
    long len = (long)strlen(str);

    /* Handle negative indices */
    if (start < 0) start = len + start;
    if (end < 0) end = len + end;

    /* Clamp to valid range */
    if (start < 0) start = 0;
    if (end > len) end = len;
    if (start >= end || start >= len) return rt_arena_strdup(arena, "");

    long sub_len = end - start;
    char *result = rt_arena_alloc(arena, sub_len + 1);
    if (result == NULL) return rt_arena_strdup(arena, "");

    memcpy(result, str + start, sub_len);
    result[sub_len] = '\0';
    return result;
}

/* Find index of substring, returns -1 if not found */
long rt_str_indexOf(const char *str, const char *search) {
    if (str == NULL || search == NULL) return -1;
    const char *pos = strstr(str, search);
    if (pos == NULL) return -1;
    return (long)(pos - str);
}

/* Split string by delimiter, returns array of strings */
char **rt_str_split(RtArena *arena, const char *str, const char *delimiter) {
    if (str == NULL || delimiter == NULL) {
        return NULL;
    }

    size_t delim_len = strlen(delimiter);
    if (delim_len == 0) {
        /* Empty delimiter: split into individual characters */
        size_t len = strlen(str);
        if (len == 0) return NULL;

        /* Allocate the result array directly */
        size_t capacity = len > 4 ? len : 4;
        ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(char *));
        if (meta == NULL) {
            fprintf(stderr, "rt_str_split: allocation failed\n");
            exit(1);
        }
        meta->arena = arena;
        meta->size = len;
        meta->capacity = capacity;
        char **result = (char **)(meta + 1);

        for (size_t i = 0; i < len; i++) {
            char *ch = rt_arena_alloc(arena, 2);
            ch[0] = str[i];
            ch[1] = '\0';
            result[i] = ch;
        }
        return result;
    }

    /* Count the number of parts */
    size_t count = 1;
    const char *p = str;
    while ((p = strstr(p, delimiter)) != NULL) {
        count++;
        p += delim_len;
    }

    /* Allocate the result array */
    size_t capacity = count > 4 ? count : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_str_split: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = count;
    meta->capacity = capacity;
    char **result = (char **)(meta + 1);

    /* Split the string */
    const char *start = str;
    size_t idx = 0;
    p = str;
    while ((p = strstr(p, delimiter)) != NULL) {
        size_t part_len = p - start;
        char *part = rt_arena_alloc(arena, part_len + 1);
        memcpy(part, start, part_len);
        part[part_len] = '\0';
        result[idx++] = part;
        p += delim_len;
        start = p;
    }
    /* Add final part */
    result[idx] = rt_arena_strdup(arena, start);

    return result;
}

/* Trim whitespace from both ends */
char *rt_str_trim(RtArena *arena, const char *str) {
    if (str == NULL) return rt_arena_strdup(arena, "");

    /* Skip leading whitespace */
    while (*str && (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r')) {
        str++;
    }

    if (*str == '\0') return rt_arena_strdup(arena, "");

    /* Find end, skipping trailing whitespace */
    const char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        end--;
    }

    size_t len = end - str + 1;
    char *result = rt_arena_alloc(arena, len + 1);
    if (result == NULL) return rt_arena_strdup(arena, "");

    memcpy(result, str, len);
    result[len] = '\0';
    return result;
}

/* Convert to uppercase */
char *rt_str_toUpper(RtArena *arena, const char *str) {
    if (str == NULL) return rt_arena_strdup(arena, "");

    char *result = rt_arena_strdup(arena, str);
    if (result == NULL) return rt_arena_strdup(arena, "");

    for (char *p = result; *p; p++) {
        if (*p >= 'a' && *p <= 'z') {
            *p = *p - 'a' + 'A';
        }
    }
    return result;
}

/* Convert to lowercase */
char *rt_str_toLower(RtArena *arena, const char *str) {
    if (str == NULL) return rt_arena_strdup(arena, "");

    char *result = rt_arena_strdup(arena, str);
    if (result == NULL) return rt_arena_strdup(arena, "");

    for (char *p = result; *p; p++) {
        if (*p >= 'A' && *p <= 'Z') {
            *p = *p - 'A' + 'a';
        }
    }
    return result;
}

/* Check if string starts with prefix */
int rt_str_startsWith(const char *str, const char *prefix) {
    if (str == NULL || prefix == NULL) return 0;
    size_t prefix_len = strlen(prefix);
    if (strlen(str) < prefix_len) return 0;
    return strncmp(str, prefix, prefix_len) == 0;
}

/* Check if string ends with suffix */
int rt_str_endsWith(const char *str, const char *suffix) {
    if (str == NULL || suffix == NULL) return 0;
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (str_len < suffix_len) return 0;
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

/* Check if string contains substring */
int rt_str_contains(const char *str, const char *search) {
    if (str == NULL || search == NULL) return 0;
    return strstr(str, search) != NULL;
}

/* Replace all occurrences of old with new */
char *rt_str_replace(RtArena *arena, const char *str, const char *old, const char *new_str) {
    if (str == NULL || old == NULL || new_str == NULL) return rt_arena_strdup(arena, str ? str : "");

    size_t old_len = strlen(old);
    if (old_len == 0) return rt_arena_strdup(arena, str);

    size_t new_len = strlen(new_str);

    /* Count occurrences */
    size_t count = 0;
    const char *p = str;
    while ((p = strstr(p, old)) != NULL) {
        count++;
        p += old_len;
    }

    if (count == 0) return rt_arena_strdup(arena, str);

    /* Calculate new length */
    size_t str_len = strlen(str);
    size_t result_len = str_len + count * (new_len - old_len);

    char *result = rt_arena_alloc(arena, result_len + 1);
    if (result == NULL) return rt_arena_strdup(arena, str);

    /* Build result */
    char *dst = result;
    p = str;
    const char *found;
    while ((found = strstr(p, old)) != NULL) {
        size_t prefix_len = found - p;
        memcpy(dst, p, prefix_len);
        dst += prefix_len;
        memcpy(dst, new_str, new_len);
        dst += new_len;
        p = found + old_len;
    }
    /* Copy remainder */
    strcpy(dst, p);

    return result;
}

/* Get character at index (returns char cast to long for consistency) */
long rt_str_charAt(const char *str, long index) {
    if (str == NULL) return 0;
    long len = (long)strlen(str);

    /* Handle negative index */
    if (index < 0) index = len + index;

    if (index < 0 || index >= len) return 0;
    return (long)(unsigned char)str[index];
}

/* ============================================================
   TextFile Static Methods
   ============================================================ */

/* Open file for reading and writing (panics if file doesn't exist) */
RtTextFile *rt_text_file_open(RtArena *arena, const char *path) {
    if (arena == NULL) {
        fprintf(stderr, "TextFile.open: arena is NULL\n");
        exit(1);
    }
    if (path == NULL) {
        fprintf(stderr, "TextFile.open: path is NULL\n");
        exit(1);
    }

    FILE *fp = fopen(path, "r+");
    if (fp == NULL) {
        /* Try to create if doesn't exist for write */
        fp = fopen(path, "w+");
        if (fp == NULL) {
            fprintf(stderr, "TextFile.open: failed to open file '%s': %s\n", path, strerror(errno));
            exit(1);
        }
    }

    /* Allocate TextFile struct from arena */
    RtTextFile *file = rt_arena_alloc(arena, sizeof(RtTextFile));
    if (file == NULL) {
        fclose(fp);
        fprintf(stderr, "TextFile.open: memory allocation failed\n");
        exit(1);
    }

    file->fp = fp;
    file->path = rt_arena_strdup(arena, path);
    file->is_open = true;

    /* Track in arena for auto-close */
    file->handle = rt_arena_track_file(arena, fp, path, true);

    return file;
}

/* Check if file exists without opening */
int rt_text_file_exists(const char *path) {
    if (path == NULL) {
        return 0;
    }
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        return 0;
    }
    fclose(fp);
    return 1;
}

/* Read entire file contents as string */
char *rt_text_file_read_all(RtArena *arena, const char *path) {
    if (arena == NULL) {
        fprintf(stderr, "TextFile.readAll: arena is NULL\n");
        exit(1);
    }
    if (path == NULL) {
        fprintf(stderr, "TextFile.readAll: path is NULL\n");
        exit(1);
    }

    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        fprintf(stderr, "TextFile.readAll: failed to open file '%s': %s\n", path, strerror(errno));
        exit(1);
    }

    /* Get file size */
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        fprintf(stderr, "TextFile.readAll: failed to seek in file '%s': %s\n", path, strerror(errno));
        exit(1);
    }

    long size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        fprintf(stderr, "TextFile.readAll: failed to get size of file '%s': %s\n", path, strerror(errno));
        exit(1);
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        fprintf(stderr, "TextFile.readAll: failed to seek in file '%s': %s\n", path, strerror(errno));
        exit(1);
    }

    /* Allocate buffer from arena */
    char *content = rt_arena_alloc(arena, (size_t)size + 1);
    if (content == NULL) {
        fclose(fp);
        fprintf(stderr, "TextFile.readAll: memory allocation failed\n");
        exit(1);
    }

    /* Read file contents */
    size_t bytes_read = fread(content, 1, (size_t)size, fp);
    if (ferror(fp)) {
        fclose(fp);
        fprintf(stderr, "TextFile.readAll: failed to read file '%s': %s\n", path, strerror(errno));
        exit(1);
    }

    content[bytes_read] = '\0';
    fclose(fp);

    return content;
}

/* Write string to file (creates or overwrites) */
void rt_text_file_write_all(const char *path, const char *content) {
    if (path == NULL) {
        fprintf(stderr, "TextFile.writeAll: path is NULL\n");
        exit(1);
    }
    if (content == NULL) {
        content = "";
    }

    FILE *fp = fopen(path, "w");
    if (fp == NULL) {
        fprintf(stderr, "TextFile.writeAll: failed to open file '%s' for writing: %s\n", path, strerror(errno));
        exit(1);
    }

    size_t len = strlen(content);
    if (len > 0) {
        size_t written = fwrite(content, 1, len, fp);
        if (written != len) {
            fclose(fp);
            fprintf(stderr, "TextFile.writeAll: failed to write to file '%s': %s\n", path, strerror(errno));
            exit(1);
        }
    }

    if (fclose(fp) != 0) {
        fprintf(stderr, "TextFile.writeAll: failed to close file '%s': %s\n", path, strerror(errno));
        exit(1);
    }
}

/* Delete file (panics if fails) */
void rt_text_file_delete(const char *path) {
    if (path == NULL) {
        fprintf(stderr, "TextFile.delete: path is NULL\n");
        exit(1);
    }

    if (remove(path) != 0) {
        fprintf(stderr, "TextFile.delete: failed to delete file '%s': %s\n", path, strerror(errno));
        exit(1);
    }
}

/* Copy file to new location */
void rt_text_file_copy(const char *src, const char *dst) {
    if (src == NULL) {
        fprintf(stderr, "TextFile.copy: source path is NULL\n");
        exit(1);
    }
    if (dst == NULL) {
        fprintf(stderr, "TextFile.copy: destination path is NULL\n");
        exit(1);
    }

    FILE *src_fp = fopen(src, "r");
    if (src_fp == NULL) {
        fprintf(stderr, "TextFile.copy: failed to open source file '%s': %s\n", src, strerror(errno));
        exit(1);
    }

    FILE *dst_fp = fopen(dst, "w");
    if (dst_fp == NULL) {
        fclose(src_fp);
        fprintf(stderr, "TextFile.copy: failed to open destination file '%s': %s\n", dst, strerror(errno));
        exit(1);
    }

    char buffer[4096];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), src_fp)) > 0) {
        if (fwrite(buffer, 1, bytes, dst_fp) != bytes) {
            fclose(src_fp);
            fclose(dst_fp);
            fprintf(stderr, "TextFile.copy: failed to write to destination file '%s': %s\n", dst, strerror(errno));
            exit(1);
        }
    }

    if (ferror(src_fp)) {
        fclose(src_fp);
        fclose(dst_fp);
        fprintf(stderr, "TextFile.copy: failed to read from source file '%s': %s\n", src, strerror(errno));
        exit(1);
    }

    fclose(src_fp);
    if (fclose(dst_fp) != 0) {
        fprintf(stderr, "TextFile.copy: failed to close destination file '%s': %s\n", dst, strerror(errno));
        exit(1);
    }
}

/* Move/rename file */
void rt_text_file_move(const char *src, const char *dst) {
    if (src == NULL) {
        fprintf(stderr, "TextFile.move: source path is NULL\n");
        exit(1);
    }
    if (dst == NULL) {
        fprintf(stderr, "TextFile.move: destination path is NULL\n");
        exit(1);
    }

    if (rename(src, dst) != 0) {
        /* rename() may fail across filesystems, try copy+delete instead */
        rt_text_file_copy(src, dst);
        if (remove(src) != 0) {
            fprintf(stderr, "TextFile.move: failed to remove source file '%s' after copy: %s\n", src, strerror(errno));
            exit(1);
        }
    }
}

/* Close an open text file */
void rt_text_file_close(RtTextFile *file) {
    if (file == NULL) {
        return;
    }
    if (file->is_open && file->fp != NULL) {
        fclose((FILE *)file->fp);
        file->is_open = false;
        file->fp = NULL;
        /* Mark handle as closed too */
        if (file->handle != NULL) {
            file->handle->is_open = false;
        }
    }
}

/* ============================================================
   TextFile Instance Reading Methods
   ============================================================ */

/* Read single character, returns -1 on EOF */
long rt_text_file_read_char(RtTextFile *file) {
    if (file == NULL) {
        fprintf(stderr, "TextFile.readChar: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.readChar: file is not open\n");
        exit(1);
    }

    int c = fgetc((FILE *)file->fp);
    if (c == EOF) {
        if (ferror((FILE *)file->fp)) {
            fprintf(stderr, "TextFile.readChar: read error on file '%s': %s\n",
                    file->path ? file->path : "(unknown)", strerror(errno));
            exit(1);
        }
        return -1;  /* EOF */
    }
    return (long)c;
}

/* Read whitespace-delimited word, returns empty string on EOF */
char *rt_text_file_read_word(RtArena *arena, RtTextFile *file) {
    if (arena == NULL) {
        fprintf(stderr, "TextFile.readWord: arena is NULL\n");
        exit(1);
    }
    if (file == NULL) {
        fprintf(stderr, "TextFile.readWord: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.readWord: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;

    /* Skip leading whitespace */
    int c;
    while ((c = fgetc(fp)) != EOF && (c == ' ' || c == '\t' || c == '\n' || c == '\r')) {
        /* Skip whitespace */
    }

    if (c == EOF) {
        /* Return empty string on EOF */
        char *empty = rt_arena_alloc(arena, 1);
        empty[0] = '\0';
        return empty;
    }

    /* Read word into a temporary buffer, then copy to arena */
    size_t capacity = 64;
    size_t length = 0;
    char *buffer = rt_arena_alloc(arena, capacity);

    buffer[length++] = (char)c;

    while ((c = fgetc(fp)) != EOF && c != ' ' && c != '\t' && c != '\n' && c != '\r') {
        if (length >= capacity - 1) {
            /* Need more space - allocate new buffer and copy */
            size_t new_capacity = capacity * 2;
            char *new_buffer = rt_arena_alloc(arena, new_capacity);
            memcpy(new_buffer, buffer, length);
            buffer = new_buffer;
            capacity = new_capacity;
        }
        buffer[length++] = (char)c;
    }

    /* Put back the whitespace character if not EOF */
    if (c != EOF) {
        ungetc(c, fp);
    }

    buffer[length] = '\0';
    return buffer;
}

/* Read single line (strips trailing newline), returns NULL on EOF */
char *rt_text_file_read_line(RtArena *arena, RtTextFile *file) {
    if (arena == NULL) {
        fprintf(stderr, "TextFile.readLine: arena is NULL\n");
        exit(1);
    }
    if (file == NULL) {
        fprintf(stderr, "TextFile.readLine: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.readLine: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;

    /* Check for immediate EOF */
    int c = fgetc(fp);
    if (c == EOF) {
        if (ferror(fp)) {
            fprintf(stderr, "TextFile.readLine: read error on file '%s': %s\n",
                    file->path ? file->path : "(unknown)", strerror(errno));
            exit(1);
        }
        return NULL;  /* EOF - return NULL */
    }
    ungetc(c, fp);

    /* Read line into buffer */
    size_t capacity = 256;
    size_t length = 0;
    char *buffer = rt_arena_alloc(arena, capacity);

    while ((c = fgetc(fp)) != EOF && c != '\n') {
        if (length >= capacity - 1) {
            size_t new_capacity = capacity * 2;
            char *new_buffer = rt_arena_alloc(arena, new_capacity);
            memcpy(new_buffer, buffer, length);
            buffer = new_buffer;
            capacity = new_capacity;
        }
        buffer[length++] = (char)c;
    }

    /* Strip trailing \r if present (for Windows line endings) */
    if (length > 0 && buffer[length - 1] == '\r') {
        length--;
    }

    buffer[length] = '\0';
    return buffer;
}

/* Read all remaining content from open file */
char *rt_text_file_instance_read_all(RtArena *arena, RtTextFile *file) {
    if (arena == NULL) {
        fprintf(stderr, "TextFile.readAll: arena is NULL\n");
        exit(1);
    }
    if (file == NULL) {
        fprintf(stderr, "TextFile.readAll: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.readAll: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;

    /* Get current position and file size */
    long current_pos = ftell(fp);
    if (current_pos < 0) {
        fprintf(stderr, "TextFile.readAll: failed to get position in file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fprintf(stderr, "TextFile.readAll: failed to seek in file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    long end_pos = ftell(fp);
    if (end_pos < 0) {
        fprintf(stderr, "TextFile.readAll: failed to get size of file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    /* Seek back to current position */
    if (fseek(fp, current_pos, SEEK_SET) != 0) {
        fprintf(stderr, "TextFile.readAll: failed to seek in file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    /* Calculate remaining bytes */
    size_t remaining = (size_t)(end_pos - current_pos);

    /* Allocate buffer from arena */
    char *content = rt_arena_alloc(arena, remaining + 1);
    if (content == NULL) {
        fprintf(stderr, "TextFile.readAll: memory allocation failed\n");
        exit(1);
    }

    /* Read remaining content */
    size_t bytes_read = fread(content, 1, remaining, fp);
    if (ferror(fp)) {
        fprintf(stderr, "TextFile.readAll: failed to read file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    content[bytes_read] = '\0';
    return content;
}

/* Read all remaining lines as array of strings */
char **rt_text_file_read_lines(RtArena *arena, RtTextFile *file) {
    if (arena == NULL) {
        fprintf(stderr, "TextFile.readLines: arena is NULL\n");
        exit(1);
    }
    if (file == NULL) {
        fprintf(stderr, "TextFile.readLines: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.readLines: file is not open\n");
        exit(1);
    }

    /* Start with empty array */
    char **lines = rt_array_create_string(arena, 0, NULL);

    /* Read lines until EOF */
    char *line;
    while ((line = rt_text_file_read_line(arena, file)) != NULL) {
        lines = rt_array_push_string(arena, lines, line);
    }

    return lines;
}

/* Read into character buffer, returns number of chars read */
long rt_text_file_read_into(RtTextFile *file, char *buffer) {
    if (file == NULL) {
        fprintf(stderr, "TextFile.readInto: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.readInto: file is not open\n");
        exit(1);
    }
    if (buffer == NULL) {
        fprintf(stderr, "TextFile.readInto: buffer is NULL\n");
        exit(1);
    }

    /* Get the buffer's length from its metadata */
    size_t buf_len = rt_array_length(buffer);
    if (buf_len == 0) {
        return 0;  /* Empty buffer, nothing to read */
    }

    FILE *fp = (FILE *)file->fp;

    /* Read up to buf_len characters */
    size_t bytes_read = fread(buffer, 1, buf_len, fp);
    if (ferror(fp)) {
        fprintf(stderr, "TextFile.readInto: read error on file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    return (long)bytes_read;
}

/* ============================================================
   TextFile Instance Writing Methods
   ============================================================ */

/* Write single character to file */
void rt_text_file_write_char(RtTextFile *file, long ch) {
    if (file == NULL) {
        fprintf(stderr, "TextFile.writeChar: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.writeChar: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;
    if (fputc((int)ch, fp) == EOF) {
        fprintf(stderr, "TextFile.writeChar: write error on file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }
}

/* Write string to file */
void rt_text_file_write(RtTextFile *file, const char *text) {
    if (file == NULL) {
        fprintf(stderr, "TextFile.write: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.write: file is not open\n");
        exit(1);
    }
    if (text == NULL) {
        return;  /* Nothing to write */
    }

    FILE *fp = (FILE *)file->fp;
    size_t len = strlen(text);
    if (len > 0) {
        size_t written = fwrite(text, 1, len, fp);
        if (written != len) {
            fprintf(stderr, "TextFile.write: write error on file '%s': %s\n",
                    file->path ? file->path : "(unknown)", strerror(errno));
            exit(1);
        }
    }
}

/* Write string followed by newline */
void rt_text_file_write_line(RtTextFile *file, const char *text) {
    if (file == NULL) {
        fprintf(stderr, "TextFile.writeLine: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.writeLine: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;

    /* Write the text if not null */
    if (text != NULL) {
        size_t len = strlen(text);
        if (len > 0) {
            size_t written = fwrite(text, 1, len, fp);
            if (written != len) {
                fprintf(stderr, "TextFile.writeLine: write error on file '%s': %s\n",
                        file->path ? file->path : "(unknown)", strerror(errno));
                exit(1);
            }
        }
    }

    /* Write the newline */
    if (fputc('\n', fp) == EOF) {
        fprintf(stderr, "TextFile.writeLine: write error on file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }
}

/* Write string to file (alias for write, used with interpolated strings) */
void rt_text_file_print(RtTextFile *file, const char *text) {
    rt_text_file_write(file, text);
}

/* Write string followed by newline (alias for writeLine) */
void rt_text_file_println(RtTextFile *file, const char *text) {
    rt_text_file_write_line(file, text);
}

/* ============================================================
   TextFile State Methods
   ============================================================ */

/* Check if more characters are available to read */
int rt_text_file_has_chars(RtTextFile *file) {
    if (file == NULL) {
        fprintf(stderr, "TextFile.hasChars: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.hasChars: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;
    int c = fgetc(fp);
    if (c == EOF) {
        return 0;  /* No more characters */
    }
    ungetc(c, fp);
    return 1;  /* More characters available */
}

/* Check if more whitespace-delimited words are available */
int rt_text_file_has_words(RtTextFile *file) {
    if (file == NULL) {
        fprintf(stderr, "TextFile.hasWords: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.hasWords: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;
    long original_pos = ftell(fp);

    /* Skip whitespace */
    int c;
    while ((c = fgetc(fp)) != EOF && (c == ' ' || c == '\t' || c == '\n' || c == '\r')) {
        /* Skip whitespace */
    }

    int has_word = (c != EOF);

    /* Restore original position */
    fseek(fp, original_pos, SEEK_SET);

    return has_word;
}

/* Check if more lines are available to read */
int rt_text_file_has_lines(RtTextFile *file) {
    if (file == NULL) {
        fprintf(stderr, "TextFile.hasLines: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.hasLines: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;
    int c = fgetc(fp);
    if (c == EOF) {
        return 0;  /* No more content */
    }
    ungetc(c, fp);
    return 1;  /* More lines available */
}

/* Check if at end of file */
int rt_text_file_is_eof(RtTextFile *file) {
    if (file == NULL) {
        fprintf(stderr, "TextFile.isEof: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.isEof: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;
    int c = fgetc(fp);
    if (c == EOF) {
        return 1;  /* At EOF */
    }
    ungetc(c, fp);
    return 0;  /* Not at EOF */
}

/* Get current byte position in file */
long rt_text_file_position(RtTextFile *file) {
    if (file == NULL) {
        fprintf(stderr, "TextFile.position: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.position: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;
    long pos = ftell(fp);
    if (pos < 0) {
        fprintf(stderr, "TextFile.position: failed to get position in file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }
    return pos;
}

/* Seek to byte position */
void rt_text_file_seek(RtTextFile *file, long pos) {
    if (file == NULL) {
        fprintf(stderr, "TextFile.seek: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.seek: file is not open\n");
        exit(1);
    }
    if (pos < 0) {
        fprintf(stderr, "TextFile.seek: invalid position %ld\n", pos);
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;
    if (fseek(fp, pos, SEEK_SET) != 0) {
        fprintf(stderr, "TextFile.seek: failed to seek in file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }
}

/* Return to beginning of file */
void rt_text_file_rewind(RtTextFile *file) {
    if (file == NULL) {
        fprintf(stderr, "TextFile.rewind: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.rewind: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;
    rewind(fp);
}

/* Force buffered data to disk */
void rt_text_file_flush(RtTextFile *file) {
    if (file == NULL) {
        fprintf(stderr, "TextFile.flush: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.flush: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;
    if (fflush(fp) != 0) {
        fprintf(stderr, "TextFile.flush: failed to flush file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }
}

/* ============================================================
   TextFile Properties
   ============================================================ */

/* Get full file path */
char *rt_text_file_get_path(RtArena *arena, RtTextFile *file) {
    if (arena == NULL) {
        fprintf(stderr, "TextFile.path: arena is NULL\n");
        exit(1);
    }
    if (file == NULL) {
        fprintf(stderr, "TextFile.path: file is NULL\n");
        exit(1);
    }

    if (file->path == NULL) {
        /* Return empty string if path is not set */
        char *empty = rt_arena_alloc(arena, 1);
        empty[0] = '\0';
        return empty;
    }

    /* Copy path string to arena */
    size_t len = strlen(file->path);
    char *result = rt_arena_alloc(arena, len + 1);
    memcpy(result, file->path, len + 1);
    return result;
}

/* Get filename only (without directory) */
char *rt_text_file_get_name(RtArena *arena, RtTextFile *file) {
    if (arena == NULL) {
        fprintf(stderr, "TextFile.name: arena is NULL\n");
        exit(1);
    }
    if (file == NULL) {
        fprintf(stderr, "TextFile.name: file is NULL\n");
        exit(1);
    }

    if (file->path == NULL) {
        /* Return empty string if path is not set */
        char *empty = rt_arena_alloc(arena, 1);
        empty[0] = '\0';
        return empty;
    }

    /* Find last path separator */
    const char *path = file->path;
    const char *last_sep = strrchr(path, '/');
    #ifdef _WIN32
    const char *last_backslash = strrchr(path, '\\');
    if (last_backslash != NULL && (last_sep == NULL || last_backslash > last_sep)) {
        last_sep = last_backslash;
    }
    #endif

    const char *name = (last_sep != NULL) ? last_sep + 1 : path;
    size_t len = strlen(name);
    char *result = rt_arena_alloc(arena, len + 1);
    memcpy(result, name, len + 1);
    return result;
}

/* Get file size in bytes */
long rt_text_file_get_size(RtTextFile *file) {
    if (file == NULL) {
        fprintf(stderr, "TextFile.size: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "TextFile.size: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;

    /* Save current position */
    long current_pos = ftell(fp);
    if (current_pos < 0) {
        fprintf(stderr, "TextFile.size: failed to get position in file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    /* Seek to end to get size */
    if (fseek(fp, 0, SEEK_END) != 0) {
        fprintf(stderr, "TextFile.size: failed to seek in file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    long size = ftell(fp);
    if (size < 0) {
        fprintf(stderr, "TextFile.size: failed to get size of file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    /* Restore original position */
    if (fseek(fp, current_pos, SEEK_SET) != 0) {
        fprintf(stderr, "TextFile.size: failed to restore position in file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    return size;
}

/* ============================================================
   BinaryFile Static Methods
   ============================================================ */

/* Open binary file for reading and writing (panics if file doesn't exist) */
RtBinaryFile *rt_binary_file_open(RtArena *arena, const char *path) {
    if (arena == NULL) {
        fprintf(stderr, "BinaryFile.open: arena is NULL\n");
        exit(1);
    }
    if (path == NULL) {
        fprintf(stderr, "BinaryFile.open: path is NULL\n");
        exit(1);
    }

    /* Try to open in r+b (read/write binary, must exist) */
    FILE *fp = fopen(path, "r+b");
    if (fp == NULL) {
        /* If file doesn't exist, create it with w+b */
        fp = fopen(path, "w+b");
        if (fp == NULL) {
            fprintf(stderr, "BinaryFile.open: failed to open file '%s': %s\n",
                    path, strerror(errno));
            exit(1);
        }
    }

    /* Allocate file handle */
    RtBinaryFile *file = rt_arena_alloc(arena, sizeof(RtBinaryFile));
    if (file == NULL) {
        fclose(fp);
        fprintf(stderr, "BinaryFile.open: memory allocation failed\n");
        exit(1);
    }

    /* Copy path to arena */
    size_t path_len = strlen(path);
    char *path_copy = rt_arena_alloc(arena, path_len + 1);
    if (path_copy == NULL) {
        fclose(fp);
        fprintf(stderr, "BinaryFile.open: memory allocation failed for path\n");
        exit(1);
    }
    memcpy(path_copy, path, path_len + 1);

    file->fp = fp;
    file->path = path_copy;
    file->is_open = true;

    /* Track file handle in arena for auto-close */
    RtFileHandle *handle = rt_arena_alloc(arena, sizeof(RtFileHandle));
    if (handle == NULL) {
        fclose(fp);
        fprintf(stderr, "BinaryFile.open: memory allocation failed for handle\n");
        exit(1);
    }
    handle->fp = fp;
    handle->path = path_copy;
    handle->is_open = true;
    handle->next = arena->open_files;
    arena->open_files = handle;
    file->handle = handle;

    return file;
}

/* Check if binary file exists without opening */
int rt_binary_file_exists(const char *path) {
    if (path == NULL) {
        return 0;
    }
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        return 0;
    }
    fclose(fp);
    return 1;
}

/* Read entire binary file contents as byte array */
unsigned char *rt_binary_file_read_all(RtArena *arena, const char *path) {
    if (arena == NULL) {
        fprintf(stderr, "BinaryFile.readAll: arena is NULL\n");
        exit(1);
    }
    if (path == NULL) {
        fprintf(stderr, "BinaryFile.readAll: path is NULL\n");
        exit(1);
    }

    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "BinaryFile.readAll: failed to open file '%s': %s\n",
                path, strerror(errno));
        exit(1);
    }

    /* Get file size */
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        fprintf(stderr, "BinaryFile.readAll: failed to seek in file '%s': %s\n",
                path, strerror(errno));
        exit(1);
    }

    long size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        fprintf(stderr, "BinaryFile.readAll: failed to get size of file '%s': %s\n",
                path, strerror(errno));
        exit(1);
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        fprintf(stderr, "BinaryFile.readAll: failed to rewind file '%s': %s\n",
                path, strerror(errno));
        exit(1);
    }

    /* Create byte array with metadata */
    unsigned char *data = rt_array_create_byte_uninit(arena, (size_t)size);

    /* Read file contents */
    if (size > 0) {
        size_t bytes_read = fread(data, 1, (size_t)size, fp);
        if (bytes_read != (size_t)size) {
            fclose(fp);
            fprintf(stderr, "BinaryFile.readAll: failed to read file '%s': %s\n",
                    path, strerror(errno));
            exit(1);
        }
    }

    fclose(fp);
    return data;
}

/* Write byte array to binary file (creates or overwrites) */
void rt_binary_file_write_all(const char *path, unsigned char *data) {
    if (path == NULL) {
        fprintf(stderr, "BinaryFile.writeAll: path is NULL\n");
        exit(1);
    }

    FILE *fp = fopen(path, "wb");
    if (fp == NULL) {
        fprintf(stderr, "BinaryFile.writeAll: failed to create file '%s': %s\n",
                path, strerror(errno));
        exit(1);
    }

    if (data != NULL) {
        size_t len = rt_array_length(data);
        if (len > 0) {
            size_t written = fwrite(data, 1, len, fp);
            if (written != len) {
                fclose(fp);
                fprintf(stderr, "BinaryFile.writeAll: failed to write file '%s': %s\n",
                        path, strerror(errno));
                exit(1);
            }
        }
    }

    fclose(fp);
}

/* Delete binary file (panics if fails) */
void rt_binary_file_delete(const char *path) {
    if (path == NULL) {
        fprintf(stderr, "BinaryFile.delete: path is NULL\n");
        exit(1);
    }

    if (remove(path) != 0) {
        fprintf(stderr, "BinaryFile.delete: failed to delete file '%s': %s\n",
                path, strerror(errno));
        exit(1);
    }
}

/* Copy binary file to new location */
void rt_binary_file_copy(const char *src, const char *dst) {
    if (src == NULL) {
        fprintf(stderr, "BinaryFile.copy: source path is NULL\n");
        exit(1);
    }
    if (dst == NULL) {
        fprintf(stderr, "BinaryFile.copy: destination path is NULL\n");
        exit(1);
    }

    FILE *src_fp = fopen(src, "rb");
    if (src_fp == NULL) {
        fprintf(stderr, "BinaryFile.copy: failed to open source file '%s': %s\n",
                src, strerror(errno));
        exit(1);
    }

    FILE *dst_fp = fopen(dst, "wb");
    if (dst_fp == NULL) {
        fclose(src_fp);
        fprintf(stderr, "BinaryFile.copy: failed to create destination file '%s': %s\n",
                dst, strerror(errno));
        exit(1);
    }

    /* Copy in chunks */
    unsigned char buffer[4096];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src_fp)) > 0) {
        size_t bytes_written = fwrite(buffer, 1, bytes_read, dst_fp);
        if (bytes_written != bytes_read) {
            fclose(src_fp);
            fclose(dst_fp);
            fprintf(stderr, "BinaryFile.copy: failed to write to '%s': %s\n",
                    dst, strerror(errno));
            exit(1);
        }
    }

    if (ferror(src_fp)) {
        fclose(src_fp);
        fclose(dst_fp);
        fprintf(stderr, "BinaryFile.copy: failed to read from '%s': %s\n",
                src, strerror(errno));
        exit(1);
    }

    fclose(src_fp);
    fclose(dst_fp);
}

/* Move/rename binary file */
void rt_binary_file_move(const char *src, const char *dst) {
    if (src == NULL) {
        fprintf(stderr, "BinaryFile.move: source path is NULL\n");
        exit(1);
    }
    if (dst == NULL) {
        fprintf(stderr, "BinaryFile.move: destination path is NULL\n");
        exit(1);
    }

    /* Try rename first (efficient, same filesystem) */
    if (rename(src, dst) == 0) {
        return;
    }

    /* If rename fails, try copy + delete (cross-filesystem) */
    rt_binary_file_copy(src, dst);
    if (remove(src) != 0) {
        fprintf(stderr, "BinaryFile.move: failed to remove source file '%s': %s\n",
                src, strerror(errno));
        exit(1);
    }
}

/* Close an open binary file */
void rt_binary_file_close(RtBinaryFile *file) {
    if (file == NULL) {
        return;
    }
    if (file->is_open && file->fp != NULL) {
        fclose((FILE *)file->fp);
        file->is_open = false;
        file->fp = NULL;
        /* Mark handle as closed too */
        if (file->handle != NULL) {
            file->handle->is_open = false;
        }
    }
}

/* ============================================================
   BinaryFile Instance Reading Methods
   ============================================================ */

/* Read single byte, returns -1 on EOF */
long rt_binary_file_read_byte(RtBinaryFile *file) {
    if (file == NULL) {
        fprintf(stderr, "BinaryFile.readByte: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "BinaryFile.readByte: file is not open\n");
        exit(1);
    }

    int c = fgetc((FILE *)file->fp);
    if (c == EOF) {
        if (ferror((FILE *)file->fp)) {
            fprintf(stderr, "BinaryFile.readByte: read error on file '%s': %s\n",
                    file->path ? file->path : "(unknown)", strerror(errno));
            exit(1);
        }
        return -1;  /* EOF */
    }
    return (long)(unsigned char)c;
}

/* Read N bytes into new array */
unsigned char *rt_binary_file_read_bytes(RtArena *arena, RtBinaryFile *file, long count) {
    if (arena == NULL) {
        fprintf(stderr, "BinaryFile.readBytes: arena is NULL\n");
        exit(1);
    }
    if (file == NULL) {
        fprintf(stderr, "BinaryFile.readBytes: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "BinaryFile.readBytes: file is not open\n");
        exit(1);
    }
    if (count < 0) {
        fprintf(stderr, "BinaryFile.readBytes: count cannot be negative\n");
        exit(1);
    }

    /* Create byte array */
    unsigned char *data = rt_array_create_byte_uninit(arena, (size_t)count);

    /* Read bytes */
    if (count > 0) {
        size_t bytes_read = fread(data, 1, (size_t)count, (FILE *)file->fp);
        /* If we read fewer bytes, we need to update the array length */
        if (bytes_read < (size_t)count) {
            /* Update the metadata in the ArrayMetadata struct */
            ArrayMetadata *meta = ((ArrayMetadata *)data) - 1;
            meta->size = bytes_read;
        }
    }

    return data;
}

/* Read all remaining bytes from open file */
unsigned char *rt_binary_file_instance_read_all(RtArena *arena, RtBinaryFile *file) {
    if (arena == NULL) {
        fprintf(stderr, "BinaryFile.readAll: arena is NULL\n");
        exit(1);
    }
    if (file == NULL) {
        fprintf(stderr, "BinaryFile.readAll: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "BinaryFile.readAll: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;

    /* Get current position and file size */
    long current_pos = ftell(fp);
    if (current_pos < 0) {
        fprintf(stderr, "BinaryFile.readAll: failed to get position in file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fprintf(stderr, "BinaryFile.readAll: failed to seek in file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    long end_pos = ftell(fp);
    if (end_pos < 0) {
        fprintf(stderr, "BinaryFile.readAll: failed to get size of file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    /* Seek back to current position */
    if (fseek(fp, current_pos, SEEK_SET) != 0) {
        fprintf(stderr, "BinaryFile.readAll: failed to seek in file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    /* Calculate remaining bytes */
    size_t remaining = (size_t)(end_pos - current_pos);

    /* Create byte array */
    unsigned char *data = rt_array_create_byte_uninit(arena, remaining);

    /* Read remaining content */
    if (remaining > 0) {
        size_t bytes_read = fread(data, 1, remaining, fp);
        if (ferror(fp)) {
            fprintf(stderr, "BinaryFile.readAll: failed to read file '%s': %s\n",
                    file->path ? file->path : "(unknown)", strerror(errno));
            exit(1);
        }
        /* Update length if fewer bytes read */
        if (bytes_read < remaining) {
            ArrayMetadata *meta = ((ArrayMetadata *)data) - 1;
            meta->size = bytes_read;
        }
    }

    return data;
}

/* Read into byte buffer, returns number of bytes read */
long rt_binary_file_read_into(RtBinaryFile *file, unsigned char *buffer) {
    if (file == NULL) {
        fprintf(stderr, "BinaryFile.readInto: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "BinaryFile.readInto: file is not open\n");
        exit(1);
    }
    if (buffer == NULL) {
        fprintf(stderr, "BinaryFile.readInto: buffer is NULL\n");
        exit(1);
    }

    /* Get the buffer's length from its metadata */
    size_t buf_len = rt_array_length(buffer);
    if (buf_len == 0) {
        return 0;  /* Empty buffer, nothing to read */
    }

    FILE *fp = (FILE *)file->fp;

    /* Read up to buf_len bytes */
    size_t bytes_read = fread(buffer, 1, buf_len, fp);
    if (ferror(fp)) {
        fprintf(stderr, "BinaryFile.readInto: read error on file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    return (long)bytes_read;
}

/* ============================================================
   BinaryFile Instance Writing Methods
   ============================================================ */

/* Write single byte to file */
void rt_binary_file_write_byte(RtBinaryFile *file, long b) {
    if (file == NULL) {
        fprintf(stderr, "BinaryFile.writeByte: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "BinaryFile.writeByte: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;
    if (fputc((unsigned char)b, fp) == EOF) {
        fprintf(stderr, "BinaryFile.writeByte: write error on file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }
}

/* Write byte array to file */
void rt_binary_file_write_bytes(RtBinaryFile *file, unsigned char *data) {
    if (file == NULL) {
        fprintf(stderr, "BinaryFile.writeBytes: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "BinaryFile.writeBytes: file is not open\n");
        exit(1);
    }
    if (data == NULL) {
        return;  /* Nothing to write */
    }

    FILE *fp = (FILE *)file->fp;
    size_t len = rt_array_length(data);
    if (len > 0) {
        size_t written = fwrite(data, 1, len, fp);
        if (written != len) {
            fprintf(stderr, "BinaryFile.writeBytes: write error on file '%s': %s\n",
                    file->path ? file->path : "(unknown)", strerror(errno));
            exit(1);
        }
    }
}

/* ============================================================
   BinaryFile State Methods
   ============================================================ */

/* Check if more bytes are available to read */
int rt_binary_file_has_bytes(RtBinaryFile *file) {
    if (file == NULL) {
        fprintf(stderr, "BinaryFile.hasBytes: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "BinaryFile.hasBytes: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;
    int c = fgetc(fp);
    if (c == EOF) {
        return 0;  /* No more bytes */
    }
    ungetc(c, fp);
    return 1;  /* More bytes available */
}

/* Check if at end of file */
int rt_binary_file_is_eof(RtBinaryFile *file) {
    if (file == NULL) {
        fprintf(stderr, "BinaryFile.isEof: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "BinaryFile.isEof: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;
    int c = fgetc(fp);
    if (c == EOF) {
        return 1;  /* At EOF */
    }
    ungetc(c, fp);
    return 0;  /* Not at EOF */
}

/* Get current byte position in file */
long rt_binary_file_position(RtBinaryFile *file) {
    if (file == NULL) {
        fprintf(stderr, "BinaryFile.position: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "BinaryFile.position: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;
    long pos = ftell(fp);
    if (pos < 0) {
        fprintf(stderr, "BinaryFile.position: failed to get position in file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }
    return pos;
}

/* Seek to byte position */
void rt_binary_file_seek(RtBinaryFile *file, long pos) {
    if (file == NULL) {
        fprintf(stderr, "BinaryFile.seek: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "BinaryFile.seek: file is not open\n");
        exit(1);
    }
    if (pos < 0) {
        fprintf(stderr, "BinaryFile.seek: invalid position %ld\n", pos);
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;
    if (fseek(fp, pos, SEEK_SET) != 0) {
        fprintf(stderr, "BinaryFile.seek: failed to seek in file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }
}

/* Return to beginning of file */
void rt_binary_file_rewind(RtBinaryFile *file) {
    if (file == NULL) {
        fprintf(stderr, "BinaryFile.rewind: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "BinaryFile.rewind: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;
    rewind(fp);
}

/* Force buffered data to disk */
void rt_binary_file_flush(RtBinaryFile *file) {
    if (file == NULL) {
        fprintf(stderr, "BinaryFile.flush: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "BinaryFile.flush: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;
    if (fflush(fp) != 0) {
        fprintf(stderr, "BinaryFile.flush: failed to flush file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }
}

/* ============================================================
   BinaryFile Properties
   ============================================================ */

/* Get full file path */
char *rt_binary_file_get_path(RtArena *arena, RtBinaryFile *file) {
    if (arena == NULL) {
        fprintf(stderr, "BinaryFile.path: arena is NULL\n");
        exit(1);
    }
    if (file == NULL) {
        fprintf(stderr, "BinaryFile.path: file is NULL\n");
        exit(1);
    }

    if (file->path == NULL) {
        /* Return empty string if path is not set */
        char *empty = rt_arena_alloc(arena, 1);
        empty[0] = '\0';
        return empty;
    }

    /* Copy path string to arena */
    size_t len = strlen(file->path);
    char *result = rt_arena_alloc(arena, len + 1);
    memcpy(result, file->path, len + 1);
    return result;
}

/* Get filename only (without directory) */
char *rt_binary_file_get_name(RtArena *arena, RtBinaryFile *file) {
    if (arena == NULL) {
        fprintf(stderr, "BinaryFile.name: arena is NULL\n");
        exit(1);
    }
    if (file == NULL) {
        fprintf(stderr, "BinaryFile.name: file is NULL\n");
        exit(1);
    }

    if (file->path == NULL) {
        /* Return empty string if path is not set */
        char *empty = rt_arena_alloc(arena, 1);
        empty[0] = '\0';
        return empty;
    }

    /* Find last path separator */
    const char *path = file->path;
    const char *last_sep = strrchr(path, '/');
    #ifdef _WIN32
    const char *last_backslash = strrchr(path, '\\');
    if (last_backslash != NULL && (last_sep == NULL || last_backslash > last_sep)) {
        last_sep = last_backslash;
    }
    #endif

    const char *name = (last_sep != NULL) ? last_sep + 1 : path;
    size_t len = strlen(name);
    char *result = rt_arena_alloc(arena, len + 1);
    memcpy(result, name, len + 1);
    return result;
}

/* Get file size in bytes */
long rt_binary_file_get_size(RtBinaryFile *file) {
    if (file == NULL) {
        fprintf(stderr, "BinaryFile.size: file is NULL\n");
        exit(1);
    }
    if (!file->is_open || file->fp == NULL) {
        fprintf(stderr, "BinaryFile.size: file is not open\n");
        exit(1);
    }

    FILE *fp = (FILE *)file->fp;

    /* Save current position */
    long current_pos = ftell(fp);
    if (current_pos < 0) {
        fprintf(stderr, "BinaryFile.size: failed to get position in file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    /* Seek to end to get size */
    if (fseek(fp, 0, SEEK_END) != 0) {
        fprintf(stderr, "BinaryFile.size: failed to seek in file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    long size = ftell(fp);
    if (size < 0) {
        fprintf(stderr, "BinaryFile.size: failed to get size of file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    /* Restore original position */
    if (fseek(fp, current_pos, SEEK_SET) != 0) {
        fprintf(stderr, "BinaryFile.size: failed to restore position in file '%s': %s\n",
                file->path ? file->path : "(unknown)", strerror(errno));
        exit(1);
    }

    return size;
}

/* ============================================================================
 * Standard Stream Operations (Stdin, Stdout, Stderr)
 * ============================================================================ */

/* Stdin - read line from standard input */
char *rt_stdin_read_line(RtArena *arena) {
    /* Read a line from stdin, stripping trailing newline */
    char buffer[4096];
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        /* EOF or error - return empty string */
        char *result = rt_arena_alloc(arena, 1);
        result[0] = '\0';
        return result;
    }
    
    /* Strip trailing newline if present */
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
        len--;
    }
    
    char *result = rt_arena_alloc(arena, len + 1);
    memcpy(result, buffer, len + 1);
    return result;
}

/* Stdin - read single character from standard input */
long rt_stdin_read_char(void) {
    int ch = fgetc(stdin);
    return ch;  /* Returns -1 on EOF */
}

/* Stdin - read whitespace-delimited word from standard input */
char *rt_stdin_read_word(RtArena *arena) {
    char buffer[4096];
    if (scanf("%4095s", buffer) != 1) {
        /* EOF or error - return empty string */
        char *result = rt_arena_alloc(arena, 1);
        result[0] = '\0';
        return result;
    }
    
    size_t len = strlen(buffer);
    char *result = rt_arena_alloc(arena, len + 1);
    memcpy(result, buffer, len + 1);
    return result;
}

/* Stdin - check if characters available */
int rt_stdin_has_chars(void) {
    int ch = fgetc(stdin);
    if (ch == EOF) {
        return 0;
    }
    ungetc(ch, stdin);
    return 1;
}

/* Stdin - check if lines available */
int rt_stdin_has_lines(void) {
    /* Same as has_chars for stdin */
    return rt_stdin_has_chars();
}

/* Stdin - check if at EOF */
int rt_stdin_is_eof(void) {
    return feof(stdin);
}

/* Stdout - write text */
void rt_stdout_write(const char *text) {
    if (text != NULL) {
        fputs(text, stdout);
    }
}

/* Stdout - write text with newline */
void rt_stdout_write_line(const char *text) {
    if (text != NULL) {
        fputs(text, stdout);
    }
    fputc('\n', stdout);
}

/* Stdout - flush output */
void rt_stdout_flush(void) {
    fflush(stdout);
}

/* Stderr - write text */
void rt_stderr_write(const char *text) {
    if (text != NULL) {
        fputs(text, stderr);
    }
}

/* Stderr - write text with newline */
void rt_stderr_write_line(const char *text) {
    if (text != NULL) {
        fputs(text, stderr);
    }
    fputc('\n', stderr);
}

/* Stderr - flush output */
void rt_stderr_flush(void) {
    fflush(stderr);
}

/* Global convenience: read line */
char *rt_read_line(RtArena *arena) {
    return rt_stdin_read_line(arena);
}

/* Global convenience: print with newline */
void rt_println(const char *text) {
    rt_stdout_write_line(text);
}

/* Global convenience: print to stderr */
void rt_print_err(const char *text) {
    rt_stderr_write(text);
}

/* Global convenience: print to stderr with newline */
void rt_print_err_ln(const char *text) {
    rt_stderr_write_line(text);
}

/* ============================================================================
 * Byte Array Extension Methods
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

/* ============================================================================
 * Path Utilities
 * ============================================================================ */

/* Platform-specific path separator */
#ifdef _WIN32
#define PATH_SEPARATOR '\\'
#define PATH_SEPARATOR_STR "\\"
#else
#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR_STR "/"
#endif

/* Helper: Check if character is a path separator */
static int is_path_separator(char c) {
#ifdef _WIN32
    return c == '/' || c == '\\';
#else
    return c == '/';
#endif
}

/* Helper: Find the last path separator in a string */
static const char *find_last_separator(const char *path) {
    const char *last = NULL;
    for (const char *p = path; *p; p++) {
        if (is_path_separator(*p)) {
            last = p;
        }
    }
    return last;
}

/* Extract directory portion of a path */
char *rt_path_directory(RtArena *arena, const char *path) {
    if (path == NULL || *path == '\0') {
        return rt_arena_strdup(arena, ".");
    }

    const char *last_sep = find_last_separator(path);
    if (last_sep == NULL) {
        /* No separator found - return current directory */
        return rt_arena_strdup(arena, ".");
    }

    /* Handle root path (/ or C:\) */
    if (last_sep == path) {
        return rt_arena_strdup(arena, PATH_SEPARATOR_STR);
    }

#ifdef _WIN32
    /* Handle Windows drive letter like C:\ */
    if (last_sep == path + 2 && path[1] == ':') {
        char *result = rt_arena_alloc(arena, 4);
        result[0] = path[0];
        result[1] = ':';
        result[2] = PATH_SEPARATOR;
        result[3] = '\0';
        return result;
    }
#endif

    /* Return everything up to (not including) the last separator */
    size_t dir_len = last_sep - path;
    char *result = rt_arena_alloc(arena, dir_len + 1);
    memcpy(result, path, dir_len);
    result[dir_len] = '\0';
    return result;
}

/* Extract filename (with extension) from a path */
char *rt_path_filename(RtArena *arena, const char *path) {
    if (path == NULL || *path == '\0') {
        return rt_arena_strdup(arena, "");
    }

    const char *last_sep = find_last_separator(path);
    if (last_sep == NULL) {
        /* No separator - the whole thing is the filename */
        return rt_arena_strdup(arena, path);
    }

    /* Return everything after the last separator */
    return rt_arena_strdup(arena, last_sep + 1);
}

/* Extract file extension (without dot) from a path */
char *rt_path_extension(RtArena *arena, const char *path) {
    if (path == NULL || *path == '\0') {
        return rt_arena_strdup(arena, "");
    }

    /* Get just the filename part first */
    const char *last_sep = find_last_separator(path);
    const char *filename = last_sep ? last_sep + 1 : path;

    /* Find the last dot in the filename */
    const char *last_dot = NULL;
    for (const char *p = filename; *p; p++) {
        if (*p == '.') {
            last_dot = p;
        }
    }

    /* No dot, or dot is at start (hidden file like .bashrc) */
    if (last_dot == NULL || last_dot == filename) {
        return rt_arena_strdup(arena, "");
    }

    /* Return extension without the dot */
    return rt_arena_strdup(arena, last_dot + 1);
}

/* Join two path components */
char *rt_path_join2(RtArena *arena, const char *path1, const char *path2) {
    if (path1 == NULL) path1 = "";
    if (path2 == NULL) path2 = "";

    size_t len1 = strlen(path1);
    size_t len2 = strlen(path2);

    /* If path2 is absolute, return it directly */
    if (len2 > 0 && is_path_separator(path2[0])) {
        return rt_arena_strdup(arena, path2);
    }
#ifdef _WIN32
    /* Check for Windows absolute path like C:\ */
    if (len2 > 2 && path2[1] == ':' && is_path_separator(path2[2])) {
        return rt_arena_strdup(arena, path2);
    }
#endif

    /* If path1 is empty, return path2 */
    if (len1 == 0) {
        return rt_arena_strdup(arena, path2);
    }

    /* Check if path1 already ends with separator */
    int has_trailing_sep = is_path_separator(path1[len1 - 1]);

    /* Allocate: path1 + optional separator + path2 + null */
    size_t result_len = len1 + (has_trailing_sep ? 0 : 1) + len2 + 1;
    char *result = rt_arena_alloc(arena, result_len);

    memcpy(result, path1, len1);
    size_t pos = len1;
    if (!has_trailing_sep) {
        result[pos++] = PATH_SEPARATOR;
    }
    memcpy(result + pos, path2, len2);
    result[pos + len2] = '\0';

    return result;
}

/* Join three path components */
char *rt_path_join3(RtArena *arena, const char *path1, const char *path2, const char *path3) {
    char *temp = rt_path_join2(arena, path1, path2);
    return rt_path_join2(arena, temp, path3);
}

/* Resolve a path to its absolute form */
char *rt_path_absolute(RtArena *arena, const char *path) {
    if (path == NULL || *path == '\0') {
        /* Empty path - return current working directory */
        char cwd[4096];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            return rt_arena_strdup(arena, cwd);
        }
        /* Fallback if getcwd fails */
        return rt_arena_strdup(arena, ".");
    }

#ifdef _WIN32
    char resolved[_MAX_PATH];
    if (_fullpath(resolved, path, _MAX_PATH) != NULL) {
        return rt_arena_strdup(arena, resolved);
    }
#else
    char resolved[PATH_MAX];
    if (realpath(path, resolved) != NULL) {
        return rt_arena_strdup(arena, resolved);
    }

    /* realpath fails if path doesn't exist - try to resolve manually */
    if (path[0] == '/') {
        /* Already absolute */
        return rt_arena_strdup(arena, path);
    }

    /* Prepend current working directory */
    char cwd[4096];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        return rt_path_join2(arena, cwd, path);
    }
#endif

    /* Fallback - return as-is */
    return rt_arena_strdup(arena, path);
}

/* Check if a path exists */
int rt_path_exists(const char *path) {
    if (path == NULL) return 0;
    struct stat st;
    return stat(path, &st) == 0;
}

/* Check if a path points to a regular file */
int rt_path_is_file(const char *path) {
    if (path == NULL) return 0;
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return S_ISREG(st.st_mode);
}

/* Check if a path points to a directory */
int rt_path_is_directory(const char *path) {
    if (path == NULL) return 0;
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return S_ISDIR(st.st_mode);
}

/* ============================================================================
 * Directory Operations
 * ============================================================================ */

#include <dirent.h>

/* Helper: Create a string array with metadata (similar to other array types) */
static char **create_string_array(RtArena *arena, size_t initial_capacity) {
    size_t capacity = initial_capacity > 4 ? initial_capacity : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "create_string_array: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = 0;
    meta->capacity = capacity;
    return (char **)(meta + 1);
}

/* Helper: Push a string onto a string array */
static char **push_string_to_array(RtArena *arena, char **arr, const char *str) {
    ArrayMetadata *meta = ((ArrayMetadata *)arr) - 1;
    RtArena *alloc_arena = meta->arena ? meta->arena : arena;

    if ((size_t)meta->size >= meta->capacity) {
        /* Need to grow the array */
        size_t new_capacity = meta->capacity * 2;
        ArrayMetadata *new_meta = rt_arena_alloc(alloc_arena, sizeof(ArrayMetadata) + new_capacity * sizeof(char *));
        if (new_meta == NULL) {
            fprintf(stderr, "push_string_to_array: allocation failed\n");
            exit(1);
        }
        new_meta->arena = alloc_arena;
        new_meta->size = meta->size;
        new_meta->capacity = new_capacity;
        char **new_arr = (char **)(new_meta + 1);
        memcpy(new_arr, arr, meta->size * sizeof(char *));
        arr = new_arr;
        meta = new_meta;
    }

    arr[meta->size] = rt_arena_strdup(alloc_arena, str);
    meta->size++;
    return arr;
}

/* List files in a directory (non-recursive) */
char **rt_directory_list(RtArena *arena, const char *path) {
    if (path == NULL) {
        fprintf(stderr, "Directory.list: path cannot be null\n");
        exit(1);
    }

    DIR *dir = opendir(path);
    if (dir == NULL) {
        fprintf(stderr, "Directory.list: cannot open directory '%s': %s\n",
                path, strerror(errno));
        exit(1);
    }

    char **result = create_string_array(arena, 16);
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        /* Skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        result = push_string_to_array(arena, result, entry->d_name);
    }

    closedir(dir);
    return result;
}

/* Helper for recursive directory listing */
static char **list_recursive_helper(RtArena *arena, char **result, const char *base_path, const char *rel_prefix) {
    DIR *dir = opendir(base_path);
    if (dir == NULL) {
        return result;  /* Skip directories we can't open */
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        /* Skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        /* Build full path for stat check */
        char *full_path = rt_path_join2(arena, base_path, entry->d_name);

        /* Build relative path for result */
        char *rel_path;
        if (rel_prefix[0] == '\0') {
            rel_path = rt_arena_strdup(arena, entry->d_name);
        } else {
            rel_path = rt_path_join2(arena, rel_prefix, entry->d_name);
        }

        /* Add this entry */
        result = push_string_to_array(arena, result, rel_path);

        /* If it's a directory, recurse */
        struct stat st;
        if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {
            result = list_recursive_helper(arena, result, full_path, rel_path);
        }
    }

    closedir(dir);
    return result;
}

/* List files in a directory recursively */
char **rt_directory_list_recursive(RtArena *arena, const char *path) {
    if (path == NULL) {
        fprintf(stderr, "Directory.listRecursive: path cannot be null\n");
        exit(1);
    }

    if (!rt_path_is_directory(path)) {
        fprintf(stderr, "Directory.listRecursive: '%s' is not a directory\n", path);
        exit(1);
    }

    char **result = create_string_array(arena, 64);
    return list_recursive_helper(arena, result, path, "");
}

/* Helper: Create directory and all parents */
static int create_directory_recursive(const char *path) {
    if (path == NULL || *path == '\0') return 0;

    /* Check if it already exists */
    struct stat st;
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return 0;  /* Already exists and is a directory */
        }
        return -1;  /* Exists but is not a directory */
    }

    /* Make a copy we can modify */
    size_t len = strlen(path);
    char *path_copy = malloc(len + 1);
    if (path_copy == NULL) return -1;
    strcpy(path_copy, path);

    /* Create parent directories first */
    char *p = path_copy;

    /* Skip leading slashes for absolute paths */
    while (*p == '/') p++;

    while (*p) {
        /* Find next slash */
        while (*p && *p != '/') p++;

        if (*p == '/') {
            *p = '\0';
            if (path_copy[0] != '\0') {
                if (stat(path_copy, &st) != 0) {
                    if (mkdir(path_copy, 0755) != 0 && errno != EEXIST) {
                        free(path_copy);
                        return -1;
                    }
                }
            }
            *p = '/';
            p++;
        }
    }

    /* Create final directory */
    int result = mkdir(path_copy, 0755);
    free(path_copy);

    if (result != 0 && errno != EEXIST) {
        return -1;
    }
    return 0;
}

/* Create a directory (including parents if needed) */
void rt_directory_create(const char *path) {
    if (path == NULL) {
        fprintf(stderr, "Directory.create: path cannot be null\n");
        exit(1);
    }

    if (create_directory_recursive(path) != 0) {
        fprintf(stderr, "Directory.create: failed to create directory '%s': %s\n",
                path, strerror(errno));
        exit(1);
    }
}

/* Delete an empty directory */
void rt_directory_delete(const char *path) {
    if (path == NULL) {
        fprintf(stderr, "Directory.delete: path cannot be null\n");
        exit(1);
    }

    if (rmdir(path) != 0) {
        if (errno == ENOTEMPTY) {
            fprintf(stderr, "Directory.delete: directory '%s' is not empty\n", path);
        } else {
            fprintf(stderr, "Directory.delete: failed to delete directory '%s': %s\n",
                    path, strerror(errno));
        }
        exit(1);
    }
}

/* Helper: Recursively delete directory contents */
static int delete_recursive_helper(const char *path) {
    DIR *dir = opendir(path);
    if (dir == NULL) {
        return -1;
    }

    struct dirent *entry;
    int result = 0;

    while ((entry = readdir(dir)) != NULL && result == 0) {
        /* Skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        /* Build full path */
        size_t path_len = strlen(path);
        size_t name_len = strlen(entry->d_name);
        char *full_path = malloc(path_len + 1 + name_len + 1);
        if (full_path == NULL) {
            result = -1;
            break;
        }

        strcpy(full_path, path);
        full_path[path_len] = '/';
        strcpy(full_path + path_len + 1, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                /* Recursively delete subdirectory */
                result = delete_recursive_helper(full_path);
                if (result == 0) {
                    result = rmdir(full_path);
                }
            } else {
                /* Delete file */
                result = unlink(full_path);
            }
        }

        free(full_path);
    }

    closedir(dir);
    return result;
}

/* Delete a directory and all its contents recursively */
void rt_directory_delete_recursive(const char *path) {
    if (path == NULL) {
        fprintf(stderr, "Directory.deleteRecursive: path cannot be null\n");
        exit(1);
    }

    if (!rt_path_is_directory(path)) {
        fprintf(stderr, "Directory.deleteRecursive: '%s' is not a directory\n", path);
        exit(1);
    }

    /* First delete contents recursively */
    if (delete_recursive_helper(path) != 0) {
        fprintf(stderr, "Directory.deleteRecursive: failed to delete contents of '%s': %s\n",
                path, strerror(errno));
        exit(1);
    }

    /* Then delete the directory itself */
    if (rmdir(path) != 0) {
        fprintf(stderr, "Directory.deleteRecursive: failed to delete directory '%s': %s\n",
                path, strerror(errno));
        exit(1);
    }
}

/* ============================================================================
 * String Splitting Methods
 * ============================================================================ */

/* Helper: Check if character is whitespace */
static int is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

/* Split string on any whitespace character */
char **rt_str_split_whitespace(RtArena *arena, const char *str) {
    if (str == NULL) {
        return create_string_array(arena, 4);
    }

    char **result = create_string_array(arena, 16);
    const char *p = str;

    while (*p) {
        /* Skip leading whitespace */
        while (*p && is_whitespace(*p)) {
            p++;
        }

        if (*p == '\0') break;

        /* Find end of word */
        const char *start = p;
        while (*p && !is_whitespace(*p)) {
            p++;
        }

        /* Add word to result */
        size_t len = p - start;
        char *word = rt_arena_alloc(arena, len + 1);
        memcpy(word, start, len);
        word[len] = '\0';
        result = push_string_to_array(arena, result, word);
    }

    return result;
}

/* Split string on line endings (\n, \r\n, \r) */
char **rt_str_split_lines(RtArena *arena, const char *str) {
    if (str == NULL) {
        return create_string_array(arena, 4);
    }

    char **result = create_string_array(arena, 16);
    const char *p = str;
    const char *start = str;

    while (*p) {
        if (*p == '\n') {
            /* Unix line ending or end of Windows \r\n */
            size_t len = p - start;
            char *line = rt_arena_alloc(arena, len + 1);
            memcpy(line, start, len);
            line[len] = '\0';
            result = push_string_to_array(arena, result, line);
            p++;
            start = p;
        } else if (*p == '\r') {
            /* Carriage return - check for \r\n or standalone \r */
            size_t len = p - start;
            char *line = rt_arena_alloc(arena, len + 1);
            memcpy(line, start, len);
            line[len] = '\0';
            result = push_string_to_array(arena, result, line);
            p++;
            if (*p == '\n') {
                /* Windows \r\n - skip the \n too */
                p++;
            }
            start = p;
        } else {
            p++;
        }
    }

    /* Add final line if there's remaining content */
    if (p > start) {
        size_t len = p - start;
        char *line = rt_arena_alloc(arena, len + 1);
        memcpy(line, start, len);
        line[len] = '\0';
        result = push_string_to_array(arena, result, line);
    }

    return result;
}

/* Check if string is empty or contains only whitespace */
int rt_str_is_blank(const char *str) {
    if (str == NULL || *str == '\0') {
        return 1;  /* NULL or empty string is blank */
    }

    const char *p = str;
    while (*p) {
        if (!is_whitespace(*p)) {
            return 0;  /* Found non-whitespace character */
        }
        p++;
    }

    return 1;  /* All characters were whitespace */
}

/* ============================================================================
 * Time Methods
 * ============================================================================ */

/* Helper function to create RtTime from milliseconds */
static RtTime *rt_time_create(RtArena *arena, long long milliseconds)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_time_create: NULL arena\n");
        return NULL;
    }
    RtTime *time = rt_arena_alloc(arena, sizeof(RtTime));
    if (time == NULL) {
        fprintf(stderr, "rt_time_create: allocation failed\n");
        exit(1);
    }
    time->milliseconds = milliseconds;
    return time;
}

/* Helper function to decompose RtTime into struct tm components */
static void rt_time_to_tm(RtTime *time, struct tm *tm_result)
{
    if (time == NULL) {
        fprintf(stderr, "rt_time_to_tm: NULL time\n");
        exit(1);
    }
    time_t secs = time->milliseconds / 1000;
    localtime_r(&secs, tm_result);
}

/* Create Time from milliseconds since Unix epoch */
RtTime *rt_time_from_millis(RtArena *arena, long long ms)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_time_from_millis: NULL arena\n");
        return NULL;
    }
    return rt_time_create(arena, ms);
}

/* Create Time from seconds since Unix epoch */
RtTime *rt_time_from_seconds(RtArena *arena, long long s)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_time_from_seconds: NULL arena\n");
        return NULL;
    }
    return rt_time_create(arena, s * 1000);
}

/* Get current local time */
RtTime *rt_time_now(RtArena *arena)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_time_now: NULL arena\n");
        return NULL;
    }
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long milliseconds = (tv.tv_sec * 1000LL) + (tv.tv_usec / 1000);
    return rt_time_create(arena, milliseconds);
}

/* Get current UTC time */
RtTime *rt_time_utc(RtArena *arena)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_time_utc: NULL arena\n");
        return NULL;
    }
    /* gettimeofday returns UTC time (seconds since Unix epoch in UTC) */
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long milliseconds = (tv.tv_sec * 1000LL) + (tv.tv_usec / 1000);
    return rt_time_create(arena, milliseconds);
}

/* Pause execution for specified number of milliseconds */
void rt_time_sleep(long ms)
{
    if (ms <= 0) {
        return;  /* No sleep for zero or negative values */
    }
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

/* Get time as milliseconds since Unix epoch */
long long rt_time_get_millis(RtTime *time)
{
    if (time == NULL) {
        fprintf(stderr, "rt_time_get_millis: NULL time\n");
        return 0;
    }
    return time->milliseconds;
}

/* Get time as seconds since Unix epoch */
long long rt_time_get_seconds(RtTime *time)
{
    if (time == NULL) {
        fprintf(stderr, "rt_time_get_seconds: NULL time\n");
        return 0;
    }
    return time->milliseconds / 1000;
}

/* Get 4-digit year */
long rt_time_get_year(RtTime *time)
{
    if (time == NULL) {
        fprintf(stderr, "rt_time_get_year: NULL time\n");
        return 0;
    }
    struct tm tm;
    rt_time_to_tm(time, &tm);
    return tm.tm_year + 1900;
}

/* Get month (1-12) */
long rt_time_get_month(RtTime *time)
{
    if (time == NULL) {
        fprintf(stderr, "rt_time_get_month: NULL time\n");
        return 0;
    }
    struct tm tm;
    rt_time_to_tm(time, &tm);
    return tm.tm_mon + 1;
}

/* Get day of month (1-31) */
long rt_time_get_day(RtTime *time)
{
    if (time == NULL) {
        fprintf(stderr, "rt_time_get_day: NULL time\n");
        return 0;
    }
    struct tm tm;
    rt_time_to_tm(time, &tm);
    return tm.tm_mday;
}

/* Get hour (0-23) */
long rt_time_get_hour(RtTime *time)
{
    if (time == NULL) {
        fprintf(stderr, "rt_time_get_hour: NULL time\n");
        return 0;
    }
    struct tm tm;
    rt_time_to_tm(time, &tm);
    return tm.tm_hour;
}

/* Get minute (0-59) */
long rt_time_get_minute(RtTime *time)
{
    if (time == NULL) {
        fprintf(stderr, "rt_time_get_minute: NULL time\n");
        return 0;
    }
    struct tm tm;
    rt_time_to_tm(time, &tm);
    return tm.tm_min;
}

/* Get second (0-59) */
long rt_time_get_second(RtTime *time)
{
    if (time == NULL) {
        fprintf(stderr, "rt_time_get_second: NULL time\n");
        return 0;
    }
    struct tm tm;
    rt_time_to_tm(time, &tm);
    return tm.tm_sec;
}

/* Get day of week (0=Sunday, 6=Saturday) */
long rt_time_get_weekday(RtTime *time)
{
    if (time == NULL) {
        fprintf(stderr, "rt_time_get_weekday: NULL time\n");
        return 0;
    }
    struct tm tm;
    rt_time_to_tm(time, &tm);
    return tm.tm_wday;
}

/* Return just the date portion (YYYY-MM-DD) */
char *rt_time_to_date(RtArena *arena, RtTime *time)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_time_to_date: NULL arena\n");
        return NULL;
    }
    if (time == NULL) {
        fprintf(stderr, "rt_time_to_date: NULL time\n");
        return NULL;
    }
    struct tm tm;
    rt_time_to_tm(time, &tm);
    char *result = rt_arena_alloc(arena, 16);
    if (result == NULL) {
        fprintf(stderr, "rt_time_to_date: allocation failed\n");
        exit(1);
    }
    sprintf(result, "%04d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
    return result;
}

/* Return just the time portion (HH:mm:ss) */
char *rt_time_to_time(RtArena *arena, RtTime *time)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_time_to_time: NULL arena\n");
        return NULL;
    }
    if (time == NULL) {
        fprintf(stderr, "rt_time_to_time: NULL time\n");
        return NULL;
    }
    struct tm tm;
    rt_time_to_tm(time, &tm);
    char *result = rt_arena_alloc(arena, 16);
    if (result == NULL) {
        fprintf(stderr, "rt_time_to_time: allocation failed\n");
        exit(1);
    }
    sprintf(result, "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
    return result;
}

/* Return time in ISO 8601 format (YYYY-MM-DDTHH:mm:ss.SSSZ) */
char *rt_time_to_iso(RtArena *arena, RtTime *time)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_time_to_iso: NULL arena\n");
        return NULL;
    }
    if (time == NULL) {
        fprintf(stderr, "rt_time_to_iso: NULL time\n");
        return NULL;
    }
    time_t secs = time->milliseconds / 1000;
    long millis = time->milliseconds % 1000;
    struct tm tm;
    gmtime_r(&secs, &tm);
    char *result = rt_arena_alloc(arena, 32);
    if (result == NULL) {
        fprintf(stderr, "rt_time_to_iso: allocation failed\n");
        exit(1);
    }
    sprintf(result, "%04d-%02d-%02dT%02d:%02d:%02d.%03ldZ",
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec, millis);
    return result;
}

/* Format time using pattern string - see pattern tokens in runtime.h */
char *rt_time_format(RtArena *arena, RtTime *time, const char *pattern)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_time_format: NULL arena\n");
        return NULL;
    }
    if (time == NULL) {
        fprintf(stderr, "rt_time_format: NULL time\n");
        return NULL;
    }
    if (pattern == NULL) {
        fprintf(stderr, "rt_time_format: NULL pattern\n");
        return NULL;
    }

    /* Decompose time into components */
    struct tm tm;
    rt_time_to_tm(time, &tm);
    long millis = time->milliseconds % 1000;

    /* Convert 24-hour to 12-hour format (0->12, 13-23->1-11, 12 stays 12) */
    int hour12 = tm.tm_hour % 12;
    if (hour12 == 0) hour12 = 12;

    /* Allocate generous output buffer (pattern * 3 should cover expansions) */
    size_t buf_size = strlen(pattern) * 3 + 1;
    char *result = rt_arena_alloc(arena, buf_size);
    if (result == NULL) {
        fprintf(stderr, "rt_time_format: allocation failed\n");
        exit(1);
    }

    /* Scan pattern and build output
     * Token matching order: longer tokens must be checked before shorter ones
     * to avoid incorrect partial matches (e.g., YYYY before YY, SSS before ss).
     * Unmatched characters are copied through as literals (-, :, /, T, space, etc.)
     */
    size_t out_pos = 0;
    for (size_t i = 0; pattern[i]; ) {
        /* Year tokens - check YYYY before YY to avoid incorrect matching */
        if (strncmp(&pattern[i], "YYYY", 4) == 0) {
            out_pos += sprintf(&result[out_pos], "%04d", tm.tm_year + 1900);
            i += 4;
        } else if (strncmp(&pattern[i], "YY", 2) == 0) {
            out_pos += sprintf(&result[out_pos], "%02d", (tm.tm_year + 1900) % 100);
            i += 2;
        }
        /* Month tokens - check MM before M to avoid incorrect matching */
        else if (strncmp(&pattern[i], "MM", 2) == 0) {
            out_pos += sprintf(&result[out_pos], "%02d", tm.tm_mon + 1);
            i += 2;
        } else if (strncmp(&pattern[i], "M", 1) == 0) {
            out_pos += sprintf(&result[out_pos], "%d", tm.tm_mon + 1);
            i += 1;
        }
        /* Day tokens - check DD before D to avoid incorrect matching */
        else if (strncmp(&pattern[i], "DD", 2) == 0) {
            out_pos += sprintf(&result[out_pos], "%02d", tm.tm_mday);
            i += 2;
        } else if (strncmp(&pattern[i], "D", 1) == 0) {
            out_pos += sprintf(&result[out_pos], "%d", tm.tm_mday);
            i += 1;
        }
        /* Hour tokens (24-hour) - check HH before H to avoid incorrect matching */
        else if (strncmp(&pattern[i], "HH", 2) == 0) {
            out_pos += sprintf(&result[out_pos], "%02d", tm.tm_hour);
            i += 2;
        } else if (strncmp(&pattern[i], "H", 1) == 0) {
            out_pos += sprintf(&result[out_pos], "%d", tm.tm_hour);
            i += 1;
        }
        /* Hour tokens (12-hour) - check hh before h to avoid incorrect matching */
        else if (strncmp(&pattern[i], "hh", 2) == 0) {
            out_pos += sprintf(&result[out_pos], "%02d", hour12);
            i += 2;
        } else if (pattern[i] == 'h') {
            out_pos += sprintf(&result[out_pos], "%d", hour12);
            i += 1;
        }
        /* Minute tokens - check mm before m to avoid incorrect matching */
        else if (strncmp(&pattern[i], "mm", 2) == 0) {
            out_pos += sprintf(&result[out_pos], "%02d", tm.tm_min);
            i += 2;
        } else if (strncmp(&pattern[i], "m", 1) == 0) {
            out_pos += sprintf(&result[out_pos], "%d", tm.tm_min);
            i += 1;
        }
        /* Milliseconds token - check SSS before ss/s to avoid incorrect matching */
        else if (strncmp(&pattern[i], "SSS", 3) == 0) {
            out_pos += sprintf(&result[out_pos], "%03ld", millis);
            i += 3;
        }
        /* Second tokens - check ss before s to avoid incorrect matching */
        else if (strncmp(&pattern[i], "ss", 2) == 0) {
            out_pos += sprintf(&result[out_pos], "%02d", tm.tm_sec);
            i += 2;
        } else if (strncmp(&pattern[i], "s", 1) == 0) {
            out_pos += sprintf(&result[out_pos], "%d", tm.tm_sec);
            i += 1;
        }
        /* AM/PM tokens */
        else if (pattern[i] == 'A') {
            out_pos += sprintf(&result[out_pos], "%s", tm.tm_hour < 12 ? "AM" : "PM");
            i += 1;
        } else if (pattern[i] == 'a') {
            out_pos += sprintf(&result[out_pos], "%s", tm.tm_hour < 12 ? "am" : "pm");
            i += 1;
        } else {
            /* No pattern match - copy character through */
            result[out_pos++] = pattern[i++];
        }
    }

    result[out_pos] = '\0';
    return result;
}

/* ============================================================================
 * Time Arithmetic Methods
 * ============================================================================ */

RtTime *rt_time_add(RtArena *arena, RtTime *time, long long ms)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_time_add: NULL arena\n");
        return NULL;
    }
    if (time == NULL) {
        fprintf(stderr, "rt_time_add: NULL time\n");
        return NULL;
    }

    long long new_ms = time->milliseconds + ms;
    return rt_time_create(arena, new_ms);
}

RtTime *rt_time_add_seconds(RtArena *arena, RtTime *time, long seconds)
{
    return rt_time_add(arena, time, seconds * 1000LL);
}

RtTime *rt_time_add_minutes(RtArena *arena, RtTime *time, long minutes)
{
    return rt_time_add(arena, time, minutes * 60 * 1000LL);
}

RtTime *rt_time_add_hours(RtArena *arena, RtTime *time, long hours)
{
    return rt_time_add(arena, time, hours * 60 * 60 * 1000LL);
}

RtTime *rt_time_add_days(RtArena *arena, RtTime *time, long days)
{
    return rt_time_add(arena, time, days * 24 * 60 * 60 * 1000LL);
}

long long rt_time_diff(RtTime *time, RtTime *other)
{
    if (time == NULL) {
        fprintf(stderr, "rt_time_diff: NULL time\n");
        return 0;
    }
    if (other == NULL) {
        fprintf(stderr, "rt_time_diff: NULL other\n");
        return 0;
    }

    return time->milliseconds - other->milliseconds;
}

/* ============================================================================
 * Time Comparison Methods
 * ============================================================================ */

int rt_time_is_before(RtTime *time, RtTime *other)
{
    if (time == NULL) {
        fprintf(stderr, "rt_time_is_before: NULL time\n");
        return 0;
    }
    if (other == NULL) {
        fprintf(stderr, "rt_time_is_before: NULL other\n");
        return 0;
    }

    return (time->milliseconds < other->milliseconds) ? 1 : 0;
}

int rt_time_is_after(RtTime *time, RtTime *other)
{
    if (time == NULL) {
        fprintf(stderr, "rt_time_is_after: NULL time\n");
        return 0;
    }
    if (other == NULL) {
        fprintf(stderr, "rt_time_is_after: NULL other\n");
        return 0;
    }

    return (time->milliseconds > other->milliseconds) ? 1 : 0;
}

int rt_time_equals(RtTime *time, RtTime *other)
{
    if (time == NULL) {
        fprintf(stderr, "rt_time_equals: NULL time\n");
        return 0;
    }
    if (other == NULL) {
        fprintf(stderr, "rt_time_equals: NULL other\n");
        return 0;
    }

    return (time->milliseconds == other->milliseconds) ? 1 : 0;
}
