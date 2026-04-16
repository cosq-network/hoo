# Hooc Language Elements - Compilation Unit Hierarchy

This document lists all language elements in Hooc that can be compiled as independent units, ordered from lightest (smallest) to heaviest (largest).

---

## Tier 1: Atomic Expressions (No Dependencies)

These are the smallest compilable units. They have no sub-elements that require compilation.

| Element | Syntax | Example |
|---------|--------|---------|
| Integer literal | `[0-9]+` | `42`, `100`, `0` |
| Float literal | `[0-9]+ '.' [0-9]+` | `3.14`, `0.5`, `2.0` |
| String literal | `'"' ... '"'` | `"hello"`, `"world"` |
| Char literal | `'\'' ... '\''` | `'a'`, `'\n'`, `'x'` |
| Boolean literal | `true` / `false` | `true`, `false` |
| Null literal | `null` | `null` |
| Identifier | `[a-zA-Z_][a-zA-Z0-9_]*` | `x`, `myVariable`, `maxCount` |

**Compilation**: These can be compiled inline as part of expressions.

---

## Tier 2: Simple Expressions (Can Stand Alone)

Expressions that combine atomic elements but don't require statements.

| Element | Syntax | Example |
|---------|--------|---------|
| Parenthesized expression | `'(' expression ')'` | `(1 + 2)`, `(x > 0)` |
| Unary negation | `'-' expression` | `-x`, `-42` |
| Unary logical NOT | `'!' expression` | `!isValid`, `!done` |
| Binary multiplicative | `expression ('*' \| '/' \| '%') expression` | `a * b`, `x / 2`, `n % 10` |
| Binary additive | `expression ('+' \| '-') expression` | `x + y`, `a - b`, `i - 1` |
| Binary relational | `expression ('<' \| '<=' \| '>' \| '>=') expression` | `x < 10`, `a >= b` |
| Binary equality | `expression ('==' \| '!=') expression` | `x == null`, `a != b` |
| Binary logical AND | `expression '&&' expression` | `x > 0 && y > 0` |
| Binary logical OR | `expression '\|\|' expression` | `isValid \|\| isDefault` |

**Compilation**: These compile to value computations.

---

## Tier 3: Postfix Expressions (Member Access)

Expressions with property/method access or function calls.

| Element | Syntax | Example |
|---------|--------|---------|
| Member access | `expression '.' IDENTIFIER` | `obj.name`, `point.x` |
| Index access | `expression '[' expression ']'` | `arr[0]`, `items[i]` |
| Function call | `expression '(' argumentList? ')'` | `print(x)`, `obj.method()` |
| New expression | `'new' IDENTIFIER typeArgs? '(' args? ')'` | `new Box()`, `new Pair<string, int64>()` |
| Function call (postfix) | `IDENTIFIER '(' argumentList? ')'` | `foo()`, `bar(1, 2)` |
| Array literal | `'[' expressionList? ']'` | `[]`, `[1, 2, 3]`, `['a', 'b']` |

**Compilation**: These involve method resolution, potentially requiring imported types.

---

## Tier 4: Assignment & Control Flow

Statements that perform assignments or control flow.

| Element | Syntax | Example |
|---------|--------|---------|
| Variable declaration (inferred) | `'var' IDENTIFIER '=' expression` | `var x = 5`, `var name = "Alice"` |
| Variable declaration (typed) | `'var' IDENTIFIER ':' type '=' expression` | `var x: int64 = 10` |
| Variable declaration (typed, no init) | `'var' IDENTIFIER ':' type` | `var items: int64[]` |
| Expression statement | `expression ';'` | `foo();`, `x = 10;` |
| Return statement (with value) | `'return' expression ';'` | `return x + 1;` |
| Return statement (void) | `'return' ';'` | `return;` |
| If statement (no else) | `'if' '(' expression ')' block` | `if (x > 0) { return x; }` |
| If statement (with else) | `'if' '(' expression ')' block 'else' block` | `if (x > 0) { ... } else { ... }` |
| While statement | `'while' '(' expression ')' block` | `while (i < 10) { i = i + 1; }` |
| For statement | `'for' IDENTIFIER 'in' expression ('..' expression)? block` | `for i in 0..10 { sum = sum + i; }` |
| Scope statement | `'scope' block` | `scope { var temp = x; x = y; y = temp; }` |

**Compilation**: These can be compiled as independent function bodies or blocks.

---

## Tier 5: Block & Compound Structures

Structural elements that contain statements.

| Element | Syntax | Example |
|---------|--------|---------|
| Block | `'{' statement* '}'` | `{ var x = 1; return x; }` |

**Compilation**: A block with statements can be a function body or a lambda body.

---

## Tier 6: Functions & Constructors

Callable units that can be compiled independently (may reference imports).

| Element | Syntax | Example |
|---------|--------|---------|
| Function (void, no params) | `'func' IDENTIFIER '(' ')' '->' 'void' block` | `func main() -> void { return; }` |
| Function (with params) | `'func' IDENTIFIER '(' params ')' '->' type block` | `func add(a: int64, b: int64) -> int64 { return a + b; }` |
| Function (no return type) | `'func' IDENTIFIER '(' params? ')' block` | `func greet(name: string) { print(name); }` |
| Generic function | `'func' IDENTIFIER '<' typeParams '>' '(' params ')' '->' type block` | `func identity<T>(x: T) -> T { return x; }` |
| Constructor | `'constructor' '(' params? ')' block` | `constructor(x: int64, y: int64) { this.x = x; this.y = y; }` |
| Event declaration | `'event' IDENTIFIER ';'` | `event onClick;`, `event onUpdate;` |

**Compilation**: Functions can be compiled to standalone object code with external references.

---

## Tier 7: Top-Level Declarations

Declarations that define types and values at module scope.

| Element | Syntax | Example |
|---------|--------|---------|
| Global variable | `'var' IDENTIFIER '=' expression ';'` | `var MAX_SIZE = 100;` |
| Global typed variable | `'var' IDENTIFIER ':' type '=' expression ';' ` | `var VERSION: string = "1.0";` |

**Compilation**: Global variables compile to data section entries.

---

## Tier 8: Type Definitions

User-defined types that group related functionality.

| Element | Syntax | Example |
|---------|--------|---------|
| Simple class | `'class' IDENTIFIER classBody` | `class Point { constructor() {} }` |
| Class with constructor | `'class' IDENTIFIER classBody` | `class Counter { constructor() { this.count = 0; } }` |
| Class with members | `'class' IDENTIFIER classBody` | `class Calculator { func add(a: int64, b: int64) -> int64 { return a + b; } }` |
| Generic class | `'class' IDENTIFIER '<' typeParams '>' classBody` | `class Box<T> { constructor() {} }` |
| Class with inheritance | `'class' IDENTIFIER 'extends' IDENTIFIER classBody` | `class Dog extends Animal { }` |
| Class with implements | `'class' IDENTIFIER 'implements' IDENTIFIER (',' IDENTIFIER)* classBody` | `class Rect implements Drawable { }` |
| Class with modifiers | `modifiers 'class' IDENTIFIER classBody` | `singleton class Config { }`, `immutable class User { }` |
| Interface | `'interface' IDENTIFIER '{' interfaceMember* '}'` | `interface Drawable { func draw() -> void; }` |
| Interface with methods | `'interface' IDENTIFIER '{' interfaceMember* '}'` | `interface Repository { func save(d: int64) -> bool; func load(id: int64) -> int64; }` |

**Compilation**: Classes and interfaces can be compiled independently with:
- Full method implementations
- Required imports for parent/super types
- Relocations for referenced types

**Class Modifiers**: `singleton`, `immutable`, `factory`, `observable`, `service`, `strategy`, `actor`, `final`

---

## Tier 9: Module Elements

Elements that connect compilation units.

| Element | Syntax | Example |
|---------|--------|---------|
| Basic import | `'import' modulePath ';'` | `import os;`, `import std.io;` |
| Import with alias | `'import' modulePath 'as' IDENTIFIER ';'` | `import math as m;`, `import json as j;` |
| Dotted module import | `'import' IDENTIFIER ('.' IDENTIFIER)* ';'` | `import os.path;`, `import std.collections;` |
| From import (single) | `'from' modulePath 'import' IDENTIFIER ';'` | `from sys import argv;` |
| From import (multiple) | `'from' modulePath 'import' IDENTIFIER (',' IDENTIFIER)* ';'` | `from std.io import read, write, close;` |
| From import with alias | `'from' modulePath 'import' IDENTIFIER 'as' IDENTIFIER ';'` | `from os.path import join as path_join;` |

**Compilation**: Imports create dependencies but can be resolved at link time.

---

## Tier 10: Compilation Unit (Heaviest)

The complete program/module that encompasses all other elements.

| Element | Syntax |
|---------|--------|
| Compilation unit | `importStatement* declaration* EOF` |

**Example**:
```hooc
import os;
import json as j;
from std.io import read, write, close;

singleton class Config {
    var version: string;

    constructor() {
        this.version = "1.0";
    }

    func getVersion() -> string {
        return this.version;
    }
}

class Point {
    var x: int64;
    var y: int64;

    constructor(x: int64, y: int64) {
        this.x = x;
        this.y = y;
    }

    func distanceTo(other: Point) -> int64 {
        var dx = this.x - other.x;
        var dy = this.y - other.y;
        return ((dx * dx) + (dy * dy)).sqrt();
    }
}

class Calculator {
    constructor() {}

    func add(a: int64, b: int64) -> int64 {
        return a + b;
    }

    func multiply(a: int64, b: int64) -> int64 {
        return a * b;
    }
}

interface Drawable {
    func draw() -> void;
}

class Circle implements Drawable {
    var radius: int64;

    constructor(radius: int64) {
        this.radius = radius;
    }

    func draw() -> void {
    }
}

func main() -> void {
    var config = Config();
    var version = config.getVersion();

    var p1 = new Point(0, 0);
    var p2 = new Point(3, 4);

    var calc = new Calculator();
    var sum = calc.add(10, 20);

    var numbers = [1, 2, 3, 4, 5];
    var matrix = [[1, 2, 3], [4, 5, 6], [7, 8, 9]];

    return;
}
```

---

## Hierarchy Summary

```
Compilation Unit (heavy)
├── Import statements
│   ├── Basic import: import module;
│   ├── Alias import: import module as alias;
│   └── From import: from module import x, y, z;
├── Class declarations
│   ├── Modifiers: singleton, immutable, factory, observable, service, strategy, actor, final
│   ├── Extends: extends BaseClass
│   ├── Implements: implements Interface1, Interface2
│   ├── Constructor
│   ├── Event declarations
│   ├── Variable declarations
│   └── Function declarations
│       └── Block
│           └── Statements
│               └── Expressions (lightest)
├── Interface declarations
│   └── Function signatures
├── Global variable declarations
└── Top-level function declarations
    └── Block
```

---

## Independent Compilation Matrix

| Element | Can Compile Standalone? | Requires Imports | Can Export |
|---------|------------------------|------------------|------------|
| Literal | Yes | No | No |
| Identifier | Yes | Maybe | No |
| Expression | Yes | Maybe | No |
| Statement | Yes | Maybe | No |
| Block | Yes | Maybe | No |
| Function | Yes | Maybe | Yes |
| Constructor | No (needs class) | Maybe | No |
| Global variable | Yes | Maybe | Yes |
| Event | No (needs class) | No | No |
| Class | Yes | Yes | Yes |
| Interface | Yes | Maybe | Yes |
| Import | N/A | Yes | No |
| Compilation unit | Yes | Yes | Yes |

---

## Common Type Patterns

| Type | Syntax | Example |
|------|--------|---------|
| Primitive int64 | `int64` | `var x: int64` |
| Primitive float | `float` | `var f: float` |
| Primitive double | `double` | `var d: double` |
| Primitive bool | `bool` | `var flag: bool` |
| Primitive byte | `byte` | `var b: byte` |
| Primitive char | `char` | `var c: char` |
| String | `string` | `var s: string` |
| Void | `void` | `func f() -> void` |
| Array (slice) | `type '[]'` | `var arr: int64[]` |
| Array (multi-dim) | `type '[][]'` | `var m: int64[][]` |
| Nullable | `type '?'` | `var x: int64?` |
| Union | `type '|' type` | `var x: int64 \| string?` |
| Generic type | `TypeName<type>` | `var list: Array<int64>` |
| Qualified type | `Identifier ('.' Identifier)*` | `var x: std.List<string>` |

---

## HVM Instruction Set Compilation Matrix

This table indicates which Hooc language elements can be compiled to HVM instructions.

### Legend
- **Yes**: Fully compilable to HVM bytecode
- **Partial**: Partially supported or requires additional runtime support
- **No**: Not directly compilable (metadata/special handling required)
- **N/A**: Not applicable (not executable code)

### Tier 1: Atomic Expressions

| Element | HVM Compilable | HVM Instructions | Notes |
|---------|----------------|-----------------|-------|
| Integer literal | Yes | `MOVI`, `SET` | Immediate values loaded into registers |
| Float literal | Yes | `MOVI`, `FCVT`, `LUI` | Loaded from constant pool or constructed |
| String literal | Yes | `LUI`, `LD.D` | Pointer to string object in data section |
| Char literal | Yes | `SET`, `MOVI` | Zero-extended to int64 |
| Boolean literal | Yes | `SET` | `true` = 1, `false` = 0 |
| Null literal | Yes | `MOV` (r0) | r0 is hardwired to zero |
| Identifier (local) | Yes | `MOV`, `LD.D` | Load from stack frame |
| Identifier (global) | Yes | `LDF` | Load from global data section |

### Tier 2: Simple Expressions

| Element | HVM Compilable | HVM Instructions | Notes |
|---------|----------------|-----------------|-------|
| Parenthesized expression | Yes | (inner expression) | No additional instructions |
| Unary negation (int) | Yes | `NEG` | Integer negation |
| Unary negation (float) | Yes | `FNEG` | Floating-point negation |
| Unary logical NOT | Yes | `NOT`, `XORI r1, r1, 1` | Boolean NOT |
| Binary `*` | Yes | `MUL` | Integer multiply |
| Binary `/` | Yes | `DIV`, `DIVU` | Signed/unsigned divide |
| Binary `%` | Yes | `REM`, `REMU` | Signed/unsigned remainder |
| Binary `+` (int) | Yes | `ADD`, `ADDI` | Integer add |
| Binary `-` (int) | Yes | `SUB`, `SUBI` | Integer subtract |
| Binary `+` (float) | Yes | `FADD` | Float add |
| Binary `-` (float) | Yes | `FSUB` | Float subtract |
| Binary `*` (float) | Yes | `FMUL` | Float multiply |
| Binary `/` (float) | Yes | `FDIV` | Float divide |
| Binary `<` | Yes | `CMPLT`, `CMPLTU` | Signed/unsigned comparison |
| Binary `<=` | Yes | `CMPLE`, `CMPLEU` | Signed/unsigned comparison |
| Binary `>` | Yes | `CMPGT`, `FCMPGT` | Greater than |
| Binary `>=` | Yes | `CMPGE`, `FCMPGE` | Greater or equal |
| Binary `==` | Yes | `CMPEQ`, `FCMPEQ` | Equality check |
| Binary `!=` | Yes | `CMPNE` | Inequality check |
| Binary `&&` | Yes | `AND` | Short-circuit AND |
| Binary `\|\|` | Yes | `OR` | Short-circuit OR |

### Tier 3: Postfix Expressions

| Element | HVM Compilable | HVM Instructions | Notes |
|---------|----------------|-----------------|-------|
| Member access (field) | Yes | `LDF`, `STF` | Object field load/store |
| Index access (array) | Yes | `LDELEM`, `STELEM` | Array element access |
| Function call | Yes | `CALL`, `CALLI` | Direct function call |
| Method call | Yes | `CALLVIRT` | Virtual method dispatch |
| Interface call | Yes | `CALLINTF` | Interface method dispatch |
| New expression | Yes | `NEW`, `NEWA` | Object/array allocation |
| New with constructor | Yes | `NEW` + `CALL` | Allocate then call constructor |
| Array literal | Yes | `NEWA` + `STELEM` | Allocate and initialize |
| Type check | Yes | `INSTANCEOF`, `CHECKCAST` | Runtime type verification |

### Tier 4: Assignment & Control Flow

| Element | HVM Compilable | HVM Instructions | Notes |
|---------|----------------|-----------------|-------|
| Variable declaration | Yes | `ENTER`, `ST.D`, `MOV` | Stack allocation + store |
| Assignment | Yes | `MOV`, `ST.D`, `STF` | Register/memory store |
| Expression statement | Yes | (expression) | Expression result discarded |
| Return (void) | Yes | `RET` | Return from function |
| Return (value) | Yes | `MOV r1, result` + `RET` | Set return register |
| If statement | Yes | `BEQ`, `BNE`, `BLT`, etc. | Conditional branches |
| If-else statement | Yes | `BEQ`/`BNE` + `JMP` | Branch with else path |
| While statement | Yes | `BEQ`/`BNE` + `JMP` | Loop with conditional exit |
| For statement | Yes | `BEQ`/`BNE` + `JMP` | Range-based loop |
| Break statement | Yes | `JMP` (to loop end) | Unconditional jump |
| Continue statement | Yes | `JMP` (to loop start) | Jump to loop condition |
| Scope statement | Yes | `ENTER`/`LEAVE` | Explicit stack frame |

### Tier 5: Block & Compound Structures

| Element | HVM Compilable | HVM Instructions | Notes |
|---------|----------------|-----------------|-------|
| Block | Yes | `ENTER`/`LEAVE` | Stack frame management |
| Empty block | Yes | (no-op) | No instructions needed |

### Tier 6: Functions & Constructors

| Element | HVM Compilable | HVM Instructions | Notes |
|---------|----------------|-----------------|-------|
| Top-level function | Yes | `CALL`, `RET` | Full function body |
| Method function | Yes | `CALLVIRT`, `RET` | Method with implicit `this` |
| Static function | Yes | `CALL`, `RET` | Class static method |
| Generic function | Partial | (monomorphized) | Requires type instantiation |
| Constructor | Yes | `NEW`, field inits, `RET` | Object initialization |
| Destructor | Yes | field cleanup, `RET` | Object cleanup |
| Event declaration | N/A | (metadata) | Not executable, metadata only |

### Tier 7: Top-Level Declarations

| Element | HVM Compilable | HVM Instructions | Notes |
|---------|----------------|-----------------|-------|
| Global variable | Yes | `LUI`, `LD.D`, `ST.D` | Static data in `.data` section |
| Global constant | Yes | (read-only data) | In `.rodata` section |

### Tier 8: Type Definitions

| Element | HVM Compilable | HVM Instructions | Notes |
|---------|----------------|-----------------|-------|
| Simple class | Yes | `NEW`, `LDF`, `STF` | Object layout + methods |
| Class with fields | Yes | Field layout + `NEW` | Field offsets calculated |
| Class with methods | Yes | Method dispatch + bodies | Vtable generation |
| Class inheritance | Yes | `NEW`, vtable setup | Parent fields + new fields |
| Generic class | Partial | (monomorphized) | Requires type instantiation |
| Class modifiers | Partial | (runtime checks) | `singleton`, `immutable`, etc. |
| Interface | Yes | `CALLINTF`, metadata | Interface dispatch table |
| Interface method | N/A | (metadata) | Signature only, not body |

### Tier 9: Module Elements

| Element | HVM Compilable | HVM Instructions | Notes |
|---------|----------------|-----------------|-------|
| Basic import | Partial | `IMPORT`, `LOADMOD` | Resolved at load time |
| From import | Partial | `IMPORT`, `RESOLVE` | Symbol resolution |
| Import with alias | Partial | `RESOLVE` | Name mapping |
| Dotted module path | Partial | `LOADMOD`, `RESOLVE` | Hierarchical modules |

### Tier 10: Compilation Unit

| Element | HVM Compilable | HVM Instructions | Notes |
|---------|----------------|-----------------|-------|
| Complete program | Yes | All above | Full `.ho` file compilation |

---

## HVM Type Mapping

| Hooc Type | HVM Size | HVM Representation | Notes |
|-----------|----------|--------------------|-------|
| `int64` | 64 bits | Register (r1-r31) | Full 64-bit integer |
| `float` | 32 bits | Register (float) | Single precision |
| `double` | 64 bits | Register (f64) | Double precision |
| `bool` | 8 bits | Register (0 or 1) | Boolean as integer |
| `byte` | 8 bits | Register | Zero/sign extended |
| `char` | 8 bits | Register | UTF-8 code unit |
| `string` | variable | Pointer (i8*) | Heap-allocated object |
| `object` | variable | Pointer (i8*) | Heap-allocated with vtable |
| `array<T>` | variable | Pointer (i8*) | Heap-allocated with length |
| `T?` (nullable) | 64 bits | Pointer (nullable) | Null allowed |

---

## HVM Instruction Categories Summary

| Category | Instructions | Hooc Features Supported |
|----------|-------------|------------------------|
| **Data Movement** | `MOV`, `MOVI`, `MOVZ`, `LUI`, `NEG`, `XCHG` | Literals, variable access |
| **Integer Arithmetic** | `ADD`, `SUB`, `MUL`, `DIV`, `REM`, `SHL`, `SHR`, `SAR` | Arithmetic operators |
| **Float Arithmetic** | `FADD`, `FSUB`, `FMUL`, `FDIV`, `FSQRT`, `FABS`, `FNEG` | Float/double operators |
| **Logical/Bitwise** | `AND`, `OR`, `XOR`, `NOT`, `CLZ`, `CTZ`, `POPCNT` | Bitwise operations |
| **Comparison** | `CMPEQ`, `CMPNE`, `CMPLT`, `CMPLE`, `CMPGT`, `CMPGE` | Relational operators |
| **Float Comparison** | `FCMPEQ`, `FCMPLT`, `FCMPLE`, `FCMPGT`, `FCMPGE` | Float comparisons |
| **Branches** | `BEQ`, `BNE`, `BLT`, `BLE`, `BGT`, `BGE`, `BLTU`, `BGEU` | `if`, `while`, `for` |
| **Jumps** | `JMP`, `JAL`, `JALR`, `RET` | Function calls, `break`, `continue` |
| **Memory Load** | `LD.B`, `LD.BU`, `LD.H`, `LD.HU`, `LD.W`, `LD.WU`, `LD.D` | Memory access |
| **Memory Store** | `ST.B`, `ST.H`, `ST.W`, `ST.D` | Memory writes |
| **Stack** | `PUSH`, `POP`, `ENTER`, `LEAVE`, `ADJSP` | Local variables, stack frames |
| **Objects/Arrays** | `NEW`, `NEWA`, `LDF`, `STF`, `LDELEM`, `STELEM`, `ARRAYLEN` | Classes, arrays |
| **Type Operations** | `INSTANCEOF`, `CHECKCAST` | Type checking |
| **Virtual Calls** | `CALLVIRT`, `CALLINTF` | Method dispatch |
| **Dynamic Linking** | `IMPORT`, `LOADMOD`, `RESOLVE` | Module imports |
| **Conversions** | `SEXT`, `ZEXT`, `TRUNC`, `FCVT` | Type conversions |
| **Vector/SIMD** | `VADD`, `VSUB`, `VMUL`, `VDOT`, etc. | SIMD operations |

---

## Notes on Compilation Independence

1. **Literal expressions** are always self-contained
2. **Identifiers** may reference globals or imports
3. **Local variables** are fully contained within their block
4. **Functions** can be compiled independently if they don't reference external types
5. **Classes** require their member functions, but member functions can reference the class
6. **Interfaces** require their method signatures but no implementation
7. **Imports** create dependencies but don't affect the ability to compile the importing unit
8. **Generic types** like `Array<T>` and `Box<T>` require type argument instantiation at use site
