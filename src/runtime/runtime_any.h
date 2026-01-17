#ifndef RUNTIME_ANY_H
#define RUNTIME_ANY_H

#include <stdint.h>
#include <stdbool.h>
#include "runtime_arena.h"

/* ============================================================================
 * Any Type - Runtime Type System
 * ============================================================================
 * The 'any' type is a tagged union that can hold any Sindarin value.
 * It provides runtime type checking and casting capabilities.
 * ============================================================================ */

/* Type tags for runtime type identification */
typedef enum {
    RT_ANY_NIL = 0,
    RT_ANY_INT,
    RT_ANY_LONG,
    RT_ANY_INT32,
    RT_ANY_UINT,
    RT_ANY_UINT32,
    RT_ANY_DOUBLE,
    RT_ANY_FLOAT,
    RT_ANY_STRING,
    RT_ANY_CHAR,
    RT_ANY_BOOL,
    RT_ANY_BYTE,
    RT_ANY_ARRAY,
    RT_ANY_FUNCTION,
    /* Built-in object types */
    RT_ANY_TEXT_FILE,
    RT_ANY_BINARY_FILE,
    RT_ANY_DATE,
    RT_ANY_TIME,
    RT_ANY_PROCESS,
    RT_ANY_TCP_LISTENER,
    RT_ANY_TCP_STREAM,
    RT_ANY_UDP_SOCKET,
    RT_ANY_RANDOM,
    RT_ANY_UUID
} RtAnyTag;

/* The any type - a tagged union */
typedef struct {
    RtAnyTag tag;
    union {
        int64_t i64;        /* int, long */
        int32_t i32;        /* int32 */
        uint64_t u64;       /* uint */
        uint32_t u32;       /* uint32 */
        double d;           /* double */
        float f;            /* float */
        char *s;            /* str */
        char c;             /* char */
        bool b;             /* bool */
        uint8_t byte;       /* byte */
        void *arr;          /* arrays (RtArray*) */
        void *fn;           /* function pointers */
        void *obj;          /* object types (files, etc.) */
    } value;
    /* For arrays: track element type tag for nested any[] support */
    RtAnyTag element_tag;
} RtAny;

/* ============================================================================
 * Boxing Functions - Convert concrete types to any
 * ============================================================================ */

RtAny rt_box_nil(void);
RtAny rt_box_int(int64_t value);
RtAny rt_box_long(int64_t value);
RtAny rt_box_int32(int32_t value);
RtAny rt_box_uint(uint64_t value);
RtAny rt_box_uint32(uint32_t value);
RtAny rt_box_double(double value);
RtAny rt_box_float(float value);
RtAny rt_box_string(const char *value);
RtAny rt_box_char(char value);
RtAny rt_box_bool(bool value);
RtAny rt_box_byte(uint8_t value);
RtAny rt_box_array(void *arr, RtAnyTag element_tag);
RtAny rt_box_function(void *fn);

/* Box object types */
RtAny rt_box_text_file(void *file);
RtAny rt_box_binary_file(void *file);
RtAny rt_box_date(void *date);
RtAny rt_box_time(void *time);
RtAny rt_box_process(void *process);
RtAny rt_box_tcp_listener(void *listener);
RtAny rt_box_tcp_stream(void *stream);
RtAny rt_box_udp_socket(void *socket);
RtAny rt_box_random(void *random);
RtAny rt_box_uuid(void *uuid);

/* ============================================================================
 * Unboxing Functions - Convert any to concrete types (panic on type mismatch)
 * ============================================================================ */

int64_t rt_unbox_int(RtAny value);
int64_t rt_unbox_long(RtAny value);
int32_t rt_unbox_int32(RtAny value);
uint64_t rt_unbox_uint(RtAny value);
uint32_t rt_unbox_uint32(RtAny value);
double rt_unbox_double(RtAny value);
float rt_unbox_float(RtAny value);
const char *rt_unbox_string(RtAny value);
char rt_unbox_char(RtAny value);
bool rt_unbox_bool(RtAny value);
uint8_t rt_unbox_byte(RtAny value);
void *rt_unbox_array(RtAny value);
void *rt_unbox_function(RtAny value);

/* Unbox object types */
void *rt_unbox_text_file(RtAny value);
void *rt_unbox_binary_file(RtAny value);
void *rt_unbox_date(RtAny value);
void *rt_unbox_time(RtAny value);
void *rt_unbox_process(RtAny value);
void *rt_unbox_tcp_listener(RtAny value);
void *rt_unbox_tcp_stream(RtAny value);
void *rt_unbox_udp_socket(RtAny value);
void *rt_unbox_random(RtAny value);
void *rt_unbox_uuid(RtAny value);

/* ============================================================================
 * Type Checking Functions
 * ============================================================================ */

/* Check if any value has a specific type */
bool rt_any_is_nil(RtAny value);
bool rt_any_is_int(RtAny value);
bool rt_any_is_long(RtAny value);
bool rt_any_is_int32(RtAny value);
bool rt_any_is_uint(RtAny value);
bool rt_any_is_uint32(RtAny value);
bool rt_any_is_double(RtAny value);
bool rt_any_is_float(RtAny value);
bool rt_any_is_string(RtAny value);
bool rt_any_is_char(RtAny value);
bool rt_any_is_bool(RtAny value);
bool rt_any_is_byte(RtAny value);
bool rt_any_is_array(RtAny value);
bool rt_any_is_function(RtAny value);

/* Get type tag */
RtAnyTag rt_any_get_tag(RtAny value);

/* Get type name as string (for error messages and debugging) */
const char *rt_any_type_name(RtAny value);
const char *rt_any_tag_name(RtAnyTag tag);

/* ============================================================================
 * Comparison Functions
 * ============================================================================ */

/* Compare two any values for equality */
bool rt_any_equals(RtAny a, RtAny b);

/* Compare type tags */
bool rt_any_same_type(RtAny a, RtAny b);

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

/* Convert any value to string representation (for debugging) */
char *rt_any_to_string(RtArena *arena, RtAny value);

/* Promote an any value's heap-allocated data to a target arena.
 * Used when returning any values from functions to ensure data survives
 * the destruction of the function's local arena. */
RtAny rt_any_promote(RtArena *target_arena, RtAny value);

#endif /* RUNTIME_ANY_H */
