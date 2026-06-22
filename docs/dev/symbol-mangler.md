# How Mangling and Demangling Works with SymbolMangler

**File:** `src/core/SymbolMangler.h` / `.cpp` (~744 lines)

`SymbolMangler` converts Hoo identifiers into unique linker-visible symbol names. The mangler and demangler must always be symmetric — any change to one requires a corresponding change to the other.

## Public API

```cpp
class SymbolMangler {
public:
    static std::string mangleFunctionName(const MangledFunctionParams& params);
    static std::string mangleModuleSymbol(
        const std::vector<std::string>& modulePath,
        const std::string& symbolName,
        const std::string& kindTag = "");
    static DemangledSymbol demangleSymbol(const std::string& mangledName);
    static std::string demangle(const std::string& mangledName);
    static std::string demangleType(const std::string& mangledType);
    static std::string mangleType(const std::string& typeName);
    static std::string typeKindToMangledString(const std::string& typeName);
};
```

## Data structures

```cpp
struct MangledFunctionParams {
    std::vector<std::string> modulePath;
    std::string className;
    std::string baseClassName;
    std::vector<std::string> classModifiers;    // e.g. {"SINGLETON", "IMMUTABLE"}
    std::string functionName;
    std::vector<std::string> functionModifiers; // e.g. {"PUBLIC", "PRIVATE"}
    std::string returnType;
    std::vector<std::string> parameterTypes;
    bool isConstructor = false;
    bool isDestructor = false;
    bool isStatic = false;
    bool isVirtual = false;
    bool isOverload = false;
};

struct DemangledSymbol {
    std::string originalName;
    std::vector<std::string> modulePath;
    std::string className;
    std::string functionName;
    std::string baseClassName;
    std::vector<std::string> classModifiers;
    std::vector<std::string> functionModifiers;
    std::string returnType;
    std::vector<std::string> parameterTypes;
    bool isConstructor = false;
    bool isDestructor = false;
    bool isStatic = false;
    bool isVirtual = false;
    bool isOverload = false;
};
```

## Prefix conventions

| Prefix | Meaning |
|---|---|
| `_F_` | Function symbol |
| `_H_` | Module-level symbol (type descriptor, global, object) |

## Function mangling format (`_F_`)

The general structure for `_F_` symbols is:

```
_F_[M_module_E_][ClassName_][BaseClassName_][ClassMods_] \
   [CT|DT|FuncName_][static_][virtual_][FuncMods_][ReturnType_][ParamTypes_...]
```

Components separated by `_`. All pieces after the function name are optional depending on context.

### Class modifier encoding

| Suffix | Mapping |
|---|---|
| `N` | SINGLETON |
| `I` | IMMUTABLE |
| `S` | SERVICE |
| `Z` | FINAL |
| `R` | SERIALIZABLE |

### Function modifier encoding

| Prefix | Mapping |
|---|---|
| `Pb` | PUBLIC |
| `Pv` | PRIVATE |

### Special tokens

| Token | Meaning |
|---|---|
| `CT` | Constructor |
| `DT` | Destructor |
| `static` | Static method |
| `virtual` | Virtual method |

### Examples

```
_F_foo_i8_i8                — fn foo(int64, int64) return int64
_F_Person_greet_s           — Person.greet() return string
_F_Person_CT_s              — Person constructor(string)
_F_Person_DT                — Person destructor
_F_Student_Person_study_v   — Student extends Person, study() return void
_F_Config_N_getInstance_s   — SINGLETON Config.getInstance() return string
_F_AllModClass_N_I_S_Z_R_execute_v  — All 5 class modifiers
_F_Person_save_Pb_v         — Person.save() PUBLIC, return void
_H_hoo_io_print             — Module symbol hoo.io.print
```

## Type codes

| Code | Type | Code | Type |
|---|---|---|---|
| `i8` | `int64` | `i1` | `int8` |
| `d`  | `double` / `f64` | `e`  | `f8` |
| `f`  | `float` | `x`  | `bit` |
| `b`  | `bool` | `c`  | `char` |
| `s`  | `string` | `v`  | `void` |
| `p`  | `ptr` | `t`  | `tensor` |
| `u1` | `byte` / `UInt8` | `y`  | `any` |
| `o`  | `unknown` (fallback) | `a`  | `array` (legacy) |

Note: The actual implementation uses `typeNameToCode()` lookup. Each type name maps to its code string.

## Structured type mangling

`mangleType()` and `demangleType()` handle compound types:

| Pattern | Meaning | Example |
|---|---|---|
| `O<inner>` | `Optional<T>` | `Oi8` = `int64?` |
| `A<inner>` | `Array<T>` | `Ab` = `bool[]` |
| `M<key><value>` | `Map<K,V>` | `Msi8` = `map[string,int64]` |
| `Q<hex>Z` | Qualified identifier | `Q666f6fZ` = `"foo"` |
| Suffixes | `[]` and `?` applied in order | `Ab?` = `bool[]?` |

The mangler handles nesting: `map[string,map[foo.bar.User[],int64?]]` round-trips correctly.

### Whitespace handling

`mangleType` trims all spaces before parsing, so `map [ string , int64 ]` mangles identically to `map[string,int64]`.

### Fallback

If structured parsing fails, the type is encoded as `Q<hex-of-original>Z`, ensuring the round trip still works:

```cpp
std::string encoded = parseType();
if (encoded.empty() || pos != normalized.size()) {
    return "Q" + toHex(normalized) + "Z";  // fallback
}
```

## Name encoding

Special characters in identifiers are hex-encoded between `_` delimiters:

```
encodeComponent("name-with.dot and space")
// → "Ename_2d_with_2e_dot_20_and_20_spaceE"

decodeComponent("Ename_2d_with_2e_dot_20_and_20_spaceE")
// → "name-with.dot and space"
```

Alphanumeric and `_` characters pass through unencoded. Everything else becomes `_<hex>_`.

## Module symbol format (`_H_`)

```
_H_<module_path_parts>_<symbol_name>[_kindTag]
```

Kind tags provide metadata:
- `_fn` — function kind
- `_ob` — object kind
- `_ty` — type descriptor
- `_tls` — thread-local
- `_nt` — native
- `_uk` — unknown

Example: `_H_hoo_collections_List_first` — module path `hoo.collections.List`, symbol `first`.

## Demangling

`demangleSymbol` reverses the process:

1. Splits the mangled name into components at `_` boundaries (handling `E...E` encoded blocks).
2. Identifies module path markers (`M...E`).
3. Extracts class name, base class name, and function name from position.
4. Recognizes modifier codes and special tokens (CT, DT, static, virtual).
5. Parses type codes (return type and parameters).
6. Reconstructs the full `DemangledSymbol` struct.

`demangle()` takes the struct and produces a human-readable string:

```
"func PUBLIC static int64 Person_save(string, int64) extends Base // modifiers: SINGLETON IMMUTABLE "
```

## Key implementation details

- The type code map and modifier code maps are stored as static `std::vector<std::pair<std::string, std::string>>` returned from accessor functions (`getTypeCodeMap()`, `getModifierCodeMap()`, `getFunctionModifierCodeMap()`).
- `encodeString`/`decodeString` handle the hex encoding of individual characters.
- `encodeComponent`/`decodeComponent` wrap the encoding with `E...E` delimiters.
- `demangleType` uses a recursive descent parser (`parseType` lambda) that mirrors the `mangleType` structure.
- The `isSpecialToken` check in `demangleSymbol` distinguishes names from modifier codes and type codes during parsing.
- Kind tags are stripped early in `demangleSymbol` before component splitting.

## Round-trip guarantees

For any valid type or function signature:
```
mangleType(type) → demangleType(mangled) == type  ✓
mangleFunctionName(params) → demangleSymbol(mangled) reconstructs params  ✓
```
