# Serializable Class Modifier — Developer Guide

**Last Updated**: 2026-08-06

---

## Table of Contents

1. [Overview](#1-overview)
2. [Declaring a Serializable Class](#2-declaring-a-serializable-class)
3. [Generated API](#3-generated-api)
4. [Validation Rules](#4-validation-rules)
5. [Allowed Field Type Reference](#5-allowed-field-type-reference)
6. [Type Promotion in Serialization](#6-type-promotion-in-serialization)
7. [JSON Format](#7-json-format)
8. [Code Generation Architecture](#8-code-generation-architecture)
9. [Symbol Mangling](#9-symbol-mangling)
10. [Modifier Compatibility](#10-modifier-compatibility)
11. [Complete Examples](#11-complete-examples)
12. [Limitations & Future Work](#12-limitations--future-work)
13. [Error Messages Reference](#13-error-messages-reference)
14. [Implementation Walkthrough](#14-implementation-walkthrough)

---

## 1. Overview

The `serializable` class modifier is a compile-time feature that automatically generates JSON serialization and deserialization methods for Hoo classes. By adding the `serializable` modifier to a class declaration, the compiler:

1. Validates that the class satisfies all serialization constraints (field types, constructor, no cycles).
2. Generates a `serialize()` instance method that produces a JSON string from the class's public fields.
3. Generates a `static deserialize(json: string)` method that reconstructs a class instance from a JSON string.

All validation happens at **compile time** — no runtime checks are added. Generated code lowers to existing HVM `CALL` instructions and runtime library helpers; no new HVM opcodes are required.

---

## 2. Declaring a Serializable Class

### Basic Syntax

```hoo
serializable class ClassName {
    public var field1: Type1;
    public var field2: Type2;
    ...

    constructor() {
        // Initialize fields to defaults
    }
}
```

The `serializable` keyword is placed before the `class` keyword, in the modifier position (alongside `final`, `immutable`, `singleton`, etc.).

### Minimal Example

```hoo
serializable class User {
    public var name: string;
    public var age: int64;

    constructor() {
        name = "";
        age = 0;
    }
}
```

### With Other Modifiers

```hoo
final serializable class ImmutableConfig {
    public var version: int64;
    public var debug: bool;

    constructor() {
        version = 1;
        debug = false;
    }
}
```

---

## 3. Generated API

For every `serializable` class, the compiler automatically generates two methods. These are not written to source but are compiled directly into the module's HVM bytecode and registered in the symbol table.

### `serialize()`

```
func :string serialize()
```

- **Kind**: Instance method
- **Parameters**: None (uses `this`)
- **Returns**: `string` — a JSON representation of the object's public fields
- **Symbol**: `_F_<ClassName>_R_serialize_p`

The method:
1. Creates a `HashMap<int64, any>` via `hoo_hashmap_new`.
2. Iterates public fields in deterministic base-first declaration order,
   storing each by numeric positional index.
3. Calls `json_serialize_hashmap` and returns the resulting string.

### `deserialize(json)`

```
static func :ClassName deserialize(json: string)
```

- **Kind**: Static method
- **Parameters**: `json: string` — a JSON string previously produced by `serialize()`
- **Returns**: `ClassName` — a new instance with fields populated from the JSON
- **Symbol**: `_F_<ClassName>_R_deserialize_static_p_s`

The method:
1. Parses the JSON string into a `HashMap<int64, any>` via `json_deserialize_hashmap`.
2. Allocates a new instance via `hoo_alloc`.
3. Calls the parameterless constructor.
4. Iterates public fields in declaration order, extracting each value by numeric index from the HashMap and assigning it to the field.
5. Returns the constructed instance.

### Usage in Hoo Code

```hoo
serializable class User {
    public var name: string;
    public var age: int64;

    constructor() {
        name = "";
        age = 0;
    }
}

func :void main() {
    var u = new User();
    u.name = "Alice";
    u.age = 30;

    // Auto-generated serialize
    var json: string = u.serialize();
    Console.println(json);   // {"0":"Alice","1":30}

    // Auto-generated deserialize
    var restored: User = User.deserialize(json);
    Console.println(restored.name);  // "Alice"
    Console.println(restored.age);   // 30
}
```

---

## 4. Validation Rules

The compiler enforces the following rules at compile time. Violations produce compilation errors.

### 4.1 At Least One Public Field

A `serializable` class must declare at least one `public var` field. Fields without the `public` modifier (private by default) are not counted.

```hoo
serializable class Empty {
    // ERROR: serializable class must have at least one public field
    private var x: int64;
    constructor() {}
}
```

### 4.2 Exactly One Parameterless Constructor

The class must have exactly one constructor, and that constructor must accept zero parameters.

```hoo
serializable class Good {
    public var x: int64;
    constructor() { x = 0; }     // OK
}

serializable class Bad {
    public var x: int64;
    constructor(x: int64) { this.x = x; }  // ERROR: constructor must have no parameters
}

serializable class AlsoBad {
    public var x: int64;
    constructor() { x = 0; }
    constructor(y: int64) { x = y; }       // ERROR: must have exactly one constructor
}
```

### 4.3 All Field Types Must Be in the Allowed Set

Every public field's type must be one of the types listed in §5. Disallowed types include `float`, `char`, `Array<T>`, `Map<K,V>`, raw pointers, and non-serializable class types.

```hoo
serializable class Profile {
    public var name: string;               // OK
    public var score: double;              // OK
    public var flag: bit;                  // OK
    public var raw: buffer;                // OK
    // public var data: float;             // ERROR: float not allowed
    // public var ch: char;                // ERROR: char not allowed
    constructor() {}
}
```

### 4.4 HashMap Value Type Restriction

When a public field is `HashMap<K, V>`, the value type `V` must be one of the restricted primitive types (int8, byte, int64, double, f64, f8, string, bool, bit, buffer) or a tensor type. Serializable class references are **not** allowed as HashMap values.

```hoo
serializable class Container {
    public var labels: HashMap<int64, string>;                // OK
    public var flags: HashMap<int64, bit>;                    // OK
    public var blobs: HashMap<int64, buffer>;                 // OK
    public var mats: HashMap<int64, tensor<double>[4,4]>;    // OK
    // public var nested: HashMap<int64, User>;               // ERROR: serializable class not allowed
    constructor() {}
}
```

### 4.5 No Cyclic References

Serializable classes cannot form cycles through their serializable-class-typed fields. The compiler performs a DFS-based cycle detection pass after all classes are registered.

```hoo
serializable class A {
    public var b: B;      // ERROR: cycle A → B → A
    constructor() {}
}

serializable class B {
    public var a: A;
    constructor() {}
}
```

Self-references are also cycles:

```hoo
serializable class Node {
    public var next: Node;  // ERROR: self-referential cycle
    constructor() {}
}
```

HashMap and AnyArray fields are **not** traversed for cycle detection (their value types cannot be serializable classes per §4.4, so they cannot introduce edges).

Safe acyclic DAG:

```hoo
serializable class Point {
    public var x: double;
    public var y: double;
    constructor() { x = 0; y = 0; }
}

serializable class Line {
    public var start: Point;    // OK: no cycle
    public var end: Point;
    constructor() {}
}
```

### 4.6 No `serializable service` Combination

A class cannot be both `serializable` and `service`. Service classes hold runtime dependencies, and serialization of service state is undefined.

```hoo
// ERROR: Service class cannot also be serializable
serializable service class BadService {
    public var x: int64;
    constructor() {}
}
```

---

## 5. Allowed Field Type Reference

### Allowed Primitive Types

| Hoo Type   | Serialized As | Notes |
|------------|---------------|-------|
| `int8`     | JSON number   | Promoted to int64 in HashMap |
| `byte`     | JSON number   | Promoted to int64 in HashMap |
| `int64`    | JSON number   | Native storage |
| `double`   | JSON number   | Serialized as f64 |
| `f64`      | JSON number   | Same as double |
| `f8`       | JSON number   | Promoted to f64 in HashMap |
| `string`   | JSON string   | Direct copy |
| `bool`     | JSON boolean  | `true` / `false` |
| `bit`      | JSON boolean  | Promoted to bool |
| `buffer`   | Tagged JSON object | Base64 payload; round-trips as `buffer` |

### Allowed Container Types

| Hoo Type                           | Serialized As | Value Restriction |
|------------------------------------|---------------|--------------------|
| `HashMap<K, V>`                    | JSON object   | `V` must be a restricted primitive, tensor, or string |
| `AnyArray`                         | JSON array    | Runtime values must be restricted primitives |
| `tensor<T>[d0, d1, ...]`          | Tagged JSON object | Preserves element type, dimensions, and raw element bits |

### Allowed Reference Types

| Hoo Type               | Serialized As | Condition |
|------------------------|---------------|-----------|
| Other serializable class | JSON object (nested) | Referenced class must also have `serializable` modifier |
| `string`               | JSON string   | Same as `string` primitive |
| `buffer`               | Tagged JSON object | Base64 payload |

### Rejected Types

| Hoo Type       | Reason |
|----------------|--------|
| `float`        | 32-bit precision loss across serialization boundary |
| `char`         | Not representable as a standalone JSON type |
| `Array<T>`     | No runtime JSON support |
| `Map<K,V>`     | Use `HashMap<K,V>` instead |
| Non-serializable class | Cannot serialize without the modifier |

---

## 6. Type Promotion in Serialization

To simplify the HashMap-based encoding, certain narrow types are **promoted** to their wider counterpart when stored in the HashMap. This is transparent to the user: values are stored in their original type in the class fields, and only widened for the serialized representation.

| Original Type | Promoted to (in HashMap) | Promotion Type ID | Rationale |
|---------------|--------------------------|-------------------|-----------|
| `int8`        | `int64`                  | 1                 | Fits in int64 without loss |
| `byte`        | `int64`                  | 1                 | Fits in int64 without loss |
| `f8`          | `double` (f64)           | 2                 | Fits in f64 without loss |
| `bit`         | `bool`                   | 3                 | Boolean domain |
| `buffer`      | `buffer`                 | 113               | Tagged base64 JSON object |

The promotion happens in `serializeFieldTypeId()` at `src/codegen/HVMCodeGenerator.cpp`. Deserialization reverses scalar promotions: narrow integers are truncated/sign-extended, `bit` is masked to one bit, and `f8` is converted through the FP8 helper. Buffers and tensors retain dedicated runtime type IDs and are reconstructed from tagged JSON objects.

---

## 7. JSON Format

### Design Decision: Numeric Keys

The current implementation uses **numeric field indices** (0, 1, 2, …) as JSON object keys instead of human-readable field names. This is because the runtime `json_serialize_hashmap` / `json_deserialize_hashmap` functions use `int64` HashMap keys, and `parseInt64Text` (used by the deserializer) converts JSON object keys by parsing them as integers.

```json
{
    "0": "Alice",
    "1": 30,
    "2": true
}
```

This format is fully functional for round-trip serialization but not human-readable. Human-readable field names are deferred pending a string-keyed HashMap serialization runtime.

### Nested Serializable Classes

Nested serializable classes produce a single JSON object whose fields follow the same numeric-key convention:

```json
{
    "0": "ModelConfig",
    "1": {
        "0": "2024-06-17",
        "1": 5
    }
}
```

### Collection Types

HashMap fields are serialized as nested JSON objects with numeric keys:

```json
{
    "0": {"0": "value1", "1": "value2"},
    "1": [1, 2, 3]
}
```

### Type Representation in JSON

| Hoo Type     | JSON Representation | Example |
|--------------|---------------------|---------|
| `int64`      | Number              | `42` |
| `double`     | Number              | `3.14` |
| `string`     | String              | `"hello"` |
| `bool`       | Boolean             | `true` |
| `bit`        | Boolean             | `false` |
| `buffer`     | Tagged object       | `{"__hoo_buffer__":true,"data":"..."}` |
| `HashMap`    | Object              | `{"0":"a","1":"b"}` |
| `AnyArray`   | Array               | `[1, "two", true]` |
| `tensor`     | Tagged object       | `{"__hoo_tensor__":true,"element_type":2,"dims":[2,2],"data":[...]}` |
| Serializable | Object              | `{"0":42,"1":"x"}` |

---

## 8. Code Generation Architecture

### 8.1 Serialize Method Generation

The compiler emits the `serialize()` method body directly as HVM instructions. The algorithm is:

```
1. ENTER (allocate stack frame)
2. keyType   = CONST(1)            // HOO_TYPE_INT64
3. valType   = CONST(0)            // HOO_TYPE_ANY
4. CALL hoo_hashmap_new(keyType, valType)  → r1 (map)
5. MOV mapReg, r1

6. FOR each public field at index i:
   a. fieldReg = LD_D(this, fieldOffset)
   b. typeId   = CONST(serializeFieldTypeId(fieldType))
   c. keyReg   = CONST(i)          // numeric field index
   d. MOV r1, mapReg
   e. MOV r2, keyReg
   f. MOV r3, typeId
   g. MOV r5, fieldReg
   h. CALL hoo_hashmap_set_any_i8  // r4 is tp, skipped

7. MOV r1, mapReg
8. CALL json_serialize_hashmap    → r1 (result string)
9. LEAVE
10. RET
```

**Register layout for `hashmap_set_any` JIT bridge:**
- `r1` = map pointer
- `r2` = key (int64)
- `r3` = type ID (int64)
- `r4` = thread pointer (not used by this call)
- `r5` = value data (uint64_t)

### 8.2 Deserialize Method Generation

```
1. ENTER (allocate stack frame)
2. CALL json_deserialize_hashmap(json)  → r1 (map)
3. MOV mapReg, r1
4. sizeReg   = CONST(totalSize)
5. typeReg   = CONST(100)            // Generic Object typeId
6. CALL hoo_alloc(sizeReg, typeReg)  → r1 (instance)
7. MOV instanceReg, r1
8. ST_D instanceReg, stackTemp       // save for later

9. CALL <ClassName>_CT(instanceReg)  // call constructor

10. FOR each public field at index i:
    a. MOV r1, mapReg
    b. keyReg = CONST(i)
    c. MOV r2, keyReg
    d. CALL hoo_hashmap_get_any_data_i8  → r1 (value.data)
    e. instReg = LD_D(stackTemp)
    f. ST_D r1, instReg[fieldOffset]

11. r1 = LD_D(stackTemp)             // reload instance
12. LEAVE
13. RET
```

### 8.3 Key Helper Functions

#### `serializeFieldTypeId` (`HVMCodeGenerator.cpp:3490`)

Maps a Hoo type to the integer type ID used by the runtime's `serializeAnyValue`:

| Hoo Type       | Type ID | Constant Name     |
|----------------|---------|-------------------|
| int64, int8, byte | 1    | `HOO_TYPE_INT64`  |
| double, f64, f8   | 2    | `HOO_TYPE_FLOAT64`|
| bool, bit         | 3    | `HOO_TYPE_BOOL`   |
| string, buffer    | 101  | `HOO_TYPE_STRING` |
| HashMap           | 117  | `HOO_TYPE_HASHMAP`|
| AnyArray          | 118  | `HOO_TYPE_ANYARRAY`|

#### `emitStringLiteral` (`HVMCodeGenerator.cpp:3430`)

Emits a string constant into the literal pool and returns the register holding a pointer to it. Used for runtime function name strings.

### 8.4 Registration in Symbol Table

Both generated methods are registered as global function symbols in the module's symbol table:

```cpp
Symbol sym;
sym.name = SymbolMangler::mangleFunctionName(mp);
sym.value = funcStart;       // byte offset in code section
sym.type = Symbol::STT_FUNC;
sym.binding = Symbol::STB_GLOBAL;
sym.section_index = 0;
module_->addSymbol(sym);
```

This makes them linkable by the JIT compiler through the standard `MangledFunctionParams` → mangled name resolution.

---

## 9. Symbol Mangling

### Mangled Names

Serializable-generated methods follow the standard Hoo symbol mangling scheme:

```
// serialize() — instance method, returns ptr (string)
_F_<ClassName>_<ModifierCode>_serialize_p

// deserialize() — static method, takes string, returns ptr
_F_<ClassName>_<ModifierCode>_deserialize_static_p_s
```

The modifier code for `SERIALIZABLE` is `"R"` (defined in `getModifierCodeMap()` in `SymbolMangler.cpp:78`).

### Examples

| Class       | Method       | Mangled Symbol                            |
|-------------|-------------|-------------------------------------------|
| `User`      | `serialize` | `_F_User_R_serialize_p`                   |
| `User`      | `deserialize` | `_F_User_R_deserialize_static_p_s`      |
| `ModelConfig` | `serialize` | `_F_ModelConfig_R_serialize_p`         |
| `DateTime`  | `serialize` | `_F_DateTime_R_serialize_p`               |

### Demangling

The demangler in `SymbolMangler.cpp` handles the `"R"` modifier code:

```cpp
auto isClassModifierCode = [](const std::string& comp) {
    return comp == "N" || comp == "I" ||
           comp == "S" || comp == "Z" || comp == "R";
};

auto pushClassModifier = [&](const std::string& comp) {
    ...
    else if (comp == "R") result.classModifiers.push_back("SERIALIZABLE");
};
```

The demangler also has a fix (lines 426–435) to handle function names that appear after class modifier codes — this was necessary because the original demangler only looked for function names at a fixed position in the components array, but modifiers shift subsequent components.

---

## 10. Modifier Compatibility

| Modifier        | Compatible with `serializable`? | Notes |
|-----------------|--------------------------------|-------|
| `singleton`     | Yes                            | Serializing a singleton serializes its current instance state |
| `immutable`     | Yes                            | Serialization is a read-only operation |
| `final`         | Yes                            | No semantic conflict |
| `service`       | **No**                         | Compile-time error: service dependency state is not serializable |

Inheritance between serializable classes is supported. Public base fields are emitted before derived fields and use the same positional schema on serialization and deserialization.

---

## 11. Complete Examples

### 11.1 Basic Primitive Types

```hoo
serializable class Person {
    public var name: string;
    public var age: int64;
    public var height: double;
    public var active: bool;

    constructor() {
        name = "";
        age = 0;
        height = 0.0;
        active = false;
    }
}

func :void main() {
    var p = new Person();
    p.name = "Bob";
    p.age = 25;
    p.height = 1.85;
    p.active = true;

    var json = p.serialize();
    // {"0":"Bob","1":25,"2":1.85,"3":true}

    var p2 = Person.deserialize(json);
    Console.println(p2.name);   // "Bob"
    Console.println(p2.age);    // 25
}
```

### 11.2 HashMap Fields

```hoo
serializable class Inventory {
    public var items: HashMap<int64, string>;
    public var counts: HashMap<int64, int64>;

    constructor() {
        items = new HashMap<int64, string>();
        counts = new HashMap<int64, int64>();
    }
}

func :void main() {
    var inv = new Inventory();
    inv.items[0] = "sword";
    inv.items[1] = "shield";
    inv.counts[0] = 3;
    inv.counts[1] = 1;

    var json = inv.serialize();
    // {"0":{"0":"sword","1":"shield"},"1":{"0":3,"1":1}}

    var restored = Inventory.deserialize(json);
    Console.println(restored.items[0]);  // "sword"
}
```

### 11.3 Nested Serializable Classes (Acyclic)

```hoo
serializable class Point {
    public var x: double;
    public var y: double;

    constructor() {
        x = 0.0;
        y = 0.0;
    }
}

serializable class Rectangle {
    public var topLeft: Point;
    public var bottomRight: Point;
    public var label: string;

    constructor() {
        label = "";
    }
}

func :void main() {
    var rect = new Rectangle();
    rect.topLeft.x = 0.0;
    rect.topLeft.y = 10.0;
    rect.bottomRight.x = 20.0;
    rect.bottomRight.y = 0.0;
    rect.label = "box";

    var json = rect.serialize();
    var restored = Rectangle.deserialize(json);
    Console.println(restored.label);          // "box"
    Console.println(restored.topLeft.x);      // 0.0
}
```

### 11.4 All Allowed Types

```hoo
serializable class AllTypes {
    // Primitives
    public var i8: int8;
    public var b: byte;
    public var i64: int64;
    public var d: double;
    public var f: f8;
    public var s: string;
    public var flag: bool;
    public var bf: bit;
    public var buf: buffer;

    // Containers
    public var hm: HashMap<int64, string>;
    public var arr: AnyArray;
    public var ten: tensor<double>[2, 3];

    constructor() {
        i8 = 0;
        b = 0;
        i64 = 0;
        d = 0.0;
        f = 0.0f8;
        s = "";
        flag = false;
        bf = 0b;
        buf = new Buffer();
        hm = new HashMap<int64, string>();
        arr = new AnyArray();
    }
}
```

### 11.5 DateTime (Serializable-Ready Runtime Class)

DateTime was reworked from a singleton into an instantiable ARC-managed class (type ID 119). It is serializable-ready as a runtime class, but the generated serializable field validator currently accepts user-defined classes carrying the `serializable` modifier; use an explicit wrapper with an `int64` timestamp when persistence is required:

```hoo
serializable class DateTimeValue {
    public var timestamp: int64;     // milliseconds since Unix epoch

    constructor() {
        timestamp = 0;
    }
}

func :void main() {
    var dt = new DateTimeValue();
    var json = dt.serialize();
    // {"0":1718640000000}

    var restored = DateTimeValue.deserialize(json);
    Console.println(restored.timestamp == dt.timestamp);  // true
}
```

---

## 12. Limitations & Future Work

### Current Limitations

| Limitation | Description | Impact |
|------------|-------------|--------|
| **Numeric JSON keys** | Field indices (0, 1, 2) used instead of field names | JSON is not human-readable; field reordering breaks backward compat |
| **No HashMap<serializable>** | Serializable classes cannot be HashMap values | Limits use cases like `HashMap<int64, User>` |
| **No user-override** | Generated methods cannot be overridden by user code | User cannot customize serialization format |
| **No custom serialization** | No way to exclude fields or add custom serialize logic | Always includes all public fields |

### Future Work Items

1. **Human-readable JSON keys** — Implement string-keyed HashMap serialization in runtime (`json_serialize_object` / `json_deserialize_object`) and use field names as keys.
2. **HashMap<serializable> values** — Extend the value-type restriction to allow serializable class references in HashMap values.
3. **User-overridable methods** — If user defines `serialize()` or `deserialize()`, skip generation and use user version.
4. **Omit-by-default or exclude annotation** — Allow selective field inclusion.
5. **Versioning** — Store a schema version or class name in the JSON output for format evolution.

---

## 13. Error Messages Reference

| Error Message | Cause |
|---------------|-------|
| `Serializable class 'X' must have at least one public field` | Class has zero `public var` fields |
| `Serializable class 'X' must have exactly one constructor` | No constructor or more than one constructor |
| `Serializable class 'X' constructor must have no parameters` | Constructor takes arguments |
| `Serializable class 'X' field 'Y': float not allowed` | Public field is `float` |
| `Serializable class 'X' field 'Y': char not allowed` | Public field is `char` |
| `Serializable class 'X' field 'Y': Array type not allowed for serialization` | Public field is `Array<T>` |
| `Serializable class 'X' field 'Y': Map type not allowed for serialization (use HashMap)` | Public field is `Map<K,V>` |
| `Serializable class 'X' field 'Y': float not allowed as HashMap value type` | HashMap value type is `float` |
| `Serializable class 'X' field 'Y': char not allowed as HashMap value type` | HashMap value type is `char` |
| `Serializable class 'X' field 'Y': serializable class not allowed as HashMap value type` | HashMap value type is a class reference |
| `Serializable class 'X' field 'Y' references non-serializable class 'Z'` | Field type is a class without `serializable` modifier |
| `Serializable class 'X' field 'Y' has unsupported type for serialization` | Catch-all for unhandled type |
| `Serializable class 'X' forms a cycle: X -> ... -> X` | Cyclic reference detected via DFS |
| `Service class 'X' cannot also be serializable` | `serializable service` combination |

---

## 14. Implementation Walkthrough

### Source Files

| File | Role |
|------|------|
| `src/parsing/Hooc.g4` | Lexer token `SERIALIZABLE: 'serializable';` (line 30) + `classModifier` rule (line 168) |
| `src/ast/ClassDeclaration.h` | `ClassModifier::SERIALIZABLE` enum value (line 22) |
| `src/ast/ASTImpl.cpp` | `classModifierToString(SERIALIZABLE)` → `"serializable"` (line 353) |
| `src/ast/SimpleASTBuilder.cpp` | `getClassModifier` case for `SERIALIZABLE` token (line 1081) |
| `src/core/SymbolMangler.cpp` | Modifier code map `{"SERIALIZABLE", "R"}` (line 84), `isClassModifierCode` + `pushClassModifier` (lines 354–364), demangler fix (lines 426–435) |
| `src/codegen/HVMCodeGenerator.h` | `isSerializable` flag (line 82), `serializableAdjacency_` (line 94), method declarations |
| `src/codegen/HVMCodeGenerator.cpp` | Layout flag (line 411), service check (lines 437–440), `validateSerializableClass` (lines 733–798), `isValidSerializableType` (lines 800–934), `detectSerializableCycles` (lines 936–979), `emitStringLiteral` (line 3430), `serializeFieldTypeId` (line 3490), `emitSerializeMethod` (lines 3518–3602), `emitDeserializeMethod` (lines 3604–3700) |
| `src/runtime/lib/hoo_json.h` | `hoo_json_serialize_hashmap` (line 23), `hoo_json_deserialize_hashmap` (line 40) |

### Data Flow

```
Source (.hoo) 
    → Parser (Hooc.g4) 
    → AST (ClassDeclaration with SERIALIZABLE modifier)
    → SimpleASTBuilder (getClassModifier)
    → HVMCodeGenerator::generateCode()
        → ClassLayout.isSerializable = true
        → validateSerializableClass()
            → isValidSerializableType() per field
            → Build serializableAdjacency_ map
        → emitSerializeMethod()
        → emitDeserializeMethod()
    → detectSerializableCycles() (after all classes processed)
    → Module with symbols → JIT compilation
```

### Validation Flow

```
validateSerializableClass(classDecl, layout, name):
  1. Check: at least one public var field
  2. Check: exactly one constructor
  3. Check: constructor has zero parameters
  4. For each public field, call isValidSerializableType(type, className, fieldName):
     a. If HashMapType: check value type is restricted primitive
     b. If AnyArrayType: OK
     c. If TensorType: check element type is allowed primitive
     d. If BaseType primitive: check kind is in allowed set (reject float, char)
     e. If BaseType class ref: check referenced class exists and has isSerializable=true
     f. If ArrayType: reject
     g. If MapType: reject (suggest HashMap)
  5. Record adjacency for class-typed fields in serializableAdjacency_

detectSerializableCycles():
  1. DFS with WHITE/GRAY/BLACK coloring
  2. For each edge to a GRAY node, report cycle with full path
```

### Code Generation Flow

```
emitSerializeMethod(layout, classDecl):
  1. Emit ENTER instruction
  2. CALL hoo_hashmap_new(1, 0) → map
  3. For each public field at index i:
     a. LD_D field from this at fieldOffset
     b. CALL hoo_hashmap_set_any_i8(map, i, typeId, fieldData)
  4. CALL json_serialize_hashmap(map) → result string
  5. Emit LEAVE, RET
  6. Register symbol _F_<Class>_R_serialize_p

emitDeserializeMethod(layout, classDecl):
  1. Emit ENTER instruction
  2. CALL json_deserialize_hashmap(json) → map
  3. CALL hoo_alloc(totalSize, 100) → instance
  4. CALL <Class>_CT(instance)  // constructor
  5. For each public field at index i (base fields first):
     a. CALL hoo_hashmap_get_any_data_i8(map, i) → value.data
     b. Reverse scalar promotion or recursively deserialize nested values
     c. ST_D converted value → instance[fieldOffset]
  6. MOV r1, instance
  7. Emit LEAVE, RET
  8. Register symbol _F_<Class>_R_deserialize_static_p_s
```

### Cycle Detection Invocation

The cycle detection runs **after** all classes in the module have been processed, not inline with per-class validation. This is because the adjacency map (`serializableAdjacency_`) must be fully populated before DFS can run:

```cpp
// In HVMCodeGenerator::generateCode():
// ... for each class declaration:
if (layout.isSerializable) {
    validateSerializableClass(...);
}
// ... after all classes:
if (!serializableAdjacency_.empty()) {
    detectSerializableCycles();
}
```

---

## Appendix: Test Coverage

### Parsing Tests (`tests/parsing/ClassDeclarationParsingTest.cpp`)

| Test Name | What It Verifies |
|-----------|------------------|
| `ClassWithSerializableModifier` | Parsing a class with `serializable` modifier produces correct AST, `hasModifier(SERIALIZABLE)` is true |
| `ClassWithSerializableAndFinal` | Combined `final serializable` modifiers both parse correctly |
| `SerializableModifierToString` | `classModifierToString(SERIALIZABLE)` returns `"serializable"` |

### Symbol Mangling Tests (`tests/core/SymbolManglerTest.cpp`)

| Test Name | What It Verifies |
|-----------|------------------|
| `SerializableModifierMangling` | Mangle `UserConfig` with `SERIALIZABLE` → `_F_UserConfig_R_serialize_p`, demangle round-trip |

### Test Count

Total test count after Phase 11.3: **2062 tests, 0 failures, 2 disabled**.
