#ifndef TYPE_CHECKER_EXPR_CALL_NET_H
#define TYPE_CHECKER_EXPR_CALL_NET_H

#include "ast.h"
#include "symbol_table.h"

/* ============================================================================
 * Network Type Method Type Checking
 * ============================================================================
 * Type checking for TcpListener, TcpStream, and UdpSocket method access.
 * Returns the function type for the method, or NULL if not a network method.
 * Caller should handle errors for invalid members.
 * ============================================================================ */

/* Type check TcpListener methods
 * Handles: port, accept, close
 */
Type *type_check_tcp_listener_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

/* Type check TcpStream methods
 * Handles: remoteAddress, read, readAll, readLine, write, writeLine, close
 */
Type *type_check_tcp_stream_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

/* Type check UdpSocket methods
 * Handles: port, lastSender, sendTo, receiveFrom, close
 */
Type *type_check_udp_socket_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

#endif /* TYPE_CHECKER_EXPR_CALL_NET_H */
