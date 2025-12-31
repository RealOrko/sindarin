#ifndef RUNTIME_H
#define RUNTIME_H

#include <stddef.h>
#include <stdbool.h>
#include <string.h>

/* ============================================================================
 * Arena Memory Management
 * ============================================================================
 * Arenas provide block-scoped memory allocation. All allocations within an
 * arena are freed together when the arena is destroyed. This eliminates
 * individual free() calls and prevents memory leaks.
 * ============================================================================ */

/* Default block size for arena allocations (64KB) */
#define RT_ARENA_DEFAULT_BLOCK_SIZE (64 * 1024)

/* Arena block - linked list of memory blocks */
typedef struct RtArenaBlock {
    struct RtArenaBlock *next;  /* Next block in chain */
    size_t size;                /* Size of this block's data area */
    size_t used;                /* Bytes used in this block */
    char data[];                /* Flexible array member for actual data */
} RtArenaBlock;

/* Forward declaration for file handle types */
typedef struct RtFileHandle RtFileHandle;

/* File handle - linked list node for tracking open files in arena */
struct RtFileHandle {
    void *fp;                   /* FILE* pointer (void* to avoid including stdio.h) */
    char *path;                 /* Path to the file */
    bool is_open;               /* Whether the file is still open */
    bool is_text;               /* True for TextFile, false for BinaryFile */
    RtFileHandle *next;         /* Next handle in chain */
};

/* Arena - manages a collection of memory blocks */
typedef struct RtArena {
    struct RtArena *parent;     /* Parent arena for hierarchy */
    RtArenaBlock *first;        /* First block in chain */
    RtArenaBlock *current;      /* Current block for allocations */
    size_t default_block_size;  /* Size for new blocks */
    size_t total_allocated;     /* Total bytes allocated (for stats) */
    RtFileHandle *open_files;   /* Head of file handle list for auto-close */
} RtArena;

/* ============================================================================
 * File Handle Types
 * ============================================================================
 * Text and binary file handles for I/O operations. Handles are automatically
 * closed when their arena is destroyed.
 * ============================================================================ */

/* Text file handle - for text-oriented file operations */
typedef struct RtTextFile {
    void *fp;                   /* FILE* pointer (void* to avoid including stdio.h) */
    char *path;                 /* Full path to file */
    bool is_open;               /* Whether file is still open */
    RtFileHandle *handle;       /* Arena tracking handle */
} RtTextFile;

/* Binary file handle - for binary file operations */
typedef struct RtBinaryFile {
    void *fp;                   /* FILE* pointer (void* to avoid including stdio.h) */
    char *path;                 /* Full path to file */
    bool is_open;               /* Whether file is still open */
    RtFileHandle *handle;       /* Arena tracking handle */
} RtBinaryFile;

/* Time handle - wrapper around epoch milliseconds */
typedef struct RtTime {
    long long milliseconds;     /* Milliseconds since Unix epoch (1970-01-01 00:00:00 UTC) */
} RtTime;

/* Create a new arena, optionally with a parent */
RtArena *rt_arena_create(RtArena *parent);

/* Create arena with custom block size */
RtArena *rt_arena_create_sized(RtArena *parent, size_t block_size);

/* Allocate memory from arena (uninitialized) */
void *rt_arena_alloc(RtArena *arena, size_t size);

/* Allocate zeroed memory from arena */
void *rt_arena_calloc(RtArena *arena, size_t count, size_t size);

/* Allocate aligned memory from arena */
void *rt_arena_alloc_aligned(RtArena *arena, size_t size, size_t alignment);

/* Duplicate a string into arena */
char *rt_arena_strdup(RtArena *arena, const char *str);

/* Duplicate n bytes of a string into arena */
char *rt_arena_strndup(RtArena *arena, const char *str, size_t n);

/* Destroy arena and free all memory */
void rt_arena_destroy(RtArena *arena);

/* Reset arena for reuse (keeps first block, frees rest) */
void rt_arena_reset(RtArena *arena);

/* Copy data from one arena to another (for promotion) */
void *rt_arena_promote(RtArena *dest, const void *src, size_t size);

/* Copy string from one arena to another (for promotion) */
char *rt_arena_promote_string(RtArena *dest, const char *src);

/* Get total bytes allocated by arena */
size_t rt_arena_total_allocated(RtArena *arena);

/* ============================================================================
 * File Handle Tracking
 * ============================================================================
 * Functions for managing file handles within arenas. Files are automatically
 * closed when the arena is destroyed.
 * ============================================================================ */

/* Track a file handle in an arena (called internally by file open functions) */
RtFileHandle *rt_arena_track_file(RtArena *arena, void *fp, const char *path, bool is_text);

/* Untrack a file handle from an arena (called when promoting to another arena) */
void rt_arena_untrack_file(RtArena *arena, RtFileHandle *handle);

/* Promote a text file handle to a destination arena */
RtTextFile *rt_text_file_promote(RtArena *dest, RtArena *src_arena, RtTextFile *src);

/* Promote a binary file handle to a destination arena */
RtBinaryFile *rt_binary_file_promote(RtArena *dest, RtArena *src_arena, RtBinaryFile *src);

/* ============================================================================
 * String operations
 * ============================================================================ */

/* String concatenation - allocates from arena (IMMUTABLE string, no metadata) */
char *rt_str_concat(RtArena *arena, const char *left, const char *right);

/* MUTABLE string operations - these create/modify strings WITH RtStringMeta.
 * Mutable strings have metadata stored before the string data, enabling
 * efficient append operations and O(1) length queries via RT_STR_META(). */
char *rt_string_with_capacity(RtArena *arena, size_t capacity);
char *rt_string_from(RtArena *arena, const char *src);
char *rt_string_ensure_mutable(RtArena *arena, char *str);
char *rt_string_append(char *dest, const char *src);

/* Print functions (no allocation) */
void rt_print_long(long val);
void rt_print_double(double val);
void rt_print_char(long c);
void rt_print_string(const char *s);
void rt_print_bool(long b);
void rt_print_byte(unsigned char b);

/* Type conversion to string - allocates from arena */
char *rt_to_string_long(RtArena *arena, long val);
char *rt_to_string_double(RtArena *arena, double val);
char *rt_to_string_char(RtArena *arena, char val);
char *rt_to_string_bool(RtArena *arena, int val);
char *rt_to_string_byte(RtArena *arena, unsigned char val);
char *rt_to_string_string(RtArena *arena, const char *val);
char *rt_to_string_void(RtArena *arena);
char *rt_to_string_pointer(RtArena *arena, void *p);

/* Long arithmetic (with overflow checking) */
long rt_add_long(long a, long b);
long rt_sub_long(long a, long b);
long rt_mul_long(long a, long b);
long rt_div_long(long a, long b);
long rt_mod_long(long a, long b);
long rt_neg_long(long a);

/* Long comparisons - inlined for performance */
static inline int rt_eq_long(long a, long b) { return a == b; }
static inline int rt_ne_long(long a, long b) { return a != b; }
static inline int rt_lt_long(long a, long b) { return a < b; }
static inline int rt_le_long(long a, long b) { return a <= b; }
static inline int rt_gt_long(long a, long b) { return a > b; }
static inline int rt_ge_long(long a, long b) { return a >= b; }

/* Double arithmetic (with overflow checking) */
double rt_add_double(double a, double b);
double rt_sub_double(double a, double b);
double rt_mul_double(double a, double b);
double rt_div_double(double a, double b);
double rt_neg_double(double a);

/* Double comparisons - inlined for performance */
static inline int rt_eq_double(double a, double b) { return a == b; }
static inline int rt_ne_double(double a, double b) { return a != b; }
static inline int rt_lt_double(double a, double b) { return a < b; }
static inline int rt_le_double(double a, double b) { return a <= b; }
static inline int rt_gt_double(double a, double b) { return a > b; }
static inline int rt_ge_double(double a, double b) { return a >= b; }

/* Boolean operations - inlined for performance */
static inline int rt_not_bool(int a) { return !a; }

/* Increment/decrement */
long rt_post_inc_long(long *p);
long rt_post_dec_long(long *p);

/* String comparisons - inlined for performance */
static inline int rt_eq_string(const char *a, const char *b) { return strcmp(a, b) == 0; }
static inline int rt_ne_string(const char *a, const char *b) { return strcmp(a, b) != 0; }
static inline int rt_lt_string(const char *a, const char *b) { return strcmp(a, b) < 0; }
static inline int rt_le_string(const char *a, const char *b) { return strcmp(a, b) <= 0; }
static inline int rt_gt_string(const char *a, const char *b) { return strcmp(a, b) > 0; }
static inline int rt_ge_string(const char *a, const char *b) { return strcmp(a, b) >= 0; }

/* ============================================================================
 * Array Metadata
 * ============================================================================
 * Array metadata structure - stored immediately before array data in memory.
 * This allows rt_array_length to be inlined for maximum performance.
 * ============================================================================ */

typedef struct {
    RtArena *arena;  /* Arena that owns this array (for reallocation) */
    size_t size;     /* Number of elements currently in the array */
    size_t capacity; /* Total allocated space for elements */
} RtArrayMetadata;

/* ============================================================================
 * String metadata structure - stored immediately before string data in memory.
 * This allows rt_str_length to be inlined for maximum performance.
 * ============================================================================
 *
 * MUTABLE VS IMMUTABLE STRINGS - DESIGN AND COMPATIBILITY
 * --------------------------------------------------------
 *
 * The Sn runtime supports two kinds of strings:
 *
 * 1. IMMUTABLE STRINGS (traditional, no metadata):
 *    - String literals: "hello" - plain char* pointing to read-only data
 *    - Strings from rt_str_concat(): returns new immutable string each time
 *    - Strings from rt_arena_strdup(): simple arena-allocated copy
 *    - These are plain char* with NO metadata stored before them
 *    - Use strlen() to get length (O(n) operation)
 *
 * 2. MUTABLE STRINGS (new, with metadata):
 *    - Created with rt_string_with_capacity(): has RtStringMeta before data
 *    - Can be appended to efficiently using rt_string_append()
 *    - Pre-allocated capacity avoids reallocation for small appends
 *    - Use RT_STR_META(s)->length for O(1) length access
 *
 * BACKWARD COMPATIBILITY APPROACH:
 * - Existing code continues to work unchanged with immutable strings
 * - Functions that modify strings MUST check if the string has metadata
 *   before using RT_STR_META (otherwise undefined behavior)
 * - Use rt_string_is_mutable(s) to check if a string has metadata
 * - Immutable strings can be converted to mutable with rt_string_from()
 *
 * IMPORTANT: RT_STR_META(s) must ONLY be called on mutable strings created
 * with rt_string_with_capacity() or rt_string_from(). Using it on literal
 * strings or strings from rt_str_concat() will access invalid memory!
 *
 * ============================================================================ */

typedef struct {
    RtArena *arena;  /* Arena that owns this string (for reallocation) */
    size_t length;   /* Number of characters in the string (excluding null) */
    size_t capacity; /* Total allocated space for characters */
} RtStringMeta;

/* STRUCTURE SIZE AND MEMORY LAYOUT:
 *
 * RtStringMeta has the same memory layout philosophy as RtArrayMetadata:
 * - Both contain: arena pointer + size/length + capacity
 * - On 64-bit systems: sizeof(RtStringMeta) = 24 bytes (3 * 8 bytes)
 * - On 32-bit systems: sizeof(RtStringMeta) = 12 bytes (3 * 4 bytes)
 *
 * Memory layout for mutable strings:
 *   [RtStringMeta (24 bytes)] [string data...] [null terminator]
 *                             ^
 *                             |-- String pointer points HERE
 *
 * The metadata is stored BEFORE the string data pointer, so:
 *   RT_STR_META(s) = (RtStringMeta*)((char*)s - sizeof(RtStringMeta))
 *
 * This matches the array metadata pattern where ArrayMetadata[-1] accesses
 * the metadata stored before the array data.
 */

/* Compile-time size verification (24 bytes on 64-bit, 12 bytes on 32-bit) */
_Static_assert(sizeof(RtStringMeta) == sizeof(RtArrayMetadata),
               "RtStringMeta and RtArrayMetadata must have same size");
_Static_assert(sizeof(RtStringMeta) == 3 * sizeof(void*),
               "RtStringMeta size should be 3 pointers (arena + length + capacity)");

/* Macro to access string metadata - stored immediately before string data.
 * WARNING: Only use on mutable strings created with rt_string_with_capacity()
 * or rt_string_from(). Using on immutable strings is undefined behavior! */
#define RT_STR_META(s) ((RtStringMeta*)((char*)(s) - sizeof(RtStringMeta)))

/* Get array length - inlined for performance (critical for loops) */
static inline size_t rt_array_length(void *arr) {
    if (arr == NULL) return 0;
    return ((RtArrayMetadata *)arr)[-1].size;
}

/* Check if string is mutable - inlined for fast path in append loops */
static inline int rt_string_is_mutable(RtArena *arena, char *str) {
    if (str == NULL) return 0;
    RtStringMeta *meta = RT_STR_META(str);
    return (meta->arena == arena &&
            meta->capacity > 0 &&
            meta->capacity < (1UL << 30) &&
            meta->length <= meta->capacity);
}

/* Fast inline ensure_mutable - avoids function call when already mutable */
static inline char *rt_string_ensure_mutable_inline(RtArena *arena, char *str) {
    if (str != NULL && rt_string_is_mutable(arena, str)) {
        return str;  /* Already mutable, fast path */
    }
    return rt_string_ensure_mutable(arena, str);  /* Slow path */
}

/* Compare a region of a string with a pattern without allocating.
 * Returns 1 if str[start:end] equals pattern, 0 otherwise. */
static inline int rt_str_region_equals(const char *str, long start, long end, const char *pattern) {
    if (str == NULL || pattern == NULL) return 0;
    long pat_len = 0;
    while (pattern[pat_len]) pat_len++;
    if (end - start != pat_len) return 0;
    for (long i = 0; i < pat_len; i++) {
        if (str[start + i] != pattern[i]) return 0;
    }
    return 1;
}

/* Array operations - allocate from arena */
long *rt_array_push_long(RtArena *arena, long *arr, long element);
double *rt_array_push_double(RtArena *arena, double *arr, double element);
char *rt_array_push_char(RtArena *arena, char *arr, char element);
char **rt_array_push_string(RtArena *arena, char **arr, const char *element);
int *rt_array_push_bool(RtArena *arena, int *arr, int element);
unsigned char *rt_array_push_byte(RtArena *arena, unsigned char *arr, unsigned char element);

/* Array print functions */
void rt_print_array_long(long *arr);
void rt_print_array_double(double *arr);
void rt_print_array_char(char *arr);
void rt_print_array_bool(int *arr);
void rt_print_array_byte(unsigned char *arr);
void rt_print_array_string(char **arr);

/* Array clear */
void rt_array_clear(void *arr);

/* Array pop functions */
long rt_array_pop_long(long *arr);
double rt_array_pop_double(double *arr);
char rt_array_pop_char(char *arr);
int rt_array_pop_bool(int *arr);
unsigned char rt_array_pop_byte(unsigned char *arr);
char *rt_array_pop_string(char **arr);

/* Array concat functions - allocate from arena */
long *rt_array_concat_long(RtArena *arena, long *dest, long *src);
double *rt_array_concat_double(RtArena *arena, double *dest, double *src);
char *rt_array_concat_char(RtArena *arena, char *dest, char *src);
int *rt_array_concat_bool(RtArena *arena, int *dest, int *src);
unsigned char *rt_array_concat_byte(RtArena *arena, unsigned char *dest, unsigned char *src);
char **rt_array_concat_string(RtArena *arena, char **dest, char **src);

/* Array slice functions - step defaults to 1 if LONG_MIN, allocate from arena */
long *rt_array_slice_long(RtArena *arena, long *arr, long start, long end, long step);
double *rt_array_slice_double(RtArena *arena, double *arr, long start, long end, long step);
char *rt_array_slice_char(RtArena *arena, char *arr, long start, long end, long step);
int *rt_array_slice_bool(RtArena *arena, int *arr, long start, long end, long step);
unsigned char *rt_array_slice_byte(RtArena *arena, unsigned char *arr, long start, long end, long step);
char **rt_array_slice_string(RtArena *arena, char **arr, long start, long end, long step);

/* Array reverse functions - allocate from arena */
long *rt_array_rev_long(RtArena *arena, long *arr);
double *rt_array_rev_double(RtArena *arena, double *arr);
char *rt_array_rev_char(RtArena *arena, char *arr);
int *rt_array_rev_bool(RtArena *arena, int *arr);
unsigned char *rt_array_rev_byte(RtArena *arena, unsigned char *arr);
char **rt_array_rev_string(RtArena *arena, char **arr);

/* Array remove at index functions - allocate from arena */
long *rt_array_rem_long(RtArena *arena, long *arr, long index);
double *rt_array_rem_double(RtArena *arena, double *arr, long index);
char *rt_array_rem_char(RtArena *arena, char *arr, long index);
int *rt_array_rem_bool(RtArena *arena, int *arr, long index);
unsigned char *rt_array_rem_byte(RtArena *arena, unsigned char *arr, long index);
char **rt_array_rem_string(RtArena *arena, char **arr, long index);

/* Array insert at index functions - allocate from arena */
long *rt_array_ins_long(RtArena *arena, long *arr, long elem, long index);
double *rt_array_ins_double(RtArena *arena, double *arr, double elem, long index);
char *rt_array_ins_char(RtArena *arena, char *arr, char elem, long index);
int *rt_array_ins_bool(RtArena *arena, int *arr, int elem, long index);
unsigned char *rt_array_ins_byte(RtArena *arena, unsigned char *arr, unsigned char elem, long index);
char **rt_array_ins_string(RtArena *arena, char **arr, const char *elem, long index);

/* Array push (copy) functions - return NEW array with element appended, allocate from arena */
long *rt_array_push_copy_long(RtArena *arena, long *arr, long elem);
double *rt_array_push_copy_double(RtArena *arena, double *arr, double elem);
char *rt_array_push_copy_char(RtArena *arena, char *arr, char elem);
int *rt_array_push_copy_bool(RtArena *arena, int *arr, int elem);
unsigned char *rt_array_push_copy_byte(RtArena *arena, unsigned char *arr, unsigned char elem);
char **rt_array_push_copy_string(RtArena *arena, char **arr, const char *elem);

/* Array indexOf functions - find first index of element, returns -1 if not found */
long rt_array_indexOf_long(long *arr, long elem);
long rt_array_indexOf_double(double *arr, double elem);
long rt_array_indexOf_char(char *arr, char elem);
long rt_array_indexOf_bool(int *arr, int elem);
long rt_array_indexOf_byte(unsigned char *arr, unsigned char elem);
long rt_array_indexOf_string(char **arr, const char *elem);

/* Array contains functions - check if element exists */
int rt_array_contains_long(long *arr, long elem);
int rt_array_contains_double(double *arr, double elem);
int rt_array_contains_char(char *arr, char elem);
int rt_array_contains_bool(int *arr, int elem);
int rt_array_contains_byte(unsigned char *arr, unsigned char elem);
int rt_array_contains_string(char **arr, const char *elem);

/* Array clone functions - create deep copy, allocate from arena */
long *rt_array_clone_long(RtArena *arena, long *arr);
double *rt_array_clone_double(RtArena *arena, double *arr);
char *rt_array_clone_char(RtArena *arena, char *arr);
int *rt_array_clone_bool(RtArena *arena, int *arr);
unsigned char *rt_array_clone_byte(RtArena *arena, unsigned char *arr);
char **rt_array_clone_string(RtArena *arena, char **arr);

/* Array join functions - join elements into string with separator, allocate from arena */
char *rt_array_join_long(RtArena *arena, long *arr, const char *separator);
char *rt_array_join_double(RtArena *arena, double *arr, const char *separator);
char *rt_array_join_char(RtArena *arena, char *arr, const char *separator);
char *rt_array_join_bool(RtArena *arena, int *arr, const char *separator);
char *rt_array_join_byte(RtArena *arena, unsigned char *arr, const char *separator);
char *rt_array_join_string(RtArena *arena, char **arr, const char *separator);

/* Array create from static data - creates runtime array with metadata, allocate from arena */
long *rt_array_create_long(RtArena *arena, size_t count, const long *data);
double *rt_array_create_double(RtArena *arena, size_t count, const double *data);
char *rt_array_create_char(RtArena *arena, size_t count, const char *data);
int *rt_array_create_bool(RtArena *arena, size_t count, const int *data);
unsigned char *rt_array_create_byte(RtArena *arena, size_t count, const unsigned char *data);
char **rt_array_create_string(RtArena *arena, size_t count, const char **data);

/* Array alloc with default value - creates array of count elements filled with default_value */
long *rt_array_alloc_long(RtArena *arena, size_t count, long default_value);
double *rt_array_alloc_double(RtArena *arena, size_t count, double default_value);
char *rt_array_alloc_char(RtArena *arena, size_t count, char default_value);
int *rt_array_alloc_bool(RtArena *arena, size_t count, int default_value);
unsigned char *rt_array_alloc_byte(RtArena *arena, size_t count, unsigned char default_value);
char **rt_array_alloc_string(RtArena *arena, size_t count, const char *default_value);

/* Array equality functions - compare arrays element by element */
int rt_array_eq_long(long *a, long *b);
int rt_array_eq_double(double *a, double *b);
int rt_array_eq_char(char *a, char *b);
int rt_array_eq_bool(int *a, int *b);
int rt_array_eq_byte(unsigned char *a, unsigned char *b);
int rt_array_eq_string(char **a, char **b);

/* Range creation - creates int[] from start to end (exclusive), allocate from arena */
long *rt_array_range(RtArena *arena, long start, long end);

/* ============================================================================
 * TextFile Static Methods
 * ============================================================================
 * Static methods for file operations. All methods panic on failure with
 * descriptive error messages.
 * ============================================================================ */

/* Open file for reading and writing (panics if file doesn't exist) */
RtTextFile *rt_text_file_open(RtArena *arena, const char *path);

/* Check if file exists without opening */
int rt_text_file_exists(const char *path);

/* Read entire file contents as string */
char *rt_text_file_read_all(RtArena *arena, const char *path);

/* Write string to file (creates or overwrites) */
void rt_text_file_write_all(const char *path, const char *content);

/* Delete file (panics if fails) */
void rt_text_file_delete(const char *path);

/* Copy file to new location */
void rt_text_file_copy(const char *src, const char *dst);

/* Move/rename file */
void rt_text_file_move(const char *src, const char *dst);

/* Close an open text file */
void rt_text_file_close(RtTextFile *file);

/* ============================================================================
 * TextFile Instance Reading Methods
 * ============================================================================
 * Instance methods for reading from open files. All methods panic on failure
 * except EOF conditions which return appropriate sentinel values.
 * ============================================================================ */

/* Read single character, returns -1 on EOF */
long rt_text_file_read_char(RtTextFile *file);

/* Read whitespace-delimited word, returns empty string on EOF */
char *rt_text_file_read_word(RtArena *arena, RtTextFile *file);

/* Read single line (strips trailing newline), returns NULL on EOF */
char *rt_text_file_read_line(RtArena *arena, RtTextFile *file);

/* Read all remaining content from open file */
char *rt_text_file_instance_read_all(RtArena *arena, RtTextFile *file);

/* Read all remaining lines as array of strings */
char **rt_text_file_read_lines(RtArena *arena, RtTextFile *file);

/* Read into character buffer, returns number of chars read */
long rt_text_file_read_into(RtTextFile *file, char *buffer);

/* ============================================================================
 * TextFile Instance Writing Methods
 * ============================================================================
 * Instance methods for writing to open files. All methods panic on failure.
 * ============================================================================ */

/* Write single character to file */
void rt_text_file_write_char(RtTextFile *file, long ch);

/* Write string to file */
void rt_text_file_write(RtTextFile *file, const char *text);

/* Write string followed by newline */
void rt_text_file_write_line(RtTextFile *file, const char *text);

/* Write string to file (alias for write, used with interpolated strings) */
void rt_text_file_print(RtTextFile *file, const char *text);

/* Write string followed by newline (alias for writeLine) */
void rt_text_file_println(RtTextFile *file, const char *text);

/* ============================================================================
 * TextFile State Methods
 * ============================================================================
 * Methods for querying and manipulating file state.
 * ============================================================================ */

/* Check if more characters are available to read */
int rt_text_file_has_chars(RtTextFile *file);

/* Check if more whitespace-delimited words are available */
int rt_text_file_has_words(RtTextFile *file);

/* Check if more lines are available to read */
int rt_text_file_has_lines(RtTextFile *file);

/* Check if at end of file */
int rt_text_file_is_eof(RtTextFile *file);

/* Get current byte position in file */
long rt_text_file_position(RtTextFile *file);

/* Seek to byte position */
void rt_text_file_seek(RtTextFile *file, long pos);

/* Return to beginning of file */
void rt_text_file_rewind(RtTextFile *file);

/* Force buffered data to disk */
void rt_text_file_flush(RtTextFile *file);

/* ============================================================================
 * TextFile Properties
 * ============================================================================
 * Property accessors for file metadata.
 * ============================================================================ */

/* Get full file path */
char *rt_text_file_get_path(RtArena *arena, RtTextFile *file);

/* Get filename only (without directory) */
char *rt_text_file_get_name(RtArena *arena, RtTextFile *file);

/* Get file size in bytes */
long rt_text_file_get_size(RtTextFile *file);

/* ============================================================================
 * BinaryFile Static Methods
 * ============================================================================
 * Static methods for binary file operations. All methods panic on failure
 * with descriptive error messages.
 * ============================================================================ */

/* Open binary file for reading and writing (panics if file doesn't exist) */
RtBinaryFile *rt_binary_file_open(RtArena *arena, const char *path);

/* Check if binary file exists without opening */
int rt_binary_file_exists(const char *path);

/* Read entire binary file contents as byte array */
unsigned char *rt_binary_file_read_all(RtArena *arena, const char *path);

/* Write byte array to binary file (creates or overwrites) */
void rt_binary_file_write_all(const char *path, unsigned char *data);

/* Delete binary file (panics if fails) */
void rt_binary_file_delete(const char *path);

/* Copy binary file to new location */
void rt_binary_file_copy(const char *src, const char *dst);

/* Move/rename binary file */
void rt_binary_file_move(const char *src, const char *dst);

/* Close an open binary file */
void rt_binary_file_close(RtBinaryFile *file);

/* ============================================================================
 * BinaryFile Instance Reading Methods
 * ============================================================================
 * Instance methods for reading from open binary files.
 * ============================================================================ */

/* Read single byte, returns -1 on EOF */
long rt_binary_file_read_byte(RtBinaryFile *file);

/* Read N bytes into new array */
unsigned char *rt_binary_file_read_bytes(RtArena *arena, RtBinaryFile *file, long count);

/* Read all remaining bytes from open file */
unsigned char *rt_binary_file_instance_read_all(RtArena *arena, RtBinaryFile *file);

/* Read into byte buffer, returns number of bytes read */
long rt_binary_file_read_into(RtBinaryFile *file, unsigned char *buffer);

/* ============================================================================
 * BinaryFile Instance Writing Methods
 * ============================================================================
 * Instance methods for writing to open binary files.
 * ============================================================================ */

/* Write single byte to file */
void rt_binary_file_write_byte(RtBinaryFile *file, long b);

/* Write byte array to file */
void rt_binary_file_write_bytes(RtBinaryFile *file, unsigned char *data);

/* ============================================================================
 * BinaryFile State Methods
 * ============================================================================
 * Methods for querying and manipulating binary file state.
 * ============================================================================ */

/* Check if more bytes are available to read */
int rt_binary_file_has_bytes(RtBinaryFile *file);

/* Check if at end of file */
int rt_binary_file_is_eof(RtBinaryFile *file);

/* Get current byte position in file */
long rt_binary_file_position(RtBinaryFile *file);

/* Seek to byte position */
void rt_binary_file_seek(RtBinaryFile *file, long pos);

/* Return to beginning of file */
void rt_binary_file_rewind(RtBinaryFile *file);

/* Force buffered data to disk */
void rt_binary_file_flush(RtBinaryFile *file);

/* ============================================================================
 * BinaryFile Properties
 * ============================================================================
 * Property accessors for binary file metadata.
 * ============================================================================ */

/* Get full file path */
char *rt_binary_file_get_path(RtArena *arena, RtBinaryFile *file);

/* Get filename only (without directory) */
char *rt_binary_file_get_name(RtArena *arena, RtBinaryFile *file);

/* Get file size in bytes */
long rt_binary_file_get_size(RtBinaryFile *file);

/* ============================================================================
 * Standard Stream Operations (Stdin, Stdout, Stderr)
 * ============================================================================
 * Functions for console I/O using standard streams.
 * ============================================================================ */

/* Stdin - read from standard input */
char *rt_stdin_read_line(RtArena *arena);
long rt_stdin_read_char(void);
char *rt_stdin_read_word(RtArena *arena);
int rt_stdin_has_chars(void);
int rt_stdin_has_lines(void);
int rt_stdin_is_eof(void);

/* Stdout - write to standard output */
void rt_stdout_write(const char *text);
void rt_stdout_write_line(const char *text);
void rt_stdout_flush(void);

/* Stderr - write to standard error */
void rt_stderr_write(const char *text);
void rt_stderr_write_line(const char *text);
void rt_stderr_flush(void);

/* Global convenience functions */
char *rt_read_line(RtArena *arena);
void rt_println(const char *text);
void rt_print_err(const char *text);
void rt_print_err_ln(const char *text);

/* ============================================================================
 * Byte Array Extension Methods
 * ============================================================================
 * Methods for converting byte arrays to strings in various encodings.
 * ============================================================================ */

/* Convert byte array to string using UTF-8 decoding */
char *rt_byte_array_to_string(RtArena *arena, unsigned char *bytes);

/* Convert byte array to string using Latin-1/ISO-8859-1 decoding */
char *rt_byte_array_to_string_latin1(RtArena *arena, unsigned char *bytes);

/* Convert byte array to hexadecimal string (e.g., "48656c6c6f") */
char *rt_byte_array_to_hex(RtArena *arena, unsigned char *bytes);

/* Convert byte array to Base64 string (e.g., "SGVsbG8=") */
char *rt_byte_array_to_base64(RtArena *arena, unsigned char *bytes);

/* ============================================================================
 * String to Byte Array Conversions
 * ============================================================================
 * Methods for converting strings to byte arrays.
 * ============================================================================ */

/* Convert string to UTF-8 byte array */
unsigned char *rt_string_to_bytes(RtArena *arena, const char *str);

/* Decode hexadecimal string to byte array */
unsigned char *rt_bytes_from_hex(RtArena *arena, const char *hex);

/* Decode Base64 string to byte array */
unsigned char *rt_bytes_from_base64(RtArena *arena, const char *b64);

/* ============================================================================
 * Path Utilities
 * ============================================================================
 * Static methods for path manipulation.
 * ============================================================================ */

/* Extract directory portion of a path */
char *rt_path_directory(RtArena *arena, const char *path);

/* Extract filename (with extension) from a path */
char *rt_path_filename(RtArena *arena, const char *path);

/* Extract file extension (without dot) from a path */
char *rt_path_extension(RtArena *arena, const char *path);

/* Join two path components */
char *rt_path_join2(RtArena *arena, const char *path1, const char *path2);

/* Join three path components */
char *rt_path_join3(RtArena *arena, const char *path1, const char *path2, const char *path3);

/* Resolve a path to its absolute form */
char *rt_path_absolute(RtArena *arena, const char *path);

/* Check if a path exists */
int rt_path_exists(const char *path);

/* Check if a path points to a regular file */
int rt_path_is_file(const char *path);

/* Check if a path points to a directory */
int rt_path_is_directory(const char *path);

/* ============================================================================
 * Directory Operations
 * ============================================================================
 * Static methods for directory manipulation.
 * ============================================================================ */

/* List files in a directory (non-recursive) */
char **rt_directory_list(RtArena *arena, const char *path);

/* List files in a directory recursively */
char **rt_directory_list_recursive(RtArena *arena, const char *path);

/* Create a directory (including parents if needed) */
void rt_directory_create(const char *path);

/* Delete an empty directory */
void rt_directory_delete(const char *path);

/* Delete a directory and all its contents recursively */
void rt_directory_delete_recursive(const char *path);

/* ============================================================================
 * String Splitting Methods
 * ============================================================================
 * Methods for splitting strings into arrays.
 * ============================================================================ */

/* Split string on any whitespace character */
char **rt_str_split_whitespace(RtArena *arena, const char *str);

/* Split string on line endings (\n, \r\n, \r) */
char **rt_str_split_lines(RtArena *arena, const char *str);

/* Check if string is empty or contains only whitespace */
int rt_str_is_blank(const char *str);

/* ============================================================================
 * Time Type
 * ============================================================================
 * Time represents an instant in time, stored internally as milliseconds since
 * the Unix epoch (January 1, 1970, 00:00:00 UTC). Time values are lightweight
 * and integrate with arena-based memory management.
 * ============================================================================ */

/* ============================================================================
 * Time Static Methods
 * ============================================================================
 * Static methods for creating Time values and utility operations.
 * ============================================================================ */

/* Get current local time */
RtTime *rt_time_now(RtArena *arena);

/* Get current UTC time */
RtTime *rt_time_utc(RtArena *arena);

/* Create Time from milliseconds since Unix epoch */
RtTime *rt_time_from_millis(RtArena *arena, long long ms);

/* Create Time from seconds since Unix epoch */
RtTime *rt_time_from_seconds(RtArena *arena, long long s);

/* Pause execution for specified number of milliseconds */
void rt_time_sleep(long ms);

/* ============================================================================
 * Time Instance Getter Methods
 * ============================================================================
 * Methods for extracting components from a Time value.
 * ============================================================================ */

/* Get time as milliseconds since Unix epoch */
long long rt_time_get_millis(RtTime *time);

/* Get time as seconds since Unix epoch */
long long rt_time_get_seconds(RtTime *time);

/* Get 4-digit year */
long rt_time_get_year(RtTime *time);

/* Get month (1-12) */
long rt_time_get_month(RtTime *time);

/* Get day of month (1-31) */
long rt_time_get_day(RtTime *time);

/* Get hour (0-23) */
long rt_time_get_hour(RtTime *time);

/* Get minute (0-59) */
long rt_time_get_minute(RtTime *time);

/* Get second (0-59) */
long rt_time_get_second(RtTime *time);

/* Get day of week (0=Sunday, 6=Saturday) */
long rt_time_get_weekday(RtTime *time);

/* ============================================================================
 * Time Instance Formatting Methods
 * ============================================================================
 * Methods for converting Time to string representations.
 * Pattern tokens for rt_time_format:
 *   YYYY - 4-digit year      YY - 2-digit year
 *   MM   - 2-digit month     M  - month without padding (1-12)
 *   DD   - 2-digit day       D  - day without padding (1-31)
 *   HH   - 2-digit hour 24h  H  - hour without padding (0-23)
 *   hh   - 2-digit hour 12h  h  - hour 12h without padding (1-12)
 *   mm   - 2-digit minute    m  - minute without padding (0-59)
 *   ss   - 2-digit second    s  - second without padding (0-59)
 *   SSS  - 3-digit milliseconds
 *   A    - AM/PM             a  - am/pm
 * ============================================================================ */

/* Format time using pattern string - see pattern tokens above */
char *rt_time_format(RtArena *arena, RtTime *time, const char *pattern);

/* Return time in ISO 8601 format (YYYY-MM-DDTHH:mm:ss.SSSZ) */
char *rt_time_to_iso(RtArena *arena, RtTime *time);

/* Return just the date portion (YYYY-MM-DD) */
char *rt_time_to_date(RtArena *arena, RtTime *time);

/* Return just the time portion (HH:mm:ss) */
char *rt_time_to_time(RtArena *arena, RtTime *time);

/* ============================================================================
 * Time Instance Arithmetic Methods
 * ============================================================================
 * Methods for time calculations. All add* methods return new Time values.
 * ============================================================================ */

/* Add milliseconds and return new Time */
RtTime *rt_time_add(RtArena *arena, RtTime *time, long long ms);

/* Add seconds and return new Time */
RtTime *rt_time_add_seconds(RtArena *arena, RtTime *time, long seconds);

/* Add minutes and return new Time */
RtTime *rt_time_add_minutes(RtArena *arena, RtTime *time, long minutes);

/* Add hours and return new Time */
RtTime *rt_time_add_hours(RtArena *arena, RtTime *time, long hours);

/* Add days and return new Time */
RtTime *rt_time_add_days(RtArena *arena, RtTime *time, long days);

/* Return difference between two times in milliseconds (this - other) */
long long rt_time_diff(RtTime *time, RtTime *other);

/* ============================================================================
 * Time Instance Comparison Methods
 * ============================================================================
 * Methods for comparing Time values. Return 1 for true, 0 for false.
 * ============================================================================ */

/* Return 1 if this time is before other time */
int rt_time_is_before(RtTime *time, RtTime *other);

/* Return 1 if this time is after other time */
int rt_time_is_after(RtTime *time, RtTime *other);

/* Return 1 if both times represent the same instant */
int rt_time_equals(RtTime *time, RtTime *other);

#endif /* RUNTIME_H */
