#pragma once

// ============================================================================
// Macro Utilities for Variadic Parameter Handling
// ============================================================================
//
// This header provides utility macros for working with variadic macro
// arguments, which is necessary for the X-Macro pattern used in the
// runtime class injection framework.

// ============================================================================
// Argument Counting
// ============================================================================

// VA_NARGS(...) - Count the number of macro arguments
// Supports up to 10 arguments
// Usage: VA_NARGS(a, b, c) expands to 3
#define VA_NARGS_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...) N
#define VA_NARGS(...) VA_NARGS_IMPL(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

// ============================================================================
// Token Manipulation
// ============================================================================

// MACRO_CONCAT(A, B) - Concatenate two tokens
// Must use MACRO_CONCAT_IMPL for proper expansion
// Usage: MACRO_CONCAT(foo, bar) expands to foobar
#define MACRO_CONCAT(A, B) MACRO_CONCAT_IMPL(A, B)
#define MACRO_CONCAT_IMPL(A, B) A##B

// MACRO_OVERLOAD(PREFIX, ...) - Select macro based on argument count
// Expands to PREFIX<N> where N is the number of arguments
// Usage: MACRO_OVERLOAD(FUNC, a, b) expands to FUNC2(a, b)
#define MACRO_OVERLOAD(PREFIX, ...) MACRO_CONCAT(PREFIX, VA_NARGS(__VA_ARGS__))(__VA_ARGS__)

// ============================================================================
// String Conversion
// ============================================================================

// STRINGIFY(x) - Convert macro argument to string literal
// Must use STRINGIFY_IMPL for proper expansion
// Usage: STRINGIFY(FOO) expands to "FOO"
#define STRINGIFY(x) STRINGIFY_IMPL(x)
#define STRINGIFY_IMPL(x) #x

// TOSTRING(x) - Alias for STRINGIFY for readability
#define TOSTRING(x) STRINGIFY(x)

// ============================================================================
// Conditional Compilation
// ============================================================================

// EMPTY() - Expands to nothing, useful for conditional macros
#define EMPTY()

// ============================================================================
// Debug Helpers (Optional)
// ============================================================================

// MESSAGE(msg) - Print debug message during preprocessing
// Uncomment to enable debug output
// #define MESSAGE(msg) #pragma message(TOSTRING(msg))
#define MESSAGE(msg)

#endif // MACRO_HELPERS_H
