#ifndef RUNTIME_ARENA_H
#define RUNTIME_ARENA_H

#include <stddef.h>
#include <stdbool.h>

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
 * Arena Function Declarations
 * ============================================================================ */

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

#endif /* RUNTIME_ARENA_H */
