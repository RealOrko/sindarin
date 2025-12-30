# Date in Sindarin

Sindarin provides a built-in `Date` type for working with calendar dates without time components. The `Date` type is ideal for birthdays, holidays, scheduling, and date arithmetic. It integrates with Sindarin's arena-based memory management.

## Overview

```sindarin
// Get current date
var today: Date = Date.today()

// Format and display
print($"Today is: {today.format("YYYY-MM-DD")}\n")

// Date arithmetic
var nextWeek: Date = today.addDays(7)
print($"Next week: {nextWeek.toIso()}\n")

// Days between dates
var birthday: Date = Date.fromYmd(2025, 6, 15)
var daysUntil: int = birthday.diffDays(today)
print($"Days until birthday: {daysUntil}\n")
```

---

## Static Methods

These methods create `Date` values or perform utility operations.

### Date.today()

Returns the current local date.

```sindarin
var today: Date = Date.today()
print($"Today is {today.toIso()}\n")
```

### Date.fromYmd(year, month, day)

Creates a `Date` from year, month, and day components.

```sindarin
var d: Date = Date.fromYmd(2025, 12, 25)
print($"Christmas: {d.toIso()}\n")
```

### Date.fromString(str)

Parses a `Date` from an ISO 8601 string (YYYY-MM-DD format).

```sindarin
var d: Date = Date.fromString("2025-07-04")
print($"Independence Day: {d.toIso()}\n")
```

### Date.fromEpochDays(days)

Creates a `Date` from the number of days since Unix epoch (January 1, 1970).

```sindarin
var d: Date = Date.fromEpochDays(20088)  // 2025-01-01
print($"Date: {d.toIso()}\n")
```

### Date.isLeapYear(year)

Returns true if the specified year is a leap year.

```sindarin
if Date.isLeapYear(2024) =>
    print("2024 is a leap year\n")

if !Date.isLeapYear(2025) =>
    print("2025 is not a leap year\n")
```

### Date.daysInMonth(year, month)

Returns the number of days in the specified month.

```sindarin
var days: int = Date.daysInMonth(2024, 2)  // 29 (leap year)
print($"February 2024 has {days} days\n")
```

---

## Instance Methods - Getters

These methods extract components from a `Date` value.

### year()

Returns the 4-digit year.

```sindarin
var today: Date = Date.today()
print($"Year: {today.year()}\n")  // e.g., 2025
```

### month()

Returns the month (1-12).

```sindarin
var today: Date = Date.today()
print($"Month: {today.month()}\n")  // 1 = January, 12 = December
```

### day()

Returns the day of the month (1-31).

```sindarin
var today: Date = Date.today()
print($"Day: {today.day()}\n")
```

### weekday()

Returns the day of the week (0 = Sunday, 6 = Saturday).

```sindarin
var today: Date = Date.today()
var wd: int = today.weekday()
var names: str[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"}
print($"Today is {names[wd]}\n")
```

### dayOfYear()

Returns the day of the year (1-366).

```sindarin
var today: Date = Date.today()
print($"Day of year: {today.dayOfYear()}\n")
```

### epochDays()

Returns the number of days since Unix epoch (January 1, 1970).

```sindarin
var today: Date = Date.today()
var days: int = today.epochDays()
print($"Days since epoch: {days}\n")
```

### daysInMonth()

Returns the number of days in this date's month.

```sindarin
var d: Date = Date.fromYmd(2024, 2, 15)
print($"Days in February 2024: {d.daysInMonth()}\n")  // 29
```

### isLeapYear()

Returns true if this date's year is a leap year.

```sindarin
var d: Date = Date.fromYmd(2024, 6, 1)
if d.isLeapYear() =>
    print("This is a leap year\n")
```

### isWeekend()

Returns true if this date falls on a Saturday or Sunday.

```sindarin
var today: Date = Date.today()
if today.isWeekend() =>
    print("It's the weekend!\n")
else =>
    print("It's a weekday\n")
```

### isWeekday()

Returns true if this date falls on Monday through Friday.

```sindarin
var today: Date = Date.today()
if today.isWeekday() =>
    print("Back to work!\n")
```

---

## Instance Methods - Formatting

These methods convert a `Date` value to a string representation.

### format(pattern)

Formats the date according to the specified pattern.

**Pattern tokens:**
| Token | Description | Example |
|-------|-------------|---------|
| `YYYY` | 4-digit year | 2025 |
| `YY` | 2-digit year | 25 |
| `MM` | 2-digit month | 01-12 |
| `M` | Month without padding | 1-12 |
| `DD` | 2-digit day | 01-31 |
| `D` | Day without padding | 1-31 |
| `ddd` | Short weekday name | Mon, Tue |
| `dddd` | Full weekday name | Monday, Tuesday |
| `MMM` | Short month name | Jan, Feb |
| `MMMM` | Full month name | January, February |

```sindarin
var d: Date = Date.fromYmd(2025, 12, 25)

// Common formats
print($"{d.format("YYYY-MM-DD")}\n")        // 2025-12-25
print($"{d.format("M/D/YYYY")}\n")          // 12/25/2025
print($"{d.format("DD/MM/YYYY")}\n")        // 25/12/2025
print($"{d.format("MMMM D, YYYY")}\n")      // December 25, 2025
print($"{d.format("ddd, MMM D")}\n")        // Thu, Dec 25
```

### toIso()

Returns the date in ISO 8601 format (YYYY-MM-DD).

```sindarin
var today: Date = Date.today()
print($"{today.toIso()}\n")  // 2025-12-30
```

### toString()

Returns a human-readable string representation.

```sindarin
var today: Date = Date.today()
print($"{today.toString()}\n")  // December 30, 2025
```

---

## Instance Methods - Arithmetic

These methods perform date calculations and return new `Date` values.

### addDays(n)

Adds days and returns a new `Date`.

```sindarin
var today: Date = Date.today()
var tomorrow: Date = today.addDays(1)
var lastWeek: Date = today.addDays(-7)
```

### addWeeks(n)

Adds weeks and returns a new `Date`.

```sindarin
var today: Date = Date.today()
var twoWeeksOut: Date = today.addWeeks(2)
```

### addMonths(n)

Adds months and returns a new `Date`. Handles month-end edge cases.

```sindarin
var d: Date = Date.fromYmd(2025, 1, 31)
var next: Date = d.addMonths(1)
print($"{next.toIso()}\n")  // 2025-02-28 (clamped to valid day)
```

### addYears(n)

Adds years and returns a new `Date`. Handles leap year edge cases.

```sindarin
var leapDay: Date = Date.fromYmd(2024, 2, 29)
var nextYear: Date = leapDay.addYears(1)
print($"{nextYear.toIso()}\n")  // 2025-02-28
```

**Note:** All arithmetic methods accept negative values for subtraction:

```sindarin
var today: Date = Date.today()
var lastMonth: Date = today.addMonths(-1)
var lastYear: Date = today.addYears(-1)
```

### diffDays(other)

Returns the difference between two dates in days.

```sindarin
var start: Date = Date.fromYmd(2025, 1, 1)
var end: Date = Date.fromYmd(2025, 12, 31)
var days: int = end.diffDays(start)
print($"Days in 2025: {days}\n")  // 364
```

The result is positive if `this` is after `other`, negative otherwise:

```sindarin
var d1: Date = Date.fromYmd(2025, 1, 1)
var d2: Date = Date.fromYmd(2025, 1, 10)

print($"{d2.diffDays(d1)}\n")   // 9 (positive, d2 is after d1)
print($"{d1.diffDays(d2)}\n")   // -9 (negative, d1 is before d2)
```

### startOfMonth()

Returns a new `Date` set to the first day of this date's month.

```sindarin
var d: Date = Date.fromYmd(2025, 6, 15)
var first: Date = d.startOfMonth()
print($"{first.toIso()}\n")  // 2025-06-01
```

### endOfMonth()

Returns a new `Date` set to the last day of this date's month.

```sindarin
var d: Date = Date.fromYmd(2025, 2, 10)
var last: Date = d.endOfMonth()
print($"{last.toIso()}\n")  // 2025-02-28
```

### startOfYear()

Returns a new `Date` set to January 1st of this date's year.

```sindarin
var d: Date = Date.fromYmd(2025, 6, 15)
var first: Date = d.startOfYear()
print($"{first.toIso()}\n")  // 2025-01-01
```

### endOfYear()

Returns a new `Date` set to December 31st of this date's year.

```sindarin
var d: Date = Date.fromYmd(2025, 6, 15)
var last: Date = d.endOfYear()
print($"{last.toIso()}\n")  // 2025-12-31
```

---

## Instance Methods - Comparison

These methods compare two `Date` values.

### isBefore(other)

Returns true if this date is before the other date.

```sindarin
var d1: Date = Date.fromYmd(2025, 1, 1)
var d2: Date = Date.fromYmd(2025, 12, 31)

if d1.isBefore(d2) =>
    print("d1 is earlier\n")
```

### isAfter(other)

Returns true if this date is after the other date.

```sindarin
var deadline: Date = Date.fromYmd(2025, 6, 30)
var today: Date = Date.today()

if today.isAfter(deadline) =>
    print("Deadline has passed!\n")
```

### equals(other)

Returns true if both dates represent the same day.

```sindarin
var d1: Date = Date.fromYmd(2025, 12, 25)
var d2: Date = Date.fromString("2025-12-25")

if d1.equals(d2) =>
    print("Same date\n")
```

---

## Common Patterns

### Age Calculation

```sindarin
fn calculateAge(birthdate: Date): int =>
    var today: Date = Date.today()
    var age: int = today.year() - birthdate.year()

    // Adjust if birthday hasn't occurred yet this year
    var birthdayThisYear: Date = Date.fromYmd(today.year(), birthdate.month(), birthdate.day())
    if today.isBefore(birthdayThisYear) =>
        age = age - 1

    return age

// Usage
var dob: Date = Date.fromYmd(1990, 5, 15)
print($"Age: {calculateAge(dob)}\n")
```

### Days Until Event

```sindarin
fn daysUntil(event: Date): int =>
    var today: Date = Date.today()
    return event.diffDays(today)

// Usage
var christmas: Date = Date.fromYmd(2025, 12, 25)
var days: int = daysUntil(christmas)
if days > 0 =>
    print($"{days} days until Christmas\n")
else if days == 0 =>
    print("Merry Christmas!\n")
else =>
    print("Christmas has passed\n")
```

### Business Days Calculation

```sindarin
fn addBusinessDays(start: Date, days: int): Date =>
    var result: Date = start
    var remaining: int = days

    while remaining > 0 =>
        result = result.addDays(1)
        if result.isWeekday() =>
            remaining = remaining - 1

    return result

fn countBusinessDays(start: Date, end: Date): int =>
    var count: int = 0
    var current: Date = start.addDays(1)

    while current.isBefore(end) || current.equals(end) =>
        if current.isWeekday() =>
            count = count + 1
        current = current.addDays(1)

    return count

// Usage
var today: Date = Date.today()
var deadline: Date = addBusinessDays(today, 5)
print($"5 business days from now: {deadline.toIso()}\n")
```

### Date Range Iteration

```sindarin
fn printDateRange(start: Date, end: Date): void =>
    var current: Date = start
    while current.isBefore(end) || current.equals(end) =>
        print($"{current.toIso()}\n")
        current = current.addDays(1)

// Usage
var start: Date = Date.fromYmd(2025, 1, 1)
var end: Date = Date.fromYmd(2025, 1, 7)
printDateRange(start, end)
```

### Month Calendar

```sindarin
fn printMonthCalendar(year: int, month: int): void =>
    var first: Date = Date.fromYmd(year, month, 1)
    var daysInMonth: int = first.daysInMonth()

    print($"{first.format("MMMM YYYY")}\n")
    print("Su Mo Tu We Th Fr Sa\n")

    // Print leading spaces
    var startDay: int = first.weekday()
    for var i: int = 0; i < startDay; i++ =>
        print("   ")

    // Print days
    for var day: int = 1; day <= daysInMonth; day++ =>
        if day < 10 =>
            print($" {day} ")
        else =>
            print($"{day} ")

        var d: Date = Date.fromYmd(year, month, day)
        if d.weekday() == 6 =>
            print("\n")
    print("\n")

// Usage
printMonthCalendar(2025, 12)
```

### Recurring Dates

```sindarin
fn getNextMonthlyOccurrence(dayOfMonth: int): Date =>
    var today: Date = Date.today()
    var thisMonth: Date = Date.fromYmd(today.year(), today.month(), dayOfMonth)

    if today.isBefore(thisMonth) || today.equals(thisMonth) =>
        return thisMonth
    else =>
        return thisMonth.addMonths(1)

// Usage: Get next 15th of the month
var nextPayday: Date = getNextMonthlyOccurrence(15)
print($"Next payday: {nextPayday.toIso()}\n")
```

### Quarter Calculations

```sindarin
fn getQuarter(d: Date): int =>
    return (d.month() - 1) / 3 + 1

fn getQuarterStart(d: Date): Date =>
    var q: int = getQuarter(d)
    var startMonth: int = (q - 1) * 3 + 1
    return Date.fromYmd(d.year(), startMonth, 1)

fn getQuarterEnd(d: Date): Date =>
    var q: int = getQuarter(d)
    var endMonth: int = q * 3
    var end: Date = Date.fromYmd(d.year(), endMonth, 1)
    return end.endOfMonth()

// Usage
var today: Date = Date.today()
print($"Current quarter: Q{getQuarter(today)}\n")
print($"Quarter start: {getQuarterStart(today).toIso()}\n")
print($"Quarter end: {getQuarterEnd(today).toIso()}\n")
```

---

## Conversion Between Date and Time

Convert between `Date` and `Time` types when needed.

### Date to Time

```sindarin
var d: Date = Date.fromYmd(2025, 6, 15)
var t: Time = d.toTime()  // Midnight on that date
print($"{t.toIso()}\n")   // 2025-06-15T00:00:00.000Z
```

### Time to Date

```sindarin
var t: Time = Time.now()
var d: Date = t.toDate()  // Extracts just the date portion
print($"{d.toIso()}\n")   // 2025-12-30
```

---

## Memory Management

`Date` values are lightweight and integrate with Sindarin's arena-based memory management. String results from formatting methods are allocated from the arena and automatically freed when the arena is destroyed.

```sindarin
fn getFormattedDate(): str =>
    var today: Date = Date.today()
    return today.format("MMMM D, YYYY")
    // Formatted string is returned and remains valid
```

---

## See Also

- [TIME.md](TIME.md) - Time and datetime operations
- [README.md](../README.md) - Language overview
- [MEMORY.md](MEMORY.md) - Arena memory management details
