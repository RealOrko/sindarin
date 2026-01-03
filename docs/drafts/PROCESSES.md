# Processes in Sindarin

> **DRAFT** - This specification is not yet implemented.

Sindarin provides the `Process` type for spawning and managing external processes. Processes integrate with the threading model, using `&` for parallel execution and `!` for synchronization.

## Basic Usage

Run a command and capture its output:

```sindarin
var p: Process = Process.run("ls", {"-la"})

print(p.stdout)      // captured output
print(p.stderr)      // captured errors
print(p.exitCode)    // 0 on success
```

Arguments are optional:

```sindarin
var p: Process = Process.run("pwd")
print(p.stdout)
```

Check for success:

```sindarin
var p: Process = Process.run("gcc", {"main.c", "-o", "main"})

if p.exitCode == 0 =>
    print("Build succeeded\n")
else =>
    print($"Build failed:\n{p.stderr}")
```

---

## Parallel Execution

### Fire and Forget

Use `&` to spawn a process in the background:

```sindarin
&Process.run("backup.sh")
print("Backup started in background\n")
```

Fire-and-forget processes are terminated when the main process exits.

### Spawn and Wait

Use `&` to spawn and `!` to wait:

```sindarin
var p: Process = &Process.run("slow-task")
// ... do other work ...
p!
print($"Exit code: {p.exitCode}\n")
```

### Parallel Processes

Run multiple processes concurrently:

```sindarin
var p1: Process = &Process.run("task1.sh")
var p2: Process = &Process.run("task2.sh")
var p3: Process = &Process.run("task3.sh")

[p1, p2, p3]!

print($"Results: {p1.exitCode}, {p2.exitCode}, {p3.exitCode}\n")
```

---

## Shell Commands

For pipes, redirection, or shell features, use `sh -c`:

```sindarin
var p: Process = Process.run("sh", {"-c", "ls -la | grep .txt"})
print(p.stdout)
```

```sindarin
var p: Process = Process.run("sh", {"-c", "cd /tmp && pwd"})
print(p.stdout)    // "/tmp\n"
```

```sindarin
var p: Process = Process.run("sh", {"-c", "API_KEY=secret ./myapp"})
```

---

## Error Handling

### Exit Codes

Non-zero exit codes indicate failure:

```sindarin
var p: Process = Process.run("false")

if p.exitCode != 0 =>
    print($"Command failed with code {p.exitCode}\n")
```

### Command Not Found

If the command doesn't exist, the process exits with code 127:

```sindarin
var p: Process = Process.run("nonexistent-command")

if p.exitCode == 127 =>
    print("Command not found\n")
```

---

## Process Type

### Static Methods

| Method | Return | Description |
|--------|--------|-------------|
| `Process.run(cmd)` | `Process` | Run command with no arguments |
| `Process.run(cmd, args)` | `Process` | Run command with arguments |

### Properties

| Property | Type | Description |
|----------|------|-------------|
| `exitCode` | `int` | Process exit code (0 typically means success) |
| `stdout` | `str` | Captured standard output |
| `stderr` | `str` | Captured standard error |

---

## Common Patterns

### Check Command Success

```sindarin
fn runOrFail(cmd: str, args: str[]): str =>
    var p: Process = Process.run(cmd, args)
    if p.exitCode != 0 =>
        panic($"Command failed: {cmd}\n{p.stderr}")
    return p.stdout
```

### Parallel Tasks

```sindarin
fn parallelBuild(targets: str[]): bool =>
    var procs: Process[] = Process[]{}

    for target in targets =>
        procs.push(&Process.run("make", {target}))

    for p in procs =>
        p!

    for p in procs =>
        if p.exitCode != 0 =>
            return false

    return true
```

### Command Pipeline

```sindarin
fn countLines(path: str): int =>
    var p: Process = Process.run("sh", {"-c", $"wc -l < {path}"})
    return p.stdout.trim().toInt()
```

---

## Synchronization

| Syntax | Description |
|--------|-------------|
| `Process.run(...)` | Blocking - waits for completion |
| `&Process.run(...)` | Background - returns pending Process |
| `p!` | Wait for pending Process to complete |
| `&Process.run(...)` (void context) | Fire and forget |
| `[p1, p2]!` | Wait for multiple processes |

---

## Memory Semantics

Process results follow standard arena rules:

```sindarin
fn build(): Process =>
    var p: Process = Process.run("make")
    return p    // promoted to caller's arena

fn main(): int =>
    var result: Process = build()
    print(result.stdout)
    return result.exitCode
```

### Frozen State

When a process is pending, captured arguments are frozen:

```sindarin
var args: str[] = {"file.txt"}
var p: Process = &Process.run("cat", args)

args.push("file2.txt")    // COMPILE ERROR: args frozen

p!
args.push("file2.txt")    // OK
```

---

## Implementation Notes

### C Runtime

```c
typedef struct RtProcess {
    int exit_code;
    RtString* stdout_data;
    RtString* stderr_data;
} RtProcess;

RtProcess* rt_process_run(RtArena* arena, const char* cmd, RtArray* args);
```

### POSIX Implementation

```c
RtProcess* rt_process_run(RtArena* arena, const char* cmd, RtArray* args) {
    int stdout_pipe[2], stderr_pipe[2];
    pipe(stdout_pipe);
    pipe(stderr_pipe);

    pid_t pid = fork();
    if (pid == 0) {
        // Child
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);

        char** argv = build_argv(cmd, args);
        execvp(cmd, argv);
        _exit(127);
    }

    // Parent
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    RtProcess* p = rt_arena_alloc(arena, sizeof(RtProcess));
    p->stdout_data = read_fd_to_string(arena, stdout_pipe[0]);
    p->stderr_data = read_fd_to_string(arena, stderr_pipe[0]);

    int status;
    waitpid(pid, &status, 0);
    p->exit_code = WEXITSTATUS(status);

    return p;
}
```

---

## Future Extensions

The following features are intentionally deferred:

- **Streaming I/O** - Reading/writing while process runs
- **Process.start()** - Non-blocking process handle
- **stdin input** - Piping data to process
- **Working directory** - Use `sh -c "cd dir && cmd"` for now
- **Environment variables** - Use `sh -c "VAR=val cmd"` for now
- **Timeouts** - Use shell timeout command if needed
- **kill/signal** - Requires streaming handle

---

## See Also

- [THREADING.md](../language/THREADING.md) - Threading model (`&` spawn, `!` sync)
- [MEMORY.md](../language/MEMORY.md) - Arena memory management
