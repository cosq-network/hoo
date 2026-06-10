# hoo Runtime API Reference

Welcome to the official API reference for the hoo Runtime Library (`hoort`). This documentation is designed for hoo developers to understand and utilize the built-in capabilities of the language.

The hoo runtime provides a set of modules that bridge the high-level language with the host system, offering efficient implementations of common data structures, mathematical operations, network communication, and system interactions.

## Core Modules

| Module | Description |
| :--- | :--- |
| **[Strings](string.md)** | Comprehensive string manipulation, UTF-8 support, and formatting. |
| **[Collections](collections.md)** | Dynamic arrays and type-safe maps for data storage. |
| **[Math](math.md)** | Mathematical constants, basic functions, and random number generation. |
| **[I/O](io.md)** | Console input and output functions. |

## System & Platform Modules

| Module | Description |
| :--- | :--- |
| **[Path](path.md)** | Cross-platform file path manipulation and normalization. |
| **[Networking](net.md)** | URL parsing and a robust HTTP client. |

## Usage Convention

In hoo source code, runtime functions are called using class-based method syntax. For example:

- `Math.abs(x)` — static methods use the class name
- `s.length()` — instance methods are called on the variable
- `Array.new()` — constructors use `.new()` syntax

Standard I/O functions like `print` and `println` are available globally (without a prefix or class qualifier).

### Memory Management

All complex objects in hoo (Strings, Arrays, Maps, etc.) are automatically managed. You generally don't need to manually free objects — the runtime handles deallocation when an object is no longer reachable.
