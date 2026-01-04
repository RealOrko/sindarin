#ifndef RUNTIME_NET_H
#define RUNTIME_NET_H

#include <stdbool.h>
#include "runtime_arena.h"

/* ============================================================================
 * Network I/O Types
 * ============================================================================
 * Sindarin provides built-in types for network communication:
 * - TcpListener: TCP socket that listens for incoming connections
 * - TcpStream: TCP connection for bidirectional communication
 * - UdpSocket: UDP socket for connectionless datagram communication
 *
 * All network types integrate with Sindarin's arena-based memory management.
 * Network handles are automatically closed when their arena is destroyed.
 * ============================================================================ */

/* ============================================================================
 * TCP Listener
 * ============================================================================
 * A TCP socket that listens for incoming connections on a specified address.
 * Use TcpListener.bind() to create and TcpListener.accept() to accept clients.
 * ============================================================================ */

typedef struct RtTcpListener {
    int fd;                     /* Socket file descriptor */
    int port;                   /* Bound port number */
} RtTcpListener;

/* ============================================================================
 * TCP Stream
 * ============================================================================
 * A TCP connection for bidirectional communication. Can be created either by
 * accepting a connection on a TcpListener or by connecting to a remote address.
 * ============================================================================ */

typedef struct RtTcpStream {
    int fd;                     /* Socket file descriptor */
    char *remote_address;       /* Remote peer address (host:port format) */
} RtTcpStream;

/* ============================================================================
 * UDP Socket
 * ============================================================================
 * A UDP socket for connectionless datagram communication. UDP sockets can
 * send and receive datagrams to/from any address without establishing a
 * connection first.
 * ============================================================================ */

typedef struct RtUdpSocket {
    int fd;                     /* Socket file descriptor */
    int port;                   /* Bound port number */
} RtUdpSocket;

/* ============================================================================
 * TcpListener Static Methods
 * ============================================================================
 * Static methods for creating TCP listeners.
 * ============================================================================ */

/* Create a TCP listener bound to the specified address.
 * Address format: "host:port" or ":port" for all interfaces.
 * Use ":0" to let the OS assign a port.
 * Panics on error (address in use, permission denied, etc.)
 */
RtTcpListener *rt_tcp_listener_bind(RtArena *arena, const char *address);

/* ============================================================================
 * TcpListener Instance Methods
 * ============================================================================
 * Instance methods for accepting connections and closing listeners.
 * ============================================================================ */

/* Accept an incoming connection.
 * Blocks until a client connects.
 * Returns a new TcpStream for the accepted connection.
 * Panics on error.
 */
RtTcpStream *rt_tcp_listener_accept(RtArena *arena, RtTcpListener *listener);

/* Close the TCP listener.
 * After closing, the listener cannot accept new connections.
 * Safe to call multiple times.
 */
void rt_tcp_listener_close(RtTcpListener *listener);

/* ============================================================================
 * TcpListener Properties
 * ============================================================================
 * Property accessors for TCP listeners.
 * ============================================================================ */

/* Get the bound port number.
 * Useful when binding to port 0 to find the OS-assigned port.
 */
static inline int rt_tcp_listener_get_port(RtTcpListener *listener) {
    return listener->port;
}

/* ============================================================================
 * TcpStream Static Methods
 * ============================================================================
 * Static methods for creating TCP streams.
 * ============================================================================ */

/* Connect to a remote address.
 * Address format: "host:port" (supports IPv4, IPv6, and hostnames).
 * Panics on error (connection refused, DNS failure, timeout, etc.)
 */
RtTcpStream *rt_tcp_stream_connect(RtArena *arena, const char *address);

/* ============================================================================
 * TcpStream Instance Reading Methods
 * ============================================================================
 * Instance methods for reading data from a TCP stream.
 * ============================================================================ */

/* Read up to max_bytes from the stream.
 * Returns a byte array with the data read (may be fewer than max_bytes).
 * Returns empty array on EOF (connection closed).
 * Panics on error.
 */
unsigned char *rt_tcp_stream_read(RtArena *arena, RtTcpStream *stream, long max_bytes);

/* Read all data until the connection closes.
 * Returns a byte array with all received data.
 * Panics on error.
 */
unsigned char *rt_tcp_stream_read_all(RtArena *arena, RtTcpStream *stream);

/* Read a line of text from the stream.
 * Returns string up to and including newline, or up to EOF.
 * Returns empty string on EOF.
 * Panics on error.
 */
char *rt_tcp_stream_read_line(RtArena *arena, RtTcpStream *stream);

/* ============================================================================
 * TcpStream Instance Writing Methods
 * ============================================================================
 * Instance methods for writing data to a TCP stream.
 * ============================================================================ */

/* Write data to the stream.
 * Returns the number of bytes written.
 * Panics on error (connection reset, broken pipe, etc.)
 */
long rt_tcp_stream_write(RtTcpStream *stream, unsigned char *data);

/* Write a string followed by newline to the stream.
 * Panics on error.
 */
void rt_tcp_stream_write_line(RtTcpStream *stream, const char *text);

/* ============================================================================
 * TcpStream Instance Control Methods
 * ============================================================================
 * Instance methods for controlling the stream.
 * ============================================================================ */

/* Close the TCP stream.
 * After closing, the stream cannot be read from or written to.
 * Safe to call multiple times.
 */
void rt_tcp_stream_close(RtTcpStream *stream);

/* ============================================================================
 * TcpStream Properties
 * ============================================================================
 * Property accessors for TCP streams.
 * ============================================================================ */

/* Get the remote peer address.
 * Returns address in "host:port" format.
 */
static inline const char *rt_tcp_stream_get_remote_address(RtTcpStream *stream) {
    return stream->remote_address;
}

/* ============================================================================
 * TcpStream Promotion
 * ============================================================================
 * Functions for promoting TCP streams between arenas.
 * ============================================================================ */

/* Promote a TCP stream to a destination arena.
 * The stream is copied to the destination arena.
 * The source stream should not be used after promotion.
 */
RtTcpStream *rt_tcp_stream_promote(RtArena *dest, RtArena *src_arena, RtTcpStream *src);

/* ============================================================================
 * TcpListener Promotion
 * ============================================================================
 * Functions for promoting TCP listeners between arenas.
 * ============================================================================ */

/* Promote a TCP listener to a destination arena.
 * The listener is copied to the destination arena.
 * The source listener should not be used after promotion.
 */
RtTcpListener *rt_tcp_listener_promote(RtArena *dest, RtArena *src_arena, RtTcpListener *src);

/* ============================================================================
 * UdpSocket Static Methods
 * ============================================================================
 * Static methods for creating UDP sockets.
 * ============================================================================ */

/* Create a UDP socket bound to the specified address.
 * Address format: "host:port" or ":port" for all interfaces.
 * Use ":0" to let the OS assign a port.
 * Panics on error.
 */
RtUdpSocket *rt_udp_socket_bind(RtArena *arena, const char *address);

/* ============================================================================
 * UdpSocket Instance Methods
 * ============================================================================
 * Instance methods for sending and receiving datagrams.
 * ============================================================================ */

/* Send a datagram to the specified address.
 * Address format: "host:port".
 * Returns the number of bytes sent.
 * Panics on error.
 */
long rt_udp_socket_send_to(RtUdpSocket *socket, unsigned char *data, const char *address);

/* Receive a datagram (up to max_bytes).
 * Sets *sender to the sender's address in "host:port" format.
 * Returns a byte array with the received data.
 * Blocks until a datagram is received.
 * Panics on error.
 */
unsigned char *rt_udp_socket_receive_from(RtArena *arena, RtUdpSocket *socket, long max_bytes, char **sender);

/* Close the UDP socket.
 * After closing, the socket cannot send or receive datagrams.
 * Safe to call multiple times.
 */
void rt_udp_socket_close(RtUdpSocket *socket);

/* ============================================================================
 * UdpSocket Properties
 * ============================================================================
 * Property accessors for UDP sockets.
 * ============================================================================ */

/* Get the bound port number.
 * Useful when binding to port 0 to find the OS-assigned port.
 */
static inline int rt_udp_socket_get_port(RtUdpSocket *socket) {
    return socket->port;
}

/* ============================================================================
 * UdpSocket Promotion
 * ============================================================================
 * Functions for promoting UDP sockets between arenas.
 * ============================================================================ */

/* Promote a UDP socket to a destination arena.
 * The socket is copied to the destination arena.
 * The source socket should not be used after promotion.
 */
RtUdpSocket *rt_udp_socket_promote(RtArena *dest, RtArena *src_arena, RtUdpSocket *src);

#endif /* RUNTIME_NET_H */
