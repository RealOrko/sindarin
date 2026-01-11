/**
 * runtime_intercept.h - Function interceptor infrastructure for Sindarin
 *
 * Provides the ability to intercept user-defined function calls for debugging,
 * profiling, mocking, and AOP-style programming patterns.
 */

#ifndef RUNTIME_INTERCEPT_H
#define RUNTIME_INTERCEPT_H

#include "runtime_any.h"
#include <stdbool.h>

// Forward declaration for arena
typedef struct RtArena RtArena;

/**
 * Continue callback type - calls the original function (or next interceptor in chain)
 * with the current args array values.
 */
typedef RtAny (*RtContinueFn)(void);

/**
 * Interceptor handler function type.
 *
 * @param arena     Arena for memory allocations (from caller's context)
 * @param name      The name of the function being called
 * @param args      Array of boxed arguments (can be modified)
 * @param arg_count Number of arguments
 * @param continue_fn Callback to invoke the original function
 * @return The result to return to the caller (can substitute the real result)
 */
typedef RtAny (*RtInterceptHandler)(
    RtArena *arena,
    const char *name,
    RtAny *args,
    int arg_count,
    RtContinueFn continue_fn
);

/**
 * Pattern-matching interceptor entry.
 */
typedef struct RtInterceptorEntry
{
    RtInterceptHandler handler;
    const char *pattern; // NULL for "match all", or a pattern like "get*", "*User", "get*Name"
} RtInterceptorEntry;

/**
 * Global interceptor count for fast check at call sites.
 * When zero, function calls bypass interception entirely.
 */
extern volatile int __rt_interceptor_count;

/**
 * Per-thread interception depth for recursion detection.
 * Used by Interceptor.isActive() to detect if we're inside an interceptor.
 */
#ifdef _WIN32
extern __declspec(thread) int __rt_intercept_depth;
#elif defined(__TINYC__)
/* TinyCC doesn't support __thread, fall back to global (not thread-safe) */
extern int __rt_intercept_depth;
#else
extern __thread int __rt_intercept_depth;
#endif

/**
 * Per-thread arguments array for thunk functions.
 * Used to pass boxed arguments to file-scope thunk functions without
 * requiring nested functions (which are a GCC extension).
 */
#ifdef _WIN32
extern __declspec(thread) RtAny *__rt_thunk_args;
extern __declspec(thread) void *__rt_thunk_arena;
#elif defined(__TINYC__)
extern RtAny *__rt_thunk_args;
extern void *__rt_thunk_arena;
#else
extern __thread RtAny *__rt_thunk_args;
extern __thread void *__rt_thunk_arena;
#endif

/**
 * Register an interceptor for all user-defined functions.
 *
 * @param handler The interceptor function to register
 */
void rt_interceptor_register(RtInterceptHandler handler);

/**
 * Register an interceptor with pattern matching.
 * Patterns support wildcard (*) at start, middle, or end.
 *
 * @param handler The interceptor function to register
 * @param pattern Pattern to match function names (e.g., "get*", "*User", "get*Name")
 */
void rt_interceptor_register_where(RtInterceptHandler handler, const char *pattern);

/**
 * Clear all registered interceptors.
 */
void rt_interceptor_clear_all(void);

/**
 * Get the current count of registered interceptors.
 *
 * @return Number of registered interceptors
 */
int rt_interceptor_count(void);

/**
 * Get the list of registered interceptor handlers.
 *
 * @param out_count Output parameter for the number of handlers
 * @return Array of interceptor entries (owned by runtime, do not free)
 */
RtInterceptorEntry *rt_interceptor_list(int *out_count);

/**
 * Check if currently inside an interceptor call.
 * Useful for preventing infinite recursion in interceptors.
 *
 * @return true if inside an interceptor, false otherwise
 */
bool rt_interceptor_is_active(void);

/**
 * Call a function through the interceptor chain.
 * This is called by generated code for user-defined function calls.
 *
 * @param name         The function name
 * @param args         Array of boxed arguments
 * @param arg_count    Number of arguments
 * @param original_fn  The original function to call if no interception
 * @return The (possibly intercepted) result
 */
RtAny rt_call_intercepted(
    const char *name,
    RtAny *args,
    int arg_count,
    RtContinueFn original_fn
);

/**
 * Check if a function name matches a pattern.
 * Patterns can contain wildcards (*) at start, middle, or end.
 *
 * @param name    The function name to check
 * @param pattern The pattern to match against
 * @return true if the name matches the pattern
 */
bool rt_pattern_matches(const char *name, const char *pattern);

#endif // RUNTIME_INTERCEPT_H
