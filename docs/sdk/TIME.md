# Time in Sindarin

The built-in `Time` type has been deprecated. Please use the SDK-based `SnTime` type instead.

## Migration to SDK

Import the time module from the SDK:

```sindarin
import "sdk/time"
```

The SDK provides the `SnTime` struct with equivalent functionality:

```sindarin
import "sdk/time"

// Get current time
var now: SnTime = SnTime.now()

// Format and display
print($"Current time: {now.format("YYYY-MM-DD HH:mm:ss")}\n")

// Time arithmetic
var later: SnTime = now.addHours(2)
print($"Two hours from now: {later.toIso()}\n")

// Measure elapsed time
var start: SnTime = SnTime.now()
doSomeWork()
var elapsed: int = SnTime.now().diff(start)
print($"Elapsed: {elapsed}ms\n")
```

## SDK Time API

See the SDK time module documentation for the complete API reference:

- `SnTime.now()` - Get current local time
- `SnTime.utc()` - Get current UTC time
- `SnTime.fromMillis(ms)` - Create from epoch milliseconds
- `SnTime.fromSeconds(s)` - Create from epoch seconds
- `SnTime.sleep(ms)` - Sleep for milliseconds

Instance methods:
- `millis()`, `seconds()` - Get epoch time
- `year()`, `month()`, `day()` - Get date components
- `hour()`, `minute()`, `second()` - Get time components
- `weekday()` - Get day of week
- `format(pattern)`, `toIso()` - Formatting
- `add(ms)`, `addSeconds()`, `addMinutes()`, `addHours()`, `addDays()` - Arithmetic
- `diff(other)`, `isBefore()`, `isAfter()`, `equals()` - Comparison

---

## See Also

- [DATE.md](DATE.md) - Calendar date operations (SDK-based)
- SDK source: `sdk/time.sn`
