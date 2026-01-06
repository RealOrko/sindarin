# Environment

Sindarin provides an `Environment` type for accessing and modifying environment variables. Environment variables configure programs without hardcoding values.

## Quick Start

```sindarin
// Read an environment variable
var dbUrl: str = Environment.get("DATABASE_URL")

// With default value (used if variable is not set)
var port: str = Environment.get("PORT", "8080")

// Check existence
if Environment.has("DEBUG") =>
  enableVerboseLogging()

// List all variables
var all: str[][] = Environment.all()
for entry in all =>
  print($"{entry[0]}={entry[1]}\n")
```

## Static Methods

`Environment` is a static-only type with no instances—all methods are static.

### Reading Variables

| Method | Signature | Description |
|--------|-----------|-------------|
| `get` | `(name: str): str` | Get variable value, panics if not set |
| `get` | `(name: str, default: str): str` | Get variable with default value |
| `has` | `(name: str): bool` | True if variable is set (even if empty) |

```sindarin
// Required variable - panics if not set
var apiKey: str = Environment.get("API_KEY")

// Optional variable with fallback
var host: str = Environment.get("HOST", "localhost")

// Check before accessing
if Environment.has("CONFIG_PATH") =>
  loadConfig(Environment.get("CONFIG_PATH"))
else =>
  loadDefaultConfig()
```

### Listing Variables

| Method | Signature | Description |
|--------|-----------|-------------|
| `all` | `(): str[][]` | All variables as `[[name, value], ...]` |

```sindarin
// Print all environment variables
for entry in Environment.all() =>
  print($"{entry[0]}={entry[1]}\n")
```

## Error Handling

### Missing Required Variables

When using `get()` without a default value, the program panics if the variable is not set:

```sindarin
// Panics if API_KEY is not set
var apiKey: str = Environment.get("API_KEY")
// Error: Environment variable 'API_KEY' is not set
```

### Empty vs Unset

A variable can be set to an empty string, which is different from not being set:

```sindarin
// Variable set to empty string
// (from shell: export EMPTY="")
Environment.has("EMPTY")         // true
Environment.get("EMPTY")         // ""
Environment.get("EMPTY", "x")    // "" (not "x")

// Variable not set at all
Environment.has("UNSET")         // false
Environment.get("UNSET")         // panics
Environment.get("UNSET", "x")    // "x"
```

## Examples

### Application Configuration

```sindarin
fn getConfig(): void =>
  var dbHost: str = Environment.get("DB_HOST", "localhost")
  var dbPort: str = Environment.get("DB_PORT", "5432")
  var dbName: str = Environment.get("DB_NAME", "myapp")
  var dbUser: str = Environment.get("DB_USER")  // Required
  var dbPass: str = Environment.get("DB_PASS")  // Required
  var debug: str = Environment.get("DEBUG", "false")
  print($"Connecting to {dbHost}:{dbPort}/{dbName}\n")
```

### Feature Flags

```sindarin
fn isFeatureEnabled(feature: str): bool =>
  var envName: str = $"FEATURE_{feature}"
  if Environment.has(envName) =>
    var value: str = Environment.get(envName)
    return value == "true" || value == "1" || value == "yes"
  return false

// Usage
if isFeatureEnabled("DARK_MODE") =>
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

### Secrets Handling

```sindarin
fn connectToDatabase(): void =>
  var password: str = Environment.get("DB_PASSWORD")

  // Don't log the actual password
  print("Connecting to database...\n")

  // Use password for connection
  connect(Environment.get("DB_HOST"), password)
```

## Notes

- **Static-only type**: `Environment` has no constructor—use static methods only.
- **Case sensitivity**: Variable names are case-sensitive on Unix and case-insensitive on Windows.
- **Read-only access**: Variables are read from the process environment; modification is not supported.
- **Changes affect children**: Environment changes made before program execution are inherited by child processes.

## See Also

- [TYPES.md](TYPES.md) - Primitive and built-in types
- [PROCESSES.md](PROCESSES.md) - Process execution (inherits environment)
