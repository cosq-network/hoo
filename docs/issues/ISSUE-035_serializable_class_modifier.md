# ISSUE-035: `serializable` Class Modifier — Declarative Serialization

## 1. Overview
This document proposes a new `serializable` class modifier that enables automatic serialization and deserialization for Hoo classes. A `serializable` class declares its structure declaratively through public fields, and the compiler generates the serialization/deserialization logic automatically. This eliminates boilerplate while ensuring type safety, and provides a natural integration point with the existing JSON runtime library.

Design constraint: all validation must happen at compile time. Generated serialization must lower to existing HVM CALL and runtime helpers — no new HVM opcodes.

---

## 2. Grammar & Syntax

### 2.1 Modifier Addition
Add `SERIALIZABLE` to the `classModifier` grammar rule:

```antlr
classModifier: SINGLETON | IMMUTABLE | SERVICE | FINAL | SERIALIZABLE;
```

### 2.2 Usage
```hoo
serializable class UserConfig {
    public var name: string;
    public var version: int64;
    public var enabled: bool;
    public var tags: HashMap<int64, string>;

    constructor() {
        name = "default";
        version = 1;
        enabled = true;
    }
}
```

### 2.3 Generated API
For each `serializable` class, the compiler automatically generates:

```hoo
// Auto-generated — conceptually equivalent
func :string serialize() { /* emits JSON via runtime helpers */ }
static func :ClassType deserialize(json: string) { /* parses JSON via runtime helpers */ }
```

These are not written to source but are resolved by the compiler as intrinsic methods. They appear in the mangled symbol table so the JIT can link them.

---

## 3. Validation Rules

### 3.1 At Least One Public Field
A `serializable` class must declare at least one public class field variable. A class with only `private` or unmarked fields is rejected:

```hoo
serializable class EmptyConfig {
    // Error: serializable class must have at least one public field
    private var internal: int64;
    constructor() {}
}
```

### 3.2 Allowed Field Types
Every public field must be one of:
- **Primitives**: `int8`, `byte`, `int64`, `double`, `f64`, `f8`, `string`, `bool`, `bit`, `buffer`
- **Tensor**: `tensor<T>[d0, ..]` where `T` is any allowed primitive element type
- **HashMap**: `HashMap<K, V>` where `V` is a **value-restricted type** (see §3.3)
- **AnyArray**: `AnyArray` whose runtime values conform to the restricted set
- **Serializable class**: Any other class that itself has the `serializable` modifier

Rejected types: `float` (32-bit loses precision across serialization boundaries), `char`, `Map<K,V>`, `Array<T>`, raw pointers, non-serializable class types.

```hoo
serializable class Profile {
    public var name: string;                    // OK: primitive
    public var age: int64;                      // OK: primitive
    public var score: double;                   // OK: primitive
    public var meta: HashMap<int64, string>;    // OK: value type is string
    public var raw: buffer;                     // OK: primitive
    public var flag: bit;                       // OK: primitive
    // public var data: float;                  // ERROR: float not allowed
    // public var map: Map<string, int64>;      // ERROR: Map not allowed (use HashMap)
    constructor() {}
}
```

### 3.3 HashMap and AnyArray Value-Type Restriction
When a public field is `HashMap<K, V>` or `AnyArray`, the value type `V` (for HashMap) or the runtime element type (for AnyArray) must be one of:
- `int8`, `byte`, `int64`, `double`, `f64`, `f8`, `string`, `bool`, `bit`, `buffer`
- `tensor<T>[d0, ..]` where `T` is any allowed primitive element type

Nested serializable classes are **not** allowed as HashMap values or AnyArray elements:

```hoo
serializable class Inner {
    public var x: int64;
    constructor() { x = 0; }
}

serializable class Outer {
    public var items: HashMap<int64, string>;            // OK: string is restricted
    public var list: AnyArray;                           // OK: runtime values restricted
    public var flags: HashMap<int64, bit>;               // OK: bit is restricted
    public var blobs: HashMap<int64, buffer>;            // OK: buffer is restricted
    public var mats: HashMap<int64, tensor<double>[4,4]>; // OK: tensor is restricted
    // public var inners: HashMap<int64, Inner>;          // ERROR: Inner is not restricted
    constructor() {}
}
```

### 3.4 Single Parameterless Constructor
A `serializable` class must have exactly one constructor, and that constructor must take zero parameters:

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
    constructor() { x = 0; }     // ERROR: only one constructor allowed
    constructor(name: string) { x = 0; }   // (even if parameterless were allowed)
}
```

The parameterless requirement ensures that `deserialize()` can construct an instance without external input and then populate fields by name.

### 3.5 No Cyclic References
The compiler must detect and reject cyclic references between serializable classes at any depth. A cycle exists when a serializable class `A` has a field referencing serializable class `B`, and `B` (transitively) has a field referencing `A`.

```hoo
serializable class A {
    public var b: B;      // ERROR: creates cycle A → B → A
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

The cycle detection algorithm:
1. Build a directed graph where each serializable class is a node and each serializable-typed public field is an edge to the referenced class.
2. Run DFS from each unvisited node.
3. If a back-edge is found (reaching a node currently in the DFS stack), report a cycle with the full path.

HashMap and AnyArray fields are not traversed for cycle detection (their value types cannot be serializable classes per §3.3, so they cannot introduce cycles).

```hoo
// Safe: HashMap<int64, int64> does not create class references
serializable class Config {
    public var pairs: HashMap<int64, int64>;
    public var name: string;
    constructor() {}
}

// Safe: No cycles in acyclic DAG
serializable class Point {
    public var x: double;
    public var y: double;
    constructor() {}
}

serializable class Line {
    public var start: Point;
    public var end: Point;
    constructor() {}
}
```

---

## 4. Futuristic Comprehensive Example

The following example demonstrates a realistic `serializable` class hierarchy using all allowed types:

```hoo
// ---- Primitives ----
serializable class Metadata {
    public var name: string;
    public var version: int64;
    public var debug: bool;
    public var threshold: double;
    public var precision: f8;
    public var active: bit;              // bit field (OK)
    public var raw: buffer;              // binary blob (OK)

    constructor() {
        name = "default";
        version = 1;
        debug = false;
        threshold = 0.001;
        precision = 0.5f8;
        active = 1b;
        raw = new Buffer();
    }
}

// ---- Nested serializable class (acyclic) ----
serializable class TensorSpec {
    public var rows: int64;
    public var cols: int64;
    public var data: tensor<double>[4, 4];  // tensor field (OK)

    constructor() {
        rows = 4;
        cols = 4;
    }
}

// ---- Top-level serializable config with all collection types ----
serializable class ModelConfig {
    // Primitives
    public var name: string;
    public var epoch: int64;
    public var learningRate: double;
    public var enableLogging: bool;
    public var checkpoint: buffer;            // buffer field (OK)

    // Nested serializable class (acyclic)
    public var metadata: Metadata;
    public var spec: TensorSpec;

    // HashMap with every allowed value type
    public var labels: HashMap<int64, string>;                  // string value
    public var flags: HashMap<int64, bit>;                      // bit value
    public var blobs: HashMap<int64, buffer>;                   // buffer value
    public var weights: HashMap<int64, tensor<double>[3, 3]>;   // tensor value

    // AnyArray
    public var extras: AnyArray;

    constructor() {
        name = "model";
        epoch = 100;
        learningRate = 0.01;
        enableLogging = true;
        checkpoint = new Buffer();
    }
}

// ---- Usage ----
func :void main() {
    var cfg = new ModelConfig();
    cfg.name = "transformer-v2";
    cfg.flags[0] = 1b;
    cfg.flags[1] = 0b;
    cfg.blobs[0] = Buffer.fromBytes([0xDE, 0xAD, 0xBE, 0xEF]);

    // Auto-generated serialize()
    var json: string = cfg.serialize();
    Console.println(json);

    // Auto-generated deserialize()
    var restored: ModelConfig = ModelConfig.deserialize(json);

    // Verify round-trip
    Console.println(restored.name);     // "transformer-v2"
    Console.println(restored.flags[0]); // 1b
}
```

The generated JSON for the above example would resemble:

```json
{
    "name": "transformer-v2",
    "epoch": 100,
    "learningRate": 0.01,
    "enableLogging": true,
    "checkpoint": "3q2+7w==",
    "metadata": {
        "name": "default",
        "version": 1,
        "debug": false,
        "threshold": 0.001,
        "precision": 0.5,
        "active": true,
        "raw": ""
    },
    "spec": {
        "rows": 4,
        "cols": 4,
        "data": [[0,0,0,0],[0,0,0,0],[0,0,0,0],[0,0,0,0]]
    },
    "labels": {},
    "flags": {"0": true, "1": false},
    "blobs": {"0": "3q2+7w=="},
    "weights": {},
    "extras": []
}
```

### 4.1 Making DateTime Serializable

The `DateTime` class was originally a **singleton** with only static methods. It has been reworked into an **instantiable ARC-managed runtime class** (type ID 119). The current serializable validator intentionally requires user-defined referenced classes to carry the `serializable` modifier, so applications should use a small serializable timestamp wrapper when persistence is needed:

| Property | Value |
|----------|-------|
| Type ID | 119 |
| Class layout | `public var timestamp: int64` (offset 0, 8 bytes) |
| Constructor | Parameterless (`timestamp = 0`) |
| Memory | ARC heap object (16-byte header + 8-byte payload) |

The class exposes factory/utility methods as static or free functions, while the core API uses instance methods on DateTime objects:

```hoo
// Explicit serializable wrapper for a DateTime timestamp
serializable class DateTimeValue {
    public var timestamp: int64;     // milliseconds since Unix epoch

    constructor() {
        timestamp = 0;               // default: Unix epoch
    }
}

// Usage
var dt = new DateTimeValue();        // persist timestamp through the wrapper
var ts = dt.timestamp;               // int64
```

The compiler auto-generates for `DateTimeValue` using the numeric positional
schema (`{"0":1705104000000}`), with the timestamp restored on deserialize.

This integrates directly into serializable hierarchies:

```hoo
serializable class ModelConfig {
    public var name: string;
    public var createdAt: DateTimeValue;      // explicit serializable wrapper
    public var tags: HashMap<int64, string>;
    public var metadata: Metadata;

    constructor() {
        name = "model";
        createdAt = new DateTimeValue();
    }
}

// Serialize
var cfg = new ModelConfig();
var json = cfg.serialize();
// {"0":"model","1":{"0":0},"2":{},"3":{...}}

// Deserialize
var restored = ModelConfig.deserialize(json);
```

#### Comparison: Singleton → Instantiable

| Aspect | DateTime (before rework) | DateTime (current) |
|--------|------------------------|--------------------|
| Modifier | `singleton` | Instantiable; ready for `serializable` |
| Fields | None (static methods only) | `public var timestamp: int64` |
| Constructor | N/A | Parameterless constructor |
| `DateTime.now()` | Returns `int64` | Returns `DateTime` instance |
| Formatting | `DateTime.iso8601(ts)` static | `dt.iso8601()` instance method |
| Arithmetic | `DateTime.add_days(ts, n)` static | `dt.addDays(n)` instance method |
| Differences | `DateTime.diff_days(a, b)` static | `a.diffDays(b)` instance method |
| Comparison | `DateTime.compare(a, b)` static | `a.compare(b)` instance method |
| Serialization | N/A (no instance state) | Use an explicit serializable wrapper |

#### Round-trip Flow
```
DateTime.now()                → dt (timestamp = 1718640000000)
  ↓ serialize()
{"timestamp": 1718640000000}
  ↓ deserialize()
DateTime(timestamp=1718640000000)
  ↓ dt.iso8601()
"2024-06-17T12:00:00.000Z"
```

The `timestamp` is an `int64` and is fully supported inside the explicit
wrapper. Direct DateTime-field serialization remains deferred until built-in
runtime classes participate in user-defined serializable metadata.

---

## 5. Code Generation & Lowering

### 5.1 Serialize Lowering
The compiler auto-generates a `serialize` method body that:
1. Allocates a `HashMap<int64, any>` as the intermediate key-value store.
2. Emits deterministic numeric positional keys for public fields, base fields
   first, and lowers nested classes, buffers, and tensors as required.
3. Calls `json_serialize_hashmap` at the end.

```
// Given: serializable class User { public var name: string; public var age: int64; }
// Auto-generated serialize() lowers to:

  CALL _F_hoo_hashmap_new_p_i8_i8     ; HashMap<int64, any>
  MOV r_map, r1
  ; field 0 -> serialize(name)
  MOV r_key, 0                         ; numeric positional key
  MOV r_val_name, r_field_name         ; field value (string)
  CALL _F_hoo_hashmap_set_fixed_i8_p_i8_i8
  ; field 1 -> serialize(age)
  MOV r_key, 1                         ; numeric positional key
  MOV r_val_age, r_field_age
  CALL _F_hoo_int64_to_string          ; int64 -> string
  CALL _F_hoo_hashmap_set_fixed_i8_p_i8_i8
  ; Final JSON
  CALL _F_hoo_json_serialize_hashmap   ; produce JSON string
  RET
```

For nested serializable classes, the serializer recursively calls the inner
class's generated `serialize()` method, parses the result into a HashMap, and
stores that object as the enclosing any value.

### 5.2 Deserialize Lowering
The compiler auto-generates a `deserialize` static method:
1. Calls `json_deserialize_hashmap` to parse the JSON into a `HashMap<int64, any>`.
2. Constructs a new instance via the parameterless constructor.
3. For each public field, extracts the value by positional key, reverses
   scalar promotion or recursively invokes a nested deserializer, and assigns it.

```hoo
// Conceptually generated deserialize(User json) -> User
  var map = json_deserialize_hashmap(json);
  var obj = new User();
  obj.name = map[0];                 // any string value
  obj.age = map[1];                  // any int64 value
  return obj;
```

### 5.3 Primitive Type Serialization Mapping
| Hoo Type | Serialized JSON Type | Round-trip Conversion |
|-----------|---------------------|----------------------|
| `int8` | JSON number | `hoo_int8_to_string` / `parseInt8` |
| `byte` | JSON number | `hoo_byte_to_string` / `parseByte` |
| `int64` | JSON number | `hoo_int64_to_string` / `parseInt64` |
| `double` | JSON number | `hoo_double_to_string` / `parseDouble` |
| `f64` | JSON number | same as double |
| `f8` | JSON number | `hoo_f8_to_string` / `parseF8` |
| `string` | JSON string | direct copy |
| `bool` | JSON bool | `hoo_bool_to_string` / `parseBool` |
| `bit` | JSON bool (`true`/`false`) | `hoo_bit_to_string` / `parseBit` |
| `buffer` | Tagged JSON object (base64) | runtime buffer reconstruction |
| `tensor<T>[d0,..]` | Tagged shape/data object | element-type, dimensions, raw-bit restore |
| `HashMap<K,V>` | JSON object | existing `json_serialize_hashmap` / `json_deserialize_hashmap` |
| `AnyArray` | JSON array | existing `json_serialize_anyarray` / `json_deserialize_anyarray` |
| serializable class | JSON object | recursive serialize/deserialize |

### 5.4 Symbol Mangling
The generated methods use the following mangled names:

```
// serialize
_F_<ClassName>_R_serialize_p       ; func :string serialize()
// deserialize
_F_<ClassName>_R_deserialize_static_p_s ; static func :<ClassName> deserialize(json: string)
```

These symbols are registered in the JIT symbol table alongside regular class methods.

---

## 6. Cyclic Reference Detection Algorithm

### 6.1 Graph Traversal
```
Algorithm: detectSerializableCycles(classes)
  Input: List of all class declarations with their modifiers
  Output: Set of error messages for each detected cycle

  Build adjacency list:
    for each class C with serializable modifier:
      for each public field F in C:
        if F.type is a class type T, and T has serializable modifier:
          add edge C -> T

  Detect cycles via DFS:
    WHITE = unvisited, GRAY = in current stack, BLACK = finished
    for each class C:
      if color[C] == WHITE:
        dfs(C, path=[])

  dfs(node, path):
    color[node] = GRAY
    path.push(node)
    for each neighbor N of node:
      if color[N] == GRAY:
        CYCLE FOUND: path from N to node to N
        emit error("Serializable cycle detected: " + format(path + [N]))
      else if color[N] == WHITE:
        dfs(N, path)
    path.pop()
    color[node] = BLACK
```

HashMap and AnyArray fields are excluded from adjacency (their value types cannot be serializable classes per §3.3, so they cannot add edges).

### 6.2 Error Messages
```
Error: Serializable class 'A' forms a cycle: A -> B -> C -> A
Error: Serializable class 'Node' forms a self-referential cycle: Node -> Node
```

---

## 7. Interaction with Existing Modifiers

### 7.1 Compatibility Matrix
| Modifier | Compatible with Serializable? | Notes |
|----------|------------------------------|-------|
| `singleton` | YES | Serializing a singleton serializes its current state |
| `immutable` | YES | Serialization is a read operation, compatible |
| `service` | NO | Service classes hold runtime dependencies; serialization of service state is undefined |
| `final` | YES | No semantic conflict |

Validation rule: reject `serializable service` at compile time.

### 7.2 Inheritance
A `serializable` class may extend another `serializable` class. Inherited public fields are included in serialization in declaration order (base first, then derived). Both classes must individually satisfy all serializable constraints, and cycle detection includes the full transitive closure.

```hoo
serializable class Base {
    public var id: int64;
    constructor() { id = 0; }
}

serializable class Derived extends Base {
    public var name: string;
    constructor() { name = ""; }
}

// serialize() on Derived produces: {"id": 0, "name": ""}
```

A `serializable` class cannot extend a non-serializable class, and vice versa:

```hoo
class Plain {
    public var x: int64;
}

// Error: cannot extend non-serializable class from serializable class
serializable class Bad extends Plain {
    public var y: int64;
    constructor() {}
}
```

---

## 8. Implementation Phases

### Phase 1: Grammar and AST
1. Add `SERIALIZABLE` keyword to `Hooc.g4` lexer and `classModifier` rule.
2. Add `ClassModifier::SERIALIZABLE` to the enum in `ClassDeclaration.h`.
3. Add `classModifierToString` case in `ASTImpl.cpp`.
4. Add `getClassModifier` case in `SimpleASTBuilder.cpp`.

### Phase 2: Validation — Field Types and Visibility
1. In `HVMCodeGenerator.cpp`, after class layout computation, add a validation pass:
   - Skip if class does not have `SERIALIZABLE` modifier.
   - Iterate public fields, verify each field type is in the allowed set (`int8`, `byte`, `int64`, `double`, `f64`, `f8`, `string`, `bool`, `bit`, `buffer`, `tensor<...>`, `HashMap<K,V>`, `AnyArray`, or serializable class).
   - For `PrimitiveType`, accept `PrimitiveTypeKind::BIT` and `PrimitiveTypeKind::BUFFER` in addition to the previously allowed kinds.
   - For `HashMapType`, inspect the value type and verify it is restricted to the expanded set (including `bit`, `buffer`, `tensor`).
   - For `AnyArrayType`, value types must conform to the same expanded restricted set (compile-time metadata may not fully enforce runtime contents, but the declared type constraint is validated).
   - For `BaseType` with an identifier, look up the referenced class and verify it is serializable.

### Phase 3: Validation — Constructor and Field Count
1. Verify exactly one constructor exists.
2. Verify that constructor has zero parameters.
3. Verify at least one public field exists.

### Phase 4: Cycle Detection
1. Implement the DFS-based cycle detection algorithm described in §6.
2. Run after all class declarations are loaded, before code generation.
3. Emit compile errors for all detected cycles.

### Phase 5: Serialize Code Generation
1. For each serializable class, generate a `serialize()` method that:
   - Creates a `HashMap<int64, any>` (the runtime HashMap ABI is numeric-keyed).
   - Emits deterministic base-first positional entries for public fields,
     including inherited fields.
   - Recursively lowers nested serializable objects, buffers, and tensors.
   - Calls `json_serialize_hashmap` and returns the result.
2. Register the modifier-aware generated symbol `_F_<Class>_R_serialize_p`.

### Phase 6: Deserialize Code Generation
1. For each serializable class, generate a static `deserialize(json: string)` method that:
   - Calls `json_deserialize_hashmap` on the input.
   - Constructs a new instance via the parameterless constructor.
   - For each public field, extracts the any value and converts promoted
     scalar values back to the declared type.
   - Recursively invokes nested generated deserializers and reconstructs
     tagged buffers and tensors.
   - Assigns the converted value to the field.
2. Register with modifier-aware mangled name `_F_<Class>_R_deserialize_static_p_s`.

### Phase 7: Validation and Tests

#### 7.1 Validation Tests (compile-time errors)
| Test Case | Expected Result |
|-----------|----------------|
| `serializable` class with no public fields | Compile error: must have at least one public field |
| `serializable` class with `float` field | Compile error: float not allowed |
| `serializable` class with `char` field | Compile error: char not allowed |
| `serializable` class with `Map<K,V>` field | Compile error: Map not allowed |
| `serializable` class with `Array<T>` field | Compile error: Array not allowed |
| `serializable` class with `HashMap<int64, Inner>` where Inner is serializable | Compile error: serializable class not allowed as HashMap value |
| `serializable` class with `HashMap<int64, float>` | Compile error: float not allowed as HashMap value |
| `serializable` class with no constructor | Compile error: must have exactly one constructor |
| `serializable` class with two constructors | Compile error: must have exactly one constructor |
| `serializable` class with parameterized constructor | Compile error: constructor must have no parameters |
| Cycle A→B→A via serializable fields | Compile error: cycle detected |
| Self-referential cycle (Node→Node) | Compile error: cycle detected |
| `serializable service` class | Compile error: incompatible modifiers |

#### 7.2 Valid Declaration Tests (no compile errors)
| Test Case | Notes |
|-----------|-------|
| All allowed primitive fields (`int8`, `byte`, `int64`, `double`, `f64`, `f8`, `string`, `bool`, `bit`, `buffer`) | Verify each individually and combined |
| `tensor<int8>[3]` field | Single-rank tensor |
| `tensor<double>[4,4]` field | Multi-rank tensor |
| `HashMap<int64, string>` field | HashMap with string value |
| `HashMap<int64, tensor<double>[3,3]>` field | HashMap with tensor value |
| `HashMap<int64, bit>` field | HashMap with bit value |
| `HashMap<int64, buffer>` field | HashMap with buffer value |
| `AnyArray` field | Plain AnyArray |
| Serializable class field (no cycle) | Acyclic class graph |
| Inheritance: `serializable` Base → `serializable` Derived | Both serializable, no cycle |
| Combined: `singleton serializable` | Compatible modifiers |
| Combined: `immutable serializable` | Compatible modifiers |
| Combined: `final serializable` | Compatible modifiers |
| Serializable timestamp wrapper with `int64` field | Parameterless ctor, one public field, valid primitive |
| Direct built-in `DateTime` field | Deferred: requires built-in class metadata integration |

#### 7.3 Round-trip Serialization Tests
1. **Primitive round-trip**: populate every allowed primitive field (`int8`, `byte`, `int64`, `double`, `f64`, `f8`, `string`, `bool`, `bit`, `buffer`), serialize to JSON, deserialize, verify all fields match.
2. **Tensor round-trip**: populate `tensor<double>[4,4]` with known values, serialize, deserialize, verify element-by-element equality.
3. **HashMap round-trip**: populate `HashMap<int64, string>` with entries, serialize, deserialize, verify key-value pairs match.
4. **HashMap with bit values**: populate `HashMap<int64, bit>`, serialize, deserialize, verify bits preserved.
5. **HashMap with buffer values**: populate `HashMap<int64, buffer>` with binary data, serialize (base64), deserialize, verify byte equality.
6. **HashMap with tensor values**: populate `HashMap<int64, tensor<double>[3,3]>`, serialize, deserialize, verify tensors match.
7. **Nested serializable class round-trip**: serialize `Line { start: Point, end: Point }`, deserialize, verify nested fields.
8. **Inheritance round-trip**: serialize `Derived extends Base`, deserialize as `Derived`, verify both base and derived fields.
9. **Empty serializable class** (with at least one field set to default): verify serialization produces correct JSON and deserialization reconstructs defaults.
10. **Timestamp-wrapper round-trip**: populate a serializable wrapper with a timestamp, serialize to JSON, deserialize, and verify the `int64` matches.
11. **Built-in DateTime nested round-trip**: deferred until built-in class metadata integration is added.
12. **AnyArray round-trip**: populate `AnyArray` with mixed allowed types (`int64`, `string`, `double`, `bit`), serialize, deserialize, verify contents.

#### 7.4 Edge-Case and Error Tests
1. **Null buffer field**: serialize a serializable class with a null buffer, verify JSON output, deserialize back to null.
2. **Empty HashMap field**: serialize with empty map, verify `{}` in JSON.
3. **Very long string field**: verify serialization handles large string values without truncation.
4. **Tensor with extreme values**: `tensor<double>[2]` containing `INF`, `-INF`, `NaN` — verify they survive round-trip or produce documented error.
5. **Nested null**: if a nested serializable class field is null during serialization, emit JSON `null` and reconstruct as null on deserialization.
6. **Inheritance with cycle in derived**: `Base` → `Derived extends Base` where `Derived` adds a `Base`-typed field — verify cycle detection catches this.

---

## 9. Status
- **Date**: 2026-08-06
- **Status**: **IMPLEMENTED - CODEGEN, RUNTIME JSON, NESTING, INHERITANCE, AND REGRESSION COVERAGE COMPLETE**
- **Priority**: **MEDIUM** (Feature enhancement — no correctness impact on existing code)
- **Implementation audit 2026-08-06**: Added modifier-aware source dispatch, inherited-field layout and traversal, nested serializable lowering, tagged buffer base64 support, tagged tensor shape/raw-bit support, and runtime round-trip tests. The JSON schema intentionally retains numeric positional keys because `HooHashMap` is a numeric-keyed ABI; ordering is deterministic and base fields precede derived fields.

### Open Questions
1. Tensors are preserved with shape metadata and raw element bits in a tagged object, enabling faithful round-trip without precision loss.
2. Should `serialize()` and `deserialize()` be overridable by user-defined methods? Recommendation: yes — if the user defines their own, the compiler uses the user version instead of the generated one.
3. Should fields be serialized in declaration order or alphabetically? Recommendation: declaration order (deterministic, matches source layout).
