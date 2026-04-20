# Hooc Standard Library Design

This document outlines the planned standard library modules for the Hooc programming language. These modules will provide essential functionality for common programming tasks.

**Last Updated:** April 20, 2026

## Table of Contents

1. [Overview](#overview)
2. [Core Modules](#core-modules)
3. [Collections Module](#collections-module)
4. [IO Module](#io-module)
5. [Math Module](#math-module)
6. [Network Module](#network-module)
7. [Time Module](#time-module)
8. [Implementation Priority](#implementation-priority)

## Overview

The Hooc standard library is organized into modules under the `hoo` namespace. Each module provides a focused set of functionality:

```hoo
// Using standard library modules
import hoo.collections;
import hoo.io;
import hoo.math;
import hoo.time;
```

## Core Modules

### `hoo` - Core Types

Already implemented:

| Class | Description |
|-------|-------------|
| `hoo.String` | UTF-8 string with ARC management |
| `hoo.Array` | Generic dynamic array |

### `hoo.io` - Input/Output

**Status:** Basic implemented, Full in progress

**Functions:**
```hoo
// Already implemented
hoo.print(message: string) -> void
hoo.println(message: string) -> void
hoo.readline() -> string
hoo.readchar() -> int64
```

**Planned:**

```hoo
// File operations
class File {
    constructor(path: string, mode: string)  // "r", "w", "a"
    func read() -> string
    func readLine() -> string
    func readLines() -> string[]
    func write(content: string) -> int64
    func writeLine(content: string) -> int64
    func seek(position: int64) -> bool
    func tell() -> int64
    func close() -> void
    func eof() -> bool
}

// Directory operations
class Directory {
    constructor(path: string)
    func exists() -> bool
    func create() -> bool
    func remove() -> bool
    func list() -> string[]
    func listFiles() -> string[]
    func listDirectories() -> string[]
    static func current() -> string
    static func home() -> string
    static func temp() -> string
}

// Path utilities
class Path {
    func join(parts: string[]) -> string
    func normalize(path: string) -> string
    func abs(path: string) -> string
    func baseName(path: string) -> string
    func dirName(path: string) -> string
    func extension(path: string) -> string
    func exists(path: string) -> bool
    func isFile(path: string) -> bool
    func isDirectory(path: string) -> bool
}

// Console
class Console {
    func read() -> string
    func readLine() -> string
    func write(message: string) -> void
    func writeLine(message: string) -> void
    func clear() -> void
    func setColor(color: string) -> void
    func resetColor() -> void
}
```

## Collections Module

**Status:** Planned

### `hoo.collections` - Collection Types

```hoo
// List - Dynamic array with additional methods
class List<T> {
    constructor()
    constructor(capacity: int64)

    func add(item: T) -> void
    func addAll(items: T[]) -> void
    func insert(index: int64, item: T) -> void
    func remove(index: int64) -> T?
    func removeItem(item: T) -> bool
    func clear() -> void

    func get(index: int64) -> T?
    func set(index: int64, item: T) -> void

    func contains(item: T) -> bool
    func indexOf(item: T) -> int64
    func lastIndexOf(item: T) -> int64

    func size() -> int64
    func isEmpty() -> bool
    func capacity() -> int64

    func forEach(action: func(T) -> void) -> void
    func map<U>(transform: func(T) -> U) -> List<U>
    func filter(predicate: func(T) -> bool) -> List<T>
    func reduce<U>(initial: U, reducer: func(U, T) -> U) -> U

    func toArray() -> T[]
    func toString() -> string
}

// Map - Key-value store
class Map<K, V> {
    constructor()
    constructor(capacity: int64)

    func set(key: K, value: V) -> void
    func get(key: K) -> V?
    func getOrDefault(key: K, default: V) -> V
    func containsKey(key: K) -> bool
    func containsValue(value: V) -> bool
    func remove(key: K) -> V?
    func clear() -> void

    func size() -> int64
    func isEmpty() -> bool
    func keys() -> K[]
    func values() -> V[]
    func entries() -> (K, V)[]

    func forEach(action: func(K, V) -> void) -> void
}

// Set - Unique elements
class Set<T> {
    constructor()
    constructor(capacity: int64)

    func add(item: T) -> bool
    func remove(item: T) -> bool
    func contains(item: T) -> bool
    func clear() -> void

    func size() -> int64
    func isEmpty() -> bool

    func union(other: Set<T>) -> Set<T>
    func intersection(other: Set<T>) -> Set<T>
    func difference(other: Set<T>) -> Set<T>

    func toArray() -> T[]
}

// Queue - FIFO
class Queue<T> {
    constructor()
    constructor(capacity: int64)

    func enqueue(item: T) -> void
    func dequeue() -> T?
    func peek() -> T?

    func size() -> int64
    func isEmpty() -> bool
    func clear() -> void
}

// Stack - LIFO
class Stack<T> {
    constructor()
    constructor(capacity: int64)

    func push(item: T) -> void
    func pop() -> T?
    func peek() -> T?

    func size() -> int64
    func isEmpty() -> bool
    func clear() -> void
}
```

## Math Module

**Status:** Planned

### `hoo.math` - Mathematical Functions

```hoo
// Math constants
const PI = 3.141592653589793
const E = 2.718281828459045
const TAU = 6.283185307179586
const INF = Infinity
const NEG_INF = -Infinity
const NAN = NaN

// Basic functions
func abs(x: int64) -> int64
func abs(x: double) -> double
func min(a: int64, b: int64) -> int64
func min(a: double, b: double) -> double
func max(a: int64, b: int64) -> int64
func max(a: double, b: double) -> double
func clamp(value: double, min: double, max: double) -> double
func sign(x: int64) -> int64
func sign(x: double) -> double

// Power and roots
func pow(base: double, exponent: double) -> double
func sqrt(x: double) -> double
func cbrt(x: double) -> double
func hypot(x: double, y: double) -> double

// Trigonometric functions
func sin(x: double) -> double
func cos(x: double) -> double
func tan(x: double) -> double
func asin(x: double) -> double
func acos(x: double) -> double
func atan(x: double) -> double
func atan2(y: double, x: double) -> double
func sinh(x: double) -> double
func cosh(x: double) -> double
func tanh(x: double) -> double

// Exponential and logarithmic
func exp(x: double) -> double
func exp2(x: double) -> double
func expm1(x: double) -> double
func log(x: double) -> double
func log10(x: double) -> double
func log2(x: double) -> double
func log1p(x: double) -> double

// Rounding
func floor(x: double) -> double
func ceil(x: double) -> double
func round(x: double) -> double
func trunc(x: double) -> double
func fract(x: double) -> double

// Random number generation
class Random {
    constructor()
    constructor(seed: int64)

    func nextInt() -> int64
    func nextInt(max: int64) -> int64
    func nextDouble() -> double
    func nextBool() -> bool
    func nextBytes(count: int64) -> int64[]
    func shuffle<T>(array: T[]) -> T[]
}

// Number utilities
func isEven(n: int64) -> bool
func isOdd(n: int64) -> bool
func isPrime(n: int64) -> bool
func gcd(a: int64, b: int64) -> int64
func lcm(a: int64, b: int64) -> int64
func factorial(n: int64) -> int64
func fibonacci(n: int64) -> int64
```

## Network Module

**Status:** Planned

### `hoo.net` - Network Operations

```hoo
// URL utilities
class URL {
    constructor(urlString: string)

    func getScheme() -> string
    func getHost() -> string
    func getPort() -> int64
    func getPath() -> string
    func getQuery() -> string
    func getFragment() -> string

    static func parse(urlString: string) -> URL?
    func toString() -> string
}

// HTTP Client
class HttpClient {
    constructor()
    func get(url: string) -> HttpResponse
    func post(url: string, body: string) -> HttpResponse
    func put(url: string, body: string) -> HttpResponse
    func delete(url: string) -> HttpResponse
    func setHeader(key: string, value: string) -> void
    func setTimeout(timeout: int64) -> void
}

class HttpResponse {
    func getStatusCode() -> int64
    func getStatusText() -> string
    func getBody() -> string
    func getHeaders() -> Map<string, string>
    func isSuccess() -> bool
}

// TCP/UDP Sockets (Future)
class Socket {
    constructor(host: string, port: int64)
    func connect() -> bool
    func close() -> void
    func send(data: int64[]) -> int64
    func receive(bufferSize: int64) -> int64[]
    func isConnected() -> bool
}

class ServerSocket {
    constructor(port: int64)
    func bind() -> bool
    func listen(backlog: int64) -> void
    func accept() -> Socket
    func close() -> void
}
```

## Time Module

**Status:** Planned

### `hoo.time` - Date and Time

```hoo
// Duration
class Duration {
    constructor(nanoseconds: int64)
    constructorMillis(millis: int64)
    constructorSeconds(seconds: int64)

    func toNanos() -> int64
    func toMillis() -> int64
    func toSeconds() -> int64
    func toMinutes() -> int64
    func toHours() -> int64
    func toDays() -> int64

    func isZero() -> bool
    func isNegative() -> bool

    func plus(other: Duration) -> Duration
    func minus(other: Duration) -> Duration
    func multipliedBy(factor: int64) -> Duration
    func dividedBy(divisor: int64) -> Duration
}

// Instant - Point in time
class Instant {
    constructor(epochSeconds: int64)
    constructorOfNow()

    func toEpochMillis() -> int64
    func toEpochNanos() -> int64

    func plus(duration: Duration) -> Instant
    func minus(duration: Duration) -> Duration
    func until(other: Instant) -> Duration

    func isBefore(other: Instant) -> bool
    func isAfter(other: Instant) -> bool
}

// Local Date
class LocalDate {
    constructor(year: int64, month: int64, day: int64)
    constructorOfToday()

    func getYear() -> int64
    func getMonth() -> int64
    func getDay() -> int64
    func getDayOfWeek() -> int64  // 1=Monday, 7=Sunday

    func plusDays(days: int64) -> LocalDate
    func minusDays(days: int64) -> LocalDate
    func plusMonths(months: int64) -> LocalDate
    func minusMonths(months: int64) -> LocalDate
    func plusYears(years: int64) -> LocalDate
    func minusYears(years: int64) -> LocalDate

    func isLeapYear() -> bool
    func daysInMonth() -> int64

    func toEpochDay() -> int64
    func format(format: string) -> string
}

// Local Time
class LocalTime {
    constructor(hour: int64, minute: int64)
    constructor(hour: int64, minute: int64, second: int64)
    constructorOfNow()

    func getHour() -> int64
    func getMinute() -> int64
    func getSecond() -> int64
    func getNano() -> int64

    func plusHours(hours: int64) -> LocalTime
    func plusMinutes(minutes: int64) -> LocalTime
    func plusSeconds(seconds: int64) -> LocalTime

    func format(format: string) -> string
}

// Local DateTime
class LocalDateTime {
    constructor(date: LocalDate, time: LocalTime)
    constructor(year: int64, month: int64, day: int64, hour: int64, minute: int64)
    constructorOfNow()

    func getDate() -> LocalDate
    func getTime() -> LocalTime

    func plus(duration: Duration) -> LocalDateTime
    func minus(duration: Duration) -> LocalDateTime

    func toInstant(offset: int64) -> Instant  // offset in seconds
    func format(format: string) -> string

    static func parse(text: string, format: string) -> LocalDateTime?
}

// TimeZone
class TimeZone {
    constructor(offsetSeconds: int64)
    constructor(zoneId: string)  // "America/New_York", "UTC"

    func getOffset(instant: Instant) -> int64
    func getDisplayName() -> string
    func isUTC() -> bool

    static func utc() -> TimeZone
    static func systemDefault() -> TimeZone
}
```

## Implementation Priority

Based on the roadmap, the following implementation order is planned:

### Phase 9 (Q3 2026)

1. **Collections Module** - High priority for data processing
   - `List<T>`, `Map<K,V>`, `Set<T>`
   - Iterator protocol

2. **Math Module** - Scientific computing needs
   - Basic math functions
   - Random number generation

### Phase 10 (Q4 2026)

3. **IO Module (Full)** - File system access
   - File I/O
   - Directory operations
   - Path utilities

4. **Time Module** - Date/time handling
   - Duration, Instant
   - LocalDate, LocalTime, LocalDateTime

### Future (2027+)

5. **Network Module** - HTTP, sockets
6. **Regex Module** - Pattern matching
7. **JSON Module** - JSON parsing/serialization
8. **XML Module** - XML processing

## See Also

- [Features Guide](features.md) - Current language features
- [Implementation Status](implementation-status.md) - Current implementation status
- [Roadmap](roadmap.md) - Development timeline
- [Module System Design](module-system-design.md) - Module loading and resolution