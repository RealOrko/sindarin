# Process Execution in Sindarin

> **Note:** An SDK-based alternative `SnProcess` is available via `import "sdk/process"`. See [SDK Process documentation](../sdk/process.md) for the SDK version.

Sindarin provides the `Process` type for spawning external processes and capturing their output. Processes can run synchronously or asynchronously using the same `&` spawn and `!` sync operators as threads.

## Running Commands

### Basic Execution

Use `Process.run()` to execute a command:

```sindarin
var p: Process = Process.run("pwd")
print(p.stdout)      // prints current directory
print(p.exitCode)    // 0 on success
```

### Commands with Arguments

Pass arguments as a string array:

```sindarin
var p: Process = Process.run("ls", {"-la", "/tmp"})
print(p.stdout)
```

Arguments are passed directly to the executable—no shell interpretation occurs.

---

## Process Properties

Every `Process` has three read-only properties:

| Property | Type | Description |
|----------|------|-------------|
| `exitCode` | `int` | Exit code (0 typically means success) |
| `stdout` | `str` | Captured standard output |
| `stderr` | `str` | Captured standard error |

### Checking Success

```sindarin
var p: Process = Process.run("make", {"build"})

if p.exitCode == 0 =>
    print("Build succeeded\n")
    print(p.stdout)
else =>
    print("Build failed\n")
    print(p.stderr)
```

### Capturing Output

Stdout and stderr are captured independently:

```sindarin
var p: Process = Process.run("sh", {"-c", "echo out; echo err >&2"})

print(p.stdout)    // "out\n"
print(p.stderr)    // "err\n"
```

---

## Parallel Execution (`&`)

The `&` operator spawns processes asynchronously using threads. This integrates with Sindarin's threading model—see [THREADING.md](THREADING.md) for full details.

### Async Spawn

```sindarin
var p: Process = &Process.run("make", {"build"})
// ... do other work while process runs ...
p!                   // synchronize
print(p.stdout)      // access result
```

### Fire and Forget

Background processes without synchronization:

```sindarin
&Process.run("logger", {"Application started"})
// continues immediately, process runs in background
```

Fire-and-forget processes are terminated when the main program exits.

### Parallel Commands

Spawn multiple processes concurrently:

```sindarin
var p1: Process = &Process.run("echo", {"one"})
var p2: Process = &Process.run("echo", {"two"})
var p3: Process = &Process.run("echo", {"three"})

// All processes running in parallel

[p1, p2, p3]!       // wait for all to complete

print(p1.stdout)    // "one\n"
print(p2.stdout)    // "two\n"
print(p3.stdout)    // "three\n"
```

### Immediate Spawn-and-Wait

Combine spawn and sync for blocking calls that run in a thread:

```sindarin
var p: Process = &Process.run("make")!
print(p.stdout)
```

---

## Shell Commands

For pipes, redirection, or shell features, use `sh -c`:

```sindarin
// Using pipes
var p1: Process = Process.run("sh", {"-c", "ls -la | grep .txt"})
print(p1.stdout)

// Command substitution
var p2: Process = Process.run("sh", {"-c", "echo $(date)"})
print(p2.stdout)

// Multiple commands
var p3: Process = Process.run("sh", {"-c", "cd /tmp && pwd"})
print(p3.stdout)    // "/tmp\n"

// Environment variables
var p4: Process = Process.run("sh", {"-c", "X=value; echo $X"})
print(p4.stdout)    // "value\n"
```

---

## Exit Codes

### Success and Failure

Non-zero exit codes indicate failure:

```sindarin
var p: Process = Process.run("false")

if p.exitCode != 0 =>
    print($"Command failed with code {p.exitCode}\n")
```

### Command Not Found

If the command doesn't exist, the exit code is 127:

```sindarin
var p: Process = Process.run("nonexistent-command")

if p.exitCode == 127 =>
    print("Command not found\n")
    print(p.stderr)    // contains error message
```

### Signal Termination

If the process is killed by a signal, the exit code is 128 + signal number:

```sindarin
var p: Process = Process.run("sh", {"-c", "kill -9 $$"})
print(p.exitCode)    // 137 (128 + 9)
```

### Exit Code Reference

| Code | Meaning |
|------|---------|
| 0 | Success |
| 1-125 | Command-specific failure |
| 126 | Command found but not executable |
| 127 | Command not found |
| 128+ | Killed by signal (128 + signal number) |

---

## Common Patterns

### Run or Fail

```sindarin
fn runOrFail(cmd: str, args: str[]): str =>
    var p: Process = Process.run(cmd, args)
    if p.exitCode != 0 =>
        panic($"Command failed: {cmd}\n{p.stderr}")
    return p.stdout
```

### Parallel Builds

```sindarin
fn main(): void =>
    // Build multiple targets concurrently
    var frontend: Process = &Process.run("make", {"frontend"})
    var backend: Process = &Process.run("make", {"backend"})
    var tests: Process = &Process.run("make", {"tests"})

    [frontend, backend, tests]!

    if frontend.exitCode + backend.exitCode + tests.exitCode == 0 =>
        print("All builds succeeded\n")
    else =>
        print("Some builds failed\n")
```

### Count Lines

```sindarin
fn countLines(path: str): int =>
    var p: Process = Process.run("sh", {"-c", $"wc -l < {path}"})
    return p.stdout.trim().toInt()
```

### Check Command Exists

```sindarin
fn commandExists(cmd: str): bool =>
    var p: Process = Process.run("which", {cmd})
    return p.exitCode == 0
```

### Pipeline with Error Checking

```sindarin
fn grepFiles(pattern: str, dir: str): str =>
    var p: Process = Process.run("sh", {"-c", $"grep -r '{pattern}' {dir}"})
    if p.exitCode == 0 =>
        return p.stdout
    else if p.exitCode == 1 =>
        return ""   // no matches
    else =>
        panic($"grep failed: {p.stderr}")
```

---

## Static Methods

| Method | Return | Description |
|--------|--------|-------------|
| `Process.run(cmd)` | `Process` | Run command with no arguments |
| `Process.run(cmd, args)` | `Process` | Run command with arguments |

Both methods block until the process completes (unless spawned with `&`).

---

## Memory Semantics

Process results follow standard arena rules. The `stdout` and `stderr` strings are allocated in the current arena. See [MEMORY.md](MEMORY.md) for details.

```sindarin
fn build(): Process =>
    var p: Process = Process.run("make")
    return p    // promoted to caller's arena

fn main(): int =>
    var result: Process = build()
    print(result.stdout)
    return result.exitCode
```

With async execution, the Process result is promoted to the caller's arena on synchronization:

```sindarin
var p: Process = &Process.run("make")
// ... do other work ...
p!                      // result promoted to caller's arena
print(p.stdout)         // safe to use
```

---

## Error Handling

Processes do not raise Sindarin panics on failure—they simply return a non-zero exit code. Check `exitCode` to handle errors:

```sindarin
var p: Process = Process.run("rm", {"nonexistent-file"})

if p.exitCode != 0 =>
    print($"Error: {p.stderr}")
```

For async processes, errors are available after synchronization:

```sindarin
var p: Process = &Process.run("rm", {"nonexistent-file"})
p!
if p.exitCode != 0 =>
    print($"Error: {p.stderr}")
```

---

## Limitations

The following features are not supported:

- **Streaming I/O** - Cannot read/write while process runs
- **stdin input** - Cannot pipe data to process stdin
- **Working directory** - Use `sh -c "cd dir && cmd"` instead
- **Environment variables** - Use `sh -c "VAR=val cmd"` instead
- **Timeouts** - Use shell `timeout` command if needed
- **Kill/signal** - Cannot terminate running processes
- **Process ID** - Cannot get PID of spawned process

---

## Quick Reference

### Syntax

| Syntax | Behavior |
|--------|----------|
| `Process.run(cmd)` | Run command, wait for completion |
| `Process.run(cmd, args)` | Run with arguments, wait for completion |
| `var p: Process = &Process.run(...)` | Spawn async, p is pending |
| `p!` | Synchronize, wait for completion |
| `[p1, p2, p3]!` | Wait for all processes |
| `&Process.run(...)` | Fire and forget |

### Properties

| Property | Type | Description |
|----------|------|-------------|
| `exitCode` | `int` | Exit code (0 = success) |
| `stdout` | `str` | Captured standard output |
| `stderr` | `str` | Captured standard error |

---

## See Also

- [THREADING.md](THREADING.md) - Threading model (`&` spawn, `!` sync)
- [MEMORY.md](MEMORY.md) - Arena memory management
