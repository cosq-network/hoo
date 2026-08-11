# ISSUE-028: Factory Constructors in the Hoo Language

**Status**: COMPLETED / RESOLVED

## 1. Overview

This issue defines and implements Dart-style **factory constructors** for the
Hoo language, extending the language across the grammar, parsing, AST, code
generation, and execution layers. All components have been implemented and verified with unit tests.

Today Hoo permits at most one generative `constructor` per class
(`SimpleASTBuilder::buildClassBody`, src/ast/SimpleASTBuilder.cpp:1176-1190),
and the documented workaround for alternative construction paths is module-level
free functions (docs/issues/ISSUE-026_native_ann_support.md:702-707). Factory
constructors remove that restriction by letting a class declare named
construction paths that may perform argument validation/preprocessing and
return either a newly created instance or an existing (cached) instance, in the
same spirit as Dart's `factory` constructors
(https://dart.dev/language/constructors#factory-constructors).

A factory constructor:

- may return an existing instance from a cache or a new instance of the class;
- may perform non-trivial work (argument validation, preprocessing) before an
  instance exists;
- **cannot access `this`** (no instance state exists yet) — it must obtain an
  instance through the generative `constructor` (e.g. `return new Point(...)`)
  or an existing reference.

### 1.1 Example

```hoo
class Point {
    var x: f64;
    var y: f64;

    constructor(x: f64, y: f64) {
        this.x = x;
        this.y = y;
    }

    // Factory: named alternative construction path.
    factory origin() {
        return new Point(0.0, 0.0);
    }

    // Factory: validation + delegation to the generative constructor.
    factory unitCircle(angle: f64) {
        if (angle < 0.0 || angle > 360.0) {
            throw new Exception("invalid angle");
        }
        return new Point(cos(angle), sin(angle));
    }
}

func :int64 main() {
    var p = new Point.origin();      // factory call
    return p.x + p.y;                // 0
}
```

## 2. Design

### 2.1 Syntax

- Keyword: `factory` (new lexer token).
- Declaration, as a class member:

```
factory <name>(<params>) { <body> }
```

- Invocation, a new form of `new` expression:

```
new <ClassName>.<factoryName>(<args>)
```

- A class may declare **zero or more** factories plus at most **one**
  generative `constructor`.
- Factory names must be unique within a class.
- Inside a factory body, `this` is not bound and is an error to reference;
  immutable field writes are disallowed (matching Dart, where factories cannot
  access `this`). The body must end in a `return` of an instance of the class.
- The returned object follows the existing ARC model: `return expr` performs a
  RETAIN and transfers a refcount-1 reference to the caller
  (src/codegen/HVMCodeGenerator.cpp:1817-1825), identical to class-typed method
  returns.

### 2.2 Grammar changes (src/parsing/Hooc.g4)

1. Lexer token:

```antlr
FACTORY: 'factory';
```

2. New class-member rule and rule addition (lines 187-192):

```antlr
classMember
    : functionModifier* (variableDeclaration SEMICOLON | functionDeclaration)
    | constructorDeclaration
    | factoryConstructorDeclaration
    ;

constructorDeclaration: CONSTRUCTOR LPAREN parameterList? RPAREN block;
factoryConstructorDeclaration: FACTORY IDENTIFIER LPAREN parameterList? RPAREN block;
```

3. `newExpression` gets a factory alternative **first** (ordered alternation
   wins in ANTLR4) so dotted names are parsed as `Class.factory`:

```antlr
newExpression
    : NEW qualifiedIdentifier DOT IDENTIFIER LPAREN argumentList? RPAREN   # newFactoryExpression
    | NEW (hashMapType | anyArrayType | qualifiedIdentifier) LPAREN argumentList? RPAREN  # newClassExpression
    ;
```

For `new a.b.C(x)`, alternative 1 parses `qualifiedIdentifier="a.b"` +
`factory="C"`. The AST/codegen resolve this semantically: if `a.b` is a
user-defined class with factory `C` → factory call; otherwise the name falls
back to the existing module-qualified class path (`mymodule.Point`,
`hoo.String`), preserving backward compatibility (see §2.5).

4. Regenerate the parser via the existing `generate_parser` target
   (CMakeLists.txt:226-240, `tools/antlr-4.13.2-complete.jar`).

### 2.3 AST changes

`src/ast/ClassDeclaration.h`:

- `ConstructorDeclaration` gains `std::string name_` and `bool isFactory_`.
  The existing 2-argument constructor remains the generative form; a new
  constructor takes `(name, parameters, body)` and sets `isFactory_ = true`.
  New accessors: `getName()`, `isFactory()`. `toString()` reflects the form.
- `ClassMember` is unchanged — a factory is stored as a
  `ConstructorDeclaration` with `isFactory() == true`, so `isConstructor()`
  continues to identify both forms.

`src/ast/Expression.h`:

- `NewObjectExpression` gains `std::string factoryName_` (empty = generative)
  and a constructor overload that accepts a factory name. New accessors:
  `getFactoryName()`, `isFactoryConstruction()`.

`src/ast/FunctionModifier.h`: unchanged (factories are not `public`/`private`/
`async` functions; they are a distinct member kind).

`src/ast/ASTImpl.cpp`: update `toString()` for both nodes.

### 2.4 Parsing changes (src/ast/SimpleASTBuilder.{h,cpp})

- New `buildFactoryConstructorDeclaration(...)` that produces a factory
  `ConstructorDeclaration`.
- `buildClassMember` (src/ast/SimpleASTBuilder.cpp:1232): handle
  `ctx->factoryConstructorDeclaration()`.
- `buildClassBody` (src/ast/SimpleASTBuilder.cpp:1176): the single-generative-
  constructor check counts only `!ctor->isFactory()` members; add a duplicate
  factory-name check.
- `buildNewExpression` (src/ast/SimpleASTBuilder.cpp:1040): when the parse
  context is the `newFactoryExpression` alternative, build a
  `NewObjectExpression` whose `className` is the dotted prefix
  (`QualifiedIdentifier` of all but the last component) and whose `factoryName`
  is the final `IDENTIFIER`.

### 2.5 Code generation (src/codegen/HVMCodeGenerator.cpp)

Symbol mangling (`src/core/SymbolMangler.{h,cpp}`):

- `MangledFunctionParams` (both structs) gains `bool isFactoryConstructor`.
- `mangleFunctionName` (src/core/SymbolMangler.cpp:217) emits a new `FC_` tag
  followed by the factory name, e.g. `_F_Point_FC_origin_p`.
- `demangleSymbol` (src/core/SymbolMangler.cpp:397-429): treat `FC` as a
  special token; on `FC`, set `isFactoryConstructor = true`, consume the
  following component as the factory name, and classify the symbol as a class
  member (className = preceding name).

Class layout (`HVMCodeGenerator.h:111`):

- `ClassLayout` gains `std::vector<std::string> factoryNames`. Populated during
  the member indexing pass (src/codegen/HVMCodeGenerator.cpp:1077) from members
  with `member->getConstructor()->isFactory()`.

`beginFunction` (src/codegen/HVMCodeGenerator.cpp:1276):

- Add `bool isFactory` derived from `ctorDecl->isFactory()`.
- Factories compile with `isMethod == false` (no `this` slot) and
  `isConstructor == false`, so arguments start at r1 and up to 7 are available.
- Set `mp.isFactoryConstructor = true`, `mp.functionName = factory name`,
  `mp.returnType = "ptr"`, and mangle via `mangleFunctionName`.
- Enforce that the factory body returns (mirroring the non-void function check
  at line 1337): a fallthrough factory is a compile error.

`visitConstructor` (src/codegen/HVMCodeGenerator.cpp:1520):

- `inConstructor_` is set **only** for generative constructors, so immutable
  field writes remain impossible inside factories and `this` is unbound.

`new` lowering (src/codegen/HVMCodeGenerator.cpp:2842-2995):

- When `factoryName` is set:
  - if `classes_` contains the prefix class and `factoryNames` contains the
    factory → emit only `CALL _F_<Class>_FC_<name>_p_...` with arguments in
    r1..; the returned object pointer (r1) is the expression value. **No
    `hoo_alloc`, no generative-constructor call.**
  - if the prefix class exists but the factory does not → compile error
    `Class 'X' has no factory constructor named 'Y'`.
  - otherwise (the prefix is a module, e.g. `new hoo.String(...)`,
    `new mymodule.Point(...)`) → fall back to the existing behavior by using
    the **last** dotted component as the class name. This preserves every
    existing `new` form.

Validation:

- Singleton classes: reject factories (a singleton pre-allocates its instance
  via module init; factories would contradict the invariant). See
  src/codegen/HVMCodeGenerator.cpp:1132-1157.
- Service classes: factories remain rejected (services cannot be constructed
  with `new` at all; line 2932).
- `validateSerializableClass` (src/codegen/HVMCodeGenerator.cpp:1549): the
  "exactly one constructor" check counts only generative constructors, so
  factories do not disturb serialization codegen.

### 2.6 Execution layer (JIT)

No new runtime ABI is required. A factory compiles to an ordinary HVM function
that returns a class pointer; the `CALL` target is resolved through the module's
own symbol table exactly like a user-defined method. The existing return-ARC
machinery (RETAIN on return, scope cleanup of locals) transfers ownership to the
caller. No `buildRuntimeSymbols()`/`lookupPlainRuntimeSymbolAddress` entries are
added (those tables serve `hoo.*` stdlib functions only).

### 2.7 Runtime library (src/runtime/lib/)

Analysis shows **no functional changes** are required in the C-ABI runtime
library. Factory constructors are a compile-time dispatch feature: the factory
is lowered to a plain function, and the generative constructor it delegates to
already runs through `hoo_alloc` + the constructor call. No new `hoo_*`
symbols, type IDs, or ownership rules are introduced.

The runtime documentation under docs/runtime/ is updated instead to describe
factory constructors (see §5), and the stdlib code-generation conventions in
docs/runtime/README.md and docs/runtime/new-module-guide.md remain unchanged.

## 3. Test Plan

| Layer | File | Cases |
| --- | --- | --- |
| Parsing | tests/parsing/ClassDeclarationParsingTest.cpp | factory declaration parses; factory + generative ctor coexist; duplicate factory name rejected; multiple generative ctors still rejected |
| Parsing | tests/parsing/NewExpressionParsingTest.cpp | `new Point.origin()` factory form; qualified factory `new a.b.C(x)` |
| AST | tests/ast/ASTCoreTest.cpp | `ConstructorDeclaration` isFactory/name accessors; `NewObjectExpression` factoryName |
| Mangling | tests/core/SymbolManglerTest.cpp | golden `_F_Point_FC_origin_p`; mangle→demangle round-trip; `isFactoryConstructor` flags |
| Codegen | tests/codegen/HVMCodeGeneratorComprehensiveTest.cpp | factory emits `CALL` to `_F_..._FC_...` and **no** `hoo_alloc`; generative `new` still emits alloc+ctor; factory-without-return error; missing-factory error; serializable/singleton validation still holds |
| JIT/execution | tests/jit/HooClassApiTest.cpp | end-to-end: `new Point.origin()` runs and returns correct value; factory delegating to generative ctor; factory returning a cached instance; `this` usage in factory rejected |

Run: `cmake --build build --target hoo-tests && ctest --test-dir build`.

## 4. Migration Steps

1. Update `src/parsing/Hooc.g4` (§2.2) and regenerate the parser
   (`cmake --build build --target generate_parser`).
2. Extend AST nodes (§2.3) and their `toString` (§`ASTImpl.cpp`).
3. Extend `SimpleASTBuilder` (§2.4).
4. Extend `SymbolMangler` (§2.5 mangling).
5. Extend codegen: layout factory names, `beginFunction`, `visitConstructor`,
   `new` lowering, validations (§2.5).
6. Update runtime docs (§5). No `src/runtime/lib/` functional changes.
7. Add tests (§3) and run the full suite.

## 5. Documentation Updates

- docs/runtime/README.md — document factory constructor syntax, semantics
  (no `this`, must return, named call form), and the "one generative
  constructor + N factories" rule in the language conventions section.
- docs/runtime/name-mangling.md — add the `FC_` factory tag to the symbol
  layout and examples.
- docs/runtime/new-module-guide.md — note that stdlib modules may expose
  alternative construction paths via Hoo factories in addition to free
  functions.
- docs/issues/ISSUE-026_native_ann_support.md:702-707 — revise the
  "no general static method / factories remain free functions" note to reflect
  that named factories are now first-class.

## 6. Acceptance Criteria & Resolution Summary

- A class can declare one generative `constructor` plus any number of named
  `factory` declarations.
- `new Class.factoryName(args)` compiles and executes end-to-end (JIT), with
  no `hoo_alloc` emitted for the factory call.
- Factories cannot access `this`, cannot write immutable fields, and must
  return an instance; violations are compile-time errors.
- Existing `new` forms (`new Point(x)`, `new hoo.String(x)`,
  `new mymodule.Point(x)`, `new AnyArray()`) are unchanged.
- All existing tests continue to pass; new parsing, AST, mangler, codegen, and
  JIT tests pass.

### Resolution Summary
- **Grammar & Lexer**: Added `FACTORY` token and updated grammar rules in `Hooc.g4`.
- **AST Builder**: Added `buildFactoryConstructorDeclaration` and updated `SimpleASTBuilder.cpp`.
- **Code Generation & JIT**: Added `FC_` symbol mangling tag and verification across AST, Codegen, and JIT layers.
- **Verification**: Verified via test suite (`ClassDeclarationParsingTest`, `HVMCodeGeneratorTest`, `HooClassApiTest`, `HooBufferJitTest`).
