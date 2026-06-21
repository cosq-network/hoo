# ISSUE-042 Decimal Intrinsic Data Type

## Goal
Introduce a new **fixed‑precision decimal** intrinsic type (`Decimal`) based on base‑10 arithmetic. The type must be first‑class in the Hoo language, supporting arithmetic, comparison, and logical operations, and be usable in all existing language constructs.

---
### Motivation
* Provide accurate financial and scientific calculations where binary floating‑point errors are unacceptable.
* Offer a deterministic, platform‑independent numeric representation.
* Align Hoo with languages that expose a decimal type (e.g., .NET `decimal`, Java `BigDecimal`).

---
## High‑Level Changes
| Area | Required Change | Reason |
|------|----------------|--------|
| **Grammar** | Add a literal form `DECIMAL_LITERAL` (`[0-9]+\.[0-9]+` or scientific notation `e`) and a type specifier `decimal` (e.g., `decimal x = 12.34;`). | Enables source‑level declaration and literals. |
| **Parsing** | Extend the lexer to recognise `DECIMAL_LITERAL`. Update the parser to create a `DecimalLiteral` AST node containing a string representation and a `Decimal` type identifier. | Captures literal values for later conversion. |
| **Semantic Analysis** | Add a `HOO_TYPE_DECIMAL` entry in `hoo_runtime.h`. Ensure type checking for binary operators (`+ – * / %`) and logical operators (`== != < <= > >=`) works with `Decimal`. | Provides compile‑time type safety. |
| **Name Mangling** | Extend mangling to include the decimal type ID (e.g., `foo__d` where `d` maps to `HOO_TYPE_DECIMAL`). | Guarantees unique symbols for overloads involving `Decimal`. |
| **Demangling** | Update `hoo_demangle` to recognise the new `d` token and map it back to `HOO_TYPE_DECIMAL`. | Needed for debugging and reflection. |
| **Code Generation** | • **Bytecode**: Add opcodes `DEC_ADD`, `DEC_SUB`, `DEC_MUL`, `DEC_DIV`, `DEC_MOD`, `DEC_NEG`, and comparison opcodes (`DEC_EQ`, `DEC_NEQ`, `DEC_LT`, `DEC_LE`, `DEC_GT`, `DEC_GE`).
• **JIT**: Emit calls to runtime functions (`hoo_decimal_add`, `hoo_decimal_sub`, …). |
| **Runtime API** | Create `runtime/lib/hoo_decimal.h/.cpp` exposing:
```c
// Allocation & conversion
HooDecimal hoo_decimal_new_from_string(const char* str);
HooDecimal hoo_decimal_new_from_int64(int64_t val);
char*      hoo_decimal_to_string(HooDecimal dec);

// Arithmetic (return new Decimal, retain ARC semantics)
HooDecimal hoo_decimal_add(HooDecimal a, HooDecimal b);
HooDecimal hoo_decimal_sub(HooDecimal a, HooDecimal b);
HooDecimal hoo_decimal_mul(HooDecimal a, HooDecimal b);
HooDecimal hoo_decimal_div(HooDecimal a, HooDecimal b);
HooDecimal hoo_decimal_mod(HooDecimal a, HooDecimal b);
HooDecimal hoo_decimal_neg(HooDecimal a);

// Comparison (return int64 bool)
int64_t    hoo_decimal_eq(HooDecimal a, HooDecimal b);
int64_t    hoo_decimal_neq(HooDecimal a, HooDecimal b);
int64_t    hoo_decimal_lt(HooDecimal a, HooDecimal b);
int64_t    hoo_decimal_le(HooDecimal a, HooDecimal b);
int64_t    hoo_decimal_gt(HooDecimal a, HooDecimal b);
int64_t    hoo_decimal_ge(HooDecimal a, HooDecimal b);
```
All functions obey ARC: returned `HooDecimal` objects have refcount 1 and must be released by the caller. |
| **Execution** | The interpreter/JIT will treat `Decimal` values like other managed objects. Arithmetic opcodes call the runtime functions, which perform **fixed‑scale** arithmetic (e.g., 28 decimal places) using an internal 128‑bit integer representation. Logical operators evaluate to `int64_t` booleans. |
| **Testing** | Add unit tests for literal parsing, each arithmetic operator, edge cases (overflow, division by zero), and logical comparisons. |
| **Documentation** | Update `docs/runtime/api/index.md` with a `Decimal` section, add a language spec entry in `docs/specs/decimal_type.md`, and mention the type in `README.md`. |

---
## Detailed Work Plan
### 1. Grammar & Lexer
* Add token `DECIMAL_LITERAL` pattern: `([0-9]+\.[0-9]*|\.[0-9]+)([eE][+-]?[0-9]+)?`.
* Update `type_specifier` rule to include `"decimal"`.
* Production `literal` → `DECIMAL_LITERAL` creates `DecimalLiteral` node.

### 2. AST & Semantic Analysis
* New AST node `DecimalLiteral` with fields `std::string text` and `int64_t type_id = HOO_TYPE_DECIMAL`.
* Extend type‑checking to allow binary operators where at least one operand is `Decimal`. Implicit conversion from integer literals to `Decimal` may be added (optional).

### 3. Mangling / Demangling
* Use short code `d` for `HOO_TYPE_DECIMAL` (e.g., `add__d` for `add(Decimal)` overloads).
* Extend `mangle_type_id` mapping and demangler parser accordingly.

### 4. Symbol Table & Resolution
* No new changes beyond type handling; overload resolution already supports new type IDs.

### 5. Code Generation
* **Bytecode**: Define the new opcodes in `opcode.h` and modify the codegen visitor to emit them when encountering `Decimal` operations.
* **JIT**: Link against the `hoo_decimal` runtime functions; generate calls with proper calling convention.

### 6. Runtime Implementation
* Implement a **fixed‑scale decimal** using 128‑bit integer (`__int128`) with a compile‑time scale (e.g., 28 digits). Operations perform integer arithmetic with scaling adjustments.
* Provide conversion utilities to/from string and `int64_t` for ease of use.
* Register the new type ID `HOO_TYPE_DECIMAL` (choose a unique value, e.g., `120` after existing types).

### 7. Exception Types
* Define `HOO_TYPE_DECIMAL_DIVIDE_BY_ZERO` (`132`) and `HOO_TYPE_DECIMAL_OVERFLOW` (`133`).
* Runtime functions set `errno` and optionally throw these exceptions via the existing exception mechanism.

### 8. Testing Strategy
* **Parsing Tests** – verify literals are recognized and produce correct AST nodes.
* **Arithmetic Tests** – cover addition, subtraction, multiplication, division, modulus, and unary negation with typical and edge values.
* **Logical Tests** – equality, inequality, ordering.
* **Error Tests** – division by zero, overflow handling.
* Include tests in `tests/runtime/DecimalTest.cpp`.

### 9. Documentation Updates
* Add a **Decimal** subsection to `docs/runtime/api/index.md` describing the API functions.
* Create `docs/specs/decimal_type.md` explaining the language syntax, literal format, and semantics.
* Update `README.md` with a short feature highlight and example code snippet.

---
## Impact Assessment
* **Binary Compatibility** – New type ID and mangling do not affect existing symbols.
* **Performance** – Fixed‑scale arithmetic is slightly slower than native floating‑point but deterministic; acceptable for financial workloads.
* **Memory** – `Decimal` objects occupy a small, fixed size (e.g., 24 bytes: 16‑byte integer + ARC header).
* **Portability** – Implementation relies only on standard C++ integer arithmetic, so it works across all supported platforms.

---
## Implementation Timeline (approx.)
| Sprint | Tasks |
|--------|-------|
| 1 | Grammar, lexer, AST node, type ID registration |
| 2 | Mangling/demangling updates, symbol table integration |
| 3 | Runtime `hoo_decimal` implementation |
| 4 | Codegen (bytecode + JIT) changes |
| 5 | Unit & integration tests, documentation |
| 6 | Performance tuning, final review, merge |

---
*Prepared by Antigravity AI – 2026‑06‑21*

## Status
- **Date**: 2026-06-21
- **Status**: **PROPOSED (UNIMPLEMENTED)**
- **Priority**: Medium
- **Audit 2026-06-21**: No decimal token/type, `HOO_TYPE_DECIMAL`, `hoo_decimal` runtime, decimal AST node, decimal bytecodes, or decimal JIT lowering was found. This remains a design proposal.
