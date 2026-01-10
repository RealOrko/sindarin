// tests/unit/runtime/runtime_net_tests.c
// Tests for the runtime network I/O system (TCP, UDP sockets)

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
    #if defined(__MINGW32__) || defined(__MINGW64__)
    /* MinGW provides pthreads but not fork/wait */
    #include <pthread.h>
    #include <unistd.h>
    #else
    #include "../platform/compat_windows.h"
    #include "../platform/compat_pthread.h"
    #endif
#else
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#endif

#include "../runtime.h"
#include "../debug.h"
#include "../test_harness.h"

/* ============================================================================
 * TcpListener Bind Tests
 * ============================================================================
 * Tests for rt_tcp_listener_bind() with various address formats.
 * ============================================================================ */

static void test_rt_tcp_listener_bind_ipv4(void)
{

    RtArena *arena = rt_arena_create(NULL);

    // Bind to localhost on a specific port (use high port to avoid permission issues)
    // Use port 0 first to get an available port, then test with that port
    RtTcpListener *listener = rt_tcp_listener_bind(arena, "127.0.0.1:0");
    assert(listener != NULL);
    assert(listener->fd >= 0);
    assert(listener->port > 0);

    // Get the assigned port and verify it's valid
    int port = rt_tcp_listener_get_port(listener);
    assert(port > 0);
    assert(port <= 65535);

    // Clean up
    rt_tcp_listener_close(listener);
    rt_arena_destroy(arena);
}

static void test_rt_tcp_listener_bind_port_only(void)
{

    RtArena *arena = rt_arena_create(NULL);

    // Bind to all interfaces with port-only syntax
    RtTcpListener *listener = rt_tcp_listener_bind(arena, ":8080");
    assert(listener != NULL);
    assert(listener->fd >= 0);
    assert(listener->port == 8080);

    // Verify port accessor returns correct value
    int port = rt_tcp_listener_get_port(listener);
    assert(port == 8080);

    // Clean up
    rt_tcp_listener_close(listener);
    rt_arena_destroy(arena);
}

static void test_rt_tcp_listener_bind_os_assigned_port(void)
{

    RtArena *arena = rt_arena_create(NULL);

    // Bind to port 0 - OS assigns an available port
    RtTcpListener *listener = rt_tcp_listener_bind(arena, ":0");
    assert(listener != NULL);
    assert(listener->fd >= 0);

    // Port should be assigned by OS (greater than 0)
    int port = rt_tcp_listener_get_port(listener);
    assert(port > 0);
    assert(port <= 65535);

    // The port field should match the accessor
    assert(listener->port == port);

    // Clean up
    rt_tcp_listener_close(listener);
    rt_arena_destroy(arena);
}

static void test_rt_tcp_listener_bind_multiple_listeners(void)
{

    RtArena *arena = rt_arena_create(NULL);

    // Bind multiple listeners on OS-assigned ports
    RtTcpListener *listener1 = rt_tcp_listener_bind(arena, ":0");
    RtTcpListener *listener2 = rt_tcp_listener_bind(arena, ":0");
    RtTcpListener *listener3 = rt_tcp_listener_bind(arena, ":0");

    assert(listener1 != NULL);
    assert(listener2 != NULL);
    assert(listener3 != NULL);

    // Each should have a unique port
    int port1 = rt_tcp_listener_get_port(listener1);
    int port2 = rt_tcp_listener_get_port(listener2);
    int port3 = rt_tcp_listener_get_port(listener3);

    assert(port1 > 0);
    assert(port2 > 0);
    assert(port3 > 0);

    // Ports should all be different (extremely high probability)
    assert(port1 != port2);
    assert(port2 != port3);
    assert(port1 != port3);

    // Clean up
    rt_tcp_listener_close(listener1);
    rt_tcp_listener_close(listener2);
    rt_tcp_listener_close(listener3);
    rt_arena_destroy(arena);
}

static void test_rt_tcp_listener_bind_localhost_alias(void)
{

    RtArena *arena = rt_arena_create(NULL);

    // Bind to localhost explicitly
    RtTcpListener *listener = rt_tcp_listener_bind(arena, "127.0.0.1:0");
    assert(listener != NULL);
    assert(listener->fd >= 0);

    int port = rt_tcp_listener_get_port(listener);
    assert(port > 0);
    assert(port <= 65535);

    rt_tcp_listener_close(listener);
    rt_arena_destroy(arena);
}

static void test_rt_tcp_listener_close_idempotent(void)
{

    RtArena *arena = rt_arena_create(NULL);

    RtTcpListener *listener = rt_tcp_listener_bind(arena, ":0");
    assert(listener != NULL);

    // Close multiple times - should be safe
    rt_tcp_listener_close(listener);
    rt_tcp_listener_close(listener);
    rt_tcp_listener_close(listener);

    rt_arena_destroy(arena);
}

static void test_rt_tcp_listener_port_range(void)
{

    RtArena *arena = rt_arena_create(NULL);

    // Create several listeners and verify port ranges
    for (int i = 0; i < 5; i++) {
        RtTcpListener *listener = rt_tcp_listener_bind(arena, ":0");
        assert(listener != NULL);

        int port = rt_tcp_listener_get_port(listener);
        // Port must be in valid range
        assert(port >= 1);
        assert(port <= 65535);
        // OS-assigned ports are typically in ephemeral range (1024+)
        assert(port >= 1024);

        rt_tcp_listener_close(listener);
    }

    rt_arena_destroy(arena);
}

static void test_rt_tcp_listener_bind_ipv6(void)
{

    RtArena *arena = rt_arena_create(NULL);

    // Bind to IPv6 loopback address with bracket notation
    RtTcpListener *listener = rt_tcp_listener_bind(arena, "[::1]:0");
    assert(listener != NULL);
    assert(listener->fd >= 0);

    // Verify port was assigned
    int port = rt_tcp_listener_get_port(listener);
    assert(port > 0);
    assert(port <= 65535);

    rt_tcp_listener_close(listener);
    rt_arena_destroy(arena);
}

static void test_rt_tcp_listener_bind_ipv6_specific_port(void)
{

    RtArena *arena = rt_arena_create(NULL);

    // Bind to IPv6 loopback on a specific port
    RtTcpListener *listener = rt_tcp_listener_bind(arena, "[::1]:8081");
    assert(listener != NULL);
    assert(listener->fd >= 0);

    // Verify port matches what was requested
    int port = rt_tcp_listener_get_port(listener);
    assert(port == 8081);

    rt_tcp_listener_close(listener);
    rt_arena_destroy(arena);
}

static void test_rt_tcp_listener_bind_hostname(void)
{

    RtArena *arena = rt_arena_create(NULL);

    // Bind using 'localhost' hostname - should resolve and bind
    RtTcpListener *listener = rt_tcp_listener_bind(arena, "localhost:0");
    assert(listener != NULL);
    assert(listener->fd >= 0);

    // Verify port was assigned
    int port = rt_tcp_listener_get_port(listener);
    assert(port > 0);
    assert(port <= 65535);

    rt_tcp_listener_close(listener);
    rt_arena_destroy(arena);
}

static void test_rt_tcp_listener_bind_hostname_specific_port(void)
{

    RtArena *arena = rt_arena_create(NULL);

    // Bind using 'localhost' on a specific port
    RtTcpListener *listener = rt_tcp_listener_bind(arena, "localhost:8082");
    assert(listener != NULL);
    assert(listener->fd >= 0);

    // Verify port matches what was requested
    int port = rt_tcp_listener_get_port(listener);
    assert(port == 8082);

    rt_tcp_listener_close(listener);
    rt_arena_destroy(arena);
}

/* ============================================================================
 * TcpListener Accept Tests
 * ============================================================================
 * Tests for rt_tcp_listener_accept() - requires creating client connections.
 * ============================================================================ */

/* Thread argument structure for client connection thread */
typedef struct {
    int port;
    int connected;
    RtArena *arena;
} ClientConnectArgs;

/* Thread function to connect to a server */
static void *client_connect_thread(void *arg)
{
    ClientConnectArgs *args = (ClientConnectArgs *)arg;

    /* Give the server a moment to be ready for accept */
    struct timespec ts = {0, 10000000}; /* 10ms */
    nanosleep(&ts, NULL);

    /* Connect to the server */
    char address[32];
    snprintf(address, sizeof(address), "127.0.0.1:%d", args->port);

    RtTcpStream *client = rt_tcp_stream_connect(args->arena, address);
    if (client != NULL) {
        args->connected = 1;
        rt_tcp_stream_close(client);
    }

    return NULL;
}

static void test_rt_tcp_listener_accept_basic(void)
{

    RtArena *server_arena = rt_arena_create(NULL);
    RtArena *client_arena = rt_arena_create(NULL);

    /* Create a listener on an OS-assigned port */
    RtTcpListener *listener = rt_tcp_listener_bind(server_arena, "127.0.0.1:0");
    assert(listener != NULL);
    int port = rt_tcp_listener_get_port(listener);
    assert(port > 0);

    /* Set up client thread arguments */
    ClientConnectArgs args = {port, 0, client_arena};

    /* Spawn a thread to connect as a client */
    pthread_t client_thread;
    int rc = pthread_create(&client_thread, NULL, client_connect_thread, &args);
    assert(rc == 0);

    /* Accept the connection */
    RtTcpStream *accepted = rt_tcp_listener_accept(server_arena, listener);
    assert(accepted != NULL);
    assert(accepted->fd >= 0);

    /* Wait for client thread to finish */
    pthread_join(client_thread, NULL);

    /* Verify client connected successfully */
    assert(args.connected == 1);

    /* Clean up */
    rt_tcp_stream_close(accepted);
    rt_tcp_listener_close(listener);
    rt_arena_destroy(client_arena);
    rt_arena_destroy(server_arena);
}

static void test_rt_tcp_listener_accept_has_remote_address(void)
{

    RtArena *server_arena = rt_arena_create(NULL);
    RtArena *client_arena = rt_arena_create(NULL);

    /* Create a listener */
    RtTcpListener *listener = rt_tcp_listener_bind(server_arena, "127.0.0.1:0");
    assert(listener != NULL);
    int port = rt_tcp_listener_get_port(listener);

    /* Set up client thread */
    ClientConnectArgs args = {port, 0, client_arena};
    pthread_t client_thread;
    pthread_create(&client_thread, NULL, client_connect_thread, &args);

    /* Accept the connection */
    RtTcpStream *accepted = rt_tcp_listener_accept(server_arena, listener);
    assert(accepted != NULL);

    /* Verify remote address is set */
    const char *remote = rt_tcp_stream_get_remote_address(accepted);
    assert(remote != NULL);
    assert(strlen(remote) > 0);

    /* Wait for client */
    pthread_join(client_thread, NULL);

    /* Clean up */
    rt_tcp_stream_close(accepted);
    rt_tcp_listener_close(listener);
    rt_arena_destroy(client_arena);
    rt_arena_destroy(server_arena);
}

static void test_rt_tcp_listener_accept_multiple_connections(void)
{

    RtArena *server_arena = rt_arena_create(NULL);
    RtArena *client_arena1 = rt_arena_create(NULL);
    RtArena *client_arena2 = rt_arena_create(NULL);

    /* Create a listener */
    RtTcpListener *listener = rt_tcp_listener_bind(server_arena, "127.0.0.1:0");
    assert(listener != NULL);
    int port = rt_tcp_listener_get_port(listener);

    /* First client connection */
    ClientConnectArgs args1 = {port, 0, client_arena1};
    pthread_t client_thread1;
    pthread_create(&client_thread1, NULL, client_connect_thread, &args1);

    RtTcpStream *accepted1 = rt_tcp_listener_accept(server_arena, listener);
    assert(accepted1 != NULL);
    assert(accepted1->fd >= 0);
    pthread_join(client_thread1, NULL);

    /* Second client connection */
    ClientConnectArgs args2 = {port, 0, client_arena2};
    pthread_t client_thread2;
    pthread_create(&client_thread2, NULL, client_connect_thread, &args2);

    RtTcpStream *accepted2 = rt_tcp_listener_accept(server_arena, listener);
    assert(accepted2 != NULL);
    assert(accepted2->fd >= 0);
    pthread_join(client_thread2, NULL);

    /* Each accepted stream should have a unique fd */
    assert(accepted1->fd != accepted2->fd);

    /* Clean up */
    rt_tcp_stream_close(accepted1);
    rt_tcp_stream_close(accepted2);
    rt_tcp_listener_close(listener);
    rt_arena_destroy(client_arena1);
    rt_arena_destroy(client_arena2);
    rt_arena_destroy(server_arena);
}

/* ============================================================================
 * TcpListener Close Tests
 * ============================================================================
 * Tests for rt_tcp_listener_close() behavior.
 * ============================================================================ */

static void test_rt_tcp_listener_close_basic(void)
{

    RtArena *arena = rt_arena_create(NULL);

    RtTcpListener *listener = rt_tcp_listener_bind(arena, ":0");
    assert(listener != NULL);
    int original_fd = listener->fd;
    assert(original_fd >= 0);

    /* Close the listener */
    rt_tcp_listener_close(listener);

    /* After close, fd should be -1 (marking it as closed) */
    assert(listener->fd == -1);

    rt_arena_destroy(arena);
}

static void test_rt_tcp_listener_close_multiple_times(void)
{

    RtArena *arena = rt_arena_create(NULL);

    RtTcpListener *listener = rt_tcp_listener_bind(arena, ":0");
    assert(listener != NULL);

    /* Close multiple times - should not crash or error */
    rt_tcp_listener_close(listener);
    assert(listener->fd == -1);

    rt_tcp_listener_close(listener);
    assert(listener->fd == -1);

    rt_tcp_listener_close(listener);
    assert(listener->fd == -1);

    rt_arena_destroy(arena);
}

static void test_rt_tcp_listener_close_releases_port(void)
{

    RtArena *arena = rt_arena_create(NULL);

    /* Bind to a specific port */
    RtTcpListener *listener1 = rt_tcp_listener_bind(arena, ":8083");
    assert(listener1 != NULL);
    assert(listener1->port == 8083);

    /* Close the listener */
    rt_tcp_listener_close(listener1);

    /* Should be able to bind to the same port again */
    RtTcpListener *listener2 = rt_tcp_listener_bind(arena, ":8083");
    assert(listener2 != NULL);
    assert(listener2->port == 8083);

    rt_tcp_listener_close(listener2);
    rt_arena_destroy(arena);
}

/* ============================================================================
 * TcpStream Connect Tests
 * ============================================================================
 * Tests for rt_tcp_stream_connect() - connecting to a server.
 * ============================================================================ */

static void test_rt_tcp_stream_connect_basic(void)
{

    RtArena *server_arena = rt_arena_create(NULL);
    RtArena *client_arena = rt_arena_create(NULL);

    /* Create a listener to accept connections */
    RtTcpListener *listener = rt_tcp_listener_bind(server_arena, "127.0.0.1:0");
    assert(listener != NULL);
    int port = rt_tcp_listener_get_port(listener);

    /* Build address string */
    char address[32];
    snprintf(address, sizeof(address), "127.0.0.1:%d", port);

    /* Connect from client in separate thread, accept in main */
    ClientConnectArgs args = {port, 0, client_arena};
    pthread_t client_thread;
    pthread_create(&client_thread, NULL, client_connect_thread, &args);

    /* Accept the connection */
    RtTcpStream *server_stream = rt_tcp_listener_accept(server_arena, listener);
    assert(server_stream != NULL);

    /* Wait for client thread */
    pthread_join(client_thread, NULL);

    /* Verify client connected successfully */
    assert(args.connected == 1);

    /* Clean up */
    rt_tcp_stream_close(server_stream);
    rt_tcp_listener_close(listener);
    rt_arena_destroy(client_arena);
    rt_arena_destroy(server_arena);
}

/* Shared struct and thread function for TCP accept tests */
typedef struct {
    RtTcpListener *listener;
    RtArena *arena;
    RtTcpStream *accepted;
} TcpAcceptArgs;

static void *tcp_accept_thread_fn(void *arg) {
    TcpAcceptArgs *a = (TcpAcceptArgs *)arg;
    a->accepted = rt_tcp_listener_accept(a->arena, a->listener);
    return NULL;
}

static void test_rt_tcp_stream_connect_has_valid_fd(void)
{

    RtArena *server_arena = rt_arena_create(NULL);
    RtArena *client_arena = rt_arena_create(NULL);

    /* Create a listener */
    RtTcpListener *listener = rt_tcp_listener_bind(server_arena, "127.0.0.1:0");
    assert(listener != NULL);
    int port = rt_tcp_listener_get_port(listener);

    TcpAcceptArgs accept_args = {listener, server_arena, NULL};

    pthread_t accept_thread;
    pthread_create(&accept_thread, NULL, tcp_accept_thread_fn, &accept_args);

    /* Give server time to start accepting */
    struct timespec ts = {0, 10000000}; /* 10ms */
    nanosleep(&ts, NULL);

    /* Connect as client */
    char address[32];
    snprintf(address, sizeof(address), "127.0.0.1:%d", port);
    RtTcpStream *client = rt_tcp_stream_connect(client_arena, address);
    assert(client != NULL);
    assert(client->fd >= 0);

    /* Wait for accept thread */
    pthread_join(accept_thread, NULL);

    /* Clean up */
    rt_tcp_stream_close(client);
    rt_tcp_stream_close(accept_args.accepted);
    rt_tcp_listener_close(listener);
    rt_arena_destroy(client_arena);
    rt_arena_destroy(server_arena);
}

static void test_rt_tcp_stream_connect_has_remote_address(void)
{

    RtArena *server_arena = rt_arena_create(NULL);
    RtArena *client_arena = rt_arena_create(NULL);

    /* Create a listener */
    RtTcpListener *listener = rt_tcp_listener_bind(server_arena, "127.0.0.1:0");
    assert(listener != NULL);
    int port = rt_tcp_listener_get_port(listener);

    TcpAcceptArgs accept_args = {listener, server_arena, NULL};

    pthread_t accept_thread;
    pthread_create(&accept_thread, NULL, tcp_accept_thread_fn, &accept_args);

    /* Give server time to start accepting */
    struct timespec ts = {0, 10000000}; /* 10ms */
    nanosleep(&ts, NULL);

    /* Connect as client */
    char address[32];
    snprintf(address, sizeof(address), "127.0.0.1:%d", port);
    RtTcpStream *client = rt_tcp_stream_connect(client_arena, address);
    assert(client != NULL);

    /* Verify remote address is set */
    const char *remote = rt_tcp_stream_get_remote_address(client);
    assert(remote != NULL);
    assert(strlen(remote) > 0);

    /* Remote address should contain the port we connected to */
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);
    assert(strstr(remote, port_str) != NULL);

    /* Wait for accept thread */
    pthread_join(accept_thread, NULL);

    /* Clean up */
    rt_tcp_stream_close(client);
    rt_tcp_stream_close(accept_args.accepted);
    rt_tcp_listener_close(listener);
    rt_arena_destroy(client_arena);
    rt_arena_destroy(server_arena);
}

static void test_rt_tcp_stream_connect_localhost_hostname(void)
{

    RtArena *server_arena = rt_arena_create(NULL);
    RtArena *client_arena = rt_arena_create(NULL);

    /* Create a listener */
    RtTcpListener *listener = rt_tcp_listener_bind(server_arena, "127.0.0.1:0");
    assert(listener != NULL);
    int port = rt_tcp_listener_get_port(listener);

    TcpAcceptArgs accept_args = {listener, server_arena, NULL};

    pthread_t accept_thread;
    pthread_create(&accept_thread, NULL, tcp_accept_thread_fn, &accept_args);

    /* Give server time to start accepting */
    struct timespec ts = {0, 10000000}; /* 10ms */
    nanosleep(&ts, NULL);

    /* Connect using 'localhost' hostname */
    char address[32];
    snprintf(address, sizeof(address), "localhost:%d", port);
    RtTcpStream *client = rt_tcp_stream_connect(client_arena, address);
    assert(client != NULL);
    assert(client->fd >= 0);

    /* Wait for accept thread */
    pthread_join(accept_thread, NULL);

    /* Clean up */
    rt_tcp_stream_close(client);
    rt_tcp_stream_close(accept_args.accepted);
    rt_tcp_listener_close(listener);
    rt_arena_destroy(client_arena);
    rt_arena_destroy(server_arena);
}

/* ============================================================================
 * TcpStream Close Tests
 * ============================================================================
 * Tests for rt_tcp_stream_close() behavior.
 * ============================================================================ */

static void test_rt_tcp_stream_close_basic(void)
{

    RtArena *server_arena = rt_arena_create(NULL);
    RtArena *client_arena = rt_arena_create(NULL);

    /* Create a listener */
    RtTcpListener *listener = rt_tcp_listener_bind(server_arena, "127.0.0.1:0");
    assert(listener != NULL);
    int port = rt_tcp_listener_get_port(listener);

    TcpAcceptArgs accept_args = {listener, server_arena, NULL};

    pthread_t accept_thread;
    pthread_create(&accept_thread, NULL, tcp_accept_thread_fn, &accept_args);

    /* Give server time to start accepting */
    struct timespec ts = {0, 10000000}; /* 10ms */
    nanosleep(&ts, NULL);

    /* Connect as client */
    char address[32];
    snprintf(address, sizeof(address), "127.0.0.1:%d", port);
    RtTcpStream *client = rt_tcp_stream_connect(client_arena, address);
    assert(client != NULL);
    int original_fd = client->fd;
    assert(original_fd >= 0);

    /* Close the stream */
    rt_tcp_stream_close(client);

    /* After close, fd should be -1 (marking it as closed) */
    assert(client->fd == -1);

    /* Wait for accept thread */
    pthread_join(accept_thread, NULL);

    /* Clean up */
    rt_tcp_stream_close(accept_args.accepted);
    rt_tcp_listener_close(listener);
    rt_arena_destroy(client_arena);
    rt_arena_destroy(server_arena);
}

static void test_rt_tcp_stream_close_multiple_times(void)
{

    RtArena *server_arena = rt_arena_create(NULL);
    RtArena *client_arena = rt_arena_create(NULL);

    /* Create a listener */
    RtTcpListener *listener = rt_tcp_listener_bind(server_arena, "127.0.0.1:0");
    assert(listener != NULL);
    int port = rt_tcp_listener_get_port(listener);

    TcpAcceptArgs accept_args = {listener, server_arena, NULL};

    pthread_t accept_thread;
    pthread_create(&accept_thread, NULL, tcp_accept_thread_fn, &accept_args);

    /* Give server time to start accepting */
    struct timespec ts = {0, 10000000}; /* 10ms */
    nanosleep(&ts, NULL);

    /* Connect as client */
    char address[32];
    snprintf(address, sizeof(address), "127.0.0.1:%d", port);
    RtTcpStream *client = rt_tcp_stream_connect(client_arena, address);
    assert(client != NULL);

    /* Close multiple times - should not crash or error */
    rt_tcp_stream_close(client);
    assert(client->fd == -1);

    rt_tcp_stream_close(client);
    assert(client->fd == -1);

    rt_tcp_stream_close(client);
    assert(client->fd == -1);

    /* Wait for accept thread */
    pthread_join(accept_thread, NULL);

    /* Clean up */
    rt_tcp_stream_close(accept_args.accepted);
    rt_tcp_listener_close(listener);
    rt_arena_destroy(client_arena);
    rt_arena_destroy(server_arena);
}

/* ============================================================================
 * TcpStream Read/Write Tests
 * ============================================================================
 * Tests for rt_tcp_stream_read, rt_tcp_stream_write, and related functions.
 * ============================================================================ */

/* Helper: Shared accept args structure (single definition) */
typedef struct {
    RtTcpListener *listener;
    RtArena *arena;
    RtTcpStream *accepted;
} AcceptThreadArgs;

/* Helper: Accept thread function (single definition) */
static void *accept_thread_helper(void *arg)
{
    AcceptThreadArgs *a = (AcceptThreadArgs *)arg;
    a->accepted = rt_tcp_listener_accept(a->arena, a->listener);
    return NULL;
}

static void test_rt_tcp_stream_write_basic(void)
{

    RtArena *server_arena = rt_arena_create(NULL);
    RtArena *client_arena = rt_arena_create(NULL);

    /* Create a listener */
    RtTcpListener *listener = rt_tcp_listener_bind(server_arena, "127.0.0.1:0");
    assert(listener != NULL);
    int port = rt_tcp_listener_get_port(listener);

    /* Start accept thread */
    AcceptThreadArgs accept_args = {listener, server_arena, NULL};
    pthread_t accept_thread;
    pthread_create(&accept_thread, NULL, accept_thread_helper, &accept_args);

    /* Give server time to start accepting */
    struct timespec ts = {0, 10000000}; /* 10ms */
    nanosleep(&ts, NULL);

    /* Connect as client */
    char address[32];
    snprintf(address, sizeof(address), "127.0.0.1:%d", port);
    RtTcpStream *client = rt_tcp_stream_connect(client_arena, address);
    assert(client != NULL);

    /* Create a byte array to send */
    unsigned char *data = rt_array_alloc_byte(client_arena, 5, 0);
    data[0] = 'h';
    data[1] = 'e';
    data[2] = 'l';
    data[3] = 'l';
    data[4] = 'o';

    /* Write data */
    long written = rt_tcp_stream_write(client, data);
    assert(written == 5);

    /* Wait for accept thread */
    pthread_join(accept_thread, NULL);

    /* Clean up */
    rt_tcp_stream_close(client);
    rt_tcp_stream_close(accept_args.accepted);
    rt_tcp_listener_close(listener);
    rt_arena_destroy(client_arena);
    rt_arena_destroy(server_arena);
}

static void test_rt_tcp_stream_read_basic(void)
{

    RtArena *server_arena = rt_arena_create(NULL);
    RtArena *client_arena = rt_arena_create(NULL);

    /* Create a listener */
    RtTcpListener *listener = rt_tcp_listener_bind(server_arena, "127.0.0.1:0");
    assert(listener != NULL);
    int port = rt_tcp_listener_get_port(listener);

    /* Start accept thread */
    AcceptThreadArgs accept_args = {listener, server_arena, NULL};
    pthread_t accept_thread;
    pthread_create(&accept_thread, NULL, accept_thread_helper, &accept_args);

    /* Give server time to start accepting */
    struct timespec ts = {0, 10000000}; /* 10ms */
    nanosleep(&ts, NULL);

    /* Connect as client */
    char address[32];
    snprintf(address, sizeof(address), "127.0.0.1:%d", port);
    RtTcpStream *client = rt_tcp_stream_connect(client_arena, address);
    assert(client != NULL);

    /* Wait for accept thread */
    pthread_join(accept_thread, NULL);
    assert(accept_args.accepted != NULL);

    /* Client sends data */
    unsigned char *send_data = rt_array_alloc_byte(client_arena, 5, 0);
    send_data[0] = 'h';
    send_data[1] = 'e';
    send_data[2] = 'l';
    send_data[3] = 'l';
    send_data[4] = 'o';
    rt_tcp_stream_write(client, send_data);

    /* Server reads data */
    unsigned char *recv_data = rt_tcp_stream_read(server_arena, accept_args.accepted, 10);
    assert(recv_data != NULL);
    size_t len = rt_array_length(recv_data);
    assert(len == 5);
    assert(recv_data[0] == 'h');
    assert(recv_data[1] == 'e');
    assert(recv_data[2] == 'l');
    assert(recv_data[3] == 'l');
    assert(recv_data[4] == 'o');

    /* Clean up */
    rt_tcp_stream_close(client);
    rt_tcp_stream_close(accept_args.accepted);
    rt_tcp_listener_close(listener);
    rt_arena_destroy(client_arena);
    rt_arena_destroy(server_arena);
}

static void test_rt_tcp_stream_read_all(void)
{

    RtArena *server_arena = rt_arena_create(NULL);
    RtArena *client_arena = rt_arena_create(NULL);

    /* Create a listener */
    RtTcpListener *listener = rt_tcp_listener_bind(server_arena, "127.0.0.1:0");
    assert(listener != NULL);
    int port = rt_tcp_listener_get_port(listener);

    /* Start accept thread */
    AcceptThreadArgs accept_args = {listener, server_arena, NULL};
    pthread_t accept_thread;
    pthread_create(&accept_thread, NULL, accept_thread_helper, &accept_args);

    /* Give server time to start accepting */
    struct timespec ts = {0, 10000000}; /* 10ms */
    nanosleep(&ts, NULL);

    /* Connect as client */
    char address[32];
    snprintf(address, sizeof(address), "127.0.0.1:%d", port);
    RtTcpStream *client = rt_tcp_stream_connect(client_arena, address);
    assert(client != NULL);

    /* Wait for accept thread */
    pthread_join(accept_thread, NULL);
    assert(accept_args.accepted != NULL);

    /* Client sends data in multiple writes */
    unsigned char *data1 = rt_array_alloc_byte(client_arena, 3, 0);
    data1[0] = 'a'; data1[1] = 'b'; data1[2] = 'c';
    rt_tcp_stream_write(client, data1);

    unsigned char *data2 = rt_array_alloc_byte(client_arena, 3, 0);
    data2[0] = 'd'; data2[1] = 'e'; data2[2] = 'f';
    rt_tcp_stream_write(client, data2);

    /* Close client to signal EOF */
    rt_tcp_stream_close(client);

    /* Server reads all data until EOF */
    unsigned char *recv_data = rt_tcp_stream_read_all(server_arena, accept_args.accepted);
    assert(recv_data != NULL);
    size_t len = rt_array_length(recv_data);
    assert(len == 6);
    assert(recv_data[0] == 'a');
    assert(recv_data[1] == 'b');
    assert(recv_data[2] == 'c');
    assert(recv_data[3] == 'd');
    assert(recv_data[4] == 'e');
    assert(recv_data[5] == 'f');

    /* Clean up */
    rt_tcp_stream_close(accept_args.accepted);
    rt_tcp_listener_close(listener);
    rt_arena_destroy(client_arena);
    rt_arena_destroy(server_arena);
}

static void test_rt_tcp_stream_read_line(void)
{

    RtArena *server_arena = rt_arena_create(NULL);
    RtArena *client_arena = rt_arena_create(NULL);

    /* Create a listener */
    RtTcpListener *listener = rt_tcp_listener_bind(server_arena, "127.0.0.1:0");
    assert(listener != NULL);
    int port = rt_tcp_listener_get_port(listener);

    /* Start accept thread */
    AcceptThreadArgs accept_args = {listener, server_arena, NULL};
    pthread_t accept_thread;
    pthread_create(&accept_thread, NULL, accept_thread_helper, &accept_args);

    /* Give server time to start accepting */
    struct timespec ts = {0, 10000000}; /* 10ms */
    nanosleep(&ts, NULL);

    /* Connect as client */
    char address[32];
    snprintf(address, sizeof(address), "127.0.0.1:%d", port);
    RtTcpStream *client = rt_tcp_stream_connect(client_arena, address);
    assert(client != NULL);

    /* Wait for accept thread */
    pthread_join(accept_thread, NULL);
    assert(accept_args.accepted != NULL);

    /* Client sends line-delimited text */
    rt_tcp_stream_write_line(client, "Hello");
    rt_tcp_stream_write_line(client, "World");

    /* Server reads lines (note: read_line strips the newline) */
    char *line1 = rt_tcp_stream_read_line(server_arena, accept_args.accepted);
    assert(line1 != NULL);
    assert(strcmp(line1, "Hello") == 0);

    char *line2 = rt_tcp_stream_read_line(server_arena, accept_args.accepted);
    assert(line2 != NULL);
    assert(strcmp(line2, "World") == 0);

    /* Clean up */
    rt_tcp_stream_close(client);
    rt_tcp_stream_close(accept_args.accepted);
    rt_tcp_listener_close(listener);
    rt_arena_destroy(client_arena);
    rt_arena_destroy(server_arena);
}

static void test_rt_tcp_stream_write_line(void)
{

    RtArena *server_arena = rt_arena_create(NULL);
    RtArena *client_arena = rt_arena_create(NULL);

    /* Create a listener */
    RtTcpListener *listener = rt_tcp_listener_bind(server_arena, "127.0.0.1:0");
    assert(listener != NULL);
    int port = rt_tcp_listener_get_port(listener);

    /* Start accept thread */
    AcceptThreadArgs accept_args = {listener, server_arena, NULL};
    pthread_t accept_thread;
    pthread_create(&accept_thread, NULL, accept_thread_helper, &accept_args);

    /* Give server time to start accepting */
    struct timespec ts = {0, 10000000}; /* 10ms */
    nanosleep(&ts, NULL);

    /* Connect as client */
    char address[32];
    snprintf(address, sizeof(address), "127.0.0.1:%d", port);
    RtTcpStream *client = rt_tcp_stream_connect(client_arena, address);
    assert(client != NULL);

    /* Wait for accept thread */
    pthread_join(accept_thread, NULL);
    assert(accept_args.accepted != NULL);

    /* Client sends line with write_line (appends newline) */
    rt_tcp_stream_write_line(client, "test message");

    /* Close client to signal EOF */
    rt_tcp_stream_close(client);

    /* Server reads all data */
    unsigned char *recv_data = rt_tcp_stream_read_all(server_arena, accept_args.accepted);
    assert(recv_data != NULL);
    size_t len = rt_array_length(recv_data);
    /* "test message\n" = 13 characters */
    assert(len == 13);
    assert(recv_data[12] == '\n');

    /* Convert to string to verify content */
    char *str = rt_byte_array_to_string(server_arena, recv_data);
    assert(strcmp(str, "test message\n") == 0);

    /* Clean up */
    rt_tcp_stream_close(accept_args.accepted);
    rt_tcp_listener_close(listener);
    rt_arena_destroy(client_arena);
    rt_arena_destroy(server_arena);
}

/* ============================================================================
 * UdpSocket Bind Tests
 * ============================================================================ */

/* Test binding UDP socket to IPv4 address */
static void test_rt_udp_socket_bind_ipv4()
{
    RtArena *arena = rt_arena_create(NULL);
    RtUdpSocket *socket = rt_udp_socket_bind(arena, "127.0.0.1:9000");

    assert(socket != NULL);
    assert(socket->fd > 0);
    assert(rt_udp_socket_get_port(socket) == 9000);

    rt_udp_socket_close(socket);
    rt_arena_destroy(arena);
}

/* Test binding UDP socket to port-only address (all interfaces) */
static void test_rt_udp_socket_bind_port_only()
{
    RtArena *arena = rt_arena_create(NULL);
    RtUdpSocket *socket = rt_udp_socket_bind(arena, ":9001");

    assert(socket != NULL);
    assert(socket->fd > 0);
    assert(rt_udp_socket_get_port(socket) == 9001);

    rt_udp_socket_close(socket);
    rt_arena_destroy(arena);
}

/* Test binding UDP socket with OS-assigned port */
static void test_rt_udp_socket_bind_os_assigned_port()
{
    RtArena *arena = rt_arena_create(NULL);
    RtUdpSocket *socket = rt_udp_socket_bind(arena, ":0");

    assert(socket != NULL);
    assert(socket->fd > 0);
    /* OS-assigned port should be > 0 (typically in ephemeral range) */
    assert(rt_udp_socket_get_port(socket) > 0);

    rt_udp_socket_close(socket);
    rt_arena_destroy(arena);
}

/* Test binding multiple UDP sockets */
static void test_rt_udp_socket_bind_multiple()
{
    RtArena *arena = rt_arena_create(NULL);
    RtUdpSocket *socket1 = rt_udp_socket_bind(arena, ":0");
    RtUdpSocket *socket2 = rt_udp_socket_bind(arena, ":0");

    assert(socket1 != NULL);
    assert(socket2 != NULL);
    assert(socket1->fd != socket2->fd);
    /* Ports may or may not be different, but both should be valid */
    assert(rt_udp_socket_get_port(socket1) > 0);
    assert(rt_udp_socket_get_port(socket2) > 0);

    rt_udp_socket_close(socket1);
    rt_udp_socket_close(socket2);
    rt_arena_destroy(arena);
}

/* ============================================================================
 * UdpSocket Close Tests
 * ============================================================================ */

/* Test basic UDP socket close */
static void test_rt_udp_socket_close_basic()
{
    RtArena *arena = rt_arena_create(NULL);
    RtUdpSocket *socket = rt_udp_socket_bind(arena, ":0");

    assert(socket != NULL);
    int fd = socket->fd;
    assert(fd > 0);

    rt_udp_socket_close(socket);
    /* After close, fd should be -1 */
    assert(socket->fd == -1);

    rt_arena_destroy(arena);
}

/* Test that UDP socket close is idempotent (safe to call multiple times) */
static void test_rt_udp_socket_close_multiple_times()
{
    RtArena *arena = rt_arena_create(NULL);
    RtUdpSocket *socket = rt_udp_socket_bind(arena, ":0");

    assert(socket != NULL);

    /* Close multiple times - should not crash or error */
    rt_udp_socket_close(socket);
    rt_udp_socket_close(socket);
    rt_udp_socket_close(socket);

    assert(socket->fd == -1);

    rt_arena_destroy(arena);
}

/* Test that closing UDP socket releases the port */
static void test_rt_udp_socket_close_releases_port()
{
    RtArena *arena = rt_arena_create(NULL);

    /* Bind to a specific port */
    RtUdpSocket *socket1 = rt_udp_socket_bind(arena, "127.0.0.1:9002");
    assert(socket1 != NULL);
    assert(rt_udp_socket_get_port(socket1) == 9002);

    /* Close the socket */
    rt_udp_socket_close(socket1);

    /* Should be able to bind to the same port again */
    RtUdpSocket *socket2 = rt_udp_socket_bind(arena, "127.0.0.1:9002");
    assert(socket2 != NULL);
    assert(rt_udp_socket_get_port(socket2) == 9002);

    rt_udp_socket_close(socket2);
    rt_arena_destroy(arena);
}

/* ============================================================================
 * UdpSocket Send/Receive Tests
 * ============================================================================ */

/* Test basic send_to and receive_from with localhost */
static void test_rt_udp_socket_send_receive_basic()
{
    RtArena *arena = rt_arena_create(NULL);

    /* Create receiver socket */
    RtUdpSocket *receiver = rt_udp_socket_bind(arena, "127.0.0.1:0");
    assert(receiver != NULL);
    int recv_port = rt_udp_socket_get_port(receiver);
    assert(recv_port > 0);

    /* Create sender socket */
    RtUdpSocket *sender = rt_udp_socket_bind(arena, "127.0.0.1:0");
    assert(sender != NULL);

    /* Create test data */
    unsigned char *data = rt_array_alloc_byte(arena, 5, 0);
    data[0] = 'H';
    data[1] = 'e';
    data[2] = 'l';
    data[3] = 'l';
    data[4] = 'o';

    /* Build destination address */
    char dest_addr[32];
    snprintf(dest_addr, sizeof(dest_addr), "127.0.0.1:%d", recv_port);

    /* Send datagram */
    long bytes_sent = rt_udp_socket_send_to(sender, data, dest_addr);
    assert(bytes_sent == 5);

    /* Receive datagram */
    char *sender_addr = NULL;
    unsigned char *recv_data = rt_udp_socket_receive_from(arena, receiver, 1024, &sender_addr);

    /* Verify received data */
    assert(recv_data != NULL);
    assert(rt_array_length(recv_data) == 5);
    assert(recv_data[0] == 'H');
    assert(recv_data[1] == 'e');
    assert(recv_data[2] == 'l');
    assert(recv_data[3] == 'l');
    assert(recv_data[4] == 'o');

    /* Verify sender address is set */
    assert(sender_addr != NULL);
    assert(strlen(sender_addr) > 0);

    rt_udp_socket_close(sender);
    rt_udp_socket_close(receiver);
    rt_arena_destroy(arena);
}

/* Test that sender address is correctly returned from receive_from */
static void test_rt_udp_socket_receive_from_sender_address()
{
    RtArena *arena = rt_arena_create(NULL);

    /* Create receiver socket */
    RtUdpSocket *receiver = rt_udp_socket_bind(arena, "127.0.0.1:0");
    assert(receiver != NULL);
    int recv_port = rt_udp_socket_get_port(receiver);

    /* Create sender socket on known port */
    RtUdpSocket *sender = rt_udp_socket_bind(arena, "127.0.0.1:0");
    assert(sender != NULL);
    int send_port = rt_udp_socket_get_port(sender);

    /* Send a test datagram */
    unsigned char *data = rt_array_alloc_byte(arena, 4, 0);
    data[0] = 'T';
    data[1] = 'E';
    data[2] = 'S';
    data[3] = 'T';

    char dest_addr[32];
    snprintf(dest_addr, sizeof(dest_addr), "127.0.0.1:%d", recv_port);

    rt_udp_socket_send_to(sender, data, dest_addr);

    /* Receive and check sender address */
    char *sender_addr = NULL;
    unsigned char *recv_data = rt_udp_socket_receive_from(arena, receiver, 1024, &sender_addr);
    assert(recv_data != NULL);
    assert(sender_addr != NULL);

    /* Sender address should contain 127.0.0.1 and the sender's port */
    assert(strstr(sender_addr, "127.0.0.1") != NULL);

    /* Extract port from sender_addr and verify it matches */
    char *colon = strrchr(sender_addr, ':');
    assert(colon != NULL);
    int reported_port = atoi(colon + 1);
    assert(reported_port == send_port);

    rt_udp_socket_close(sender);
    rt_udp_socket_close(receiver);
    rt_arena_destroy(arena);
}

/* Test data integrity - sent data matches received data */
static void test_rt_udp_socket_data_integrity()
{
    RtArena *arena = rt_arena_create(NULL);

    /* Create receiver and sender */
    RtUdpSocket *receiver = rt_udp_socket_bind(arena, "127.0.0.1:0");
    RtUdpSocket *sender = rt_udp_socket_bind(arena, "127.0.0.1:0");
    int recv_port = rt_udp_socket_get_port(receiver);

    char dest_addr[32];
    snprintf(dest_addr, sizeof(dest_addr), "127.0.0.1:%d", recv_port);

    /* Create test data with various byte values */
    unsigned char *data = rt_array_alloc_byte(arena, 256, 0);
    for (int i = 0; i < 256; i++) {
        data[i] = (unsigned char)i;
    }

    /* Send datagram */
    long bytes_sent = rt_udp_socket_send_to(sender, data, dest_addr);
    assert(bytes_sent == 256);

    /* Receive datagram */
    char *sender_addr = NULL;
    unsigned char *recv_data = rt_udp_socket_receive_from(arena, receiver, 1024, &sender_addr);

    /* Verify all bytes match */
    assert(recv_data != NULL);
    assert(rt_array_length(recv_data) == 256);
    for (int i = 0; i < 256; i++) {
        assert(recv_data[i] == (unsigned char)i);
    }

    rt_udp_socket_close(sender);
    rt_udp_socket_close(receiver);
    rt_arena_destroy(arena);
}

/* Test sending multiple datagrams */
static void test_rt_udp_socket_multiple_datagrams()
{
    RtArena *arena = rt_arena_create(NULL);

    RtUdpSocket *receiver = rt_udp_socket_bind(arena, "127.0.0.1:0");
    RtUdpSocket *sender = rt_udp_socket_bind(arena, "127.0.0.1:0");
    int recv_port = rt_udp_socket_get_port(receiver);

    char dest_addr[32];
    snprintf(dest_addr, sizeof(dest_addr), "127.0.0.1:%d", recv_port);

    /* Send first datagram */
    unsigned char *data1 = rt_array_alloc_byte(arena, 3, 0);
    data1[0] = 'O';
    data1[1] = 'N';
    data1[2] = 'E';
    rt_udp_socket_send_to(sender, data1, dest_addr);

    /* Send second datagram */
    unsigned char *data2 = rt_array_alloc_byte(arena, 3, 0);
    data2[0] = 'T';
    data2[1] = 'W';
    data2[2] = 'O';
    rt_udp_socket_send_to(sender, data2, dest_addr);

    /* Receive first datagram */
    char *sender_addr1 = NULL;
    unsigned char *recv1 = rt_udp_socket_receive_from(arena, receiver, 1024, &sender_addr1);
    assert(recv1 != NULL);
    assert(rt_array_length(recv1) == 3);
    assert(recv1[0] == 'O');
    assert(recv1[1] == 'N');
    assert(recv1[2] == 'E');

    /* Receive second datagram */
    char *sender_addr2 = NULL;
    unsigned char *recv2 = rt_udp_socket_receive_from(arena, receiver, 1024, &sender_addr2);
    assert(recv2 != NULL);
    assert(rt_array_length(recv2) == 3);
    assert(recv2[0] == 'T');
    assert(recv2[1] == 'W');
    assert(recv2[2] == 'O');

    rt_udp_socket_close(sender);
    rt_udp_socket_close(receiver);
    rt_arena_destroy(arena);
}

/* Test sending empty datagram */
static void test_rt_udp_socket_empty_datagram()
{
    RtArena *arena = rt_arena_create(NULL);

    RtUdpSocket *receiver = rt_udp_socket_bind(arena, "127.0.0.1:0");
    RtUdpSocket *sender = rt_udp_socket_bind(arena, "127.0.0.1:0");
    int recv_port = rt_udp_socket_get_port(receiver);

    char dest_addr[32];
    snprintf(dest_addr, sizeof(dest_addr), "127.0.0.1:%d", recv_port);

    /* Send empty datagram */
    unsigned char *data = rt_array_alloc_byte(arena, 0, 0);
    long bytes_sent = rt_udp_socket_send_to(sender, data, dest_addr);
    assert(bytes_sent == 0);

    /* Receive empty datagram */
    char *sender_addr = NULL;
    unsigned char *recv_data = rt_udp_socket_receive_from(arena, receiver, 1024, &sender_addr);
    assert(recv_data != NULL);
    assert(rt_array_length(recv_data) == 0);

    rt_udp_socket_close(sender);
    rt_udp_socket_close(receiver);
    rt_arena_destroy(arena);
}

/* ============================================================================
 * Address Parsing Error Handling Tests
 * ============================================================================
 * These tests verify that invalid address formats cause appropriate errors.
 * Since the runtime calls exit(1) on errors, we use fork() to test in a
 * child process and verify the exit code.
 * NOTE: These tests only work on POSIX systems (fork/wait not available on Windows)
 * ============================================================================ */

#ifndef _WIN32  /* fork-based tests only on POSIX */

/* Helper function to test that a function call exits with failure.
 * Returns 1 if the child exited with non-zero status, 0 otherwise.
 */
static int expect_exit_failure(void (*test_func)(void))
{
    fflush(stdout);
    fflush(stderr);

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork failed");
        return 0;
    }

    if (pid == 0) {
        /* Child process: suppress stderr and run the test */
        FILE *f = freopen("/dev/null", "w", stderr);
        (void)f; /* Suppress unused variable warning */
        test_func();
        /* If we get here, the test didn't exit - that's a failure */
        _exit(0);
    }

    /* Parent process: wait for child and check exit status */
    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status) != 0;
    }
    return 0;
}

/* Test helper functions that attempt invalid address parsing */

static void try_bind_empty_address(void)
{
    RtArena *arena = rt_arena_create(NULL);
    rt_tcp_listener_bind(arena, "");
    rt_arena_destroy(arena);
}

static void try_bind_missing_port(void)
{
    RtArena *arena = rt_arena_create(NULL);
    rt_tcp_listener_bind(arena, "127.0.0.1");
    rt_arena_destroy(arena);
}

static void try_bind_invalid_port_letters(void)
{
    RtArena *arena = rt_arena_create(NULL);
    rt_tcp_listener_bind(arena, "127.0.0.1:abc");
    rt_arena_destroy(arena);
}

static void try_bind_port_too_high(void)
{
    RtArena *arena = rt_arena_create(NULL);
    rt_tcp_listener_bind(arena, "127.0.0.1:99999");
    rt_arena_destroy(arena);
}

static void try_bind_port_negative(void)
{
    RtArena *arena = rt_arena_create(NULL);
    /* Port -1 would be parsed as host=127.0.0.1:-1 which is invalid */
    rt_tcp_listener_bind(arena, ":-1");
    rt_arena_destroy(arena);
}

static void try_bind_ipv6_missing_bracket(void)
{
    RtArena *arena = rt_arena_create(NULL);
    rt_tcp_listener_bind(arena, "[::1:8080");
    rt_arena_destroy(arena);
}

static void try_bind_ipv6_missing_port(void)
{
    RtArena *arena = rt_arena_create(NULL);
    rt_tcp_listener_bind(arena, "[::1]");
    rt_arena_destroy(arena);
}

static void try_bind_ipv6_no_colon_after_bracket(void)
{
    RtArena *arena = rt_arena_create(NULL);
    rt_tcp_listener_bind(arena, "[::1]8080");
    rt_arena_destroy(arena);
}

static void try_bind_empty_port(void)
{
    RtArena *arena = rt_arena_create(NULL);
    rt_tcp_listener_bind(arena, "127.0.0.1:");
    rt_arena_destroy(arena);
}

/* Actual test functions */

static void test_address_error_empty_string(void)
{
    assert(expect_exit_failure(try_bind_empty_address) == 1);
}

static void test_address_error_missing_port(void)
{
    assert(expect_exit_failure(try_bind_missing_port) == 1);
}

static void test_address_error_invalid_port_letters(void)
{
    assert(expect_exit_failure(try_bind_invalid_port_letters) == 1);
}

static void test_address_error_port_too_high(void)
{
    assert(expect_exit_failure(try_bind_port_too_high) == 1);
}

static void test_address_error_port_negative(void)
{
    assert(expect_exit_failure(try_bind_port_negative) == 1);
}

static void test_address_error_ipv6_missing_bracket(void)
{
    assert(expect_exit_failure(try_bind_ipv6_missing_bracket) == 1);
}

static void test_address_error_ipv6_missing_port(void)
{
    assert(expect_exit_failure(try_bind_ipv6_missing_port) == 1);
}

static void test_address_error_ipv6_no_colon(void)
{
    assert(expect_exit_failure(try_bind_ipv6_no_colon_after_bracket) == 1);
}

static void test_address_error_empty_port(void)
{
    assert(expect_exit_failure(try_bind_empty_port) == 1);
}

/* Test UDP address errors too */

static void try_udp_bind_empty_address(void)
{
    RtArena *arena = rt_arena_create(NULL);
    rt_udp_socket_bind(arena, "");
    rt_arena_destroy(arena);
}

static void try_udp_bind_missing_port(void)
{
    RtArena *arena = rt_arena_create(NULL);
    rt_udp_socket_bind(arena, "127.0.0.1");
    rt_arena_destroy(arena);
}

static void test_udp_address_error_empty_string(void)
{
    assert(expect_exit_failure(try_udp_bind_empty_address) == 1);
}

static void test_udp_address_error_missing_port(void)
{
    assert(expect_exit_failure(try_udp_bind_missing_port) == 1);
}

#endif  /* _WIN32 - end of fork-based tests */

/* ============================================================================
 * Test Main Entry Point
 * ============================================================================ */

void test_rt_net_main(void)
{
    TEST_SECTION("Network I/O");

    /* TcpListener bind tests - IPv4 */
    TEST_RUN("tcp_listener_bind_ipv4", test_rt_tcp_listener_bind_ipv4);
    TEST_RUN("tcp_listener_bind_port_only", test_rt_tcp_listener_bind_port_only);
    TEST_RUN("tcp_listener_bind_os_assigned_port", test_rt_tcp_listener_bind_os_assigned_port);
    TEST_RUN("tcp_listener_bind_multiple_listeners", test_rt_tcp_listener_bind_multiple_listeners);
    TEST_RUN("tcp_listener_bind_localhost_alias", test_rt_tcp_listener_bind_localhost_alias);
    TEST_RUN("tcp_listener_close_idempotent", test_rt_tcp_listener_close_idempotent);
    TEST_RUN("tcp_listener_port_range", test_rt_tcp_listener_port_range);

    /* TcpListener bind tests - IPv6 */
    TEST_RUN("tcp_listener_bind_ipv6", test_rt_tcp_listener_bind_ipv6);
    TEST_RUN("tcp_listener_bind_ipv6_specific_port", test_rt_tcp_listener_bind_ipv6_specific_port);

    /* TcpListener bind tests - hostname */
    TEST_RUN("tcp_listener_bind_hostname", test_rt_tcp_listener_bind_hostname);
    TEST_RUN("tcp_listener_bind_hostname_specific_port", test_rt_tcp_listener_bind_hostname_specific_port);

    /* TcpListener accept tests */
    TEST_RUN("tcp_listener_accept_basic", test_rt_tcp_listener_accept_basic);
    TEST_RUN("tcp_listener_accept_has_remote_address", test_rt_tcp_listener_accept_has_remote_address);
    TEST_RUN("tcp_listener_accept_multiple_connections", test_rt_tcp_listener_accept_multiple_connections);

    /* TcpListener close tests */
    TEST_RUN("tcp_listener_close_basic", test_rt_tcp_listener_close_basic);
    TEST_RUN("tcp_listener_close_multiple_times", test_rt_tcp_listener_close_multiple_times);
    TEST_RUN("tcp_listener_close_releases_port", test_rt_tcp_listener_close_releases_port);

    TEST_RUN("tcp_stream_connect_basic", test_rt_tcp_stream_connect_basic);
    TEST_RUN("tcp_stream_connect_has_valid_fd", test_rt_tcp_stream_connect_has_valid_fd);
    TEST_RUN("tcp_stream_connect_has_remote_address", test_rt_tcp_stream_connect_has_remote_address);
    TEST_RUN("tcp_stream_connect_localhost_hostname", test_rt_tcp_stream_connect_localhost_hostname);

    /* TcpStream close tests */
    TEST_RUN("tcp_stream_close_basic", test_rt_tcp_stream_close_basic);
    TEST_RUN("tcp_stream_close_multiple_times", test_rt_tcp_stream_close_multiple_times);

    /* TcpStream read/write tests */
    TEST_RUN("tcp_stream_write_basic", test_rt_tcp_stream_write_basic);
    TEST_RUN("tcp_stream_read_basic", test_rt_tcp_stream_read_basic);
    TEST_RUN("tcp_stream_read_all", test_rt_tcp_stream_read_all);
    TEST_RUN("tcp_stream_read_line", test_rt_tcp_stream_read_line);
    TEST_RUN("tcp_stream_write_line", test_rt_tcp_stream_write_line);

    /* UdpSocket bind tests */
    TEST_RUN("udp_socket_bind_ipv4", test_rt_udp_socket_bind_ipv4);
    TEST_RUN("udp_socket_bind_port_only", test_rt_udp_socket_bind_port_only);
    TEST_RUN("udp_socket_bind_os_assigned_port", test_rt_udp_socket_bind_os_assigned_port);
    TEST_RUN("udp_socket_bind_multiple", test_rt_udp_socket_bind_multiple);

    /* UdpSocket close tests */
    TEST_RUN("udp_socket_close_basic", test_rt_udp_socket_close_basic);
    TEST_RUN("udp_socket_close_multiple_times", test_rt_udp_socket_close_multiple_times);
    TEST_RUN("udp_socket_close_releases_port", test_rt_udp_socket_close_releases_port);

    /* UdpSocket send/receive tests */
    TEST_RUN("udp_socket_send_receive_basic", test_rt_udp_socket_send_receive_basic);
    TEST_RUN("udp_socket_receive_from_sender_address", test_rt_udp_socket_receive_from_sender_address);
    TEST_RUN("udp_socket_data_integrity", test_rt_udp_socket_data_integrity);
    TEST_RUN("udp_socket_multiple_datagrams", test_rt_udp_socket_multiple_datagrams);
    TEST_RUN("udp_socket_empty_datagram", test_rt_udp_socket_empty_datagram);

    /* Address parsing error tests (use fork to test exit behavior) */
#ifndef _WIN32  /* fork-based tests only on POSIX */
    TEST_RUN("address_error_empty_string", test_address_error_empty_string);
    TEST_RUN("address_error_missing_port", test_address_error_missing_port);
    TEST_RUN("address_error_invalid_port_letters", test_address_error_invalid_port_letters);
    TEST_RUN("address_error_port_too_high", test_address_error_port_too_high);
    TEST_RUN("address_error_port_negative", test_address_error_port_negative);
    TEST_RUN("address_error_ipv6_missing_bracket", test_address_error_ipv6_missing_bracket);
    TEST_RUN("address_error_ipv6_missing_port", test_address_error_ipv6_missing_port);
    TEST_RUN("address_error_ipv6_no_colon", test_address_error_ipv6_no_colon);
    TEST_RUN("address_error_empty_port", test_address_error_empty_port);
    TEST_RUN("udp_address_error_empty_string", test_udp_address_error_empty_string);
    TEST_RUN("udp_address_error_missing_port", test_udp_address_error_missing_port);
#endif
}
