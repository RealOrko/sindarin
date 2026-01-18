/* ============================================================================
 * type_checker_expr_call_net.c - Network Type Method Type Checking
 * ============================================================================
 * Type checking for TcpListener, TcpStream, and UdpSocket method access.
 * Returns the function type for the method, or NULL if not a network method.
 * Caller should handle errors for invalid members.
 * ============================================================================ */

#include "type_checker/type_checker_expr_call_net.h"
#include "type_checker/type_checker_expr_call.h"
#include "debug.h"

/* ============================================================================
 * TcpListener Method Type Checking
 * ============================================================================
 * Handles type checking for TcpListener method/property access.
 * TcpListener has:
 * - Properties: port (int)
 * - Methods: accept() -> TcpStream, close() -> void
 * ============================================================================ */

Type *type_check_tcp_listener_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table)
{
    (void)expr; /* Reserved for future use (e.g., error location) */

    /* Only handle TcpListener types */
    if (object_type->kind != TYPE_TCP_LISTENER)
    {
        return NULL;
    }

    /* listener.port -> int */
    if (token_equals(member_name, "port"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        DEBUG_VERBOSE("Returning int type for TcpListener port property");
        return int_type;
    }

    /* listener.accept() -> TcpStream */
    if (token_equals(member_name, "accept"))
    {
        Type *stream_type = ast_create_primitive_type(table->arena, TYPE_TCP_STREAM);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for TcpListener accept method");
        return ast_create_function_type(table->arena, stream_type, param_types, 0);
    }

    /* listener.close() -> void */
    if (token_equals(member_name, "close"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for TcpListener close method");
        return ast_create_function_type(table->arena, void_type, param_types, 0);
    }

    /* Not a TcpListener method/property */
    return NULL;
}

/* ============================================================================
 * TcpStream Method Type Checking
 * ============================================================================
 * Handles type checking for TcpStream method/property access.
 * TcpStream has:
 * - Properties: remoteAddress (str)
 * - Methods: read(maxBytes: int) -> byte[], readAll() -> byte[],
 *            readLine() -> str, write(data: byte[]) -> void,
 *            writeLine(line: str) -> void, close() -> void
 * ============================================================================ */

Type *type_check_tcp_stream_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table)
{
    (void)expr; /* Reserved for future use (e.g., error location) */

    /* Only handle TcpStream types */
    if (object_type->kind != TYPE_TCP_STREAM)
    {
        return NULL;
    }

    /* stream.remoteAddress -> str */
    if (token_equals(member_name, "remoteAddress"))
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        DEBUG_VERBOSE("Returning string type for TcpStream remoteAddress property");
        return string_type;
    }

    /* stream.read(maxBytes: int) -> byte[] */
    if (token_equals(member_name, "read"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *byte_type = ast_create_primitive_type(table->arena, TYPE_BYTE);
        Type *byte_array_type = ast_create_array_type(table->arena, byte_type);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for TcpStream read method");
        return ast_create_function_type(table->arena, byte_array_type, param_types, 1);
    }

    /* stream.readAll() -> byte[] */
    if (token_equals(member_name, "readAll"))
    {
        Type *byte_type = ast_create_primitive_type(table->arena, TYPE_BYTE);
        Type *byte_array_type = ast_create_array_type(table->arena, byte_type);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for TcpStream readAll method");
        return ast_create_function_type(table->arena, byte_array_type, param_types, 0);
    }

    /* stream.readLine() -> str */
    if (token_equals(member_name, "readLine"))
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for TcpStream readLine method");
        return ast_create_function_type(table->arena, string_type, param_types, 0);
    }

    /* stream.write(data: byte[]) -> void */
    if (token_equals(member_name, "write"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *byte_type = ast_create_primitive_type(table->arena, TYPE_BYTE);
        Type *byte_array_type = ast_create_array_type(table->arena, byte_type);
        Type *param_types[1] = {byte_array_type};
        DEBUG_VERBOSE("Returning function type for TcpStream write method");
        return ast_create_function_type(table->arena, void_type, param_types, 1);
    }

    /* stream.writeLine(line: str) -> void */
    if (token_equals(member_name, "writeLine"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for TcpStream writeLine method");
        return ast_create_function_type(table->arena, void_type, param_types, 1);
    }

    /* stream.close() -> void */
    if (token_equals(member_name, "close"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for TcpStream close method");
        return ast_create_function_type(table->arena, void_type, param_types, 0);
    }

    /* Not a TcpStream method/property */
    return NULL;
}

/* ============================================================================
 * UdpSocket Method Type Checking
 * ============================================================================
 * Handles type checking for UdpSocket method/property access.
 * UdpSocket has:
 * - Properties: port (int), lastSender (str)
 * - Methods: sendTo(data: byte[], address: str) -> void,
 *            receiveFrom(maxBytes: int) -> byte[],
 *            close() -> void
 * ============================================================================ */

Type *type_check_udp_socket_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table)
{
    (void)expr; /* Reserved for future use (e.g., error location) */

    /* Only handle UdpSocket types */
    if (object_type->kind != TYPE_UDP_SOCKET)
    {
        return NULL;
    }

    /* socket.port -> int */
    if (token_equals(member_name, "port"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        DEBUG_VERBOSE("Returning int type for UdpSocket port property");
        return int_type;
    }

    /* socket.lastSender -> str */
    if (token_equals(member_name, "lastSender"))
    {
        Type *str_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        DEBUG_VERBOSE("Returning str type for UdpSocket lastSender property");
        return str_type;
    }

    /* socket.sendTo(data: byte[], address: str) -> void */
    if (token_equals(member_name, "sendTo"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *byte_type = ast_create_primitive_type(table->arena, TYPE_BYTE);
        Type *byte_array_type = ast_create_array_type(table->arena, byte_type);
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[2] = {byte_array_type, string_type};
        DEBUG_VERBOSE("Returning function type for UdpSocket sendTo method");
        return ast_create_function_type(table->arena, void_type, param_types, 2);
    }

    /* socket.receiveFrom(maxBytes: int) -> (byte[], str)
     * Note: Multiple return values are handled as returning a str[] with [data, sender]
     * This is a simplification - actual implementation returns data via out params
     * For now, we return byte[] and the sender address is accessible separately */
    if (token_equals(member_name, "receiveFrom"))
    {
        /* Returns byte[] - sender address handled through special codegen */
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *byte_type = ast_create_primitive_type(table->arena, TYPE_BYTE);
        Type *byte_array_type = ast_create_array_type(table->arena, byte_type);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for UdpSocket receiveFrom method");
        return ast_create_function_type(table->arena, byte_array_type, param_types, 1);
    }

    /* socket.close() -> void */
    if (token_equals(member_name, "close"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for UdpSocket close method");
        return ast_create_function_type(table->arena, void_type, param_types, 0);
    }

    /* Not a UdpSocket method/property */
    return NULL;
}

/* ============================================================================
 * Process Property Type Checking
 * ============================================================================
 * Handles type checking for Process property access.
 * Process has:
 * - Properties: exitCode (int), stdout (str), stderr (str)
 * ============================================================================ */

Type *type_check_process_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table)
{
    (void)expr; /* Reserved for future use (e.g., error location) */

    /* Only handle Process types */
    if (object_type->kind != TYPE_PROCESS)
    {
        return NULL;
    }

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
        DEBUG_VERBOSE("Returning str type for Process stdout property");
        return string_type;
    }

    /* process.stderr -> str */
    if (token_equals(member_name, "stderr"))
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        DEBUG_VERBOSE("Returning str type for Process stderr property");
        return string_type;
    }

    /* Not a Process property */
    return NULL;
}
