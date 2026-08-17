# ISSUE-029: Constructor Overloading, Delegation, and Base Class Constructor Calls

**Status**: PROPOSED

## 1. Overview

This issue adds three related features to the Hoo language's constructor system:

1. **Constructor overloading** — a class may declare multiple generative
   constructors distinguished by parameter types, just as regular methods
   may be overloaded today.
2. **Delegating constructors** — a constructor may call another constructor
   of the same class via `this(args)`, forwarding initialization.
3. **Base class constructor calls** — a derived-class constructor may call
   a base class constructor via `super(args)`, initializing inherited fields.

Today Hoo permits at most **one** generative `constructor` per class
(`SimpleASTBuilder::buildClassBody`, src/ast/SimpleASTBuilder.cpp:1248-1249),
and there is no syntax for calling another constructor or a base class
constructor. The `extends` keyword copies the base class field layout
(src/codegen/HVMCodeGenerator.cpp:1063-1078) but never invokes the base
constructor, leaving inherited fields uninitialized.

### 1.1 Example

```hoo
class Animal {
    var name: string;

    constructor(name: string) {
        this.name = name;
    }

    constructor() {
        this("unknown");       // delegate to the named constructor
    }
}

class Dog extends Animal {
    var breed: string;

    constructor(name: string, breed: string) {
        super(name);           // call Animal(name)
        this.breed = breed;
    }

    constructor(name: string) {
        super(name);           // call Animal(name)
        this.breed = "mixed";
    }

    constructor() {
        super();               // call Animal()
        this.breed = "mixed";
    }
}

func :int64 main() {
    var d = new Dog("Rex", "Labrador");
    return 0;
}
```

## 2. Design

### 2.1 Syntax changes

#### 2.1.1 Constructor overloading (no grammar change needed)

The existing `constructorDeclaration` rule already permits any parameter
list. The restriction is in the AST builder, not the grammar. Removing
the single-constructor check is a parser-level change only.

```
constructorDeclaration: CONSTRUCTOR LPAREN parameterList? RPAREN block;
```

Multiple `constructor(...)` declarations with different parameter lists
become legal.

#### 2.1.2 Delegating constructor calls (`this`)

A new statement form inside a constructor body:

```
thisStatement: THIS LPAREN argumentList? RPAREN SEMICOLON;
```

Added to the existing `statement` rule. Grammar change in `Hooc.g4`:

```antlr
statement
    : ...
    | thisStatement
    ;
```

This is **not** an expression — it is a statement, following Java/C#
convention. `this(args)` cannot appear on the right side of an assignment
or inside an expression.

#### 2.1.3 Base class constructor calls (`super`)

A new keyword `super` and a new statement form:

```antlr
SUPER: 'super';

superStatement: SUPER LPAREN argumentList? RPAREN SEMICOLON;
```

Added to the `statement` rule alongside `thisStatement`:

```antlr
statement
    : ...
    | thisStatement
    | superStatement
    ;
```

#### 2.1.4 Updated `new` expression (no grammar change)

The existing `newClassNameExpression` rule already handles `new Class(args)`.
With overloading, the codegen resolves the correct constructor at compile
time by argument type inference (see §2.5).

### 2.2 AST changes

#### 2.2.1 `ConstructorDeclaration` (src/ast/ClassDeclaration.h)

No structural changes to the node itself. The existing `ConstructorDeclaration`
already supports parameters and a body. Multiple `ConstructorDeclaration`
instances per class are now allowed.

A helper method is added:

```cpp
class ConstructorDeclaration : public ASTNode {
    // ... existing fields ...
    void setParameterTypeIds(const std::vector<uint32_t>& ids);
    const std::vector<uint32_t>& getParameterTypeIds() const;
private:
    std::vector<uint32_t> parameterTypeIds_;  // populated during codegen first pass
};
```

These type IDs are populated during codegen's first pass (when all type
declarations are known) and used for overloaded constructor resolution.

#### 2.2.2 New AST nodes (src/ast/Expression.h or src/ast/Statement.h)

```cpp
class ThisConstructorCall : public Statement {
public:
    ThisConstructorCall(std::unique_ptr<ArgumentList> args);
    const ArgumentList& getArguments() const;
private:
    std::unique_ptr<ArgumentList> args_;
};

class SuperConstructorCall : public Statement {
public:
    SuperConstructorCall(std::unique_ptr<ArgumentList> args);
    const ArgumentList& getArguments() const;
private:
    std::unique_ptr<ArgumentList> args_;
};
```

### 2.3 Parser changes (src/ast/SimpleASTBuilder.cpp)

#### 2.3.1 Remove single-constructor restriction

In `buildClassBody` (line 1248-1249), remove:

```cpp
constructorCount++;
if (constructorCount > 1) {
    throw std::runtime_error("Class cannot have multiple constructors.");
}
```

Replace with overload detection — when more than one generative
constructor exists, they form an overload group:

```cpp
if (member->isConstructor() && !ctor->isFactory()) {
    generativeCtors.push_back(ctor);
}
```

After processing all members, if `generativeCtors.size() > 1`, mark
each as part of an overload group. Store the overload group on the
`ClassBody` or on each `ConstructorDeclaration`.

#### 2.3.2 Build `this` / `super` statements

New methods:

```cpp
std::unique_ptr<ASTNode> SimpleASTBuilder::buildThisStatement(
    HoocParser::ThisStatementContext* ctx);

std::unique_ptr<ASTNode> SimpleASTBuilder::buildSuperStatement(
    HoocParser::SuperStatementContext* ctx);
```

Routed from `buildStatement` when the corresponding context is present.

#### 2.3.3 Validation

During AST building, enforce:

1. **`this(args)` and `super(args)` are mutually exclusive** within a single
   constructor — a constructor cannot delegate to a sibling and also call
   the base constructor.
2. **`this(args)` may only appear in a generative constructor body**, not
   in a factory constructor body.
3. **`super(args)` may only appear in a derived class constructor body** —
   a class with no `extends` that uses `super` is a compile error.
4. **`super(args)` is required in derived classes** if the base class
   constructor has parameters and no zero-arg overload exists. (This may
   be relaxed to a runtime check in a future iteration.)

### 2.4 Symbol mangling changes (src/core/SymbolMangler.{h,cpp})

#### 2.4.1 Overloaded constructor mangling

When multiple generative constructors exist for a class, they use the
**overload mangling format** instead of the standard `CT_` format:

| Scenario | Mangled Symbol |
|---|---|
| `Point(x: f64, y: f64)` — sole constructor | `_F_Point_CT_ptr_ptr` |
| `Point(x: f64, y: f64)` — overloaded | `_F_Point_CT__f64_f64` |
| `Point(x: string)` — overloaded | `_F_Point_CT__s` |
| `Person()` — overloaded | `_F_Point_CT__` |

The overload format embeds the parameter type IDs in the mangled name,
using double-underscore `__` to separate the class name from the type
list, identical to the existing function overload convention
(src/core/SymbolMangler.cpp:174-187).

The `MangledFunctionParams` struct already has `isOverload`. The
`isConstructor` flag is replaced by (or combined with) the overload
format when `isOverload == true && isConstructor == true`.

New mangling logic:

```cpp
if (params.isConstructor && params.isOverload) {
    oss << "CT__";
    for (const auto& param : params.parameterTypes) {
        oss << encodeComponent(param);
    }
} else if (params.isConstructor) {
    oss << "CT_";
}
```

### 2.5 Code generation changes (src/codegen/HVMCodeGenerator.cpp)

#### 2.5.1 First pass: collect constructor overloads

During the class member indexing pass (~line 1077), when multiple
generative constructors are found:

```cpp
std::vector<const ConstructorDeclaration*> generativeCtors;
// ... collect them ...

if (generativeCtors.size() > 1) {
    for (auto* ctor : generativeCtors) {
        isOverloadedConstructor_[className] = true;
        // Record parameter type IDs for each overload
    }
}
```

New member on `HVMCodeGenerator`:

```cpp
std::unordered_map<std::string, bool> isOverloadedConstructor_;
std::unordered_map<std::string,
    std::vector<OverloadReturnInfo>> constructorOverloads_;
```

#### 2.5.2 Constructor prologue (`visitConstructor` / `beginFunction`)

When a constructor is overloaded, `beginFunction` sets:

```cpp
if (isOverloadedConstructor_[className]) {
    mp.isOverload = true;
    // Parameter types inferred from declared types, not "ptr"
    for (const auto& param : ctorDecl->getParameters()) {
        mp.parameterTypes.push_back(
            mangleTypeId(typeIdFromDeclaredType(&param->getType()),
                         isNullableDeclaredType(&param->getType())));
    }
}
```

When a constructor is not overloaded, the existing `CT_` format with
`"ptr"` parameters is retained for backward compatibility.

#### 2.5.3 `new` expression: overloaded constructor resolution

In the `visitNewExpression` general path (~line 3125), when the target
class has overloaded constructors:

```cpp
if (isOverloadedConstructor_[className]) {
    // 1. Infer argument types
    std::vector<uint32_t> argTypeIds;
    for (const auto* arg : newExpr->getArguments()->getArguments()) {
        auto typeInfo = inferExpressionTypeInfo(*arg);
        argTypeIds.push_back(typeInfo.typeId);
    }

    // 2. Resolve best overload (same scoring as methods)
    const OverloadReturnInfo* best = selectOverload(constructorOverloads_[className], argTypeIds);

    // 3. Mangle with actual types
    MangledFunctionParams mp;
    mp.modulePath = modulePath_;
    mp.className = className;
    mp.isOverload = true;
    mp.isConstructor = true;
    for (auto tid : argTypeIds) {
        mp.parameterTypes.push_back(mangleTypeId(tid, false));
    }
    std::string ctorName = SymbolMangler::mangleFunctionName(mp);

    // 4. Allocate + call
    // ... existing hoo_alloc + stack spill ...

    // 5. Use CALL_OVERLOADED
    emitCall(Opcode::CALL_OVERLOADED, ctorName);
}
```

When the class has a single constructor, the existing code path is
unchanged.

#### 2.5.4 `this(args)` codegen

When visiting a `ThisConstructorCall` inside a constructor body:

1. `this` is already allocated (the `new` expression called `hoo_alloc`
   before entering the constructor).
2. `this` is already in the local variable slot (set up by `beginFunction`
   prologue).
3. The delegation is lowered to a CALL to the target constructor with
   `this` in r1 and the forwarded arguments.

```cpp
void HVMCodeGenerator::visitThisConstructorCall(const ast::ThisConstructorCall& stmt) {
    // Resolve target constructor by argument types
    MangledFunctionParams mp;
    mp.modulePath = modulePath_;
    mp.className = currentClass_->name;
    mp.isConstructor = true;
    // ... infer arg types and resolve overload ...

    // Load 'this' into r1
    emit(Opcode::LD_D, OperandsI{1, 30, thisOffset_});

    // Evaluate arguments into r2..r8
    // ...

    emitCall(Opcode::CALL_OVERLOADED, mangledName);
}
```

#### 2.5.5 `super(args)` codegen

When visiting a `SuperConstructorCall` inside a derived class constructor:

```cpp
void HVMCodeGenerator::visitSuperConstructorCall(const ast::SuperConstructorCall& stmt) {
    std::string baseClassName = currentClass_->baseClass;

    // Resolve base class constructor by argument types
    MangledFunctionParams mp;
    mp.modulePath = modulePath_;
    mp.className = baseClassName;
    mp.isConstructor = true;
    // ... infer arg types and resolve overload ...

    // Load 'this' into r1 (same pointer — derived object contains base fields)
    emit(Opcode::LD_D, OperandsI{1, 30, thisOffset_});

    // Evaluate arguments into r2..r8
    // ...

    emitCall(Opcode::CALL_OVERLOADED, mangledName);
}
```

After `super()` returns, `this` is fully initialized with base class
fields and the derived constructor continues to set derived fields.

### 2.6 Runtime library changes

**No changes required.** Constructor overloading, delegation, and
super calls are all resolved at compile time to ordinary `CALL` or
`CALL_OVERLOADED` instructions. The runtime ABI is unchanged.

### 2.7 Validation rules

| Rule | Enforced at | Error message |
|---|---|---|
| Duplicate constructor signatures | Parser (`buildClassBody`) | `"Class 'X' already has a constructor with these parameter types."` |
| `this()` in factory constructor | Parser | `"Cannot use 'this()' in a factory constructor."` |
| `this()` and `super()` in same constructor | Parser | `"Cannot use both 'this()' and 'super()' in the same constructor."` |
| `super()` in non-derived class | Parser / Codegen | `"Class 'X' has no base class; 'super()' is not allowed."` |
| `super()` in singleton class | Codegen | `"Singleton class cannot call super constructor."` |
| Serializable class with overloads | Codegen | `"Serializable class must have exactly one zero-parameter constructor."` |
| Singleton class with overloads | Codegen | `"Singleton class must have exactly one zero-parameter constructor."` |
| Missing `super()` in derived class with no zero-arg base constructor | Codegen (warning) | `"Base class 'X' has no zero-arg constructor; derived constructors should call 'super(args)'."` |

### 2.8 Interaction with existing features

- **Factory constructors**: Unchanged. A class may have overloaded
  generative constructors plus any number of named factories.
- **Serializable classes**: Must still have exactly one zero-parameter
  generative constructor. Overloads are rejected for serializable classes.
- **Singleton classes**: Must still have exactly one zero-parameter
  constructor. Overloads and `super()` are rejected.
- **Service classes**: Constructor parameter restrictions (no primitives)
  apply per-constructor. Multiple constructors with service-class
  parameters are allowed.
- **Immutable fields**: Can only be written in a constructor body.
  After `super()` or `this()` delegation, immutable field writes
  remain legal until the constructor returns.
- **Overloaded methods**: The existing method overload system is
  unaffected. Constructor overloads use the same overload resolution
  scoring but are a distinct overload category.

## 3. Test Plan

| Layer | File | Cases |
| --- | --- | --- |
| Grammar | tests/parsing/HoocGrammarTest.cpp | `this()` statement parses; `super()` statement parses |
| Parsing | tests/parsing/ClassDeclarationParsingTest.cpp | Multiple constructors parse; duplicate signature rejected; `this()` only in constructor; `super()` only in derived class; `this()` + `super()` mutual exclusion; overloads + factories coexist; serializable rejection of overloads; singleton rejection of overloads |
| AST | tests/ast/ASTCoreTest.cpp | `ThisConstructorCall` and `SuperConstructorCall` node construction and accessors |
| Mangling | tests/core/SymbolManglerTest.cpp | Overloaded constructor mangling `_F_Point_CT__f64_f64`; round-trip mangle/demangle; `isOverload` + `isConstructor` flags |
| Codegen | tests/codegen/HVMCodeGeneratorComprehensiveTest.cpp | Overloaded constructor emits `CALL_OVERLOADED` with correct mangled name; single constructor unchanged; `this()` delegation emits CALL to sibling; `super()` emits CALL to base; serializable/singleton validation still holds |
| JIT/execution | tests/jit/HooClassApiTest.cpp | End-to-end: overloaded constructors resolve correctly; `this()` delegation works; `super()` initializes base fields; chained delegation; missing super when required |

Run: `cmake --build build/ninja-relwithdebinfo --target hoo-tests && ./build/ninja-relwithdebinfo/hoo-tests`.

## 4. Migration Steps

1. **Grammar**: Add `SUPER` token, `thisStatement` rule, `superStatement`
   rule, and extend `statement` in `Hooc.g4`. Regenerate parser.
2. **AST**: Add `ThisConstructorCall` and `SuperConstructorCall` nodes.
   Add `parameterTypeIds` to `ConstructorDeclaration`.
3. **Parser**: Remove single-constructor restriction in `buildClassBody`.
   Add `buildThisStatement` and `buildSuperStatement` methods.
   Add validation rules (§2.7).
4. **Symbol mangling**: Update `mangleFunctionName` for overloaded
   constructors with `CT__` format. Update `demangleSymbol` to
   handle the new format.
5. **Codegen first pass**: Collect constructor overloads per class.
   Record parameter type IDs.
6. **Codegen `beginFunction`**: Set `isOverload` for overloaded
   constructors and emit type-specific mangled names.
7. **Codegen `new` expression**: Add overloaded constructor resolution
   path using `inferExpressionTypeInfo` and `selectOverload`.
8. **Codegen `this`/`super`**: Implement `visitThisConstructorCall`
   and `visitSuperConstructorCall`.
9. **Update validation**: Add singleton, serializable, and service
   class validation for overloaded constructors.
10. **Tests**: Add parsing, AST, mangling, codegen, and JIT tests.
11. **Documentation**: Update `docs/language-guide.md` with constructor
    overloading, delegation, and super call syntax.

## 5. Documentation Updates

- `docs/language-guide.md` — Add "Constructors" section covering
  overloading, `this()` delegation, and `super()` base calls with
  examples.
- `docs/runtime/name-mangling.md` — Document the `CT__` overloaded
  constructor mangling format.
- `docs/issues/ISSUE-028_factory_constructors.md` — Note that factory
  constructors coexist with overloaded generative constructors.

## 6. Acceptance Criteria

- A class can declare multiple constructors with different parameter
  types. `new Class(args)` resolves the correct constructor at compile
  time by argument type inference.
- A constructor can call another constructor of the same class via
  `this(args)`. The delegation is lowered to a `CALL` with the already-
  allocated `this` pointer.
- A derived class constructor can call a base class constructor via
  `super(args)`. The call initializes inherited fields on the same
  `this` pointer.
- `this(args)` and `super(args)` are mutually exclusive in a single
  constructor.
- Existing features (factory constructors, single-constructor classes,
  serializable/singleton/service validation) remain unaffected.
- All existing tests pass; new tests cover every new code path.
