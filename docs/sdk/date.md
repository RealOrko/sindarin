# Date in Sindarin

The built-in `Date` type has been deprecated. Please use the SDK-based `SnDate` type instead.

## Migration to SDK

Import the date module from the SDK:

```sindarin
import "sdk/date"
```

The SDK provides the `SnDate` struct with equivalent functionality:

```sindarin
import "sdk/date"

// Get current date
var today: SnDate = SnDate.today()

// Format and display
print($"Today is: {today.format("YYYY-MM-DD")}\n")

// Date arithmetic
var nextWeek: SnDate = today.addDays(7)
print($"Next week: {nextWeek.toIso()}\n")

// Days between dates
var birthday: SnDate = SnDate.fromYmd(2025, 6, 15)
var daysUntil: int = birthday.diffDays(today)
print($"Days until birthday: {daysUntil}\n")
```

## SDK Date API

See the SDK date module documentation for the complete API reference:

- `SnDate.today()` - Get current local date
- `SnDate.fromYmd(year, month, day)` - Create from components
- `SnDate.fromString(str)` - Parse from ISO string
- `SnDate.fromEpochDays(days)` - Create from epoch days
- `SnDate.isLeapYear(year)` - Check leap year
- `SnDate.daysInMonth(year, month)` - Get days in month

Instance methods:
- `year()`, `month()`, `day()` - Get components
- `weekday()`, `dayOfYear()`, `epochDays()` - Get derived values
- `daysInMonth()`, `isLeapYear()` - Query methods
- `isWeekend()`, `isWeekday()` - Day type checks
- `format(pattern)`, `toIso()`, `toString()` - Formatting
- `addDays()`, `addWeeks()`, `addMonths()`, `addYears()` - Arithmetic
- `diffDays()` - Days between dates
- `startOfMonth()`, `endOfMonth()`, `startOfYear()`, `endOfYear()` - Boundaries
- `isBefore()`, `isAfter()`, `equals()` - Comparison

---

## See Also

- [Time](time.md) - Time operations
- [SDK Overview](readme.md) - All SDK modules
- SDK source: `sdk/date.sn`
