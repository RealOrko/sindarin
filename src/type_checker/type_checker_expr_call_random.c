/* ============================================================================
 * type_checker_expr_call_random.c - Random/UUID/Process Type Method Type Checking
 * ============================================================================
 * Type checking for Random, UUID, and Process method access.
 * Returns the function type for the method, or NULL if not a valid method.
 * Caller should handle errors for invalid members.
 * ============================================================================ */

#include "type_checker/type_checker_expr_call_random.h"
#include "type_checker/type_checker_expr_call.h"
#include "debug.h"

/* ============================================================================
 * Process Property Type Checking
 * ============================================================================
 * Handles type checking for Process property access (not calls).
 * Process has three properties: exitCode (int), stdout (str), stderr (str).
 * Returns the property type, or NULL if not a Process property.
 * ============================================================================ */

Type *type_check_process_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table)
{
    (void)expr; /* Reserved for future use (e.g., error location) */

    /* Only handle Process types */
    if (object_type->kind != TYPE_PROCESS)
    {
        return NULL;
    }

    /* Process properties (accessed as member, return value directly) */

    /* process.exitCode -> int */
    if (token_equals(member_name, "exitCode"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        DEBUG_VERBOSE("Returning int type for Process exitCode property");
        return int_type;
    }

    /* process.stdout -> str */
    if (token_equals(member_name, "stdout"))
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        DEBUG_VERBOSE("Returning string type for Process stdout property");
        return string_type;
    }

    /* process.stderr -> str */
    if (token_equals(member_name, "stderr"))
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        DEBUG_VERBOSE("Returning string type for Process stderr property");
        return string_type;
    }

    /* Not a Process property */
    return NULL;
}

/* ============================================================================
 * Random Type Method Type Checking
 * ============================================================================
 * Handles type checking for Random instance method calls like:
 *   rng.int(min, max), rng.long(min, max), rng.double(min, max),
 *   rng.bool(), rng.byte(), rng.bytes(count), rng.gaussian(mean, stddev)
 * ============================================================================ */

Type *type_check_random_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table)
{
    (void)expr; /* Reserved for future use (e.g., error location) */

    /* Only handle Random types */
    if (object_type->kind != TYPE_RANDOM)
    {
        return NULL;
    }

    /* rng.int(min, max) -> int */
    if (token_equals(member_name, "int"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[2] = {int_type, int_type};
        DEBUG_VERBOSE("Returning function type for Random int method");
        return ast_create_function_type(table->arena, int_type, param_types, 2);
    }

    /* rng.long(min, max) -> long */
    if (token_equals(member_name, "long"))
    {
        Type *long_type = ast_create_primitive_type(table->arena, TYPE_LONG);
        Type *param_types[2] = {long_type, long_type};
        DEBUG_VERBOSE("Returning function type for Random long method");
        return ast_create_function_type(table->arena, long_type, param_types, 2);
    }

    /* rng.double(min, max) -> double */
    if (token_equals(member_name, "double"))
    {
        Type *double_type = ast_create_primitive_type(table->arena, TYPE_DOUBLE);
        Type *param_types[2] = {double_type, double_type};
        DEBUG_VERBOSE("Returning function type for Random double method");
        return ast_create_function_type(table->arena, double_type, param_types, 2);
    }

    /* rng.bool() -> bool */
    if (token_equals(member_name, "bool"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Random bool method");
        return ast_create_function_type(table->arena, bool_type, param_types, 0);
    }

    /* rng.byte() -> byte */
    if (token_equals(member_name, "byte"))
    {
        Type *byte_type = ast_create_primitive_type(table->arena, TYPE_BYTE);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Random byte method");
        return ast_create_function_type(table->arena, byte_type, param_types, 0);
    }

    /* rng.bytes(count) -> byte[] */
    if (token_equals(member_name, "bytes"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *byte_type = ast_create_primitive_type(table->arena, TYPE_BYTE);
        Type *byte_array_type = ast_create_array_type(table->arena, byte_type);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for Random bytes method");
        return ast_create_function_type(table->arena, byte_array_type, param_types, 1);
    }

    /* rng.gaussian(mean, stddev) -> double */
    if (token_equals(member_name, "gaussian"))
    {
        Type *double_type = ast_create_primitive_type(table->arena, TYPE_DOUBLE);
        Type *param_types[2] = {double_type, double_type};
        DEBUG_VERBOSE("Returning function type for Random gaussian method");
        return ast_create_function_type(table->arena, double_type, param_types, 2);
    }

    /* ========================================================================
     * Batch Generation Methods
     * ======================================================================== */

    /* rng.intMany(min, max, count) -> int[] */
    if (token_equals(member_name, "intMany"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *int_array_type = ast_create_array_type(table->arena, int_type);
        Type *param_types[3] = {int_type, int_type, int_type};
        DEBUG_VERBOSE("Returning function type for Random intMany method");
        return ast_create_function_type(table->arena, int_array_type, param_types, 3);
    }

    /* rng.longMany(min, max, count) -> long[] */
    if (token_equals(member_name, "longMany"))
    {
        Type *long_type = ast_create_primitive_type(table->arena, TYPE_LONG);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *long_array_type = ast_create_array_type(table->arena, long_type);
        Type *param_types[3] = {long_type, long_type, int_type};
        DEBUG_VERBOSE("Returning function type for Random longMany method");
        return ast_create_function_type(table->arena, long_array_type, param_types, 3);
    }

    /* rng.doubleMany(min, max, count) -> double[] */
    if (token_equals(member_name, "doubleMany"))
    {
        Type *double_type = ast_create_primitive_type(table->arena, TYPE_DOUBLE);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *double_array_type = ast_create_array_type(table->arena, double_type);
        Type *param_types[3] = {double_type, double_type, int_type};
        DEBUG_VERBOSE("Returning function type for Random doubleMany method");
        return ast_create_function_type(table->arena, double_array_type, param_types, 3);
    }

    /* rng.boolMany(count) -> bool[] */
    if (token_equals(member_name, "boolMany"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *bool_array_type = ast_create_array_type(table->arena, bool_type);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for Random boolMany method");
        return ast_create_function_type(table->arena, bool_array_type, param_types, 1);
    }

    /* rng.gaussianMany(mean, stddev, count) -> double[] */
    if (token_equals(member_name, "gaussianMany"))
    {
        Type *double_type = ast_create_primitive_type(table->arena, TYPE_DOUBLE);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *double_array_type = ast_create_array_type(table->arena, double_type);
        Type *param_types[3] = {double_type, double_type, int_type};
        DEBUG_VERBOSE("Returning function type for Random gaussianMany method");
        return ast_create_function_type(table->arena, double_array_type, param_types, 3);
    }

    /* Not a Random method */
    return NULL;
}

/* ============================================================================
 * UUID Instance Method Type Checking
 * ============================================================================
 * Handles type checking for UUID instance methods like toString(), version(),
 * toBytes(), equals(), etc.
 * ============================================================================ */

Type *type_check_uuid_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table)
{
    (void)expr; /* Reserved for future use (e.g., error location) */

    /* Only handle UUID types */
    if (object_type->kind != TYPE_UUID)
    {
        return NULL;
    }

    /* uuid.toString() -> str */
    if (token_equals(member_name, "toString"))
    {
        Type *str_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for UUID toString method");
        return ast_create_function_type(table->arena, str_type, param_types, 0);
    }

    /* uuid.toHex() -> str */
    if (token_equals(member_name, "toHex"))
    {
        Type *str_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for UUID toHex method");
        return ast_create_function_type(table->arena, str_type, param_types, 0);
    }

    /* uuid.toBase64() -> str */
    if (token_equals(member_name, "toBase64"))
    {
        Type *str_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for UUID toBase64 method");
        return ast_create_function_type(table->arena, str_type, param_types, 0);
    }

    /* uuid.toBytes() -> byte[] */
    if (token_equals(member_name, "toBytes"))
    {
        Type *byte_type = ast_create_primitive_type(table->arena, TYPE_BYTE);
        Type *byte_array_type = ast_create_array_type(table->arena, byte_type);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for UUID toBytes method");
        return ast_create_function_type(table->arena, byte_array_type, param_types, 0);
    }

    /* uuid.version() -> int */
    if (token_equals(member_name, "version"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for UUID version method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* uuid.variant() -> int */
    if (token_equals(member_name, "variant"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for UUID variant method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* uuid.isNil() -> bool */
    if (token_equals(member_name, "isNil"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for UUID isNil method");
        return ast_create_function_type(table->arena, bool_type, param_types, 0);
    }

    /* uuid.timestamp() -> long (v7 only) */
    if (token_equals(member_name, "timestamp"))
    {
        Type *long_type = ast_create_primitive_type(table->arena, TYPE_LONG);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for UUID timestamp method");
        return ast_create_function_type(table->arena, long_type, param_types, 0);
    }

    /* uuid.equals(other: UUID) -> bool */
    if (token_equals(member_name, "equals"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *uuid_type = ast_create_primitive_type(table->arena, TYPE_UUID);
        Type *param_types[1] = {uuid_type};
        DEBUG_VERBOSE("Returning function type for UUID equals method");
        return ast_create_function_type(table->arena, bool_type, param_types, 1);
    }

    /* Not a UUID method */
    return NULL;
}
