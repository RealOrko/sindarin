# TCP

TCP provides connection-oriented, reliable communication using `SnTcpListener` for servers and `SnTcpStream` for connections.

## Import

```sindarin
import "sdk/net/tcp"
```

---

## SnTcpListener

A TCP socket that listens for incoming connections.

```sindarin
var server: SnTcpListener = SnTcpListener.bind(":8080")
print($"Listening on port {server.port()}\n")

while true =>
    var client: SnTcpStream = server.accept()
    &handleClient(client)  // handle in background thread

server.close()
```

### Static Methods

#### SnTcpListener.bind(address)

Creates a listener bound to the specified address.

```sindarin
// Bind to all interfaces on port 8080
var server: SnTcpListener = SnTcpListener.bind(":8080")

// Bind to localhost only
var local: SnTcpListener = SnTcpListener.bind("127.0.0.1:8080")

// Let OS assign a port
var dynamic: SnTcpListener = SnTcpListener.bind(":0")
print($"Assigned port: {dynamic.port()}\n")
```

### Instance Methods

#### accept()

Waits for and accepts an incoming connection. Returns a `SnTcpStream` for the connected client.

```sindarin
var client: SnTcpStream = server.accept()
print($"Client connected from {client.remoteAddress()}\n")
```

#### port()

Returns the bound port number. Useful when binding to `:0` for OS-assigned ports.

```sindarin
var server: SnTcpListener = SnTcpListener.bind(":0")
print($"Server listening on port {server.port()}\n")
```

#### close()

Closes the listener socket.

```sindarin
server.close()
```

---

## SnTcpStream

A TCP connection for bidirectional communication.

```sindarin
// Client connection
var conn: SnTcpStream = SnTcpStream.connect("example.com:80")
conn.writeLine("GET / HTTP/1.0")
conn.writeLine("Host: example.com")
conn.writeLine("")
var response: byte[] = conn.readAll()
print(response.toString())
conn.close()
```

### Static Methods

#### SnTcpStream.connect(address)

Connects to a remote address and returns a stream.

```sindarin
var conn: SnTcpStream = SnTcpStream.connect("example.com:80")
```

### Instance Methods

#### read(maxBytes)

Reads up to `maxBytes` from the stream. May return fewer bytes if less data is available.

```sindarin
var data: byte[] = conn.read(1024)
if data.length == 0 =>
    print("Connection closed\n")
```

#### readAll()

Reads all data until the connection closes.

```sindarin
var response: byte[] = conn.readAll()
print($"Received {response.length} bytes\n")
```

#### readLine()

Reads until a newline character, returning the line without the trailing newline.

```sindarin
var line: str = conn.readLine()
print($"Received: {line}\n")
```

#### write(data)

Writes bytes to the stream. Returns the number of bytes written.

```sindarin
var bytes: byte[] = "Hello".toBytes()
var written: int = conn.write(bytes)
```

#### writeLine(text)

Writes a string followed by a newline.

```sindarin
conn.writeLine("Hello, World!")
conn.writeLine($"Value: {x}")
```

#### remoteAddress()

Returns the remote peer's address as a string.

```sindarin
print($"Connected to: {conn.remoteAddress()}\n")
```

#### close()

Closes the connection.

```sindarin
conn.close()
```

---

## Common Patterns

### Echo Server

```sindarin
fn main(): int =>
    var server: SnTcpListener = SnTcpListener.bind(":8080")
    print("Echo server on :8080\n")

    while true =>
        var client: SnTcpStream = server.accept()

        while true =>
            var data: byte[] = client.read(1024)
            if data.length == 0 =>
                break
            client.write(data)

        client.close()

    return 0
```

### Threaded Server

```sindarin
fn handleClient(client: SnTcpStream): void =>
    var line: str = client.readLine()
    client.writeLine($"Echo: {line}")
    client.close()

fn main(): int =>
    var server: SnTcpListener = SnTcpListener.bind(":8080")

    while true =>
        var client: SnTcpStream = server.accept()
        &handleClient(client)

    return 0
```

### Line-Based Protocol

```sindarin
fn handleClient(client: SnTcpStream): void =>
    client.writeLine("Welcome! Commands: PING, TIME, QUIT")

    while true =>
        var line: str = client.readLine().trim().toUpper()

        if line == "PING" =>
            client.writeLine("PONG")
        else if line == "TIME" =>
            client.writeLine(SnTime.now().toIso())
        else if line == "QUIT" =>
            client.writeLine("Goodbye!")
            break
        else =>
            client.writeLine($"Unknown: {line}")

    client.close()
```

### Simple HTTP Request

```sindarin
fn httpGet(url: str): str =>
    // Parse "host/path" or just "host"
    var parts: str[] = url.split("/", 2)
    var host: str = parts[0]
    var path: str = "/"
    if parts.length > 1 =>
        path = $"/{parts[1]}"

    var conn: SnTcpStream = SnTcpStream.connect($"{host}:80")
    conn.writeLine($"GET {path} HTTP/1.0")
    conn.writeLine($"Host: {host}")
    conn.writeLine("Connection: close")
    conn.writeLine("")

    var response: byte[] = conn.readAll()
    conn.close()
    return response.toString()
```

### Parallel Connections

```sindarin
var c1: SnTcpStream = &SnTcpStream.connect("api1.example.com:80")
var c2: SnTcpStream = &SnTcpStream.connect("api2.example.com:80")
var c3: SnTcpStream = &SnTcpStream.connect("api3.example.com:80")

[c1, c2, c3]!

// All connected, use them...
c1.close()
c2.close()
c3.close()
```

---

## Error Handling

TCP operations panic on errors. Common error conditions:

- `SnTcpListener.bind()` - Address already in use, permission denied
- `SnTcpStream.connect()` - Connection refused, DNS failure, timeout
- `.read()` / `.write()` - Connection reset, broken pipe

```sindarin
// Connection may fail - will panic if it does
var conn: SnTcpStream = SnTcpStream.connect("example.com:80")
// If we reach here, connection succeeded
```

---

## See Also

- [Net Overview](readme.md) - Network I/O concepts
- [UDP](udp.md) - UDP socket operations
- [SDK Overview](../readme.md) - All SDK modules
- SDK source: `sdk/net/tcp.sn`
