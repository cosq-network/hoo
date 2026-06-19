# ISSUE-034: Language Ergonomics Proposals from Perl, Lua, and Rust Influences

## 1. Overview
This document proposes six language ergonomics enhancements for Hoo, inspired by proven developer-experience features from Perl, Lua, and Rust. Each proposal is evaluated against Hoo's design constraints: aggressive lowering to HVM RISC, ARC-based memory management, static typing, and hardware/JIT parity.

Design constraint: every feature must lower to existing HVM arithmetic, load/store, branch, CALL, and existing runtime mechanisms. No new HVM opcodes, hidden object instructions, or JIT-only behavior are permitted.

---

## 2. Proposal 1: `Result<T, E>` Algebraic Type with `?` Propagation Operator

### 2.1 Motivation
Hoo currently uses try/catch/throw for error handling. Exceptions require a runtime shadow stack (`hoo_push_handler`/`hoo_pop_handler`) and make failure paths invisible at call sites. A `Result<T, E>` algebraic type with a `?` propagation operator provides explicit, zero-cost error handling that compiles to simple conditional branches.

### 2.2 Design
`Result<T, E>` is a compiler intrinsic represented as a two-slot tagged union:
- Slot 0: discriminator (`0` = Ok, `1` = Err)
- Slot 1: payload (T on Ok, E on Err)

At the HVM level, it behaves like a fat register pair similar to `any`, but with a two-state discriminator instead of a type ID.

### 2.3 Grammar & Syntax
```hoo
// Function returning Result
func :Result<string, io.Error> readFile(path: string) {
    var f = Fs.open(path)?;      // ? propagates Err to caller
    return f.readAll();
}

// Chained ? propagation
func :Result<int64, io.Error> getSize(path: string) {
    return readFile(path)?.length();
}

// Pattern matching on Result
func handleError(err: io.Error) : void {
    Console.println("failed: " + err.message());
}

func main() : void {
    var res = readFile("config.hoo");
    // match expression (see Proposal 6)
    match res {
        case Ok(content) => Console.println(content);
        case Err(e)      => handleError(e);
    }
}
```

### 2.4 Lowering Semantics
The `?` operator lowers to a branch sequence:

```lir
// Given: var content = readFile(path)?;
// Equivalent HVM pseudo-code:
  CALL readFile, r1           ; r1 = discriminator, r2 = payload
  CMPI r1, 1                  ; is it Err?
  BNE .ok                     ; if Ok, continue
  MOV r1, r2                  ; propagate the Err payload
  RET                         ; early return from enclosing function
.ok:
  MOV r_content, r2           ; unwrap the Ok payload
```

No shadow stack operations, no heap allocation, no exception handler registration.

### 2.5 Standard Library Integration
```hoo
// Built-in Result combinators (no allocation)
namespace result {
    func :Result<T, E> ok(value: T);
    func :Result<T, E> err(error: E);
    func :bool isOk(res: Result<T, E>);
    func :bool isErr(res: Result<T, E>);
    func :T unwrap(res: Result<T, E>);       // panics on Err
    func :T unwrapOr(res: Result<T, E>, default: T);
    func :Result<U, E> map(res: Result<T, E>, fn: (T) -> U);
    func :Result<T, F> mapErr(res: Result<T, E>, fn: (E) -> F);
    func :Result<U, E> andThen(res: Result<T, E>, fn: (T) -> Result<U, E>);
}
```

### 2.6 Compatibility
- HVM: Uses only CMPI, BNE, MOV, RET — all existing opcodes.
- Runtime: No new runtime helpers required. The compiler generates the branch pattern inline.
- ARC: Ok payloads are retained/released via existing ARC rules on the enclosing scope. The `?` propagation releases any intermediate values as it unwinds.

---

## 3. Proposal 2: Lua-Style Multiple Return Values

### 3.1 Motivation
Returning multiple values from a function currently requires creating an Array, Map, or a wrapper class. For a RISC target with multiple registers, returning values in registers is free. Lua's multiple return convention maps perfectly to HVM's existing call ABI (r1 = return, r2–r8 = args).

### 3.2 Design
A function declares multiple return types separated by commas. The caller destructures with parenthesized binding. The compiler assigns each return value to consecutive registers starting at r1.

### 3.3 Grammar & Syntax
```hoo
// Declaration
func : (int64, int64) divmod(a: int64, b: int64) {
    return a / b, a % b;
}

// Destructuring bind
var (q, r) = divmod(17, 5);

// Discard with underscore
var (q, _) = divmod(17, 5);

// Type inference works
func : (string, double, bool) parseConfig(line: string) {
    // ...
    return name, value, valid;
}
var (name, val, ok) = parseConfig("key=3.14");
```

### 3.4 Nested and Chained Returns
```hoo
func : (int64, string) inner() {
    return 42, "hello";
}

// Functions can pass through multi-returns
func : (int64, string) outer() {
    return inner();   // forwards both values
}

// Works with Result too
func :Result<(int64, string>, io.Error> getPair() {
    return Result.ok((42, "hello"));
}
```

### 3.5 Lowering
Multiple return values map directly to HVM registers:

```lir
// var (q, r) = divmod(17, 5);
  MOV r2, 17
  MOV r3, 5
  CALL divmod      ; r1 = q, r2 = r
  MOV r_q, r1
  MOV r_r, r2
```

No stack spills, no heap allocation. The calling convention already supports this -- it is purely a language-level relaxation of the single-return restriction.

### 3.6 Compatibility
- HVM: Uses existing CALL convention. The callee stores up to 7 return values in r1–r7.
- Runtime: No changes. Existing `hoort` functions that conceptually return multiple values (e.g., DateTime components) can be exposed via this syntax.
- ARC: Each returned value is independently retained/released.

---

## 4. Proposal 3: Perl-Style Built-in Regex Literals

### 4.1 Motivation
Currently, regex usage requires `Regex.compile("[a-z]+")` at every call site. Perl's first-class regex literals (`/pattern/`) remain one of the most ergonomic features in any language. A compile-time regex literal eliminates runtime parse overhead and makes pattern-heavy code dramatically more readable.

### 4.2 Design
A regex literal `/pattern/flags` is recognized at the lexer level and compiled to a pre-built regex struct at compile time. The result is a `Regex` intrinsic value — not a string — that can be used with match, replace, and split operations.

### 4.3 Grammar & Syntax
```hoo
// Basic matching
if /^[a-z0-9]+$/.matches(input) {
    Console.println("valid identifier");
}

// With flags
var re = /hello/i;           // case-insensitive
var re2 = /^foo.*bar$/m;     // multiline

// Match extraction
var m = /name:\s*(\w+)/.match(line);
if m.isMatched() {
    Console.println("name = " + m.group(1));
}

// Replace
var result = /old\s+value/.replace(text, "new_value");

// Split
var parts = /\s*,\s*/.split(csvLine);

// Chained operations
var clean = /^\s+|\s+$/.replaceAll(
    /[^a-zA-Z0-9\s]/.replaceAll(raw, ""),
    ""
);
```

### 4.4 Supported Flags
| Flag | Meaning |
|------|---------|
| `i` | Case-insensitive |
| `m` | Multiline (^/$ match line boundaries) |
| `s` | Dot-all (. matches newline) |
| `x` | Extended mode (ignore whitespace, allow comments) |

### 4.5 Lowering
```hoo
// Source: var re = /hello/i;
// Lowers to compile-time constant:
var re: Regex = __regex_compile("hello", REGEX_FLAG_I);
```

The `__regex_compile` intrinsic is evaluated at compile time during code generation. The generated HVM module embeds the pre-compiled regex as a constant data blob. If compile-time evaluation is infeasible (e.g., cross-compilation), it falls back to a runtime call that caches the result.

```lir
// HVM output: load pre-compiled regex struct address
  LDA r_re, .regex_const_hello_i
  ...
.data:
.regex_const_hello_i:
  .quad 111              ; type_id = Regex
  .quad ...              ; pre-compiled DFA/NFA state
```

### 4.6 Match Result API
```hoo
var re = /(\w+)=(\d+)/;
var m = re.match("key=42");

m.isMatched()     -> bool
m.groupCount()    -> int64
m.group(n)        -> string    // nth capture group
m.start(n)        -> int64     // start offset of nth group
m.end(n)          -> int64     // end offset of nth group
m.prefix()        -> string    // text before match
m.suffix()        -> string    // text after match
m.replace(replacement) -> string
```

### 4.7 Compatibility
- HVM: Uses `LDA` for constant loading and `CALL` for match/replace operations. No new opcodes.
- Runtime: Regex struct type already exists (`typeId 111`). The compiler embeds a serialized compiled regex in the `.data` section.
- Compile-time: The regex is compiled once during codegen, not once per execution.

---

## 5. Proposal 4: Rust-Style `Option<T>` with Combinators

### 5.1 Motivation
Hoo has `type?` nullable types, but these are raw pointers that may be null — no safety guarantees, no combinators, no forced checking. An `Option<T>` algebraic type provides safe null handling with a rich combinator API that all lowers to simple null comparisons.

### 5.2 Design
`Option<T>` is a compiler intrinsic represented as a discriminator + payload pair (identical layout to `Result<T, E>` but with `None`/`Some` states):

- Slot 0: discriminator (`0` = None, `1` = Some)
- Slot 1: payload (T)

### 5.3 Grammar & Syntax
```hoo
// Return type
func :Option<int64> findUser(id: int64) {
    if (id < 0) { return Option.none(); }
    return Option.some(id * 1000);
}

// Pattern matching
var uid = findUser(42);
match uid {
    case Some(val) => Console.println("User: " + val);
    case None      => Console.println("not found");
}

// Combinator chaining
func :Option<int64> lookup(id: int64) {
    return findUser(id)
        .map(|v| v + 1)
        .filter(|v| v > 0);
}

// Combined with ? propagation
func :int64 getUserId(id: int64) : Result<int64, string> {
    var uid = findUser(id)?;           // None propagates
    if (uid < 0) { return Result.err("invalid"); }
    return Result.ok(uid);
}
```

### 5.4 Standard Combinators
```hoo
namespace option {
    func :Option<T> some(value: T);
    func :Option<T> none();
    func :bool isSome(opt: Option<T>);
    func :bool isNone(opt: Option<T>);
    func :T unwrap(opt: Option<T>);          // panics on None
    func :T unwrapOr(opt: Option<T>, default: T);
    func :Option<U> map(opt: Option<T>, fn: (T) -> U);
    func :Option<T> filter(opt: Option<T>, pred: (T) -> bool);
    func :T orElse(opt: Option<T>, fallback: () -> T);
    func :Option<U> andThen(opt: Option<T>, fn: (T) -> Option<U>);
    func :T expect(opt: Option<T>, msg: string);  // panics with message
}
```

### 5.5 Interaction with `type?` Nullables
The existing `type?` nullable syntax desugars to `Option<T>` internally:

```hoo
var x: int64? = null;    // equivalent to: var x: Option<int64> = Option.none();
var y: int64? = 42;      // equivalent to: var y: Option<int64> = Option.some(42);
```

Over a transition period, `type?` becomes sugar for `Option<T>`, maintaining backward compatibility while unlocking the full combinator API.

### 5.6 Lowering
```lir
// var v = opt.unwrapOr(0);
// Equivalent to:
  LDA r_disc, opt+0     ; load discriminator
  CMPI r_disc, 0        ; is None?
  BNE .some
  MOV r_v, 0            ; default value
  JMP .done
.some:
  LDE r_v, opt+8        ; load payload
.done:
```

### 5.7 Compatibility
- HVM: CMPI, BNE, MOV, LDE/STE — all existing.
- Runtime: No new helpers. The discriminator is an `int64` in a known offset from the value pointer.
- ARC: The Some payload is retained/released by the enclosing scope.

---

## 6. Proposal 5: Lua-Style Metatables for Operator Overloading

### 6.1 Motivation
Hoo currently has no mechanism for user-defined operator overloading — `+` on custom types either fails or produces unintended behavior. Lua's metatable system provides a clean, explicit mechanism where each operator maps to a named method (`__add`, `__sub`, etc.) without introducing complex type-class machinery.

### 6.2 Design
Any class may define methods with reserved `__` names that the compiler recognizes as operator overloads. The compiler emits the corresponding method call in place of the operator. No runtime dispatch table is needed — the method is resolved at compile time based on the left operand's type.

### 6.3 Supported Operators
| Operator | Method Name | Signature |
|----------|-------------|-----------|
| `a + b`  | `__add` | `(other: T) -> T` |
| `a - b`  | `__sub` | `(other: T) -> T` |
| `a * b`  | `__mul` | `(other: T) -> T` |
| `a / b`  | `__div` | `(other: T) -> T` |
| `a % b`  | `__mod` | `(other: T) -> T` |
| `-a`     | `__neg` | `() -> T` |
| `a[b]`   | `__index` | `(key: K) -> V` |
| `a[b] = v` | `__newindex` | `(key: K, value: V) -> void` |
| `a == b` | `__eq` | `(other: T) -> bool` |
| `a < b`  | `__lt` | `(other: T) -> bool` |
| `a <= b` | `__le` | `(other: T) -> bool` |
| `a() `   | `__call` | `(...args) -> R` |
| `to_string(a)` | `__tostring` | `() -> string` |

### 6.4 Grammar & Syntax
```hoo
class Vector3 {
    var x: float;
    var y: float;
    var z: float;

    constructor(x: float, y: float, z: float) {
        this.x = x; this.y = y; this.z = z;
    }

    func __add(other: Vector3) : Vector3 {
        return new Vector3(x + other.x, y + other.y, z + other.z);
    }

    func __sub(other: Vector3) : Vector3 {
        return new Vector3(x - other.x, y - other.y, z - other.z);
    }

    func __mul(scalar: float) : Vector3 {
        return new Vector3(x * scalar, y * scalar, z * scalar);
    }

    func __eq(other: Vector3) : bool {
        return x == other.x && y == other.y && z == other.z;
    }

    func __tostring() : string {
        return "Vector3(" + x + ", " + y + ", " + z + ")";
    }
}

// Usage
var a = new Vector3(1.0, 2.0, 3.0);
var b = new Vector3(4.0, 5.0, 6.0);
var c = a + b;          // calls a.__add(b)
var d = a - b;          // calls a.__sub(b)
var e = a * 2.0;        // calls a.__mul(2.0)
if (a == b) { ... }      // calls a.__eq(b)
```

### 6.5 Custom Containers with `__index` / `__newindex`
```hoo
class SparseMatrix {
    var data: HashMap<int64, HashMap<int64, float>>;

    constructor() {
        data = new HashMap<int64, HashMap<int64, float>>();
    }

    func __index(key: (int64, int64)) : float {
        var (row, col) = key;                         // destructuring
        var rowMap = data[row];                        // existing HashMap subscript
        if (rowMap == null) { return 0.0; }
        return rowMap[col];
    }

    func __newindex(key: (int64, int64), value: float) : void {
        var (row, col) = key;
        if (data[row] == null) {
            data[row] = new HashMap<int64, float>();
        }
        data[row][col] = value;
    }
}

var m = new SparseMatrix();
m[0, 0] = 1.0;         // calls __newindex((0, 0), 1.0)
var v = m[0, 0];        // calls __index((0, 0))
```

### 6.6 Callable Objects with `__call`
```hoo
class Adder {
    var base: int64;
    constructor(b: int64) { base = b; }
    func __call(x: int64) : int64 { return base + x; }
}

var add5 = new Adder(5);
var result = add5(10);   // 15
```

### 6.7 Lowering
Operator overloading is resolved at compile time (not runtime). If the left operand's type defines the relevant `__` method, the compiler emits:

```lir
// a + b where a : Vector3
// Desugars to: a.__add(b)
// Equivalent HVM:
  MOV r2, r_a           ; self = a
  MOV r3, r_b           ; other = b
  CALL Vector3___add    ; CALL to resolved method
  MOV r_c, r1           ; result
```

If no overload is found and the types are not primitives, the compiler emits a compile-time type error. There is no runtime method dispatch — the call target is known at compile time.

### 6.8 Compatibility
- HVM: Uses existing CALL opcode. Name mangling appends `__add`, `__sub`, etc. to the class name.
- Runtime: No new runtime support. Overloaded operators are ordinary methods.
- Polymorphism: Works with inheritance — if a parent class defines `__add`, the child inherits it via existing method dispatch.

---

## 7. Proposal 6: Rust-Style `match` Expressions with Pattern Matching

### 7.1 Motivation
Deeply nested `if/else if` chains are error-prone and verbose. A `match` expression compiles to efficient decision trees or jump tables and provides exhaustiveness checking. Hoo's type system (algebraic types, Option, Result, enums) is well-positioned to benefit from pattern matching.

### 7.2 Design
A `match` expression takes a value and evaluates it against a sequence of `case` arms. The first matching arm is evaluated. Patterns can destructure compound types, bind variables, match ranges, and include guards.

### 7.3 Grammar & Syntax
```hoo
// Basic integer matching
func :string describe(n: int64) {
    return match n {
        case 0       => "zero";
        case 1       => "one";
        case 2..9    => "small";
        case 10..99  => "medium";
        case _       => "large";
    };
}

// Matching Options
func :string greet(name: Option<string>) {
    return match name {
        case Some(n) => "Hello, " + n;
        case None    => "Hello, stranger";
    };
}

// Matching Results with guards
func :void handleResult(res: Result<int64, string>) {
    match res {
        case Ok(v) if v < 0  => Console.println("negative: " + v);
        case Ok(v)           => Console.println("value: " + v);
        case Err(msg)        => Console.println("error: " + msg);
    };
}

// Destructuring tuples
func :string describePoint(p: (int64, int64)) {
    return match p {
        case (0, 0)       => "origin";
        case (x, 0)       => "on x-axis at " + x;
        case (0, y)       => "on y-axis at " + y;
        case (x, y)       => "at (" + x + ", " + y + ")";
    };
}

// Type matching with any
func :void printAny(val: any) {
    match val {
        case as int64   => Console.println("int: " + val);
        case as string  => Console.println("str: " + val);
        case as Vector3 => Console.println("vec: " + val.to_string());
        case _          => Console.println("unknown type");
    };
}
```

### 7.4 Pattern Types
| Pattern | Example | Matches |
|---------|---------|---------|
| Literal | `case 42` | Exact value |
| Range | `case 1..10` | Inclusive range |
| Wildcard | `case _` | Anything |
| Variable | `case x` | Binds to variable |
| Destructure | `case (x, y)` | Tuple/struct destructuring |
| Enum variant | `case Some(v)` | Algebraic type variant |
| Guard | `case x if x > 0` | Pattern + condition |
| Type match | `case as string` | Runtime type check (any) |
| Or | `case 1 \| 2 \| 3` | Multiple alternatives |

### 7.5 Exhaustiveness Checking
The compiler verifies that all cases are covered. Missing arms produce a compile-time warning or error:

```hoo
func :string describeOpt(x: Option<int64>) {
    return match x {
        case Some(v) => "got " + v;
        // Missing: case None => ...
    };
}
// Compile error: match must be exhaustive. Missing arm: None
// Help: add `case None => ...` or `case _ => ...`
```

### 7.6 match as Expression
`match` is an expression and can appear anywhere a value is expected:

```hoo
var description = match x {
    case 0..9  => "digit";
    case _     => "non-digit";
};

var result = match opt {
    case Some(v) => v * 2;
    case None    => 0;
};
```

When used as an expression, all arms must return the same type.

### 7.7 Lowering
```lir
// match x { case 0 => "a"; case 1 => "b"; case _ => "c"; }
// Lowers to comparison tree or jump table:
  CMPI r_x, 0
  BNE .case1
  LDA r_result, .str_a
  JMP .done
.case1:
  CMPI r_x, 1
  BNE .default
  LDA r_result, .str_b
  JMP .done
.default:
  LDA r_result, .str_c
.done:
```

For dense integer ranges with 4+ cases, the compiler may emit a jump table:

```lir
  BGEI r_x, 3, .default     ; bounds check
  LDA r_tmp, .jumptable     ; jump table base
  SHL r_idx, r_x, 3         ; index * 8
  ADD r_tmp, r_tmp, r_idx
  LDE r_target, r_tmp, 0    ; load jump target
  JMPR r_target             ; indirect jump
```

### 7.8 Compatibility
- HVM: Uses only CMPI, BNE, BGEI, SHL, ADD, LDE, JMPR — all existing opcodes.
- Runtime: No new runtime support. Patterns are compiled to conditional branches.
- Jump tables: Use existing `JMPR` (indirect jump) with a `.data` section table.

---

## 8. Implementation Phases

### Phase 1: `Result<T, E>` with `?` Operator
1. Add `Result<T, E>` representation as compiler intrinsic (discriminator + payload).
2. Implement `?` desugaring in AST/codegen: branch to early return on Err.
3. Add standard combinators (`map`, `andThen`, `unwrapOr`, etc.) as compiler intrinsics or runtime helpers.
4. Deprecate optional try/catch paths in favor of `Result` for library APIs.

### Phase 2: Multiple Return Values
1. Relax parser to accept `: (T, U, ...)` return syntax.
2. Update `SimpleASTBuilder` to create multi-return function types.
3. Update `HVMCodeGenerator` to store consecutive return values in r1–r7 and emit destructuring binds.
4. Update existing runtime functions (e.g., `DateTime.components()`) to expose multi-return signatures.

### Phase 3: Regex Literals
1. Add `/pattern/flags` lexer token in `Hooc.g4`.
2. At codegen time, compile the regex to a serialized struct using existing regex runtime.
3. Embed the compiled regex as a constant in the `.data` section.
4. Expose `.matches()`, `.match()`, `.replace()`, `.split()` on the literal's result type.

### Phase 4: `Option<T>` and `type?` Desugaring
1. Create `Option<T>` representation (same layout as `Result<T,E>`).
2. Implement `Option.none()`, `Option.some()`, and all combinators.
3. Desugar `type?` to `Option<T>` in the parser/AST layer.
4. Add `?` to Option propagation (it already works from Phase 1 if Option uses the same discriminator convention).

### Phase 5: Operator Overloading via Metatables
1. Define the set of recognized `__` method names in the compiler.
2. In codegen, when a binary operator is encountered, check if the left operand's type defines the corresponding `__` method.
3. If an overload exists, emit a CALL instead of the default ARITH instruction.
4. If no overload and types are non-primitive, emit compile error with suggestion to define `__method`.

### Phase 6: `match` Expressions with Pattern Matching
1. Add `match` / `case` keywords and grammar to `Hooc.g4`.
2. Implement pattern types: literal, range, wildcard, variable binding, destructuring, guard.
3. Add exhaustiveness checker in the type system.
4. Codegen: emit comparison trees for small patterns, jump tables for dense integer patterns.
5. Add `case as Type` pattern for `any` runtime type dispatch.

---

## 9. Status
- **Date**: 2026-06-19
- **Status**: **PROPOSED (UNIMPLEMENTED)**
- **Priority**: **MEDIUM** (Ergonomics enhancement — no correctness bugs but significant developer experience improvement)

### Recommended Ordering
1. **Phase 1 (Result + ?)**: Highest ROI. Zero-cost error handling with minimal compiler changes.
2. **Phase 2 (Multi-return)**: Nearly free on RISC. Register-based returns cost nothing.
3. **Phase 3 (Regex literals)**: Pure ergonomics. No runtime cost, only compile-time regex compilation.
4. **Phase 4 (Option combinators)**: Builds on Phase 1 infrastructure. Natural evolution of nullable types.
5. **Phase 5 (Operator overloading)**: Enables expressive math types (Vector3, Matrix, Complex).
6. **Phase 6 (match)**: Ties everything together — makes Result, Option, and multi-return truly ergonomic.
