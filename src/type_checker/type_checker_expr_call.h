#ifndef TYPE_CHECKER_EXPR_CALL_H
#define TYPE_CHECKER_EXPR_CALL_H

#include "ast.h"
#include "symbol_table.h"
#include <stdbool.h>

/* ============================================================================
 * Call Expression Type Checking
 * ============================================================================
 * Type checking for function calls, method calls, static method calls,
 * and built-in functions. This module handles all call-related type checking
 * extracted from type_checker_expr.c.
 *
 * The implementation is split across multiple modules:
 * - type_checker_expr_call_core.c: Main dispatchers and helpers
 * - type_checker_expr_call_array.c: Array method type checking
 * - type_checker_expr_call_string.c: String method type checking
 * - type_checker_expr_call_file.c: TextFile/BinaryFile method type checking
 * - type_checker_expr_call_time.c: Time/Date method type checking
 * - type_checker_expr_call_net.c: TcpListener/TcpStream/UdpSocket type checking
 * - type_checker_expr_call_random.c: Random/UUID/Process method type checking
 * ============================================================================ */

/* ============================================================================
 * Core Functions (type_checker_expr_call_core.c)
 * ============================================================================ */

/* Main call expression type checker
 * Dispatches to appropriate handler based on call type:
 * - Built-in functions (len)
 * - User-defined function calls
 * - Method calls on objects
 * - Static method calls (e.g., TextFile.open)
 */
Type *type_check_call_expression(Expr *expr, SymbolTable *table);

/* Static method type checking for built-in types
 * Handles: TextFile, BinaryFile, Time, Date, Stdin, Stdout, Stderr, Bytes,
 *          Path, Directory, Process, TcpListener, TcpStream, UdpSocket,
 *          Random, UUID, Environment
 */
Type *type_check_static_method_call(Expr *expr, SymbolTable *table);

/* Helper to check if callee matches a built-in function name */
bool is_builtin_name(Expr *callee, const char *name);

/* Helper to compare a token's text against a string */
bool token_equals(Token tok, const char *str);

/* ============================================================================
 * Array Methods (type_checker_expr_call_array.c)
 * ============================================================================ */

/* Type check array instance methods:
 * push, pop, reverse, remove, insert, slice, contains, indexOf, lastIndexOf,
 * first, last, isEmpty, clear, copy, join, sort, filter, map, reduce, forEach,
 * any, all, count, find, findIndex, take, drop, unique, flatten, zip, sum,
 * average, min, max
 */
Type *type_check_array_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

/* ============================================================================
 * String Methods (type_checker_expr_call_string.c)
 * ============================================================================ */

/* Type check string instance methods:
 * length, isEmpty, trim, trimStart, trimEnd, upper, lower, reverse, split,
 * replace, replaceAll, startsWith, endsWith, contains, indexOf, lastIndexOf,
 * substring, charAt, repeat, padStart, padEnd, lines, toInt, toLong, toDouble,
 * toBytes, toHex, toBase64, fromHex, fromBase64
 */
Type *type_check_string_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

/* ============================================================================
 * File Methods (type_checker_expr_call_file.c)
 * ============================================================================ */

/* Type check TextFile instance methods:
 * path, isOpen, isEof, read, readLine, readAll, write, writeLine, flush,
 * close, seek, position, size
 */
Type *type_check_text_file_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

/* Type check BinaryFile instance methods:
 * path, isOpen, isEof, read, readAll, write, flush, close, seek, position, size
 */
Type *type_check_binary_file_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

/* ============================================================================
 * Time/Date Methods (type_checker_expr_call_time.c)
 * ============================================================================ */

/* Type check Time instance methods:
 * millis, seconds, year, month, day, hour, minute, second, weekday, format,
 * toIso, toDate, toTime, add, addSeconds, addMinutes, addHours, addDays,
 * diff, isBefore, isAfter, equals
 */
Type *type_check_time_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

/* Type check Date instance methods:
 * year, month, day, weekday, dayOfYear, epochDays, daysInMonth, isLeapYear,
 * isWeekend, isWeekday, format, toIso, toString, addDays, addWeeks, addMonths,
 * addYears, diffDays, startOfMonth, endOfMonth, startOfYear, endOfYear,
 * isBefore, isAfter, equals, toTime
 */
Type *type_check_date_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

/* ============================================================================
 * Network Methods (type_checker_expr_call_net.c)
 * ============================================================================ */

/* Type check TcpListener instance methods:
 * port, accept, close
 */
Type *type_check_tcp_listener_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

/* Type check TcpStream instance methods:
 * remoteAddress, read, readAll, readLine, write, writeLine, close
 */
Type *type_check_tcp_stream_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

/* Type check UdpSocket instance methods:
 * port, lastSender, sendTo, receiveFrom, close
 */
Type *type_check_udp_socket_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

/* ============================================================================
 * Random/UUID/Process Methods (type_checker_expr_call_random.c)
 * ============================================================================ */

/* Type check Random instance methods:
 * int, long, double, bool, byte, bytes, gaussian, intMany, longMany,
 * doubleMany, boolMany, gaussianMany, choice, shuffle, weightedChoice, sample
 */
Type *type_check_random_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

/* Type check UUID instance methods:
 * toString, toHex, toBase64, toBytes, version, variant, isNil, timestamp,
 * time, equals
 */
Type *type_check_uuid_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

/* Type check Process instance methods:
 * exitCode, stdout, stderr
 */
Type *type_check_process_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

#endif /* TYPE_CHECKER_EXPR_CALL_H */
