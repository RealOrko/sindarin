#ifndef RUNTIME_H
#define RUNTIME_H

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

/* Arena - manages a collection of memory blocks */
typedef struct RtArena {
    struct RtArena *parent;     /* Parent arena for hierarchy */
    RtArenaBlock *first;        /* First block in chain */
    RtArenaBlock *current;      /* Current block for allocations */
    size_t default_block_size;  /* Size for new blocks */
    size_t total_allocated;     /* Total bytes allocated (for stats) */
} RtArena;

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
 * String operations
 * ============================================================================ */

/* String concatenation - allocates from arena */
char *rt_str_concat(RtArena *arena, const char *left, const char *right);

/* Print functions (no allocation) */
void rt_print_long(long val);
void rt_print_double(double val);
void rt_print_char(long c);
void rt_print_string(const char *s);
void rt_print_bool(long b);

/* Type conversion to string - allocates from arena */
char *rt_to_string_long(RtArena *arena, long val);
char *rt_to_string_double(RtArena *arena, double val);
char *rt_to_string_char(RtArena *arena, char val);
char *rt_to_string_bool(RtArena *arena, int val);
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

/* Long comparisons */
int rt_eq_long(long a, long b);
int rt_ne_long(long a, long b);
int rt_lt_long(long a, long b);
int rt_le_long(long a, long b);
int rt_gt_long(long a, long b);
int rt_ge_long(long a, long b);

/* Double arithmetic (with overflow checking) */
double rt_add_double(double a, double b);
double rt_sub_double(double a, double b);
double rt_mul_double(double a, double b);
double rt_div_double(double a, double b);
double rt_neg_double(double a);

/* Double comparisons */
int rt_eq_double(double a, double b);
int rt_ne_double(double a, double b);
int rt_lt_double(double a, double b);
int rt_le_double(double a, double b);
int rt_gt_double(double a, double b);
int rt_ge_double(double a, double b);

/* Boolean operations */
int rt_not_bool(int a);

/* Increment/decrement */
long rt_post_inc_long(long *p);
long rt_post_dec_long(long *p);

/* String comparisons */
int rt_eq_string(const char *a, const char *b);
int rt_ne_string(const char *a, const char *b);
int rt_lt_string(const char *a, const char *b);
int rt_le_string(const char *a, const char *b);
int rt_gt_string(const char *a, const char *b);
int rt_ge_string(const char *a, const char *b);

/* Array operations - allocate from arena */
long *rt_array_push_long(RtArena *arena, long *arr, long element);
double *rt_array_push_double(RtArena *arena, double *arr, double element);
char *rt_array_push_char(RtArena *arena, char *arr, char element);
char **rt_array_push_string(RtArena *arena, char **arr, const char *element);
int *rt_array_push_bool(RtArena *arena, int *arr, int element);

/* Array length */
size_t rt_array_length(void *arr);

/* Array print functions */
void rt_print_array_long(long *arr);
void rt_print_array_double(double *arr);
void rt_print_array_char(char *arr);
void rt_print_array_bool(int *arr);
void rt_print_array_string(char **arr);

/* Array clear */
void rt_array_clear(void *arr);

/* Array pop functions */
long rt_array_pop_long(long *arr);
double rt_array_pop_double(double *arr);
char rt_array_pop_char(char *arr);
int rt_array_pop_bool(int *arr);
char *rt_array_pop_string(char **arr);

/* Array concat functions - allocate from arena */
long *rt_array_concat_long(RtArena *arena, long *dest, long *src);
double *rt_array_concat_double(RtArena *arena, double *dest, double *src);
char *rt_array_concat_char(RtArena *arena, char *dest, char *src);
int *rt_array_concat_bool(RtArena *arena, int *dest, int *src);
char **rt_array_concat_string(RtArena *arena, char **dest, char **src);

/* Array slice functions - step defaults to 1 if LONG_MIN, allocate from arena */
long *rt_array_slice_long(RtArena *arena, long *arr, long start, long end, long step);
double *rt_array_slice_double(RtArena *arena, double *arr, long start, long end, long step);
char *rt_array_slice_char(RtArena *arena, char *arr, long start, long end, long step);
int *rt_array_slice_bool(RtArena *arena, int *arr, long start, long end, long step);
char **rt_array_slice_string(RtArena *arena, char **arr, long start, long end, long step);

/* Array reverse functions - allocate from arena */
long *rt_array_rev_long(RtArena *arena, long *arr);
double *rt_array_rev_double(RtArena *arena, double *arr);
char *rt_array_rev_char(RtArena *arena, char *arr);
int *rt_array_rev_bool(RtArena *arena, int *arr);
char **rt_array_rev_string(RtArena *arena, char **arr);

/* Array remove at index functions - allocate from arena */
long *rt_array_rem_long(RtArena *arena, long *arr, long index);
double *rt_array_rem_double(RtArena *arena, double *arr, long index);
char *rt_array_rem_char(RtArena *arena, char *arr, long index);
int *rt_array_rem_bool(RtArena *arena, int *arr, long index);
char **rt_array_rem_string(RtArena *arena, char **arr, long index);

/* Array insert at index functions - allocate from arena */
long *rt_array_ins_long(RtArena *arena, long *arr, long elem, long index);
double *rt_array_ins_double(RtArena *arena, double *arr, double elem, long index);
char *rt_array_ins_char(RtArena *arena, char *arr, char elem, long index);
int *rt_array_ins_bool(RtArena *arena, int *arr, int elem, long index);
char **rt_array_ins_string(RtArena *arena, char **arr, const char *elem, long index);

/* Array push (copy) functions - return NEW array with element appended, allocate from arena */
long *rt_array_push_copy_long(RtArena *arena, long *arr, long elem);
double *rt_array_push_copy_double(RtArena *arena, double *arr, double elem);
char *rt_array_push_copy_char(RtArena *arena, char *arr, char elem);
int *rt_array_push_copy_bool(RtArena *arena, int *arr, int elem);
char **rt_array_push_copy_string(RtArena *arena, char **arr, const char *elem);

/* Array indexOf functions - find first index of element, returns -1 if not found */
long rt_array_indexOf_long(long *arr, long elem);
long rt_array_indexOf_double(double *arr, double elem);
long rt_array_indexOf_char(char *arr, char elem);
long rt_array_indexOf_bool(int *arr, int elem);
long rt_array_indexOf_string(char **arr, const char *elem);

/* Array contains functions - check if element exists */
int rt_array_contains_long(long *arr, long elem);
int rt_array_contains_double(double *arr, double elem);
int rt_array_contains_char(char *arr, char elem);
int rt_array_contains_bool(int *arr, int elem);
int rt_array_contains_string(char **arr, const char *elem);

/* Array clone functions - create deep copy, allocate from arena */
long *rt_array_clone_long(RtArena *arena, long *arr);
double *rt_array_clone_double(RtArena *arena, double *arr);
char *rt_array_clone_char(RtArena *arena, char *arr);
int *rt_array_clone_bool(RtArena *arena, int *arr);
char **rt_array_clone_string(RtArena *arena, char **arr);

/* Array join functions - join elements into string with separator, allocate from arena */
char *rt_array_join_long(RtArena *arena, long *arr, const char *separator);
char *rt_array_join_double(RtArena *arena, double *arr, const char *separator);
char *rt_array_join_char(RtArena *arena, char *arr, const char *separator);
char *rt_array_join_bool(RtArena *arena, int *arr, const char *separator);
char *rt_array_join_string(RtArena *arena, char **arr, const char *separator);

/* Array create from static data - creates runtime array with metadata, allocate from arena */
long *rt_array_create_long(RtArena *arena, size_t count, const long *data);
double *rt_array_create_double(RtArena *arena, size_t count, const double *data);
char *rt_array_create_char(RtArena *arena, size_t count, const char *data);
int *rt_array_create_bool(RtArena *arena, size_t count, const int *data);
char **rt_array_create_string(RtArena *arena, size_t count, const char **data);

/* Array equality functions - compare arrays element by element */
int rt_array_eq_long(long *a, long *b);
int rt_array_eq_double(double *a, double *b);
int rt_array_eq_char(char *a, char *b);
int rt_array_eq_bool(int *a, int *b);
int rt_array_eq_string(char **a, char **b);

/* Range creation - creates int[] from start to end (exclusive), allocate from arena */
long *rt_array_range(RtArena *arena, long start, long end);

#endif /* RUNTIME_H */
