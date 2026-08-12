# Name Mangling & Demangling Conventions

This document describes how the Symbol Mangler (`SymbolMangler.cpp`), the Bytecode
Code Generator (`HVMCodeGenerator.cpp`), and the JIT engine (`HVMJIT.cpp`) produce
and consume mangled symbol names. It covers the `_F_` (function) and `_H_` (header)
name families, the type encoding scheme, module qualification, class member
qualification, and the special conventions for runtime module functions.

---

## Table of Contents

1. [Symbol Prefixes](#1-symbol-prefixes)
2. [Type Encoding](#2-type-encoding)
3. [Function Name Mangling (`_F_`)](#3-function-name-mangling-_f_)
   - [3.1 Module Path](#31-module-path)
   - [3.2 Class Member Qualification](#32-class-member-qualification)
   - [3.3 Return Type and Parameters](#33-return-type-and-parameters)
   - [3.4 Special Conventions for Runtime Functions](#34-special-conventions-for-runtime-functions)
   - [3.5 Complete Examples](#35-complete-examples)
4. [Module Symbol Mangling (`_H_`)](#4-module-symbol-mangling-_h_)
5. [Name Demangling](#5-name-demangling)
   - [5.1 Demangling Algorithm](#51-demangling-algorithm)
   - [5.2 Component Splitting](#52-component-splitting)
   - [5.3 Name Classification](#53-name-classification)
6. [JIT Symbol Resolution](#6-jit-symbol-resolution)
   - [6.1 `buildLookupCandidates`](#61-buildlookupcandidates)
   - [6.2 `buildRuntimeSymbols`](#62-buildruntimesymbols)
7. [Practical Guide](#7-practical-guide)
   - [7.1 Adding a New Runtime Module Function](#71-adding-a-new-runtime-module-function)
   - [7.2 Debugging Mismatches](#72-debugging-mismatches)
   - [7.3 Type Codes Quick Reference](#73-type-codes-quick-reference)

---

## 1. Symbol Prefixes

Every symbol name begins with a two-character prefix that identifies the kind of entity:

| Prefix | Kind | Mangled by | Example |
|--------|------|------------|---------|
| `_F_` | Function | `SymbolMangler::mangleFunctionName` | `_F_M_hoo_E_math_abs_int64_v_p` |
| `_H_` | Module-level (global variable, data symbol) | `SymbolMangler::mangleModuleSymbol` | `_H_hoo_E_someGlobal` |

**Source:** `SymbolMangler.cpp` lines 168-170 (mangleFunctionName) and lines 259-276 (mangleModuleSymbol)

### Backward compatibility path (`HVMJIT.cpp`)

The JIT also recognizes plain names that lack a prefix (e.g. `"constructor"`, `"main"`)
as legacy unqualified function names used by the bytecode interpreter and older tests.

---

## 2. Type Encoding

Type names are encoded to short codes via `SymbolMangler::mangleType()` and
decoded via `SymbolMangler::demangleType()`.

### 2.1 Primitive Type Codes

Defined in `getTypeCodeMap()` (SymbolMangler.cpp lines 55-72):

| Hoo Type | Code |
|-----------|------|
| `int8` | `i1` |
| `byte` | `u1` |
| `int64`, `int` | `i8` |
| `float` | `f` |
| `double`, `f64` | `d` |
| `bool` | `b` |
| `char` | `c` |
| `string` | `s` |
| `any` | `y` |
| `void` | `v` |
| `ptr` | `p` |
| `array` | `a` |

Unknown or object types fall back to `"o"`.

### 2.2 Structured Types

| Pattern | Encoding | Example |
|---------|----------|---------|
| `nullable<T>` / `T?` | `O` prefix | `Oi8` = `int64?` |
| `T[]` | `A` prefix | `Ai8` = `int64[]` |
| `map[K,V]` | `M` prefix | `Mi8s` = `map[int64,string]` |
| Named objects | `Q` + hex(name) + `Z` | `Q4d657863Z` = `"Exc"` (hex encoded) |

**Source:** `SymbolMangler::mangleType()` lines 624-696, `SymbolMangler::demangleType()` lines 517-622

---

## 3. Function Name Mangling (`_F_`)

The `mangleFunctionName` method in `SymbolMangler.cpp` (lines 168-257) builds a
function symbol from a `MangledFunctionParams` struct:

```
_F_ [M_<module>_E_] [<className>_ [<baseClass>_] [<modifiers>_] [CT_|DT_|<fn>_] ]
    [static_] [virtual_] [<modifierCodes>_] <returnType>_ <paramTypes>_
```

Trailing `_` is stripped from the final string.

### 3.1 Module Path

When `MangledFunctionParams.modulePath` is non-empty, the mangler inserts:

```
M_<part1>_<part2>_..._E_
```

- `M_` marks the start of the module path
- Each path component is `encodeComponent()`-encoded
- `_E_` marks the end of the module path

**Example:** `modulePath = {"hoo"}` → `M_hoo_E_`

The component encoder (`encodeComponent`, line 144) replaces non-alphanumeric
characters with `_hex_` sequences. Simple names like `"hoo"`, `"test"` pass through
verbatim.

### 3.2 Class Member Qualification

When `MangledFunctionParams.className` is non-empty, the mangler emits:

```
<className>_ [<baseClassName>_] [<modifierCodes>_] [CT_|DT_|<fn>_] [static_] [virtual_] [<modifierCodes>_]
```

**Modifier codes** (from `getModifierCodeMap()`, line 74):
- `SINGLETON` → `N`
- `IMMUTABLE` → `I`
- `SERVICE` → `S`
- `FINAL` → `Z`

**Special constructors/destructors:**
- Constructor → `CT_` instead of function name
- Destructor → `DT_` instead of function name

**Function modifier codes** (from `getFunctionModifierCodeMap()`, line 84):
- `PUBLIC` → `Pb`
- `PRIVATE` → `Pv`

### 3.3 Return Type and Parameters

After the function name (or `CT_`/`DT_`) and modifiers, the mangler appends:

```
<returnType>_ <paramType1>_ <paramType2>_ ...
```

Each type is mangled via `SymbolMangler::mangleType()` using the type code table
above.

### 3.4 Special Conventions for Runtime Functions

**This is the most important convention for developers.**

In Hoo source code, runtime functions are called using class-based method syntax:
`Math.abs(x)`, `s.length()`, `Thread.spawn()`. The code generator internally maps
each class name to a module prefix via `classToPrefix()` (e.g. `Math` → `math_`,
`Thread` → `thread_`, `String` → `string_`), then detects the resulting prefix
at line 1048-1058 of `HVMCodeGenerator.cpp` and **hardcodes** the mangling
parameters (lines 1072-1077):

```cpp
mp.returnType = "void";  // → code "v"
if (funcCall->getArguments()) {
    for (const auto& arg : funcCall->getArguments()->getArguments()) {
         mp.parameterTypes.push_back("ptr");  // → code "p"
    }
}
```

**Therefore, every runtime module function registered in `buildRuntimeSymbols()` MUST use:**

| Arity | Pattern | Example |
|-------|---------|---------|
| 0 params | `_F_M_hoo_E_<name>_p` | `thread_self()` → `_F_M_hoo_E_thread_self_p` |
| 1 param | `_F_M_hoo_E_<name>_p_p` | `thread_join(t)` → `_F_M_hoo_E_thread_join_p_p` |
| 2 params | `_F_M_hoo_E_<name>_p_p_p` | `thread_spawn(f,a)` → `_F_M_hoo_E_thread_spawn_p_p_p` |
| 3 params | `_F_M_hoo_E_<name>_v_p_p_p` | `m.set(key,val)` → `_F_M_hoo_E_map_set_string_int64_v_p_p_p` |

The `v` (void return) and `p` (ptr parameters) are **hardcoded irrespective of the
actual C function signature**. The JIT wrappers handle the actual type conversions.

**Class-to-prefix mapping** in `HVMCodeGenerator.cpp::classToPrefix()` determines
which class names map to `hoo` module prefixes:

```cpp
if (functionName == "print" || functionName == "println" ||
    functionName == "readline" || functionName == "readchar" ||
    functionName.rfind("fs_", 0) == 0 ||
    functionName.rfind("system_", 0) == 0 ||
    functionName.rfind("regex_", 0) == 0 ||
    functionName.rfind("uuid_", 0) == 0 ||
    functionName.rfind("encoding_", 0) == 0 ||
    functionName.rfind("math_", 0) == 0 ||
    functionName.rfind("character_", 0) == 0 ||
    functionName.rfind("thread_", 0) == 0) {
    mp.modulePath = {"hoo"};
}
```

### 3.5 Complete Examples

| Context | Example | Mangled Name |
|---------|---------|--------------|
| User function, no module | `func test() int64 {}` | `_F_test_i8` |
| User function, module `test` | `func test() int64 {}` in module `test` | `_F_M_test_E_test_i8` |
| User function, module `test`, two int64 params | `func test(a: int64, b: int64) int64 {}` | `_F_M_test_E_test_i8_i8_i8` |
| Runtime, hoo module, 0 params | `thread_self()` | `_F_M_hoo_E_thread_self_p` |
| Runtime, hoo module, 1 param | `thread_join(tid)` | `_F_M_hoo_E_thread_join_p_p` |
| Runtime, hoo module, 2 params | `thread_spawn(f, a)` | `_F_M_hoo_E_thread_spawn_p_p_p` |
| Built-in, hoo module | `print(x)` | `_F_M_hoo_E_print_v_p` |
| Class method, no module | method `bar` in class `Foo` | `_F_Foo_bar_v` |
| Class constructor, module `app` | constructor of class `Foo` | `_F_M_app_E_Foo_CT_v` |
| Core runtime intrinsic | `hoo_alloc(size, typeId)` | `_F_hoo_alloc_p_i8_i8` |
| List core intrinsic | `hoo_anyarray_push(arr,type,data)` | `_F_hoo_anyarray_push_i8_p_i8_i8` |
| Dict core intrinsic | `hoo_hashmap_new(keyType,valueType)` | `_F_hoo_hashmap_new_p_i8_i8` |

---

## 4. Module Symbol Mangling (`_H_`)

Global variables and data symbols use a separate format produced by
`SymbolMangler::mangleModuleSymbol()` (lines 259-276):

```
_H_<modulePart1>_<modulePart2>_..._<symbolName>[_<kindTag>]
```

- `_H_` prefix
- Each component is `encodeComponent()`-encoded
- Optional kind tag suffix: `_fn`, `_ob`, `_ty`, `_tls`, `_nt`, `_uk`

**Example:** A global variable `counter` in module `app` → `_H_app_counter`

This form is used in `HVMCodeGenerator.cpp` line 119 for global variable symbols.

---

## 5. Name Demangling

`SymbolMangler::demangleSymbol()` (lines 278-462) parses both `_F_` and `_H_`
symbols into a `DemangledSymbol` struct with fields:

| Field | Meaning |
|-------|---------|
| `originalName` | The full mangled name |
| `modulePath` | Vector of module path components |
| `className` | Class name (if a member) |
| `baseClassName` | Base class name (if present) |
| `classModifiers` | e.g. `SINGLETON`, `FINAL` |
| `functionName` | Function name (or last component of `_H_` path) |
| `functionModifiers` | e.g. `PUBLIC`, `PRIVATE` |
| `returnType` | Demangled return type string |
| `parameterTypes` | Vector of demangled parameter type strings |
| `isConstructor` | Boolean |
| `isDestructor` | Boolean |
| `isStatic` | Boolean |
| `isVirtual` | Boolean |

### 5.1 Demangling Algorithm

1. Strip any kind tag suffix (`_fn`, `_ob`, `_ty`, `_tls`, `_nt`, `_uk`)
2. If starts with `_F_`:
   - Remove `_F_` prefix, split remaining by `_` into components
   - Look for `M_`...`_E_` module path markers
   - Extract class/function names (components before modifier/signature tokens)
   - Recognize modifier codes (`N`, `I`, `S`, `Z` for classes; `Pb`, `Pv` for functions)
   - Recognize `CT` and `DT` for constructors/destructors
   - Parse remaining components as return type + parameter types via `demangleType()`
3. If starts with `_H_`:
   - Remove `_H_` prefix, split remaining by `_` into components
   - Last component = symbol name, preceding components = module path

### 5.2 Component Splitting

The `splitComponents` function (lines 302-339) handles both plain and
hex-encoded components:

- **Encoded components** are delimited by `E...E` where the inner content uses
  `_hex_hex_` replacements for non-alphanumeric characters
- **Plain components** are split on `_` boundaries

### 5.3 Name Classification

The demangler determines whether a `_F_` name represents a class member or a
plain function based on the presence of `CT`/`DT` or class modifier codes:

- **If `CT` or `DT` present** → definitely a class member
- **If modifier codes present** → definitely a class member
- **Otherwise** → uses heuristic:
  - 3+ names before the signature → Class _ Base _ Func
  - 2 names → Class _ Func
  - 1 name → legacy or plain function

---

## 6. JIT Symbol Resolution

### 6.1 `buildLookupCandidates`

`HVMJIT.cpp` lines 82-133 generates a list of candidate symbol names when
resolving a call target. Given an input symbol name:

1. **Original name** is tried first
2. **Demangle + remangle** with the current module path injected
3. **Demangle + remangle as class member** (className, baseClassName from
   demangled result)

This three-step process ensures backward compatibility with legacy test names
and module-qualified names:

```cpp
// Legacy tests often ask for plain "_F_name_sig" entry points, while
// source compilation now emits module-qualified names.
```

### 6.2 `buildRuntimeSymbols`

`HVMJIT.cpp` lines 1130+ produces a `std::vector<RuntimeSymbolContract>` mapping
mangled names to `jit_*` wrapper function pointers. This map is registered in the
`hoo` JITDylib by `registerRuntimeSymbolsInJITDylib()` (line 1921).

**Two naming families coexist:**

| Family | Prefix Pattern | Example |
|--------|----------------|---------|
| Core runtime intrinsics | `_F_hoo_<name>_<ret>_<params>` | `_F_hoo_alloc_p_i8_i8` |
| Module namespace (hoo) | `_F_M_hoo_E_<name>_v_p*` | `_F_M_hoo_E_thread_spawn_v_p_p` |

The core family uses real type signatures (`i8` for int64, `p` for pointer), while
the module namespace family uses the hardcoded `v`/`p` convention described in
[§3.4](#34-special-conventions-for-runtime-functions).

---

## 7. Practical Guide

### 7.1 Adding a New Runtime Module Function

1. **Identify the class name and method** as it will appear in Hoo source, e.g.
   `Thread.spawn(func, arg)`. The codegen maps each module class to a prefix via
   `classToPrefix()` (defined in `HVMCodeGenerator.cpp`).
2. **Register the class name** in `HVMCodeGenerator.cpp:classToPrefix()` if it's a
   new module class. This maps `Thread` → `thread_`, `Fs` → `fs_`, etc.
3. **Add the prefix** to the redirect block in `HVMCodeGenerator.cpp:1048-1058`
   (one-time per module prefix)
4. **Compute the mangled name** by the rule `_F_M_hoo_E_<name>_v_p*` where `_p`
   repeats for each parameter
5. **Register the JIT wrapper** in `buildRuntimeSymbols()` using the mangled name
6. **Test** by calling the function from Hoo source through `jit.run()`

### 7.2 Debugging Mismatches

If `jit.run()` returns -1 or the JIT reports a missing symbol:

1. **Check `jit.getLastError()`** — it often contains the exact mangled name
   the codegen produced
2. **Verify the codegen's redirect prefix** — if the function name starts with
   `thread_`, the codegen must have `rfind("thread_", 0) == 0` in the redirect
   block
3. **Verify the symbol table entry** — the mangled name in `buildRuntimeSymbols()`
   must match exactly what the codegen produces (same case, same underscores)
4. **Confirm parameter count** — each argument in the Hoo call adds one `_p`
   to the mangled name

### 7.3 Type Codes Quick Reference

| Code | Type |
|------|------|
| `i1` | int8 |
| `u1` | byte |
| `i8` | int64 / int |
| `f` | float |
| `d` | double / f64 |
| `b` | bool |
| `c` | char |
| `s` | string |
| `y` | any |
| `v` | void |
| `p` | ptr |
| `a` | array |
| `o` | object / unknown (fallback) |
| `O` | nullable prefix |
| `A` | array prefix |
| `M` | map prefix |
| `Q...Z` | hex-encoded named type |
