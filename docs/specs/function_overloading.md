# Function Overloading Specification

## Overview
Hoo supports function and method overloading, allowing multiple functions or methods in the same scope to share the same name, provided their parameter signatures differ. This feature increases expressiveness, eliminating the need for manually disambiguated function names like `print_int64` and `print_string`.

## Mangling Scheme
To maintain binary compatibility and ensure unique identification at the runtime level, overloaded functions undergo a specific name mangling process:
- Standard functions maintain their usual mangled format.
- Overloaded functions are mangled with the pattern `<name>__<param_type_ids>` (e.g., `abs__i8`, `push__i64`).
- Type identifiers correspond to single-character mappings (e.g., `i` = int64, `f` = float64/double, `s` = string, `b` = boolean, `p` = pointer/object).

## Overload Resolution Rules
When an overloaded function is called, the compiler (or the runtime JIT via `hoo_resolve_overload`) evaluates the argument types to determine the best match:
1. **Exact Match**: A signature where all parameter types perfectly match the argument types.
2. **Implicit Widening**: If an exact match is not found, the compiler looks for compatible widening conversions (e.g., `int8` to `int64`, `f8` to `double`).
3. **Any/Object Fallback**: Signatures accepting generic `any` or object types are ranked lowest and chosen only as a last resort.
4. **Ambiguity**: If two or more signatures match equally well, an `AmbiguousOverloadException` is raised. If no signatures match, a `NoMatchingOverloadException` is raised.

## AST and Compilation
In the AST, multiple functions or methods with the same name are grouped into an `OverloadList` node. This grouping is performed during the AST construction phase to prevent parser ambiguity. The bytecode generation translates statically unresolved overloaded calls into a `CALL_OVERLOADED` instruction, which defers resolution to the LLVM ORC JIT engine at runtime.

## Supported Built-ins
Overloading is supported for user-defined code as well as many core standard library APIs:
- `Math.abs`, `Math.min`, `Math.max`, `Math.sign`
- `String.from`
- `Regex.compile`
- `new DateTime(...)`, `DateTime.parse`
- `new Buffer(...)`
- `Tensor.new`

*Document generated for Hoo v0.2.0.*
