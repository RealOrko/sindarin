# Environment in Sindarin

The built-in `Environment` type provides access to environment variables. For an SDK alternative, use the `SnEnvironment` type.

## SDK Alternative

Import the environment module from the SDK:

```sindarin
import "sdk/env"
```

The SDK provides the `SnEnvironment` struct with equivalent functionality:

```sindarin
import "sdk/env"

// Get required environment variable (panics if not set)
var dbUrl: str = SnEnvironment.get("DATABASE_URL")

// Get with default value
var port: str = SnEnvironment.getOr("PORT", "8080")

// Check if variable exists
if SnEnvironment.has("DEBUG") =>
  print("Debug mode enabled\n")

// List all variables
var all: str[][] = SnEnvironment.all()
for entry in all =>
  print($"{entry[0]}={entry[1]}\n")
```

## API Differences

The SDK `SnEnvironment` type differs slightly from the built-in `Environment`:

| Built-in | SDK | Description |
|----------|-----|-------------|
| `Environment.get(name)` | `SnEnvironment.get(name)` | Get required variable |
| `Environment.get(name, default)` | `SnEnvironment.getOr(name, default)` | Get with default |
| `Environment.has(name)` | `SnEnvironment.has(name)` | Check existence |
| `Environment.all()` | `SnEnvironment.all()` | List all variables |

**Note:** The SDK version uses `getOr()` instead of overloaded `get()` because user-defined structs don't support method overloading in Sindarin.

## SDK Environment API

### Static Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `get` | `(name: str): str` | Get variable value, panics if not set |
| `getOr` | `(name: str, defaultValue: str): str` | Get variable with default value |
| `has` | `(name: str): bool` | True if variable is set (even if empty) |
| `all` | `(): str[][]` | All variables as `[[name, value], ...]` |

### Reading Variables

```sindarin
import "sdk/env"

// Required variable - panics if not set
var apiKey: str = SnEnvironment.get("API_KEY")

// Optional variable with fallback
var host: str = SnEnvironment.getOr("HOST", "localhost")

// Check before accessing
if SnEnvironment.has("CONFIG_PATH") =>
  loadConfig(SnEnvironment.get("CONFIG_PATH"))
else =>
  loadDefaultConfig()
```

### Listing Variables

```sindarin
import "sdk/env"

// Print all environment variables
var all: str[][] = SnEnvironment.all()
for entry in all =>
  print($"{entry[0]}={entry[1]}\n")
```

## Error Handling

### Missing Required Variables

When using `get()`, the program panics if the variable is not set:

```sindarin
// Panics if API_KEY is not set
var apiKey: str = SnEnvironment.get("API_KEY")
// Error: Environment variable 'API_KEY' is not set
```

### Empty vs Unset

A variable can be set to an empty string, which is different from not being set:

```sindarin
// Variable set to empty string (export EMPTY="")
SnEnvironment.has("EMPTY")              // true
SnEnvironment.get("EMPTY")              // ""
SnEnvironment.getOr("EMPTY", "x")       // "" (not "x")

// Variable not set at all
SnEnvironment.has("UNSET")              // false
SnEnvironment.get("UNSET")              // panics
SnEnvironment.getOr("UNSET", "x")       // "x"
```

## Examples

### Application Configuration

```sindarin
import "sdk/env"

fn loadConfig(): void =>
  var dbHost: str = SnEnvironment.getOr("DB_HOST", "localhost")
  var dbPort: str = SnEnvironment.getOr("DB_PORT", "5432")
  var dbName: str = SnEnvironment.getOr("DB_NAME", "myapp")
  var dbUser: str = SnEnvironment.get("DB_USER")  // Required
  var dbPass: str = SnEnvironment.get("DB_PASS")  // Required
  print($"Connecting to {dbHost}:{dbPort}/{dbName}\n")
```

### Feature Flags

```sindarin
import "sdk/env"

fn isFeatureEnabled(feature: str): bool =>
  var envName: str = $"FEATURE_{feature}"
  if SnEnvironment.has(envName) =>
    var value: str = SnEnvironment.get(envName)
    return value == "true" || value == "1" || value == "yes"
  return false

// Usage
if isFeatureEnabled("DARK_MODE") =>
  enableDarkMode()
```

### Platform Detection

```sindarin
import "sdk/env"

fn getPlatform(): str =>
  // Unix-like systems
  if SnEnvironment.has("HOME") =>
    return "unix"
  // Windows
  if SnEnvironment.has("USERPROFILE") =>
    return "windows"
  return "unknown"

fn getHomeDirectory(): str =>
  if SnEnvironment.has("HOME") =>
    return SnEnvironment.get("HOME")
  return SnEnvironment.get("USERPROFILE")
```

## Notes

- **Static-only type**: `SnEnvironment` has no constructor—use static methods only
- **Case sensitivity**: Variable names are case-sensitive on Unix and case-insensitive on Windows
- **Read-only access**: Variables are read from the process environment; modification is not supported
- **Changes affect children**: Environment changes made before program execution are inherited by child processes

---

## See Also

- [Environment (Built-in)](../language/ENVIRONMENT.md) - Built-in Environment type
- [SDK Overview](readme.md) - All SDK modules
- SDK source: `sdk/env.sn`
