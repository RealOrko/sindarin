#ifndef RUNTIME_ARRAY_H
#define RUNTIME_ARRAY_H

#include <stddef.h>
#include "runtime_arena.h"

/* ============================================================================
 * Array Metadata
 * ============================================================================
 * Array metadata structure - stored immediately before array data in memory.
 * This allows rt_array_length to be inlined for maximum performance.
 *
 * Memory layout for arrays:
 *   [RtArrayMetadata (24 bytes)] [array data...]
 *                                ^
 *                                |-- Array pointer points HERE
 *
 * The metadata is stored BEFORE the array data pointer, so:
 *   metadata = ((RtArrayMetadata *)arr)[-1]
 *
 * On 64-bit systems: sizeof(RtArrayMetadata) = 24 bytes (3 * 8 bytes)
 * On 32-bit systems: sizeof(RtArrayMetadata) = 12 bytes (3 * 4 bytes)
 * ============================================================================ */

typedef struct {
    RtArena *arena;  /* Arena that owns this array (for reallocation) */
    size_t size;     /* Number of elements currently in the array */
    size_t capacity; /* Total allocated space for elements */
} RtArrayMetadata;

/* ============================================================================
 * Array Operations
 * ============================================================================ */

/* Get the length of an array (O(1) operation) */
static inline size_t rt_array_length(void *arr) {
    if (arr == NULL) return 0;
    return ((RtArrayMetadata *)arr)[-1].size;
}

/* Clear all elements from an array (sets size to 0, keeps capacity) */
void rt_array_clear(void *arr);

/* ============================================================================
 * Array Push Functions
 * ============================================================================
 * Push element to end of array, growing capacity if needed.
 */
long long *rt_array_push_long(RtArena *arena, long long *arr, long long element);
double *rt_array_push_double(RtArena *arena, double *arr, double element);
char *rt_array_push_char(RtArena *arena, char *arr, char element);
int *rt_array_push_bool(RtArena *arena, int *arr, int element);
unsigned char *rt_array_push_byte(RtArena *arena, unsigned char *arr, unsigned char element);
void **rt_array_push_ptr(RtArena *arena, void **arr, void *element);
char **rt_array_push_string(RtArena *arena, char **arr, const char *element);

/* ============================================================================
 * Array Pop Functions
 * ============================================================================
 * Remove and return the last element of an array.
 */
long long rt_array_pop_long(long long *arr);
double rt_array_pop_double(double *arr);
char rt_array_pop_char(char *arr);
int rt_array_pop_bool(int *arr);
unsigned char rt_array_pop_byte(unsigned char *arr);
void *rt_array_pop_ptr(void **arr);
char *rt_array_pop_string(char **arr);

/* ============================================================================
 * Array Concat Functions
 * ============================================================================
 * Return a NEW array containing elements from both arrays (non-mutating).
 */
long long *rt_array_concat_long(RtArena *arena, long long *arr1, long long *arr2);
double *rt_array_concat_double(RtArena *arena, double *arr1, double *arr2);
char *rt_array_concat_char(RtArena *arena, char *arr1, char *arr2);
int *rt_array_concat_bool(RtArena *arena, int *arr1, int *arr2);
unsigned char *rt_array_concat_byte(RtArena *arena, unsigned char *arr1, unsigned char *arr2);
void **rt_array_concat_ptr(RtArena *arena, void **arr1, void **arr2);
char **rt_array_concat_string(RtArena *arena, char **arr1, char **arr2);

/* ============================================================================
 * Array Slice Functions
 * ============================================================================
 * Create a new array from a portion of the source array.
 */
long long *rt_array_slice_long(RtArena *arena, long long *arr, long start, long end, long step);
double *rt_array_slice_double(RtArena *arena, double *arr, long start, long end, long step);
char *rt_array_slice_char(RtArena *arena, char *arr, long start, long end, long step);
int *rt_array_slice_bool(RtArena *arena, int *arr, long start, long end, long step);
unsigned char *rt_array_slice_byte(RtArena *arena, unsigned char *arr, long start, long end, long step);
char **rt_array_slice_string(RtArena *arena, char **arr, long start, long end, long step);

/* ============================================================================
 * Array Reverse Functions
 * ============================================================================
 * Return a new reversed array - the original array is not modified.
 */
long long *rt_array_rev_long(RtArena *arena, long long *arr);
double *rt_array_rev_double(RtArena *arena, double *arr);
char *rt_array_rev_char(RtArena *arena, char *arr);
int *rt_array_rev_bool(RtArena *arena, int *arr);
unsigned char *rt_array_rev_byte(RtArena *arena, unsigned char *arr);
char **rt_array_rev_string(RtArena *arena, char **arr);

/* ============================================================================
 * Array Remove At Index Functions
 * ============================================================================
 * Return a new array without the element at the specified index.
 */
long long *rt_array_rem_long(RtArena *arena, long long *arr, long index);
double *rt_array_rem_double(RtArena *arena, double *arr, long index);
char *rt_array_rem_char(RtArena *arena, char *arr, long index);
int *rt_array_rem_bool(RtArena *arena, int *arr, long index);
unsigned char *rt_array_rem_byte(RtArena *arena, unsigned char *arr, long index);
char **rt_array_rem_string(RtArena *arena, char **arr, long index);

/* ============================================================================
 * Array Insert At Index Functions
 * ============================================================================
 * Return a new array with the element inserted at the specified index.
 */
long long *rt_array_ins_long(RtArena *arena, long long *arr, long long elem, long index);
double *rt_array_ins_double(RtArena *arena, double *arr, double elem, long index);
char *rt_array_ins_char(RtArena *arena, char *arr, char elem, long index);
int *rt_array_ins_bool(RtArena *arena, int *arr, int elem, long index);
unsigned char *rt_array_ins_byte(RtArena *arena, unsigned char *arr, unsigned char elem, long index);
char **rt_array_ins_string(RtArena *arena, char **arr, const char *elem, long index);

/* ============================================================================
 * Array Push Copy Functions
 * ============================================================================
 * Create a NEW array with element appended (non-mutating push).
 */
long long *rt_array_push_copy_long(RtArena *arena, long long *arr, long long elem);
double *rt_array_push_copy_double(RtArena *arena, double *arr, double elem);
char *rt_array_push_copy_char(RtArena *arena, char *arr, char elem);
int *rt_array_push_copy_bool(RtArena *arena, int *arr, int elem);
unsigned char *rt_array_push_copy_byte(RtArena *arena, unsigned char *arr, unsigned char elem);
char **rt_array_push_copy_string(RtArena *arena, char **arr, const char *elem);

/* ============================================================================
 * Array IndexOf Functions
 * ============================================================================
 * Find first index of element, returns -1 if not found.
 */
long rt_array_indexOf_long(long long *arr, long long elem);
long rt_array_indexOf_double(double *arr, double elem);
long rt_array_indexOf_char(char *arr, char elem);
long rt_array_indexOf_bool(int *arr, int elem);
long rt_array_indexOf_byte(unsigned char *arr, unsigned char elem);
long rt_array_indexOf_string(char **arr, const char *elem);

/* ============================================================================
 * Array Contains Functions
 * ============================================================================
 * Check if element exists in array.
 */
int rt_array_contains_long(long long *arr, long long elem);
int rt_array_contains_double(double *arr, double elem);
int rt_array_contains_char(char *arr, char elem);
int rt_array_contains_bool(int *arr, int elem);
int rt_array_contains_byte(unsigned char *arr, unsigned char elem);
int rt_array_contains_string(char **arr, const char *elem);

/* ============================================================================
 * Array Clone Functions
 * ============================================================================
 * Create a deep copy of the array.
 */
long long *rt_array_clone_long(RtArena *arena, long long *arr);
double *rt_array_clone_double(RtArena *arena, double *arr);
char *rt_array_clone_char(RtArena *arena, char *arr);
int *rt_array_clone_bool(RtArena *arena, int *arr);
unsigned char *rt_array_clone_byte(RtArena *arena, unsigned char *arr);
char **rt_array_clone_string(RtArena *arena, char **arr);

/* ============================================================================
 * Array Join Functions
 * ============================================================================
 * Join array elements into a string with separator.
 */
char *rt_array_join_long(RtArena *arena, long long *arr, const char *separator);
char *rt_array_join_double(RtArena *arena, double *arr, const char *separator);
char *rt_array_join_char(RtArena *arena, char *arr, const char *separator);
char *rt_array_join_bool(RtArena *arena, int *arr, const char *separator);
char *rt_array_join_byte(RtArena *arena, unsigned char *arr, const char *separator);
char *rt_array_join_string(RtArena *arena, char **arr, const char *separator);

/* ============================================================================
 * Array Create Functions
 * ============================================================================
 * Create runtime array from static C array.
 */
long long *rt_array_create_long(RtArena *arena, size_t count, const long long *data);
double *rt_array_create_double(RtArena *arena, size_t count, const double *data);
char *rt_array_create_char(RtArena *arena, size_t count, const char *data);
int *rt_array_create_bool(RtArena *arena, size_t count, const int *data);
unsigned char *rt_array_create_byte(RtArena *arena, size_t count, const unsigned char *data);
char **rt_array_create_string(RtArena *arena, size_t count, const char **data);
unsigned char *rt_array_create_byte_uninit(RtArena *arena, size_t count);

/* ============================================================================
 * Array Alloc Functions
 * ============================================================================
 * Create array of count elements filled with default_value.
 */
long long *rt_array_alloc_long(RtArena *arena, size_t count, long long default_value);
double *rt_array_alloc_double(RtArena *arena, size_t count, double default_value);
char *rt_array_alloc_char(RtArena *arena, size_t count, char default_value);
int *rt_array_alloc_bool(RtArena *arena, size_t count, int default_value);
unsigned char *rt_array_alloc_byte(RtArena *arena, size_t count, unsigned char default_value);
char **rt_array_alloc_string(RtArena *arena, size_t count, const char *default_value);

/* ============================================================================
 * Array Equality Functions
 * ============================================================================
 * Check if two arrays are equal (same length and all elements equal).
 */
int rt_array_eq_long(long long *a, long long *b);
int rt_array_eq_double(double *a, double *b);
int rt_array_eq_char(char *a, char *b);
int rt_array_eq_bool(int *a, int *b);
int rt_array_eq_byte(unsigned char *a, unsigned char *b);
int rt_array_eq_string(char **a, char **b);

/* ============================================================================
 * Array Range Function
 * ============================================================================
 * Create array of longs from start to end (exclusive).
 */
long long *rt_array_range(RtArena *arena, long long start, long long end);

/* ============================================================================
 * Array Print Functions
 * ============================================================================
 * Print array contents to stdout.
 */
void rt_print_array_long(long long *arr);
void rt_print_array_double(double *arr);
void rt_print_array_char(char *arr);
void rt_print_array_bool(int *arr);
void rt_print_array_byte(unsigned char *arr);
void rt_print_array_string(char **arr);

#endif /* RUNTIME_ARRAY_H */
