# Network I/O in Sindarin

> **DRAFT - NOT IMPLEMENTED** - This document specifies the planned network I/O API. It describes the target design; implementation has not yet begun.

Sindarin provides built-in types for network communication: `TcpListener` and `TcpStream` for TCP connections, `UdpSocket` for UDP datagrams, and `UnixListener`/`UnixStream` for local inter-process communication. All network types integrate with Sindarin's arena-based memory management.

---

## Overview

```sindarin
// TCP Server
var server: TcpListener = TcpListener.bind(":8080")
print("Listening on port 8080\n")

while true =>
    var client: TcpStream = server.accept()
    print($"Connection from {client.remoteAddress}\n")
    client.writeLine("Hello!")
    client.close()

// TCP Client
var connection: TcpStream = TcpStream.connect("example.com:80")
connection.writeLine("GET / HTTP/1.0")
connection.writeLine("")
var response: byte[] = connection.readAll()
print(response.toString())
connection.close()
```

> **Threading:** All blocking network operations work naturally with Sindarin's threading model. Use `&` to spawn I/O operations in background threads and `!` to synchronize. See the Threading section for details.

---

## TcpListener

A TCP socket that listens for incoming connections. Used to build servers.

### Static Methods

#### TcpListener.bind(address)

Creates a listener bound to the specified address. The listener immediately begins accepting connections.

```sindarin
// Bind to specific interface and port
var server: TcpListener = TcpListener.bind("127.0.0.1:8080")

// Bind to all interfaces
var server: TcpListener = TcpListener.bind("0.0.0.0:8080")
var server: TcpListener = TcpListener.bind(":8080")  // Shorthand

// Let OS assign an available port
var server: TcpListener = TcpListener.bind(":0")
print($"Assigned port: {server.port}\n")
```

#### TcpListener.bindWithBacklog(address, backlog)

Creates a listener with a custom connection backlog (queue size for pending connections).

```sindarin
// High-traffic server with larger backlog
var server: TcpListener = TcpListener.bindWithBacklog(":8080", 1024)
```

### Instance Methods

#### accept()

Waits for and accepts an incoming connection. Returns a `TcpStream` for the new connection.

```sindarin
var server: TcpListener = TcpListener.bind(":8080")

while true =>
    var client: TcpStream = server.accept()  // Blocks until connection
    handleClient(client)
    client.close()
```

#### acceptWithAddress()

Accepts a connection and returns both the stream and the client's address.

```sindarin
var client: TcpStream
var clientAddress: str
client, clientAddress = server.acceptWithAddress()
print($"Connection from {clientAddress}\n")
```

#### close()

Closes the listener and stops accepting new connections.

```sindarin
server.close()
```

### Properties

```sindarin
var address: str = server.localAddress   // "127.0.0.1:8080"
var port: int = server.port              // 8080
var open: bool = server.isOpen           // true if listener is active
```

---

## TcpStream

A TCP connection for bidirectional byte stream communication. Obtained from `TcpListener.accept()` or `TcpStream.connect()`.

### Static Methods

#### TcpStream.connect(address)

Connects to a remote TCP server.

```sindarin
var connection: TcpStream = TcpStream.connect("example.com:80")
var connection: TcpStream = TcpStream.connect("192.168.1.1:8080")
var connection: TcpStream = TcpStream.connect("[::1]:8080")  // IPv6
```

#### TcpStream.connectWithTimeout(address, timeoutMilliseconds)

Connects with a timeout. Panics if connection cannot be established within the timeout.

```sindarin
// 5 second timeout
var connection: TcpStream = TcpStream.connectWithTimeout("example.com:80", 5000)
```

### Instance Methods - Reading

#### read(maxBytes)

Reads up to `maxBytes` bytes. Returns immediately when any data is available (may return fewer bytes than requested).

```sindarin
var data: byte[] = connection.read(1024)
print($"Received {data.length} bytes\n")
```

#### readExact(count)

Reads exactly `count` bytes. Blocks until all bytes are received or connection closes.

```sindarin
var header: byte[] = connection.readExact(4)  // Read 4-byte header
```

#### readInto(buffer)

Reads into an existing buffer. Returns the number of bytes read.

```sindarin
var buffer: byte[] = byte[4096]{}
var bytesRead: int = connection.readInto(buffer)
// buffer[0..bytesRead] contains the data
```

#### readAll()

Reads all remaining data until the connection closes.

```sindarin
var response: byte[] = connection.readAll()
```

#### readLine()

Reads until a newline character (`\n`). Returns the line without the newline.

```sindarin
var line: str = connection.readLine()
```

#### readUntil(delimiter)

Reads until the specified byte delimiter is encountered.

```sindarin
var data: byte[] = connection.readUntil(0)  // Read until null byte
```

### Instance Methods - Writing

#### write(data)

Writes bytes to the connection. Returns the number of bytes written (may be less than `data.length`).

```sindarin
var bytesSent: int = connection.write(data)
```

#### writeAll(data)

Writes all bytes, blocking until complete.

```sindarin
connection.writeAll(data)
```

#### writeString(text)

Writes a string as UTF-8 bytes.

```sindarin
connection.writeString("Hello, World!")
```

#### writeLine(text)

Writes a string followed by a newline.

```sindarin
connection.writeLine("HTTP/1.0 200 OK")
connection.writeLine("Content-Type: text/plain")
connection.writeLine("")  // Empty line ends headers
```

#### flush()

Flushes any buffered output data to the network.

```sindarin
connection.writeString("Important data")
connection.flush()
```

### Instance Methods - Control

#### close()

Closes the connection.

```sindarin
connection.close()
```

#### shutdown()

Shuts down both reading and writing.

```sindarin
connection.shutdown()
```

#### shutdownRead()

Shuts down the read half. Further reads return empty.

```sindarin
connection.shutdownRead()
```

#### shutdownWrite()

Shuts down the write half. Signals EOF to the remote peer.

```sindarin
connection.shutdownWrite()  // Signal we're done sending
var response: byte[] = connection.readAll()  // But still read response
```

### Instance Methods - Options

#### setNoDelay(enabled)

Enables or disables Nagle's algorithm. When enabled (`true`), data is sent immediately without waiting to batch small packets.

```sindarin
connection.setNoDelay(true)  // Low latency, disable batching
```

#### setReadTimeout(milliseconds)

Sets the read timeout in milliseconds. `0` means no timeout.

```sindarin
connection.setReadTimeout(5000)  // 5 second timeout
```

#### setWriteTimeout(milliseconds)

Sets the write timeout in milliseconds.

```sindarin
connection.setWriteTimeout(5000)
```

#### setKeepAlive(enabled)

Enables TCP keep-alive probes.

```sindarin
connection.setKeepAlive(true)
```

### Properties

```sindarin
var localAddress: str = connection.localAddress    // "192.168.1.100:54321"
var remoteAddress: str = connection.remoteAddress  // "93.184.216.34:80"
var isOpen: bool = connection.isOpen               // true if connection is active
```

---

## UdpSocket

A UDP socket for connectionless datagram communication.

### Static Methods

#### UdpSocket.bind(address)

Creates a UDP socket bound to the specified local address.

```sindarin
var socket: UdpSocket = UdpSocket.bind(":9000")      // All interfaces, port 9000
var socket: UdpSocket = UdpSocket.bind(":0")         // OS-assigned port
var socket: UdpSocket = UdpSocket.bind("127.0.0.1:9000")
```

### Instance Methods - Sending

#### sendTo(data, address)

Sends a datagram to the specified address. Returns the number of bytes sent.

```sindarin
var socket: UdpSocket = UdpSocket.bind(":0")
var bytesSent: int = socket.sendTo("Hello".toBytes(), "192.168.1.1:9000")
```

#### send(data)

Sends to the connected address (see `connectTo`). Panics if not connected.

```sindarin
socket.connectTo("192.168.1.1:9000")
socket.send("Hello".toBytes())
```

### Instance Methods - Receiving

#### receiveFrom(maxBytes)

Receives a datagram and the sender's address.

```sindarin
var data: byte[]
var address: str
data, address = socket.receiveFrom(1024)
print($"Received {data.length} bytes from {address}\n")
```

#### receive(maxBytes)

Receives from the connected address only (see `connectTo`).

```sindarin
socket.connectTo("192.168.1.1:9000")
var data: byte[] = socket.receive(1024)
```

### Instance Methods - Connection

#### connectTo(address)

Sets a default destination address. After connecting:
- `send()` sends to this address
- `receive()` only receives from this address

```sindarin
var socket: UdpSocket = UdpSocket.bind(":0")
socket.connectTo("8.8.8.8:53")  // DNS server
socket.send(dnsQuery)
var response: byte[] = socket.receive(512)
```

### Instance Methods - Options

#### setBroadcast(enabled)

Enables sending to broadcast addresses.

```sindarin
socket.setBroadcast(true)
socket.sendTo(data, "255.255.255.255:9000")
```

#### setMulticastTTL(timeToLive)

Sets the time-to-live for multicast packets.

```sindarin
socket.setMulticastTTL(64)
```

#### joinMulticast(group)

Joins a multicast group to receive multicast packets.

```sindarin
socket.joinMulticast("224.0.0.1")
```

#### leaveMulticast(group)

Leaves a multicast group.

```sindarin
socket.leaveMulticast("224.0.0.1")
```

#### close()

Closes the socket.

```sindarin
socket.close()
```

### Properties

```sindarin
var address: str = socket.localAddress   // "0.0.0.0:9000"
var port: int = socket.port              // 9000
var open: bool = socket.isOpen
```

---

## UnixListener

A Unix domain socket listener for local inter-process communication (IPC). More efficient than TCP for same-machine communication.

### Static Methods

#### UnixListener.bind(path)

Creates a listener at the specified filesystem path.

```sindarin
var server: UnixListener = UnixListener.bind("/tmp/myapp.sock")
```

### Instance Methods

#### accept()

Accepts an incoming connection.

```sindarin
var client: UnixStream = server.accept()
```

#### close()

Closes the listener. Does not remove the socket file.

```sindarin
server.close()
```

#### remove()

Closes the listener and removes the socket file.

```sindarin
server.remove()
```

### Properties

```sindarin
var path: str = server.path   // "/tmp/myapp.sock"
var open: bool = server.isOpen
```

---

## UnixStream

A Unix domain socket connection. Supports the same read/write methods as `TcpStream`.

### Static Methods

#### UnixStream.connect(path)

Connects to a Unix domain socket.

```sindarin
var conn: UnixStream = UnixStream.connect("/tmp/myapp.sock")
```

### Instance Methods

All read/write methods are identical to `TcpStream`:

```sindarin
connection.writeString("Hello")
var response: str = connection.readLine()
connection.close()
```

### Properties

```sindarin
var path: str = conn.path
var open: bool = conn.isOpen
```

---

## UnixDatagram

Unix domain datagram socket for connectionless local IPC.

### Static Methods

#### UnixDatagram.bind(path)

Creates a datagram socket at the specified path.

```sindarin
var sock: UnixDatagram = UnixDatagram.bind("/tmp/myapp-dgram.sock")
```

### Instance Methods

#### sendTo(data, path)

Sends a datagram to another Unix socket.

```sindarin
socket.sendTo("Hello".toBytes(), "/tmp/other.sock")
```

#### receiveFrom(maxBytes)

Receives a datagram and the sender's path.

```sindarin
var data: byte[]
var senderPath: str
data, senderPath = socket.receiveFrom(1024)
```

#### close() / remove()

Same as `UnixListener`.

---

## Dns

DNS resolution utilities.

### Static Methods

#### Dns.lookup(hostname)

Resolves a hostname to IP addresses.

```sindarin
var addresses: str[] = Dns.lookup("example.com")
// ["93.184.216.34", "2606:2800:220:1:248:1893:25c8:1946"]

for address in addresses =>
    print($"  {address}\n")
```

#### Dns.lookupIPv4(hostname)

Resolves to IPv4 addresses only.

```sindarin
var addresses: str[] = Dns.lookupIPv4("example.com")
// ["93.184.216.34"]
```

#### Dns.lookupIPv6(hostname)

Resolves to IPv6 addresses only.

```sindarin
var addresses: str[] = Dns.lookupIPv6("example.com")
// ["2606:2800:220:1:248:1893:25c8:1946"]
```

#### Dns.reverse(ipAddress)

Performs reverse DNS lookup.

```sindarin
var hostname: str = Dns.reverse("8.8.8.8")
// "dns.google"
```

---

## Net (Polling)

Utilities for multiplexing I/O on multiple sockets without threads.

### Static Methods

#### Net.poll(sockets, timeoutMilliseconds)

Waits for I/O readiness on multiple sockets. Returns when at least one socket is ready or timeout expires.

```sindarin
var server: TcpListener = TcpListener.bind(":8080")
var clients: TcpStream[] = TcpStream[]{}

while true =>
    // Build list of sockets to poll
    var pollList: Pollable[] = Pollable[]{server}
    for client in clients =>
        pollList.push(client)

    // Wait for activity (1 second timeout)
    var ready: PollResult[] = Net.poll(pollList, 1000)

    for result in ready =>
        if result.socket == server =>
            // New connection
            var client: TcpStream = server.accept()
            clients.push(client)
        else if result.canRead =>
            // Data available
            handleClient(result.socket)
```

### PollResult

Result from `Net.poll()`:

```sindarin
var socket: Pollable = result.socket   // The socket that's ready
var canRead: bool = result.canRead     // Data available to read
var canWrite: bool = result.canWrite   // Can write without blocking
var hasError: bool = result.hasError   // Error condition
```

---

## Common Patterns

### Echo Server

```sindarin
fn main(): int =>
    var server: TcpListener = TcpListener.bind(":8080")
    print("Echo server listening on :8080\n")

    while true =>
        var client: TcpStream = server.accept()
        print($"Client connected: {client.remoteAddress}\n")

        while client.isOpen =>
            var data: byte[] = client.read(1024)
            if data.length == 0 =>
                break
            client.writeAll(data)

        client.close()
        print("Client disconnected\n")

    return 0
```

### HTTP GET Request

```sindarin
fn httpGet(host: str, path: str): str =>
    var connection: TcpStream = TcpStream.connect($"{host}:80")

    // Send HTTP request
    connection.writeLine($"GET {path} HTTP/1.0")
    connection.writeLine($"Host: {host}")
    connection.writeLine("Connection: close")
    connection.writeLine("")
    connection.flush()

    // Read response
    var response: byte[] = connection.readAll()
    connection.close()

    return response.toString()

fn main(): int =>
    var html: str = httpGet("example.com", "/")
    print(html)
    return 0
```

### UDP Echo

```sindarin
fn udpEchoServer(): void =>
    var socket: UdpSocket = UdpSocket.bind(":9000")
    print("UDP echo server on :9000\n")

    while true =>
        var data: byte[]
        var address: str
        data, address = socket.receiveFrom(1024)
        print($"Received from {address}: {data.toString()}\n")
        socket.sendTo(data, address)

fn udpClient(): void =>
    var socket: UdpSocket = UdpSocket.bind(":0")

    socket.sendTo("Hello, UDP!".toBytes(), "127.0.0.1:9000")

    var response: byte[]
    var address: str
    response, address = socket.receiveFrom(1024)
    print($"Response: {response.toString()}\n")

    socket.close()
```

### Line-Based Protocol

```sindarin
fn handleClient(client: TcpStream): void =>
    client.writeLine("Welcome! Commands: HELLO, TIME, QUIT")

    while client.isOpen =>
        var line: str = client.readLine().trim().toUpper()

        if line == "HELLO" =>
            client.writeLine("Hello to you too!")
        else if line == "TIME" =>
            var now: Time = Time.now()
            client.writeLine($"Current time: {now.toIso()}")
        else if line == "QUIT" =>
            client.writeLine("Goodbye!")
            break
        else =>
            client.writeLine($"Unknown command: {line}")

    client.close()
```

### Connection Pool Pattern

```sindarin
fn withConnection(host: str, callback: fn(connection: TcpStream): void): void =>
    var connection: TcpStream = TcpStream.connect(host)
    callback(connection)
    connection.close()

// Usage
withConnection("api.example.com:80", fn(connection: TcpStream): void =>
    connection.writeLine("GET /data HTTP/1.0")
    connection.writeLine("")
    var response: byte[] = connection.readAll()
    processResponse(response)
)
```

### Timeout with Polling

```sindarin
fn readWithTimeout(connection: TcpStream, maxBytes: int, timeoutMilliseconds: int): byte[] =>
    var ready: PollResult[] = Net.poll(Pollable[]{connection}, timeoutMilliseconds)

    if ready.length == 0 =>
        panic("Read timeout")

    return connection.read(maxBytes)
```

### Multi-Client Server with Polling

```sindarin
fn main(): int =>
    var server: TcpListener = TcpListener.bind(":8080")
    var clients: TcpStream[] = TcpStream[]{}

    while true =>
        // Prepare poll list
        var pollList: Pollable[] = Pollable[]{server}
        for client in clients =>
            pollList.push(client)

        var ready: PollResult[] = Net.poll(pollList, 1000)

        for result in ready =>
            if result.socket == server =>
                var client: TcpStream = server.accept()
                clients.push(client)
                print($"New client: {client.remoteAddress}\n")

            else if result.canRead =>
                var client: TcpStream = result.socket as TcpStream
                var data: byte[] = client.read(1024)

                if data.length == 0 =>
                    client.close()
                    clients.remove(client)
                else =>
                    // Echo back
                    client.writeAll(data)

    return 0
```

---

## Memory Management

Network handles integrate with Sindarin's arena-based memory management, following the same patterns as file handles.

### Automatic Cleanup

Connections are closed when their arena is destroyed:

```sindarin
fn fetchData(host: str): byte[] =>
    var connection: TcpStream = TcpStream.connect(host)
    var data: byte[] = connection.readAll()
    // connection automatically closed when function returns
    return data
```

### Private Blocks

Use private blocks for guaranteed cleanup:

```sindarin
var result: byte[] = byte[]{}
private =>
    var connection: TcpStream = TcpStream.connect("api.example.com:80")
    connection.writeLine("GET /data HTTP/1.0")
    connection.writeLine("")
    result = connection.readAll()
    // connection GUARANTEED to close here, even on panic
```

### Handle Promotion

Network handles can be returned from functions:

```sindarin
fn createConnection(host: str): TcpStream =>
    var connection: TcpStream = TcpStream.connect(host)
    connection.setNoDelay(true)
    connection.setKeepAlive(true)
    return connection  // Handle promoted to caller's arena
```

---

## Error Handling

Network operations panic on errors. Check conditions before operations when possible:

```sindarin
// DNS resolution may fail
var addresses: str[] = Dns.lookup("example.com")
if addresses.length == 0 =>
    print("DNS resolution failed\n")
    return

// Connection may be refused
var connection: TcpStream = TcpStream.connect($"{addresses[0]}:80")
```

Panic conditions include:
- `TcpStream.connect()` - Connection refused, timeout, DNS failure
- `TcpListener.bind()` - Address already in use, permission denied
- `connection.read()` / `connection.write()` - Connection reset, broken pipe
- `Dns.lookup()` - DNS resolution failure

Future versions may introduce a `Result` type for recoverable errors:

```sindarin
// Future syntax (not implemented)
var result: Result<TcpStream> = TcpStream.tryConnect("example.com:80")
if result.isOk() =>
    var connection: TcpStream = result.unwrap()
else =>
    print($"Connection failed: {result.error()}\n")
```

---

## Address Format

All address strings use the format `host:port`:

| Format | Description |
|--------|-------------|
| `"127.0.0.1:8080"` | IPv4 with port |
| `"[::1]:8080"` | IPv6 with port (brackets required) |
| `":8080"` | All interfaces, specific port |
| `":0"` | All interfaces, OS-assigned port |
| `"example.com:80"` | Hostname (resolved via DNS) |
| `"0.0.0.0:8080"` | Explicit all IPv4 interfaces |
| `"[::]:8080"` | Explicit all IPv6 interfaces |

---

## Threading

Network operations integrate naturally with Sindarin's threading model. Use `&` to spawn I/O in background threads and `!` to synchronize.

### Parallel Requests

```sindarin
fn httpGet(host: str, path: str): str =>
    var conn: TcpStream = TcpStream.connect($"{host}:80")
    conn.writeLine($"GET {path} HTTP/1.0")
    conn.writeLine($"Host: {host}")
    conn.writeLine("")
    var response: byte[] = conn.readAll()
    conn.close()
    return response.toString()

// Spawn concurrent requests
var r1: str = &httpGet("api1.example.com", "/users")
var r2: str = &httpGet("api2.example.com", "/orders")
var r3: str = &httpGet("api3.example.com", "/products")

// Wait for all to complete
[r1, r2, r3]!

print($"Got {r1.length + r2.length + r3.length} bytes total\n")
```

### Threaded Server

```sindarin
fn handleClient(client: TcpStream): void =>
    while client.isOpen =>
        var data: byte[] = client.read(1024)
        if data.length == 0 => break
        client.writeAll(data)
    client.close()

fn main(): int =>
    var server: TcpListener = TcpListener.bind(":8080")
    print("Server listening on :8080\n")

    while true =>
        var client: TcpStream = server.accept()
        &handleClient(client)  // fire and forget - each client in own thread

    return 0
```

### Background DNS

```sindarin
var a1: str[] = &Dns.lookup("example.com")
var a2: str[] = &Dns.lookup("google.com")
var a3: str[] = &Dns.lookup("github.com")

[a1, a2, a3]!

for addr in a1 => print($"example.com: {addr}\n")
```

### Inline Sync

```sindarin
// Spawn and immediately wait
var conn: TcpStream = &TcpStream.connect("slow-server.com:80")!

// Use in expressions
var total: int = &fetchSize("a.com")! + &fetchSize("b.com")!
```

### Memory Semantics

Network handles follow the same frozen reference rules as other types:

```sindarin
var conn: TcpStream = TcpStream.connect("example.com:80")
var response: byte[] = &conn.readAll()  // conn frozen

conn.write(data)  // COMPILE ERROR: conn frozen

response!         // sync releases freeze
conn.close()      // OK
```

---

## Implementation Checklist

### Phase 1: Core TCP
- [ ] `TcpListener` type with `bind`, `accept`, `close`
- [ ] `TcpStream` type with `connect`, `read`, `write`, `close`
- [ ] Basic socket options (`setNoDelay`, `setKeepAlive`)
- [ ] Runtime integration with arena memory management

### Phase 2: Extended TCP
- [ ] Timeouts (`setReadTimeout`, `setWriteTimeout`, `connectWithTimeout`)
- [ ] Shutdown operations (`shutdown`, `shutdownRead`, `shutdownWrite`)
- [ ] Additional read methods (`readLine`, `readExact`, `readAll`)
- [ ] Additional write methods (`writeLine`, `writeString`, `flush`)

### Phase 3: UDP
- [ ] `UdpSocket` type with `bind`, `sendTo`, `receiveFrom`
- [ ] Connected UDP (`connectTo`, `send`, `receive`)
- [ ] Multicast support (`joinMulticast`, `leaveMulticast`)
- [ ] Broadcast support (`setBroadcast`)

### Phase 4: Unix Sockets
- [ ] `UnixListener` and `UnixStream` types
- [ ] `UnixDatagram` type
- [ ] Socket file cleanup (`remove`)

### Phase 5: Utilities
- [ ] `Dns` module (`lookup`, `lookupIPv4`, `lookupIPv6`, `reverse`)
- [ ] `Net.poll` for I/O multiplexing
- [ ] `PollResult` type

### Deferred
- [ ] `Result` type for recoverable errors
- [ ] TLS/SSL support

---

## Revision History

| Date | Changes |
|------|---------|
| 2025-01-02 | Initial draft with comprehensive API design |
| 2025-01-02 | Replaced async/await with threading model |

---

## See Also

- [THREADING.md](THREADING.md) - Threading model (`&` spawn, `!` sync)
- [FILE_IO.md](../language/FILE_IO.md) - File I/O operations
- [MEMORY.md](../language/MEMORY.md) - Arena memory management
- [TYPES.md](../language/TYPES.md) - Built-in types
