# ISSUE-041 Function Overloading Support

## Goal
Enable the Hoo language to allow **function overloading** for both free (top‑level) functions and member functions of classes. Multiple functions may share the same identifier **iff** their parameter type signatures differ. Overloaded functions must be uniquely identified at the binary level through a **mangling scheme** that incorporates the full parameter type list.

---
### Motivation
* Provide a more expressive API surface similar to C++/Java.
* Allow idiomatic generic libraries (e.g., `print(int)`, `print(string)`).
* Reduce boilerplate by avoiding manually‑named variants (`print_i`, `print_s`).

---
## High‑Level Changes
| Area | What needs to change | Rationale |
|------|----------------------|-----------|
| **Grammar** | Add `overload_list` production to allow multiple function declarations with identical name. | Parser must accept repeated identifiers and defer resolution to the semantic phase. |
| **Parsing** | Extend the AST with a `FunctionDecl` node that stores a `vector<Type>` for parameters and a `bool is_overload` flag. | Enables later mangling and overload set construction. |
| **Name Mangling** | New mangling format: `<name>__<param_type_ids>` (e.g., `foo__i64s`). Existing mangling for non‑overloaded functions stays unchanged. | Guarantees a unique symbol per overload and keeps backward compatibility. |
| **Demangling** | Update `hoo_demangle` utility to parse the new pattern and retrieve the base name plus parameter list. | Required for stack‑traces, reflection, and debugging. |
| **Symbol Table** | Change from a *single* entry per name to an **overload set** (`unordered_map<string, vector<FunctionSymbol>>`). | Allows fast lookup of the correct overload based on argument types during resolution. |
| **Type Resolution** | Implement overload resolution algorithm (exact match, then implicit conversion ranking). Emit `AmbiguousOverloadException` when multiple candidates qualify. | Mirrors typical language semantics and provides clear error reporting. |
| **Code Generation** |
- **Bytecode**: New opcodes `CALL_OVERLOADED` that include a **type‑signature identifier**. |
- **JIT**: Emit calls to runtime `hoo_resolve_overload` to obtain the concrete function pointer before invocation. |
| **Runtime API** |
- Add `hoo_resolve_overload(const char* mangled_name, int64_t* arg_type_ids, size_t argc)` returning a function pointer. |
- Introduce exception types: `AmbiguousOverloadException`, `NoMatchingOverloadException`. |
| **Execution** | During a call site, evaluate argument types, construct the mangled signature, and invoke the resolved address. | Ensures proper dispatch at runtime for dynamic languages (JIT & interpreter). |
| **Testing** | New unit tests covering: simple overloads, member overloads, ambiguous calls, and error cases. |
| **Documentation** | Update `docs/runtime/api/index.md`, `README.md`, and language spec sections about functions. |

---
## Detailed Work Plan
### 1. Grammar & Parser
1. Modify `parser.y` (or equivalent) to allow repeated `function_decl` productions with the same identifier.
2. Introduce rule `function_decl_list : function_decl_list function_decl | function_decl`.
3. Attach semantic actions that build an `OverloadSet` entry in the AST.
4. Update lexer to recognize the same identifier token without conflict.

### 2. AST & Semantic Analysis
* Extend `FunctionDecl` structure:
```c
struct FunctionDecl {
    std::string name;
    std::vector<Type> param_types; // concrete Hoo type IDs
    Type return_type;
    bool is_method; // true for class members
    // ... existing fields
};
```
* In the symbol builder, group declarations with identical `name` into an `OverloadSet`.
* Perform **overload resolution** during type‑checking of call expressions:
  - Gather candidate functions.
  - Rank by exact match, then by implicit conversion rules.
  - If none match → emit `NoMatchingOverloadException`.
  - If multiple best matches → emit `AmbiguousOverloadException`.

### 3. Name Mangling & Demangling
* New format: `<base>__<type1>_<type2>_…` where each `typeN` is the numeric type ID defined in `hoo_runtime.h` (e.g., `i64` for `HOO_TYPE_INT64`).
* Implement helper `std::string mangle_overload(const FunctionDecl&)`.
* Extend `hoo_demangle.cpp` to parse this schema and return both base name and vector of type IDs.

### 4. Symbol Table & Resolution Engine
* Change symbol table entry type from `FunctionSymbol*` to `OverloadSet*`:
```c
struct OverloadSet {
    std::string base_name;
    std::vector<FunctionSymbol*> overloads;
};
```
* Provide API `FunctionSymbol* resolve_overload(const std::string& base, const std::vector<int64_t>& arg_types)`.
* Throw the new runtime exceptions on failure.

### 5. Code Generation
* **Bytecode**: Add opcode `CALL_OVERLOADED` that carries the mangled name as an immediate operand.
* **JIT**: Emit a call to `hoo_resolve_overload` at the call site, then an indirect call to the returned pointer.
* Update the optimizer to treat overloaded calls like normal calls after resolution.

### 6. Runtime API Adjustments
Create header `runtime/lib/hoo_overload.h` with:
```c
/* Resolve an overloaded function at runtime */
void* hoo_resolve_overload(const char* mangled_name,
                           const int64_t* arg_type_ids,
                           size_t argc);

/* Exception registration (optional) */
void hoo_register_overload_exception(int64_t type_id, const char* message);
```
Add corresponding implementations that look up the overload set, perform matching, and return the concrete function pointer.

### 7. Exception Types
Add new type IDs in `hoo_runtime.h`:
```c
#define HOO_TYPE_AMBIGUOUS_OVERLOAD 130
#define HOO_TYPE_NO_MATCHING_OVERLOAD 131
```
Provide constructors `hoo_ambiguous_overload_new(message)` and `hoo_no_matching_overload_new(message)`.

### 8. Testing Strategy
* **Positive Tests** – simple overloads, method overloads, generic overloads.
* **Negative Tests** – ambiguous calls, mismatched argument count, unsupported conversions.
* **Integration Tests** – verify that generated bytecode/JIT correctly resolves at runtime.
* Update `tests/runtime/FunctionOverloadTest.cpp` with coverage of all cases.

### 9. Documentation Updates
* Add a section *Function Overloading* to `docs/runtime/api/index.md`.
* Create a new spec file `docs/specs/function_overloading.md` describing the language rule.
* Update `README.md` with a short feature overview and examples.

---
## Impact Assessment
* **Binary Compatibility** – Existing symbols remain unchanged; new mangled names are added, so old binaries continue to work.
* **Performance** – Overload resolution adds a small overhead at call sites (type introspection + hash lookup). The JIT can cache resolved pointers after the first call to mitigate.
* **Build System** – No changes needed beyond recompiling the runtime and compiler components.
* **Backward Compatibility** – Code that previously relied on manual name mangling will still compile because the old mangling scheme is retained for non‑overloaded functions.

---
## Implementation Timeline (approx.)
| Sprint | Tasks |
|--------|-------|
| 1 | Grammar, parser changes, AST extensions |
| 2 | Symbol table overhaul, overload set construction |
| 3 | Mangling/demangling implementation, runtime API stub |
| 4 | Bytecode & JIT integration, exception types |
| 5 | Unit & integration tests, documentation updates |
| 6 | Performance tuning, final review, merge |

---
*Prepared by Antigravity AI – 2026‑06‑21*

## Status
- **Date**: 2026-06-21
- **Status**: **PROPOSED (UNIMPLEMENTED)**
- **Priority**: High
- **Audit 2026-06-21**: No overload-set resolver, `CALL_OVERLOADED` opcode, runtime `hoo_resolve_overload`, or argument-type-aware call lowering was found. Existing symbol mangling tests do not amount to function overloading support.
