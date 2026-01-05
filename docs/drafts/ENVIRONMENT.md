# Environment Variables in Sindarin

> **DRAFT** - This specification is not yet implemented. This document explores design options for an `Environment` type in Sindarin's runtime.

## Overview

Environment variables provide configuration to programs without hardcoding values. They're essential for:
- Database connection strings
- API keys and secrets
- Feature flags
- Runtime configuration (ports, hosts, paths)
- Platform detection

```sindarin
// Read an environment variable
var dbUrl: str = Environment.get("DATABASE_URL")

// With default value
var port: int = Environment.getInt("PORT", 8080)

// Check existence
if Environment.has("DEBUG") =>
    enableVerboseLogging()

// Set for child processes
Environment.set("NODE_ENV", "production")
```

---

## Why a Dedicated Type?

Environment variables could be accessed via raw C interop, but a dedicated type provides:

1. **Type safety**: Typed getters (`getInt`, `getBool`) with validation
2. **Default values**: Clean fallback syntax without null checking
3. **Portability**: Abstracts platform differences (Windows vs Unix)
4. **Discoverability**: All environment operations in one place
5. **Security**: Optional masking for sensitive values in logs

---

## Proposed Design for Sindarin

### The `Environment` Type

`Environment` is a static-only type (no instances) providing access to the process environment.

```sindarin
// Get a required variable (throws if not set)
var apiKey: str = Environment.get("API_KEY")

// Get with default (never throws)
var host: str = Environment.get("HOST", "localhost")
var port: int = Environment.getInt("PORT", 3000)

// Check existence
if Environment.has("VERBOSE") =>
    print("Verbose mode enabled\n")

// Set a variable
Environment.set("MY_VAR", "my_value")

// Remove a variable
Environment.remove("TEMP_VAR")

// List all variables
var all: str[][] = Environment.list()
for entry in all =>
    print($"{entry[0]}={entry[1]}\n")
```

---

## Static Methods

### Reading Variables

| Method | Return | Description |
|--------|--------|-------------|
| `Environment.get(name)` | `str` | Get variable, throws if not set |
| `Environment.get(name, default)` | `str` | Get variable with default value |
| `Environment.getInt(name)` | `int` | Get as int, throws if not set or invalid |
| `Environment.getInt(name, default)` | `int` | Get as int with default |
| `Environment.getLong(name)` | `long` | Get as long, throws if not set or invalid |
| `Environment.getLong(name, default)` | `long` | Get as long with default |
| `Environment.getDouble(name)` | `double` | Get as double, throws if not set or invalid |
| `Environment.getDouble(name, default)` | `double` | Get as double with default |
| `Environment.getBool(name)` | `bool` | Get as bool, throws if not set or invalid |
| `Environment.getBool(name, default)` | `bool` | Get as bool with default |

### Boolean Parsing

`getBool` accepts common truthy/falsy values (case-insensitive):

| Truthy | Falsy |
|--------|-------|
| `"true"`, `"1"`, `"yes"`, `"on"` | `"false"`, `"0"`, `"no"`, `"off"` |

```sindarin
// All evaluate to true
Environment.set("DEBUG", "true")
Environment.set("DEBUG", "1")
Environment.set("DEBUG", "yes")
Environment.set("DEBUG", "YES")

var debug: bool = Environment.getBool("DEBUG")  // true
```

### Checking Existence

| Method | Return | Description |
|--------|--------|-------------|
| `Environment.has(name)` | `bool` | True if variable is set (even if empty) |

```sindarin
// Check before accessing
if Environment.has("CONFIG_PATH") =>
    loadConfig(Environment.get("CONFIG_PATH"))
else =>
    loadDefaultConfig()
```

### Modifying Variables

| Method | Return | Description |
|--------|--------|-------------|
| `Environment.set(name, value)` | `void` | Set or update a variable |
| `Environment.remove(name)` | `bool` | Remove a variable, returns true if existed |

```sindarin
// Set a variable
Environment.set("MY_APP_MODE", "production")

// Remove a variable
var existed: bool = Environment.remove("TEMP_TOKEN")
```

**Note**: Changes affect the current process and any child processes spawned afterward. They do not affect the parent shell or other processes.

### Listing Variables

| Method | Return | Description |
|--------|--------|-------------|
| `Environment.list()` | `str[][]` | All variables as `[[name, value], ...]` |
| `Environment.names()` | `str[]` | All variable names |

```sindarin
// Print all environment variables
for entry in Environment.list() =>
    print($"{entry[0]}={entry[1]}\n")

// Get just the names
var names: str[] = Environment.names()
print($"Found {names.length()} environment variables\n")
```

---

## Use Cases

### Application Configuration

```sindarin
fn loadConfig(): Config =>
    return Config {
        dbHost: Environment.get("DB_HOST", "localhost"),
        dbPort: Environment.getInt("DB_PORT", 5432),
        dbName: Environment.get("DB_NAME", "myapp"),
        dbUser: Environment.get("DB_USER"),  // Required
        dbPass: Environment.get("DB_PASS"),  // Required
        debug: Environment.getBool("DEBUG", false),
        logLevel: Environment.get("LOG_LEVEL", "info")
    }
```

### Feature Flags

```sindarin
fn isFeatureEnabled(feature: str): bool =>
    var envName: str = $"FEATURE_{feature.toUpper()}"
    return Environment.getBool(envName, false)

// Usage
if isFeatureEnabled("dark_mode") =>
    enableDarkMode()
```

### Platform Detection

```sindarin
fn getPlatform(): str =>
    // Unix-like systems
    if Environment.has("HOME") =>
        return "unix"

    // Windows
    if Environment.has("USERPROFILE") =>
        return "windows"

    return "unknown"

fn getHomeDirectory(): str =>
    if Environment.has("HOME") =>
        return Environment.get("HOME")
    return Environment.get("USERPROFILE")
```

### Setting Up Child Process Environment

```sindarin
fn runBuild(): int =>
    // Configure environment for build
    Environment.set("NODE_ENV", "production")
    Environment.set("CI", "true")
    Environment.set("BUILD_NUMBER", "123")

    // Spawn build process (inherits environment)
    var proc: Process = Process.spawn("npm", {"run", "build"})
    return proc.wait()
```

### Secrets Handling

```sindarin
fn connectToDatabase(): Connection =>
    var password: str = Environment.get("DB_PASSWORD")

    // Don't log the actual password
    print("Connecting to database...\n")

    return Database.connect(
        host: Environment.get("DB_HOST"),
        user: Environment.get("DB_USER"),
        password: password
    )
```

---

## Error Handling

### Missing Required Variables

```sindarin
// Throws RuntimeError if API_KEY is not set
var apiKey: str = Environment.get("API_KEY")
// Error: Environment variable 'API_KEY' is not set
```

### Invalid Type Conversion

```sindarin
Environment.set("PORT", "not_a_number")

// Throws RuntimeError due to invalid integer
var port: int = Environment.getInt("PORT")
// Error: Environment variable 'PORT' has invalid integer value: 'not_a_number'

// With default, invalid values also throw (default only used when unset)
var port2: int = Environment.getInt("PORT", 8080)
// Error: Environment variable 'PORT' has invalid integer value: 'not_a_number'
```

### Safe Pattern with has()

```sindarin
fn getPortSafe(): int =>
    if !Environment.has("PORT") =>
        return 8080

    var portStr: str = Environment.get("PORT")
    // Manual validation if needed
    return parseInt(portStr)
```

---

## Implementation Considerations

### C Runtime Interface

```c
// Get environment variable (returns NULL if not set)
char *rt_env_get(const char *name);

// Set environment variable
int rt_env_set(const char *name, const char *value);

// Remove environment variable
int rt_env_remove(const char *name);

// Check if variable exists
int rt_env_has(const char *name);

// Get all environment variables
// Returns array of [name, value] pairs, NULL-terminated
char ***rt_env_list(RtArena *arena);
```

### Platform Abstraction

```c
#ifdef _WIN32
    // Windows: GetEnvironmentVariable, SetEnvironmentVariable
    #include <windows.h>

    char *rt_env_get(const char *name) {
        DWORD size = GetEnvironmentVariableA(name, NULL, 0);
        if (size == 0) return NULL;
        char *buf = malloc(size);
        GetEnvironmentVariableA(name, buf, size);
        return buf;
    }
#else
    // POSIX: getenv, setenv, unsetenv
    #include <stdlib.h>

    char *rt_env_get(const char *name) {
        return getenv(name);  // Note: returns pointer to static storage
    }
#endif
```

### Thread Safety

Environment variable access is inherently not thread-safe on most platforms:
- POSIX `setenv`/`getenv` can race with each other
- Windows `SetEnvironmentVariable`/`GetEnvironmentVariable` are safer but still have caveats

**Recommendation**: Document that `Environment.set()` and `Environment.remove()` should only be called from the main thread or before spawning threads.

---

## Comparison with Other Languages

| Language | Read | Write | Typed Access |
|----------|------|-------|--------------|
| Python | `os.environ["KEY"]` | `os.environ["KEY"] = val` | Manual |
| Node.js | `process.env.KEY` | `process.env.KEY = val` | Manual |
| Rust | `std::env::var("KEY")` | `std::env::set_var("KEY", val)` | Returns `Result` |
| Go | `os.Getenv("KEY")` | `os.Setenv("KEY", val)` | Manual |
| Java | `System.getenv("KEY")` | Not supported | Manual |
| **Sindarin** | `Environment.get("KEY")` | `Environment.set("KEY", val)` | `getInt`, `getBool`, etc. |

Sindarin's approach:
- Static type with clear namespace
- Typed getters with validation
- Default value support built-in
- Throws on missing required values (explicit error handling)

---

## Design Decisions

### Static-Only Type

`Environment` has no instances - all methods are static. This matches the reality that there's only one process environment.

```sindarin
// This works
var port: int = Environment.getInt("PORT", 8080)

// This is not allowed (no constructor)
var env: Environment = Environment()  // Error
```

### Defaults Only Apply to Missing Variables

When using `get(name, default)`, the default is only used if the variable is not set. If the variable is set but has an invalid value for typed getters, an error is thrown.

```sindarin
Environment.set("COUNT", "abc")

// Throws error - "abc" is not a valid integer
var count: int = Environment.getInt("COUNT", 10)

// To truly fall back on any failure, use has() + try/catch
```

**Rationale**: Silent fallback to defaults on invalid values would hide configuration errors. It's better to fail fast when the environment is misconfigured.

### Case Sensitivity

Environment variable names are case-sensitive on Unix and case-insensitive on Windows. Sindarin preserves platform behavior:

```sindarin
// On Unix: these are different variables
Environment.set("MyVar", "a")
Environment.set("MYVAR", "b")
Environment.get("MyVar")  // "a"
Environment.get("MYVAR")  // "b"

// On Windows: these refer to the same variable
Environment.set("MyVar", "a")
Environment.set("MYVAR", "b")
Environment.get("MyVar")  // "b"
```

### Empty vs Unset

A variable can be set to an empty string, which is different from not being set:

```sindarin
Environment.set("EMPTY", "")

Environment.has("EMPTY")       // true
Environment.get("EMPTY")       // ""
Environment.get("EMPTY", "x")  // "" (not "x")

Environment.has("UNSET")       // false
Environment.get("UNSET")       // throws
Environment.get("UNSET", "x")  // "x"
```

---

## Future Considerations

### Environment Snapshots

For testing or isolation, capture and restore environment state:

```sindarin
// Potential future API
var snapshot: EnvironmentSnapshot = Environment.snapshot()

Environment.set("TEST_MODE", "true")
runTests()

snapshot.restore()  // Restore original environment
```

### .env File Loading

Integration with `.env` files is common in modern development:

```sindarin
// Potential future API
Environment.loadFile(".env")
Environment.loadFile(".env.local", required: false)
```

### Structured Values

Support for JSON or structured values in environment variables:

```sindarin
// Potential future API
Environment.set("CONFIG", '{"port": 8080, "debug": true}')
var config: Config = Environment.getJson("CONFIG", Config)
```

---

## References

- [POSIX Environment Variables](https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap08.html)
- [The Twelve-Factor App: Config](https://12factor.net/config)
- [Windows Environment Variables](https://docs.microsoft.com/en-us/windows/win32/procthread/environment-variables)
