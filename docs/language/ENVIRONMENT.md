# Environment

The built-in `Environment` type has been removed. Please use the SDK `SnEnvironment` type instead.

## Migration to SDK

Import the environment module from the SDK:

```sindarin
import "sdk/env"
```

The SDK provides the `SnEnvironment` struct with equivalent functionality:

```sindarin
import "sdk/env"

// Read an environment variable (panics if not set)
var dbUrl: str = SnEnvironment.get("DATABASE_URL")

// With default value (used if variable is not set)
var port: str = SnEnvironment.getOr("PORT", "8080")

// Check existence
if SnEnvironment.has("DEBUG") =>
  enableVerboseLogging()

// List all variables
var all: str[][] = SnEnvironment.all()
for entry in all =>
  print($"{entry[0]}={entry[1]}\n")
```

## API Differences

The SDK `SnEnvironment` type differs slightly from the former built-in `Environment`:

| Former Built-in | SDK | Description |
|-----------------|-----|-------------|
| `Environment.get(name)` | `SnEnvironment.get(name)` | Get required variable |
| `Environment.get(name, default)` | `SnEnvironment.getOr(name, default)` | Get with default |
| `Environment.has(name)` | `SnEnvironment.has(name)` | Check existence |
| `Environment.all()` | `SnEnvironment.all()` | List all variables |

**Note:** The SDK version uses `getOr()` instead of overloaded `get()` because user-defined structs don't support method overloading in Sindarin.

## SDK Environment API

See the [SDK Environment documentation](../sdk/env.md) for the complete API reference.

---

## See Also

- [SDK Environment](../sdk/env.md) - Full SDK documentation
- [SDK Overview](../sdk/readme.md) - All SDK modules
