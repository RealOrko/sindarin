# Network I/O in Sindarin

Sindarin provides built-in types for network communication: `TcpListener` and `TcpStream` for TCP connections, and `UdpSocket` for UDP datagrams. All network types integrate with Sindarin's arena-based memory management and threading model.

## TCP

### TcpListener

A TCP socket that listens for incoming connections.

```sindarin
var server: TcpListener = TcpListener.bind(":8080")
print($"Listening on port {server.port}\n")

while true =>
    var client: TcpStream = server.accept()
    &handleClient(client)  // handle in background thread

server.close()
```

#### Static Methods

| Method | Return | Description |
|--------|--------|-------------|
| `TcpListener.bind(address)` | `TcpListener` | Create listener bound to address |

#### Instance Methods

| Method | Return | Description |
|--------|--------|-------------|
| `.accept()` | `TcpStream` | Wait for and accept a connection |
| `.close()` | `void` | Close the listener |

#### Properties

| Property | Type | Description |
|----------|------|-------------|
| `.port` | `int` | Bound port number |

---

### TcpStream

A TCP connection for bidirectional communication.

```sindarin
// Client connection
var conn: TcpStream = TcpStream.connect("example.com:80")
conn.writeLine("GET / HTTP/1.0")
conn.writeLine("Host: example.com")
conn.writeLine("")
var response: byte[] = conn.readAll()
print(response.toString())
conn.close()
```

#### Static Methods

| Method | Return | Description |
|--------|--------|-------------|
| `TcpStream.connect(address)` | `TcpStream` | Connect to remote address |

#### Instance Methods

| Method | Return | Description |
|--------|--------|-------------|
| `.read(maxBytes)` | `byte[]` | Read up to maxBytes (may return fewer) |
| `.readAll()` | `byte[]` | Read until connection closes |
| `.readLine()` | `str` | Read until newline |
| `.write(data)` | `int` | Write bytes, return count written |
| `.writeLine(text)` | `void` | Write string + newline |
| `.close()` | `void` | Close the connection |

#### Properties

| Property | Type | Description |
|----------|------|-------------|
| `.remoteAddress` | `str` | Remote peer address |

---

## UDP

### UdpSocket

A UDP socket for connectionless datagram communication.

```sindarin
// Server
var socket: UdpSocket = UdpSocket.bind(":9000")

while true =>
    var data: byte[]
    var sender: str
    data, sender = socket.receiveFrom(1024)
    print($"From {sender}: {data.toString()}\n")
    socket.sendTo(data, sender)  // echo back

// Client
var socket: UdpSocket = UdpSocket.bind(":0")  // OS assigns port
socket.sendTo("Hello".toBytes(), "127.0.0.1:9000")

var response: byte[]
var from: str
response, from = socket.receiveFrom(1024)
print(response.toString())
socket.close()
```

#### Static Methods

| Method | Return | Description |
|--------|--------|-------------|
| `UdpSocket.bind(address)` | `UdpSocket` | Create socket bound to address |

#### Instance Methods

| Method | Return | Description |
|--------|--------|-------------|
| `.sendTo(data, address)` | `int` | Send datagram, return bytes sent |
| `.receiveFrom(maxBytes)` | `byte[], str` | Receive datagram and sender address |
| `.close()` | `void` | Close the socket |

#### Properties

| Property | Type | Description |
|----------|------|-------------|
| `.port` | `int` | Bound port number |

---

## Address Format

All addresses use `host:port` format:

| Format | Description |
|--------|-------------|
| `"127.0.0.1:8080"` | IPv4 with port |
| `"[::1]:8080"` | IPv6 with port |
| `":8080"` | All interfaces, specific port |
| `":0"` | All interfaces, OS-assigned port |
| `"example.com:80"` | Hostname (resolved via DNS) |

---

## Threading

Network operations integrate with Sindarin's threading model using `&` and `!`.

### Parallel Connections

```sindarin
var c1: TcpStream = &TcpStream.connect("api1.example.com:80")
var c2: TcpStream = &TcpStream.connect("api2.example.com:80")
var c3: TcpStream = &TcpStream.connect("api3.example.com:80")

[c1, c2, c3]!

// All connected, use them...
```

### Threaded Server

```sindarin
fn handleClient(client: TcpStream): void =>
    var line: str = client.readLine()
    client.writeLine($"You said: {line}")
    client.close()

fn main(): int =>
    var server: TcpListener = TcpListener.bind(":8080")

    while true =>
        var client: TcpStream = server.accept()
        &handleClient(client)  // fire and forget

    return 0
```

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

// Fetch in parallel
var r1: str = &httpGet("example.com", "/page1")
var r2: str = &httpGet("example.com", "/page2")
var r3: str = &httpGet("example.com", "/page3")

[r1, r2, r3]!

print($"Total bytes: {r1.length + r2.length + r3.length}\n")
```

### Background UDP

```sindarin
fn listenForMessages(socket: UdpSocket): void =>
    while true =>
        var data: byte[]
        var sender: str
        data, sender = socket.receiveFrom(1024)
        print($"Message from {sender}: {data.toString()}\n")

var socket: UdpSocket = UdpSocket.bind(":9000")
&listenForMessages(socket)  // background listener

// Main thread continues...
```

---

## Common Patterns

### Echo Server

```sindarin
fn main(): int =>
    var server: TcpListener = TcpListener.bind(":8080")
    print("Echo server on :8080\n")

    while true =>
        var client: TcpStream = server.accept()

        while true =>
            var data: byte[] = client.read(1024)
            if data.length == 0 =>
                break
            client.write(data)

        client.close()

    return 0
```

### Line-Based Protocol

```sindarin
fn handleClient(client: TcpStream): void =>
    client.writeLine("Welcome! Commands: PING, TIME, QUIT")

    while true =>
        var line: str = client.readLine().trim().toUpper()

        if line == "PING" =>
            client.writeLine("PONG")
        else if line == "TIME" =>
            client.writeLine(Time.now().toIso())
        else if line == "QUIT" =>
            client.writeLine("Goodbye!")
            break
        else =>
            client.writeLine($"Unknown: {line}")

    client.close()
```

### UDP Echo

```sindarin
fn main(): int =>
    var socket: UdpSocket = UdpSocket.bind(":9000")
    print("UDP echo on :9000\n")

    while true =>
        var data: byte[]
        var sender: str
        data, sender = socket.receiveFrom(1024)
        socket.sendTo(data, sender)

    return 0
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

    var conn: TcpStream = TcpStream.connect($"{host}:80")
    conn.writeLine($"GET {path} HTTP/1.0")
    conn.writeLine($"Host: {host}")
    conn.writeLine("Connection: close")
    conn.writeLine("")

    var response: byte[] = conn.readAll()
    conn.close()
    return response.toString()
```

---

## Memory Management

Network handles integrate with arena-based memory management.

### Automatic Cleanup

```sindarin
fn fetchData(host: str): byte[] =>
    var conn: TcpStream = TcpStream.connect(host)
    var data: byte[] = conn.readAll()
    // conn automatically closed when function returns
    return data
```

### Handle Promotion

```sindarin
fn acceptClient(server: TcpListener): TcpStream =>
    var client: TcpStream = server.accept()
    return client  // promoted to caller's arena

fn main(): int =>
    var server: TcpListener = TcpListener.bind(":8080")
    var client: TcpStream = acceptClient(server)
    // client is valid here
    client.close()
    return 0
```

---

## Error Handling

Network operations panic on errors:

- `TcpStream.connect()` - Connection refused, DNS failure, timeout
- `TcpListener.bind()` - Address in use, permission denied
- `.read()` / `.write()` - Connection reset, broken pipe

```sindarin
// Connection may fail
var conn: TcpStream = TcpStream.connect("example.com:80")
// If we reach here, connection succeeded
```

---

## See Also

- [THREADING.md](THREADING.md) - Threading model (`&` spawn, `!` sync)
- [MEMORY.md](MEMORY.md) - Arena memory management
