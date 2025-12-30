# Time in Sindarin

Sindarin provides a built-in `Time` type for working with dates, times, and durations. The `Time` type integrates with Sindarin's arena-based memory management and provides comprehensive methods for time manipulation, formatting, and arithmetic.

## Overview

```sindarin
// Get current time
var now: Time = Time.now()

// Format and display
print($"Current time: {now.format("YYYY-MM-DD HH:mm:ss")}\n")

// Time arithmetic
var later: Time = now.addHours(2)
print($"Two hours from now: {later.toIso()}\n")

// Measure elapsed time
var start: Time = Time.now()
doSomeWork()
var elapsed: int = Time.now().diff(start)
print($"Elapsed: {elapsed}ms\n")
```

---

## Static Methods

These methods create `Time` values or perform utility operations.

### Time.now()

Returns the current local time.

```sindarin
var now: Time = Time.now()
print($"Local time: {now.toIso()}\n")
```

### Time.utc()

Returns the current UTC time.

```sindarin
var utc: Time = Time.utc()
print($"UTC time: {utc.toIso()}\n")
```

### Time.fromMillis(ms)

Creates a `Time` from milliseconds since Unix epoch (January 1, 1970).

```sindarin
var t: Time = Time.fromMillis(1735500000000)
print($"Time: {t.toIso()}\n")
```

### Time.fromSeconds(s)

Creates a `Time` from seconds since Unix epoch.

```sindarin
var t: Time = Time.fromSeconds(1735500000)
print($"Time: {t.toIso()}\n")
```

### Time.sleep(ms)

Pauses execution for the specified number of milliseconds.

```sindarin
print("Starting...\n")
Time.sleep(1000)  // Sleep for 1 second
print("Done!\n")
```

---

## Instance Methods - Getters

These methods extract components from a `Time` value.

### millis()

Returns the time as milliseconds since Unix epoch.

```sindarin
var now: Time = Time.now()
var ms: int = now.millis()
print($"Epoch milliseconds: {ms}\n")
```

### seconds()

Returns the time as seconds since Unix epoch.

```sindarin
var now: Time = Time.now()
var s: int = now.seconds()
print($"Epoch seconds: {s}\n")
```

### year()

Returns the 4-digit year.

```sindarin
var now: Time = Time.now()
print($"Year: {now.year()}\n")  // e.g., 2025
```

### month()

Returns the month (1-12).

```sindarin
var now: Time = Time.now()
print($"Month: {now.month()}\n")  // 1 = January, 12 = December
```

### day()

Returns the day of the month (1-31).

```sindarin
var now: Time = Time.now()
print($"Day: {now.day()}\n")
```

### hour()

Returns the hour (0-23).

```sindarin
var now: Time = Time.now()
print($"Hour: {now.hour()}\n")
```

### minute()

Returns the minute (0-59).

```sindarin
var now: Time = Time.now()
print($"Minute: {now.minute()}\n")
```

### second()

Returns the second (0-59).

```sindarin
var now: Time = Time.now()
print($"Second: {now.second()}\n")
```

### weekday()

Returns the day of the week (0 = Sunday, 6 = Saturday).

```sindarin
var now: Time = Time.now()
var day: int = now.weekday()
var names: str[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"}
print($"Today is {names[day]}\n")
```

---

## Instance Methods - Formatting

These methods convert a `Time` value to a string representation.

### format(pattern)

Formats the time according to the specified pattern.

**Pattern tokens:**
| Token | Description | Example |
|-------|-------------|---------|
| `YYYY` | 4-digit year | 2025 |
| `YY` | 2-digit year | 25 |
| `MM` | 2-digit month | 01-12 |
| `M` | Month without padding | 1-12 |
| `DD` | 2-digit day | 01-31 |
| `D` | Day without padding | 1-31 |
| `HH` | 2-digit hour (24h) | 00-23 |
| `H` | Hour without padding | 0-23 |
| `hh` | 2-digit hour (12h) | 01-12 |
| `h` | Hour (12h) without padding | 1-12 |
| `mm` | 2-digit minute | 00-59 |
| `m` | Minute without padding | 0-59 |
| `ss` | 2-digit second | 00-59 |
| `s` | Second without padding | 0-59 |
| `SSS` | 3-digit milliseconds | 000-999 |
| `A` | AM/PM | AM, PM |
| `a` | am/pm | am, pm |

```sindarin
var now: Time = Time.now()

// Common formats
print($"{now.format("YYYY-MM-DD")}\n")           // 2025-12-30
print($"{now.format("HH:mm:ss")}\n")             // 14:30:45
print($"{now.format("YYYY-MM-DD HH:mm:ss")}\n")  // 2025-12-30 14:30:45
print($"{now.format("M/D/YYYY")}\n")             // 12/30/2025
print($"{now.format("h:mm A")}\n")               // 2:30 PM
```

### toIso()

Returns the time in ISO 8601 format.

```sindarin
var now: Time = Time.now()
print($"{now.toIso()}\n")  // 2025-12-30T14:30:45.123Z
```

### toDate()

Returns just the date portion.

```sindarin
var now: Time = Time.now()
print($"{now.toDate()}\n")  // 2025-12-30
```

### toTime()

Returns just the time portion.

```sindarin
var now: Time = Time.now()
print($"{now.toTime()}\n")  // 14:30:45
```

---

## Instance Methods - Arithmetic

These methods perform time calculations and return new `Time` values.

### add(ms)

Adds milliseconds and returns a new `Time`.

```sindarin
var now: Time = Time.now()
var later: Time = now.add(5000)  // 5 seconds later
```

### addSeconds(s)

Adds seconds and returns a new `Time`.

```sindarin
var now: Time = Time.now()
var later: Time = now.addSeconds(30)
```

### addMinutes(m)

Adds minutes and returns a new `Time`.

```sindarin
var now: Time = Time.now()
var meetingEnd: Time = now.addMinutes(45)
```

### addHours(h)

Adds hours and returns a new `Time`.

```sindarin
var now: Time = Time.now()
var tomorrow: Time = now.addHours(24)
```

### addDays(d)

Adds days and returns a new `Time`.

```sindarin
var now: Time = Time.now()
var nextWeek: Time = now.addDays(7)
```

**Note:** All arithmetic methods accept negative values for subtraction:

```sindarin
var now: Time = Time.now()
var yesterday: Time = now.addDays(-1)
var anHourAgo: Time = now.addHours(-1)
```

### diff(other)

Returns the difference between two times in milliseconds.

```sindarin
var start: Time = Time.now()
doExpensiveOperation()
var end: Time = Time.now()
var elapsed: int = end.diff(start)
print($"Operation took {elapsed}ms\n")
```

The result is positive if `this` is after `other`, negative otherwise:

```sindarin
var t1: Time = Time.now()
Time.sleep(100)
var t2: Time = Time.now()

print($"{t2.diff(t1)}\n")   // ~100 (positive, t2 is after t1)
print($"{t1.diff(t2)}\n")   // ~-100 (negative, t1 is before t2)
```

---

## Instance Methods - Comparison

These methods compare two `Time` values.

### isBefore(other)

Returns true if this time is before the other time.

```sindarin
var t1: Time = Time.now()
Time.sleep(10)
var t2: Time = Time.now()

if t1.isBefore(t2) =>
    print("t1 is earlier\n")
```

### isAfter(other)

Returns true if this time is after the other time.

```sindarin
var deadline: Time = Time.fromSeconds(1735689600)
var now: Time = Time.now()

if now.isAfter(deadline) =>
    print("Deadline has passed!\n")
```

### equals(other)

Returns true if both times represent the same instant.

```sindarin
var t1: Time = Time.fromMillis(1735500000000)
var t2: Time = Time.fromMillis(1735500000000)

if t1.equals(t2) =>
    print("Same time\n")
```

---

## Common Patterns

### Benchmarking Code

```sindarin
fn benchmark(name: str, iterations: int, f: fn(): void): void =>
    var start: Time = Time.now()
    for var i: int = 0; i < iterations; i++ =>
        f()
    var elapsed: int = Time.now().diff(start)
    var perOp: int = elapsed / iterations
    print($"{name}: {elapsed}ms total, {perOp}ms per operation\n")

// Usage
benchmark("fibonacci", 100, fn(): void => fib(30))
```

### Simple Stopwatch

```sindarin
var start: Time = Time.now()

// ... do work ...

var elapsed: int = Time.now().diff(start)
print($"Elapsed time: {elapsed}ms\n")
```

### Timeout Loop

```sindarin
var timeout: int = 5000  // 5 seconds
var start: Time = Time.now()

while Time.now().diff(start) < timeout =>
    if checkCondition() =>
        print("Condition met!\n")
        break
    Time.sleep(100)  // Check every 100ms
```

### Formatting Dates for Display

```sindarin
var now: Time = Time.now()

// Log timestamp
print($"[{now.format("YYYY-MM-DD HH:mm:ss")}] Event occurred\n")

// User-friendly format
print($"Today is {now.format("M/D/YYYY")}\n")

// File name with timestamp
var filename: str = $"backup_{now.format("YYYYMMDD_HHmmss")}.txt"
```

### Calculating Durations

```sindarin
fn formatDuration(ms: int): str =>
    var seconds: int = ms / 1000
    var minutes: int = seconds / 60
    var hours: int = minutes / 60

    if hours > 0 =>
        return $"{hours}h {minutes % 60}m {seconds % 60}s"
    else if minutes > 0 =>
        return $"{minutes}m {seconds % 60}s"
    else if seconds > 0 =>
        return $"{seconds}.{ms % 1000}s"
    return $"{ms}ms"

// Usage
var start: Time = Time.now()
longRunningTask()
var elapsed: int = Time.now().diff(start)
print($"Task completed in {formatDuration(elapsed)}\n")
```

### Scheduling Future Events

```sindarin
var now: Time = Time.now()
var eventTime: Time = now.addHours(2).addMinutes(30)

print($"Event scheduled for: {eventTime.format("h:mm A")}\n")

var waitMs: int = eventTime.diff(now)
print($"Time until event: {waitMs / 1000 / 60} minutes\n")
```

### Date Comparisons

```sindarin
var deadline: Time = Time.fromSeconds(1735689600)  // Some future date
var now: Time = Time.now()

if now.isBefore(deadline) =>
    var remaining: int = deadline.diff(now)
    var days: int = remaining / 1000 / 60 / 60 / 24
    print($"{days} days until deadline\n")
else =>
    print("Deadline has passed!\n")
```

---

## Memory Management

`Time` values are lightweight and integrate with Sindarin's arena-based memory management. String results from formatting methods are allocated from the arena and automatically freed when the arena is destroyed.

```sindarin
fn getTimestamp(): str =>
    var now: Time = Time.now()
    return now.format("YYYY-MM-DD HH:mm:ss")
    // Formatted string is returned and remains valid
```

---

## See Also

- [README.md](../README.md) - Language overview
- [MEMORY.md](MEMORY.md) - Arena memory management details
- [FILE_IO.md](FILE_IO.md) - File I/O operations
