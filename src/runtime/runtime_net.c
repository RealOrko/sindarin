#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "runtime_net.h"
#include "runtime_array.h"

/* ============================================================================
 * Network I/O Implementation
 * ============================================================================
 *
 * This module provides network communication for the Sindarin runtime.
 * It supports TCP (TcpListener, TcpStream) and UDP (UdpSocket) operations.
 * All network handles integrate with arena-based memory management.
 * ============================================================================ */

/* ============================================================================
 * Address Parsing Helper
 * ============================================================================
 *
 * Parses address strings in the following formats:
 * - IPv4 with port: "127.0.0.1:8080"
 * - Port only: ":8080" (binds to all interfaces)
 * - Hostname with port: "localhost:8080"
 * - IPv6 with port: "[::1]:8080"
 *
 * Returns the parsed host (may be empty for INADDR_ANY) and port.
 * Panics with a clear error message on invalid format.
 * ============================================================================ */

/* Structure to hold parsed address components */
typedef struct {
    char host[256];     /* Hostname or IP address (empty for INADDR_ANY) */
    int port;           /* Port number */
    int is_ipv6;        /* Non-zero if IPv6 address */
} ParsedAddress;

/* Parse an address string in host:port format.
 * Supports IPv4, IPv6 (with brackets), hostname, and port-only formats.
 * Panics on invalid format.
 */
static ParsedAddress parse_address(const char *address)
{
    ParsedAddress result;
    memset(&result, 0, sizeof(result));

    if (address == NULL) {
        fprintf(stderr, "Network error: address is NULL\n");
        exit(1);
    }

    size_t len = strlen(address);
    if (len == 0) {
        fprintf(stderr, "Network error: address is empty\n");
        exit(1);
    }

    const char *host_start;
    const char *host_end;
    const char *port_str;

    /* Check for IPv6 format: [host]:port */
    if (address[0] == '[') {
        result.is_ipv6 = 1;
        host_start = address + 1;

        /* Find closing bracket */
        const char *bracket = strchr(address, ']');
        if (bracket == NULL) {
            fprintf(stderr, "Network error: invalid IPv6 address format, missing closing bracket: '%s'\n", address);
            exit(1);
        }
        host_end = bracket;

        /* Check for :port after bracket */
        if (bracket[1] == '\0') {
            fprintf(stderr, "Network error: missing port after IPv6 address: '%s'\n", address);
            exit(1);
        }
        if (bracket[1] != ':') {
            fprintf(stderr, "Network error: expected ':' after IPv6 address bracket: '%s'\n", address);
            exit(1);
        }
        port_str = bracket + 2;
    } else {
        result.is_ipv6 = 0;

        /* Find the last colon (port separator) */
        const char *colon = strrchr(address, ':');
        if (colon == NULL) {
            fprintf(stderr, "Network error: missing port in address (expected host:port format): '%s'\n", address);
            exit(1);
        }

        host_start = address;
        host_end = colon;
        port_str = colon + 1;
    }

    /* Extract host */
    size_t host_len = (size_t)(host_end - host_start);
    if (host_len >= sizeof(result.host)) {
        fprintf(stderr, "Network error: hostname too long: '%s'\n", address);
        exit(1);
    }
    if (host_len > 0) {
        memcpy(result.host, host_start, host_len);
        result.host[host_len] = '\0';
    } else {
        /* Empty host means INADDR_ANY */
        result.host[0] = '\0';
    }

    /* Parse port */
    if (port_str[0] == '\0') {
        fprintf(stderr, "Network error: empty port number in address: '%s'\n", address);
        exit(1);
    }

    /* Validate port string contains only digits */
    for (const char *p = port_str; *p != '\0'; p++) {
        if (!isdigit((unsigned char)*p)) {
            fprintf(stderr, "Network error: invalid port number '%s' in address: '%s'\n", port_str, address);
            exit(1);
        }
    }

    long port = strtol(port_str, NULL, 10);
    if (port < 0 || port > 65535) {
        fprintf(stderr, "Network error: port number out of range (0-65535): %ld in address: '%s'\n", port, address);
        exit(1);
    }
    result.port = (int)port;

    return result;
}

/* ============================================================================
 * Socket Creation Helpers
 * ============================================================================ */

/* Resolve hostname to sockaddr.
 * Returns 0 on success, -1 on failure.
 * Fills in addr and addrlen for the resolved address.
 */
static int resolve_address(const char *host, int port, int is_ipv6,
                           struct sockaddr_storage *addr, socklen_t *addrlen)
{
    memset(addr, 0, sizeof(*addr));

    /* Handle empty host as INADDR_ANY / IN6ADDR_ANY */
    if (host[0] == '\0') {
        if (is_ipv6) {
            struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr;
            addr6->sin6_family = AF_INET6;
            addr6->sin6_port = htons((uint16_t)port);
            addr6->sin6_addr = in6addr_any;
            *addrlen = sizeof(struct sockaddr_in6);
        } else {
            struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;
            addr4->sin_family = AF_INET;
            addr4->sin_port = htons((uint16_t)port);
            addr4->sin_addr.s_addr = INADDR_ANY;
            *addrlen = sizeof(struct sockaddr_in);
        }
        return 0;
    }

    /* Try to parse as numeric address first */
    if (is_ipv6) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr;
        if (inet_pton(AF_INET6, host, &addr6->sin6_addr) == 1) {
            addr6->sin6_family = AF_INET6;
            addr6->sin6_port = htons((uint16_t)port);
            *addrlen = sizeof(struct sockaddr_in6);
            return 0;
        }
    } else {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;
        if (inet_pton(AF_INET, host, &addr4->sin_addr) == 1) {
            addr4->sin_family = AF_INET;
            addr4->sin_port = htons((uint16_t)port);
            *addrlen = sizeof(struct sockaddr_in);
            return 0;
        }
    }

    /* Need to resolve hostname via DNS */
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = is_ipv6 ? AF_INET6 : AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);

    int err = getaddrinfo(host, port_str, &hints, &res);
    if (err != 0) {
        /* Try with AF_UNSPEC if specific family failed */
        hints.ai_family = AF_UNSPEC;
        err = getaddrinfo(host, port_str, &hints, &res);
        if (err != 0) {
            return -1;
        }
    }

    memcpy(addr, res->ai_addr, res->ai_addrlen);
    *addrlen = res->ai_addrlen;
    freeaddrinfo(res);
    return 0;
}

/* Format a sockaddr as a "host:port" string.
 * Returns a string allocated in the arena.
 */
static char *format_address(RtArena *arena, struct sockaddr_storage *addr, socklen_t addrlen)
{
    char host[INET6_ADDRSTRLEN];
    int port;

    if (addr->ss_family == AF_INET) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;
        inet_ntop(AF_INET, &addr4->sin_addr, host, sizeof(host));
        port = ntohs(addr4->sin_port);
    } else if (addr->ss_family == AF_INET6) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr;
        inet_ntop(AF_INET6, &addr6->sin6_addr, host, sizeof(host));
        port = ntohs(addr6->sin6_port);
    } else {
        return rt_arena_strdup(arena, "<unknown>");
    }

    (void)addrlen;  /* Unused but kept for API consistency */

    /* Format as host:port or [host]:port for IPv6 */
    size_t len = strlen(host) + 16;  /* Extra space for port and brackets */
    char *result = rt_arena_alloc(arena, len);
    if (addr->ss_family == AF_INET6) {
        snprintf(result, len, "[%s]:%d", host, port);
    } else {
        snprintf(result, len, "%s:%d", host, port);
    }
    return result;
}

/* ============================================================================
 * TcpListener Implementation
 * ============================================================================ */

RtTcpListener *rt_tcp_listener_bind(RtArena *arena, const char *address)
{
    if (arena == NULL) {
        fprintf(stderr, "TcpListener.bind: arena is NULL\n");
        exit(1);
    }

    ParsedAddress parsed = parse_address(address);

    /* Create socket */
    int family = parsed.is_ipv6 ? AF_INET6 : AF_INET;
    int fd = socket(family, SOCK_STREAM, 0);
    if (fd < 0) {
        fprintf(stderr, "TcpListener.bind: failed to create socket: %s\n", strerror(errno));
        exit(1);
    }

    /* Set SO_REUSEADDR to allow quick restart */
    int optval = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        close(fd);
        fprintf(stderr, "TcpListener.bind: failed to set SO_REUSEADDR: %s\n", strerror(errno));
        exit(1);
    }

    /* Resolve and bind address */
    struct sockaddr_storage addr;
    socklen_t addrlen;
    if (resolve_address(parsed.host, parsed.port, parsed.is_ipv6, &addr, &addrlen) < 0) {
        close(fd);
        fprintf(stderr, "TcpListener.bind: failed to resolve address '%s': %s\n", address, strerror(errno));
        exit(1);
    }

    if (bind(fd, (struct sockaddr *)&addr, addrlen) < 0) {
        close(fd);
        fprintf(stderr, "TcpListener.bind: failed to bind to '%s': %s\n", address, strerror(errno));
        exit(1);
    }

    /* Start listening */
    if (listen(fd, SOMAXCONN) < 0) {
        close(fd);
        fprintf(stderr, "TcpListener.bind: failed to listen on '%s': %s\n", address, strerror(errno));
        exit(1);
    }

    /* Get actual bound port (useful when port was 0) */
    struct sockaddr_storage bound_addr;
    socklen_t bound_len = sizeof(bound_addr);
    if (getsockname(fd, (struct sockaddr *)&bound_addr, &bound_len) < 0) {
        close(fd);
        fprintf(stderr, "TcpListener.bind: failed to get socket name: %s\n", strerror(errno));
        exit(1);
    }

    int actual_port;
    if (bound_addr.ss_family == AF_INET) {
        actual_port = ntohs(((struct sockaddr_in *)&bound_addr)->sin_port);
    } else {
        actual_port = ntohs(((struct sockaddr_in6 *)&bound_addr)->sin6_port);
    }

    /* Allocate and return listener */
    RtTcpListener *listener = rt_arena_alloc(arena, sizeof(RtTcpListener));
    if (listener == NULL) {
        close(fd);
        fprintf(stderr, "TcpListener.bind: memory allocation failed\n");
        exit(1);
    }

    listener->fd = fd;
    listener->port = actual_port;
    return listener;
}

RtTcpStream *rt_tcp_listener_accept(RtArena *arena, RtTcpListener *listener)
{
    if (arena == NULL) {
        fprintf(stderr, "TcpListener.accept: arena is NULL\n");
        exit(1);
    }
    if (listener == NULL) {
        fprintf(stderr, "TcpListener.accept: listener is NULL\n");
        exit(1);
    }
    if (listener->fd < 0) {
        fprintf(stderr, "TcpListener.accept: listener is closed\n");
        exit(1);
    }

    struct sockaddr_storage client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(listener->fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) {
        fprintf(stderr, "TcpListener.accept: accept failed: %s\n", strerror(errno));
        exit(1);
    }

    /* Allocate stream */
    RtTcpStream *stream = rt_arena_alloc(arena, sizeof(RtTcpStream));
    if (stream == NULL) {
        close(client_fd);
        fprintf(stderr, "TcpListener.accept: memory allocation failed\n");
        exit(1);
    }

    stream->fd = client_fd;
    stream->remote_address = format_address(arena, &client_addr, client_len);
    return stream;
}

void rt_tcp_listener_close(RtTcpListener *listener)
{
    if (listener != NULL && listener->fd >= 0) {
        close(listener->fd);
        listener->fd = -1;
    }
}

/* ============================================================================
 * TcpStream Implementation
 * ============================================================================ */

RtTcpStream *rt_tcp_stream_connect(RtArena *arena, const char *address)
{
    if (arena == NULL) {
        fprintf(stderr, "TcpStream.connect: arena is NULL\n");
        exit(1);
    }

    ParsedAddress parsed = parse_address(address);

    /* Resolve address */
    struct sockaddr_storage addr;
    socklen_t addrlen;
    if (resolve_address(parsed.host, parsed.port, parsed.is_ipv6, &addr, &addrlen) < 0) {
        fprintf(stderr, "TcpStream.connect: failed to resolve address '%s': %s\n", address, strerror(errno));
        exit(1);
    }

    /* Create socket */
    int family = addr.ss_family;
    int fd = socket(family, SOCK_STREAM, 0);
    if (fd < 0) {
        fprintf(stderr, "TcpStream.connect: failed to create socket: %s\n", strerror(errno));
        exit(1);
    }

    /* Connect */
    if (connect(fd, (struct sockaddr *)&addr, addrlen) < 0) {
        close(fd);
        fprintf(stderr, "TcpStream.connect: failed to connect to '%s': %s\n", address, strerror(errno));
        exit(1);
    }

    /* Allocate stream */
    RtTcpStream *stream = rt_arena_alloc(arena, sizeof(RtTcpStream));
    if (stream == NULL) {
        close(fd);
        fprintf(stderr, "TcpStream.connect: memory allocation failed\n");
        exit(1);
    }

    stream->fd = fd;
    stream->remote_address = format_address(arena, &addr, addrlen);
    return stream;
}

unsigned char *rt_tcp_stream_read(RtArena *arena, RtTcpStream *stream, long max_bytes)
{
    if (arena == NULL) {
        fprintf(stderr, "TcpStream.read: arena is NULL\n");
        exit(1);
    }
    if (stream == NULL) {
        fprintf(stderr, "TcpStream.read: stream is NULL\n");
        exit(1);
    }
    if (stream->fd < 0) {
        fprintf(stderr, "TcpStream.read: stream is closed\n");
        exit(1);
    }
    if (max_bytes <= 0) {
        fprintf(stderr, "TcpStream.read: max_bytes must be positive: %ld\n", max_bytes);
        exit(1);
    }

    /* Allocate buffer with metadata */
    unsigned char *buffer = rt_array_create_byte_uninit(arena, (size_t)max_bytes);
    if (buffer == NULL) {
        fprintf(stderr, "TcpStream.read: memory allocation failed\n");
        exit(1);
    }

    /* Read data */
    ssize_t bytes_read = recv(stream->fd, buffer, (size_t)max_bytes, 0);
    if (bytes_read < 0) {
        fprintf(stderr, "TcpStream.read: recv failed: %s\n", strerror(errno));
        exit(1);
    }

    /* Adjust array size to actual bytes read */
    if (bytes_read == 0) {
        /* Return empty array on EOF */
        return rt_array_create_byte(arena, 0, NULL);
    }

    /* Create array with actual data */
    return rt_array_create_byte(arena, (size_t)bytes_read, buffer);
}

unsigned char *rt_tcp_stream_read_all(RtArena *arena, RtTcpStream *stream)
{
    if (arena == NULL) {
        fprintf(stderr, "TcpStream.readAll: arena is NULL\n");
        exit(1);
    }
    if (stream == NULL) {
        fprintf(stderr, "TcpStream.readAll: stream is NULL\n");
        exit(1);
    }
    if (stream->fd < 0) {
        fprintf(stderr, "TcpStream.readAll: stream is closed\n");
        exit(1);
    }

    /* Read all data in chunks */
    size_t capacity = 4096;
    size_t length = 0;
    unsigned char *buffer = rt_arena_alloc(arena, capacity);
    if (buffer == NULL) {
        fprintf(stderr, "TcpStream.readAll: memory allocation failed\n");
        exit(1);
    }

    while (1) {
        /* Grow buffer if needed */
        if (length >= capacity) {
            size_t new_capacity = capacity * 2;
            unsigned char *new_buffer = rt_arena_alloc(arena, new_capacity);
            if (new_buffer == NULL) {
                fprintf(stderr, "TcpStream.readAll: memory allocation failed\n");
                exit(1);
            }
            memcpy(new_buffer, buffer, length);
            buffer = new_buffer;
            capacity = new_capacity;
        }

        ssize_t bytes_read = recv(stream->fd, buffer + length, capacity - length, 0);
        if (bytes_read < 0) {
            fprintf(stderr, "TcpStream.readAll: recv failed: %s\n", strerror(errno));
            exit(1);
        }
        if (bytes_read == 0) {
            break;  /* EOF */
        }
        length += (size_t)bytes_read;
    }

    return rt_array_create_byte(arena, length, buffer);
}

char *rt_tcp_stream_read_line(RtArena *arena, RtTcpStream *stream)
{
    if (arena == NULL) {
        fprintf(stderr, "TcpStream.readLine: arena is NULL\n");
        exit(1);
    }
    if (stream == NULL) {
        fprintf(stderr, "TcpStream.readLine: stream is NULL\n");
        exit(1);
    }
    if (stream->fd < 0) {
        fprintf(stderr, "TcpStream.readLine: stream is closed\n");
        exit(1);
    }

    /* Read character by character until newline */
    size_t capacity = 256;
    size_t length = 0;
    char *buffer = rt_arena_alloc(arena, capacity);
    if (buffer == NULL) {
        fprintf(stderr, "TcpStream.readLine: memory allocation failed\n");
        exit(1);
    }

    while (1) {
        /* Grow buffer if needed */
        if (length + 1 >= capacity) {
            size_t new_capacity = capacity * 2;
            char *new_buffer = rt_arena_alloc(arena, new_capacity);
            if (new_buffer == NULL) {
                fprintf(stderr, "TcpStream.readLine: memory allocation failed\n");
                exit(1);
            }
            memcpy(new_buffer, buffer, length);
            buffer = new_buffer;
            capacity = new_capacity;
        }

        char c;
        ssize_t bytes_read = recv(stream->fd, &c, 1, 0);
        if (bytes_read < 0) {
            fprintf(stderr, "TcpStream.readLine: recv failed: %s\n", strerror(errno));
            exit(1);
        }
        if (bytes_read == 0) {
            /* EOF before newline - panic */
            fprintf(stderr, "TcpStream.readLine: connection closed before newline\n");
            exit(1);
        }

        if (c == '\n') {
            break;
        }

        buffer[length++] = c;
    }

    /* Strip trailing \r if present (handles \r\n line endings) */
    if (length > 0 && buffer[length - 1] == '\r') {
        length--;
    }

    buffer[length] = '\0';
    return buffer;
}

long rt_tcp_stream_write(RtTcpStream *stream, unsigned char *data)
{
    if (stream == NULL) {
        fprintf(stderr, "TcpStream.write: stream is NULL\n");
        exit(1);
    }
    if (stream->fd < 0) {
        fprintf(stderr, "TcpStream.write: stream is closed\n");
        exit(1);
    }
    if (data == NULL) {
        return 0;
    }

    size_t len = rt_array_length(data);
    if (len == 0) {
        return 0;
    }

    /* Loop to handle partial writes */
    size_t total_written = 0;
    while (total_written < len) {
        ssize_t bytes_written = send(stream->fd, data + total_written, len - total_written, 0);
        if (bytes_written < 0) {
            fprintf(stderr, "TcpStream.write: send failed: %s\n", strerror(errno));
            exit(1);
        }
        total_written += (size_t)bytes_written;
    }

    return (long)total_written;
}

void rt_tcp_stream_write_line(RtTcpStream *stream, const char *text)
{
    if (stream == NULL) {
        fprintf(stderr, "TcpStream.writeLine: stream is NULL\n");
        exit(1);
    }
    if (stream->fd < 0) {
        fprintf(stderr, "TcpStream.writeLine: stream is closed\n");
        exit(1);
    }
    if (text == NULL) {
        text = "";
    }

    /* Use MSG_NOSIGNAL to avoid SIGPIPE on broken connections */
#ifdef MSG_NOSIGNAL
    int flags = MSG_NOSIGNAL;
#else
    int flags = 0;
#endif

    /* Write the text content with loop for partial writes */
    size_t len = strlen(text);
    size_t total_written = 0;
    while (total_written < len) {
        ssize_t written = send(stream->fd, text + total_written, len - total_written, flags);
        if (written < 0) {
            fprintf(stderr, "TcpStream.writeLine: send failed: %s\n", strerror(errno));
            exit(1);
        }
        total_written += (size_t)written;
    }

    /* Send newline - loop in case of partial write (unlikely for 1 byte but correct) */
    ssize_t written = send(stream->fd, "\n", 1, flags);
    if (written < 0) {
        fprintf(stderr, "TcpStream.writeLine: send newline failed: %s\n", strerror(errno));
        exit(1);
    }
}

void rt_tcp_stream_close(RtTcpStream *stream)
{
    if (stream != NULL && stream->fd >= 0) {
        close(stream->fd);
        stream->fd = -1;
    }
}

/* ============================================================================
 * TcpStream Promotion
 * ============================================================================ */

RtTcpStream *rt_tcp_stream_promote(RtArena *dest, RtArena *src_arena, RtTcpStream *src)
{
    (void)src_arena;  /* Unused - we don't need to clean up from source */

    if (dest == NULL || src == NULL) {
        return NULL;
    }

    RtTcpStream *promoted = rt_arena_alloc(dest, sizeof(RtTcpStream));
    if (promoted == NULL) {
        return NULL;
    }

    promoted->fd = src->fd;
    promoted->remote_address = rt_arena_strdup(dest, src->remote_address);

    /* Mark source as invalid to prevent double-close */
    src->fd = -1;
    return promoted;
}

RtTcpListener *rt_tcp_listener_promote(RtArena *dest, RtArena *src_arena, RtTcpListener *src)
{
    (void)src_arena;  /* Unused */

    if (dest == NULL || src == NULL) {
        return NULL;
    }

    RtTcpListener *promoted = rt_arena_alloc(dest, sizeof(RtTcpListener));
    if (promoted == NULL) {
        return NULL;
    }

    promoted->fd = src->fd;
    promoted->port = src->port;

    /* Mark source as invalid to prevent double-close */
    src->fd = -1;
    return promoted;
}

/* ============================================================================
 * UdpSocket Implementation
 * ============================================================================ */

RtUdpSocket *rt_udp_socket_bind(RtArena *arena, const char *address)
{
    if (arena == NULL) {
        fprintf(stderr, "UdpSocket.bind: arena is NULL\n");
        exit(1);
    }

    ParsedAddress parsed = parse_address(address);

    /* Create socket */
    int family = parsed.is_ipv6 ? AF_INET6 : AF_INET;
    int fd = socket(family, SOCK_DGRAM, 0);
    if (fd < 0) {
        fprintf(stderr, "UdpSocket.bind: failed to create socket: %s\n", strerror(errno));
        exit(1);
    }

    /* Set SO_REUSEADDR */
    int optval = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        close(fd);
        fprintf(stderr, "UdpSocket.bind: failed to set SO_REUSEADDR: %s\n", strerror(errno));
        exit(1);
    }

    /* Resolve and bind address */
    struct sockaddr_storage addr;
    socklen_t addrlen;
    if (resolve_address(parsed.host, parsed.port, parsed.is_ipv6, &addr, &addrlen) < 0) {
        close(fd);
        fprintf(stderr, "UdpSocket.bind: failed to resolve address '%s': %s\n", address, strerror(errno));
        exit(1);
    }

    if (bind(fd, (struct sockaddr *)&addr, addrlen) < 0) {
        close(fd);
        fprintf(stderr, "UdpSocket.bind: failed to bind to '%s': %s\n", address, strerror(errno));
        exit(1);
    }

    /* Get actual bound port */
    struct sockaddr_storage bound_addr;
    socklen_t bound_len = sizeof(bound_addr);
    if (getsockname(fd, (struct sockaddr *)&bound_addr, &bound_len) < 0) {
        close(fd);
        fprintf(stderr, "UdpSocket.bind: failed to get socket name: %s\n", strerror(errno));
        exit(1);
    }

    int actual_port;
    if (bound_addr.ss_family == AF_INET) {
        actual_port = ntohs(((struct sockaddr_in *)&bound_addr)->sin_port);
    } else {
        actual_port = ntohs(((struct sockaddr_in6 *)&bound_addr)->sin6_port);
    }

    /* Allocate and return socket */
    RtUdpSocket *sock = rt_arena_alloc(arena, sizeof(RtUdpSocket));
    if (sock == NULL) {
        close(fd);
        fprintf(stderr, "UdpSocket.bind: memory allocation failed\n");
        exit(1);
    }

    sock->fd = fd;
    sock->port = actual_port;
    sock->last_sender = NULL;
    return sock;
}

long rt_udp_socket_send_to(RtUdpSocket *socket, unsigned char *data, const char *address)
{
    if (socket == NULL) {
        fprintf(stderr, "UdpSocket.sendTo: socket is NULL\n");
        exit(1);
    }
    if (socket->fd < 0) {
        fprintf(stderr, "UdpSocket.sendTo: socket is closed\n");
        exit(1);
    }
    if (data == NULL) {
        return 0;
    }

    ParsedAddress parsed = parse_address(address);

    /* Resolve destination address */
    struct sockaddr_storage addr;
    socklen_t addrlen;
    if (resolve_address(parsed.host, parsed.port, parsed.is_ipv6, &addr, &addrlen) < 0) {
        fprintf(stderr, "UdpSocket.sendTo: failed to resolve address '%s': %s\n", address, strerror(errno));
        exit(1);
    }

    size_t len = rt_array_length(data);
    ssize_t bytes_sent = sendto(socket->fd, data, len, 0, (struct sockaddr *)&addr, addrlen);
    if (bytes_sent < 0) {
        fprintf(stderr, "UdpSocket.sendTo: sendto failed: %s\n", strerror(errno));
        exit(1);
    }

    return (long)bytes_sent;
}

unsigned char *rt_udp_socket_receive_from(RtArena *arena, RtUdpSocket *socket, long max_bytes, char **sender)
{
    if (arena == NULL) {
        fprintf(stderr, "UdpSocket.receiveFrom: arena is NULL\n");
        exit(1);
    }
    if (socket == NULL) {
        fprintf(stderr, "UdpSocket.receiveFrom: socket is NULL\n");
        exit(1);
    }
    if (socket->fd < 0) {
        fprintf(stderr, "UdpSocket.receiveFrom: socket is closed\n");
        exit(1);
    }
    if (max_bytes <= 0) {
        fprintf(stderr, "UdpSocket.receiveFrom: max_bytes must be positive: %ld\n", max_bytes);
        exit(1);
    }
    /* Allocate receive buffer */
    unsigned char *buffer = rt_arena_alloc(arena, (size_t)max_bytes);
    if (buffer == NULL) {
        fprintf(stderr, "UdpSocket.receiveFrom: memory allocation failed\n");
        exit(1);
    }

    /* Receive datagram */
    struct sockaddr_storage src_addr;
    socklen_t src_len = sizeof(src_addr);
    ssize_t bytes_received = recvfrom(socket->fd, buffer, (size_t)max_bytes, 0,
                                       (struct sockaddr *)&src_addr, &src_len);
    if (bytes_received < 0) {
        fprintf(stderr, "UdpSocket.receiveFrom: recvfrom failed: %s\n", strerror(errno));
        exit(1);
    }

    /* Format sender address - always store in socket for lastSender property */
    char *sender_addr = format_address(arena, &src_addr, src_len);
    socket->last_sender = sender_addr;

    /* Also return via out parameter if provided */
    if (sender != NULL) {
        *sender = sender_addr;
    }

    /* Create byte array result */
    return rt_array_create_byte(arena, (size_t)bytes_received, buffer);
}

void rt_udp_socket_close(RtUdpSocket *socket)
{
    if (socket != NULL && socket->fd >= 0) {
        close(socket->fd);
        socket->fd = -1;
    }
}

RtUdpSocket *rt_udp_socket_promote(RtArena *dest, RtArena *src_arena, RtUdpSocket *src)
{
    (void)src_arena;  /* Unused */

    if (dest == NULL || src == NULL) {
        return NULL;
    }

    RtUdpSocket *promoted = rt_arena_alloc(dest, sizeof(RtUdpSocket));
    if (promoted == NULL) {
        return NULL;
    }

    promoted->fd = src->fd;
    promoted->port = src->port;
    /* Copy last_sender if present */
    if (src->last_sender != NULL) {
        size_t len = strlen(src->last_sender) + 1;
        promoted->last_sender = rt_arena_alloc(dest, len);
        if (promoted->last_sender != NULL) {
            memcpy(promoted->last_sender, src->last_sender, len);
        }
    } else {
        promoted->last_sender = NULL;
    }

    /* Mark source as invalid to prevent double-close */
    src->fd = -1;
    return promoted;
}
