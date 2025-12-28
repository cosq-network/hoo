# Runtime Class Injection Framework

This document explains how runtime modules, classes, functions, and variables from **hoort** (the Hoo runtime library) are injected into **HoocJIT** and **LLVMCodeGenerator** using the X-Macro pattern.

## Overview

The **Runtime Class Injection Framework** is a compile-time metaprogramming system that enables:
- Single source of truth for runtime class metadata
- Zero-cost abstraction (all code generation at compile time)
- Automatic JIT registration of runtime functions
- Automatic LLVM function declarations
- Extensible operator dispatch for runtime types
- Minimal boilerplate when adding new runtime classes

### Key Components

| Component | Purpose |
|-----------|---------|
| `src/runtime/RuntimeClassRegistry.h` | Central registry (X-Macro) defining all runtime classes |
| `src/runtime/RuntimeClassCodeGen.h` | Code generation patterns and documentation |
| `src/HoocJIT.cpp` | JIT symbol registration |
| `src/LLVMCodeGenerator.cpp` | LLVM function declarations and operator dispatch |
| `src/rt/` | C/C++ implementation of runtime classes (hoo_string, hoo_array, etc.) |

---

## The X-Macro Pattern

The X-Macro (eXtensible Macro) pattern allows the same metadata to be **re-included multiple times** with different macro definitions, generating different code for each use case.

### How It Works

1. **Define the data** in a header without include guards:
   ```cpp
   // RuntimeClassRegistry.h (NO #pragma once or #ifndef!)
   #define RUNTIME_CLASSES \
       DEFINE_RUNTIME_CLASS(String, HooString, isPointerTy) \
           BEGIN_RUNTIME_FUNCTIONS \
               RUNTIME_FUNCTION(from_cstr, HooString, LLVM_PTR, ...) \
               RUNTIME_FUNCTION(concat, HooString, LLVM_PTR, ...) \
           END_RUNTIME_FUNCTIONS \
           BEGIN_RUNTIME_OPERATORS \
               RUNTIME_OPERATOR(PLUS, concat) \
           END_RUNTIME_OPERATORS
   ```

2. **Use it multiple times with different macro definitions:**
   ```cpp
   // In HoocJIT.cpp - generates JIT registration
   #define DEFINE_RUNTIME_CLASS(Name, ...) \
       void HoocJIT::register##Name##Functions() { ... }
   #define RUNTIME_FUNCTION(FuncName, ...) \
       // Register with JIT
   #include "runtime/RuntimeClassRegistry.h"
   #undef DEFINE_RUNTIME_CLASS
   #undef RUNTIME_FUNCTION

   // In LLVMCodeGenerator.cpp - generates LLVM declarations
   #define DEFINE_RUNTIME_CLASS(Name, ...) \
       void LLVMCodeGenerator::declare##Name##Functions() { ... }
   #define RUNTIME_FUNCTION(FuncName, ...) \
       // Create LLVM function declaration
   #include "runtime/RuntimeClassRegistry.h"
   #undef DEFINE_RUNTIME_CLASS
   #undef RUNTIME_FUNCTION
   ```

### Benefits

✅ **Single Source of Truth**: Define each runtime class once
✅ **DRY (Don't Repeat Yourself)**: No duplication across JIT, LLVM, and header files
✅ **Type Safety**: Function signatures defined once, used everywhere
✅ **Maintainability**: Add one entry to registry, all code auto-generates
✅ **Zero Runtime Cost**: All code generation happens at compile time

---

## Runtime Class Registry

### File Location
`src/runtime/RuntimeClassRegistry.h`

### Structure

```cpp
#define RUNTIME_CLASSES \
    DEFINE_RUNTIME_CLASS(ClassName, HandleType, DetectionPredicate) \
        BEGIN_RUNTIME_FUNCTIONS \
            RUNTIME_FUNCTION(func_name, ReturnType, LLVM_TYPE, (ParamType, LLVM_TYPE)...) \
            ... \
        END_RUNTIME_FUNCTIONS \
        BEGIN_RUNTIME_OPERATORS \
            RUNTIME_OPERATOR(AST_OPERATOR, function_name) \
            ... \
        END_RUNTIME_OPERATORS
```

### Current Runtime Classes

#### 1. String (HooString)
```cpp
DEFINE_RUNTIME_CLASS(String, HooString, isPointerTy)
    BEGIN_RUNTIME_FUNCTIONS
        RUNTIME_FUNCTION(new, HooString, LLVM_PTR, )
        RUNTIME_FUNCTION(from_cstr, HooString, LLVM_PTR, (const char*, LLVM_PTR))
        RUNTIME_FUNCTION(from_bytes, HooString, LLVM_PTR, (const char*, LLVM_PTR), (int64_t, LLVM_I64))
        RUNTIME_FUNCTION(concat, HooString, LLVM_PTR, (HooString, LLVM_PTR), (HooString, LLVM_PTR))
        RUNTIME_FUNCTION(equals, int64_t, LLVM_I64, (HooString, LLVM_PTR), (HooString, LLVM_PTR))
        RUNTIME_FUNCTION(length, int64_t, LLVM_I64, (HooString, LLVM_PTR))
        // ... more functions ...
    END_RUNTIME_FUNCTIONS
    BEGIN_RUNTIME_OPERATORS
        RUNTIME_OPERATOR(PLUS, concat)
        RUNTIME_OPERATOR(EQUALS, equals)
        RUNTIME_OPERATOR(LESS, compare)
        // ... more operators ...
    END_RUNTIME_OPERATORS
```

- **ClassName**: `String` - used for method naming
- **HandleType**: `HooString` - C type representing the object (void* pointer)
- **DetectionPredicate**: `isPointerTy` - LLVM type check to identify this runtime type

#### 2. Array (HooArray)
```cpp
DEFINE_RUNTIME_CLASS(Array, HooArray, isPointerTy)
    BEGIN_RUNTIME_FUNCTIONS
        RUNTIME_FUNCTION(new, HooArray, LLVM_PTR, )
        RUNTIME_FUNCTION(push, void, LLVM_VOID, (HooArray, LLVM_PTR), (const void*, LLVM_PTR))
        RUNTIME_FUNCTION(get, const void*, LLVM_PTR, (HooArray, LLVM_PTR), (int64_t, LLVM_I64))
        RUNTIME_FUNCTION(set, void, LLVM_VOID, (HooArray, LLVM_PTR), (int64_t, LLVM_I64), (const void*, LLVM_PTR))
        RUNTIME_FUNCTION(length, int64_t, LLVM_I64, (HooArray, LLVM_PTR))
        // ... more functions ...
    END_RUNTIME_FUNCTIONS
    BEGIN_RUNTIME_OPERATORS
        // Arrays typically don't have operator overloads
    END_RUNTIME_OPERATORS
```

---

## Injection Pipeline

### From C Runtime → Compiled .hoo → LLVM IR → JIT Execution

```
┌─────────────────────────────────────────────────────────────┐
│  1. RUNTIME IMPLEMENTATION (src/rt/)                        │
│  ────────────────────────────────────────────────────────── │
│  C/C++ implementations:                                     │
│  - hoo_string_new()  ← actual C function                    │
│  - hoo_string_concat()                                      │
│  - hoo_array_push()                                         │
│  - etc.                                                      │
└─────────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────────┐
│  2. REGISTRY DEFINITION (RuntimeClassRegistry.h)            │
│  ────────────────────────────────────────────────────────── │
│  Metadata: function names, signatures, types               │
│  (Single source of truth)                                   │
└─────────────────────────────────────────────────────────────┘
                          ↓
              ┌───────────┴───────────┐
              ↓                       ↓
    ┌──────────────────────┐  ┌──────────────────────┐
    │  3A. HoocJIT         │  │  3B. LLVMCodeGen     │
    │  ─────────────────   │  │  ──────────────────  │
    │  • Register symbols  │  │  • Declare functions │
    │  • Link runtime C    │  │  • Storage pointers  │
    │  • Enable JIT exec   │  │  • Operator dispatch │
    └──────────────────────┘  └──────────────────────┘
              ↓                       ↓
    ┌──────────────────────────────────────┐
    │  4. LLVM IR GENERATION                │
    │  ──────────────────────────           │
    │  %0 = call i8* @hoo_string_new()     │
    │  %1 = call i8* @hoo_string_concat(   │
    │        i8* %0, i8* %str_lit)         │
    │  ... etc ...                          │
    └──────────────────────────────────────┘
              ↓
    ┌──────────────────────────────────────┐
    │  5. JIT EXECUTION                     │
    │  ──────────────────────              │
    │  • Link LLVM module with JIT         │
    │  • Resolve hoo_string_* symbols      │
    │  • Execute compiled code              │
    └──────────────────────────────────────┘
```

---

## HoocJIT: Symbol Registration

### Purpose
Register runtime C functions as symbols in the LLVM ORC JIT dylib so they can be called from compiled code.

### Location
`src/HoocJIT.cpp` (lines ~40-145)

### How It Works

```cpp
// Generated method for each runtime class
void HoocJIT::registerStringFunctions() {
    auto& mainJD = JIT->getMainJITDylib();
    llvm::orc::SymbolMap symbols;

    // Register each function in the String class
    symbols[JIT->mangleAndIntern("hoo_string_new")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_new),
            JITSymbolFlags::Exported);

    symbols[JIT->mangleAndIntern("hoo_string_concat")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_concat),
            JITSymbolFlags::Exported);

    // ... more functions ...

    auto Err = mainJD.define(absoluteSymbols(symbols));
    if (Err) exit(1);
}
```

**What This Does:**
1. Gets the main JIT dylib (where symbols live)
2. Creates a map of symbol names → function pointers
3. For each runtime function, adds an entry mapping:
   - **Symbol name**: `hoo_string_new` (for linker resolution)
   - **Function pointer**: Address of actual C implementation `&hoo_string_new`
4. Registers all symbols with the JIT

**Result:** When LLVM generates `call @hoo_string_new()`, the JIT can resolve it to the actual C function.

---

## LLVMCodeGenerator: Function Declaration & Dispatch

### Purpose
1. **Declare** runtime functions as LLVM external functions
2. **Store** function pointers for code generation
3. **Dispatch** operators to correct runtime implementations

### Location
`src/LLVMCodeGenerator.cpp` and `src/LLVMCodeGenerator.h`

### 1. Function Declaration

In `declareStringFunctions()` and similar methods:

```cpp
void LLVMCodeGenerator::declareStringFunctions() {
    // Declare hoo_string_new: HooString hoo_string_new()
    if (!hoo_string_new_func_) {
        std::vector<llvm::Type*> params;  // No parameters
        FunctionType* funcType = FunctionType::get(
            llvm::PointerType::get(context_, 0),  // return void*
            params,
            false);
        hoo_string_new_func_ = Function::Create(
            funcType,
            Function::ExternalLinkage,
            "hoo_string_new",  // ← Must match C function name
            module_.get());
    }

    // ... declare more functions ...
}
```

**What This Does:**
1. Creates LLVM function type matching the C signature
2. Creates LLVM Function declaration (not definition)
3. Stores pointer in `hoo_string_new_func_` for later use
4. Links to external C implementation via name matching

### 2. Function Pointer Storage

In `src/LLVMCodeGenerator.h` private section:

```cpp
// Generated from RUNTIME_CLASSES registry
llvm::Function* hoo_string_new_func_ = nullptr;
llvm::Function* hoo_string_from_cstr_func_ = nullptr;
llvm::Function* hoo_string_concat_func_ = nullptr;
llvm::Function* hoo_string_equals_func_ = nullptr;
// ... one for each runtime function ...
```

### 3. Operator Dispatch

When the code generator encounters a binary operator on a runtime type, it dispatches to the correct runtime function:

```cpp
llvm::Value* LLVMCodeGenerator::tryStringOperator(
    ast::BinaryOperator op, llvm::Value* left, llvm::Value* right) {

    // Check if both operands are strings (pointers)
    if (!left->getType()->isPointerTy() ||
        !right->getType()->isPointerTy()) {
        return nullptr;
    }

    // Ensure functions are declared
    declareStringFunctions();

    // Dispatch operator to correct runtime function
    switch (op) {
        case ast::BinaryOperator::PLUS:
            // String concatenation: str1 + str2 → hoo_string_concat(str1, str2)
            return builder_->CreateCall(hoo_string_concat_func_,
                                       {left, right}, "concat_result");

        case ast::BinaryOperator::EQUALS:
            // String comparison: str1 == str2 → hoo_string_equals(str1, str2)
            Value* result = builder_->CreateCall(hoo_string_equals_func_,
                                                {left, right}, "equals_result");
            return builder_->CreateICmpNE(result,
                ConstantInt::get(llvm::Type::getInt64Ty(context_), 0));

        // ... more operators ...

        default:
            return nullptr;  // Not a string operator
    }
}
```

**Flow Example: `str1 + str2`**
```
User code:        var result = str1 + str2;
↓
Parser:           BinaryOp(PLUS, Identifier("str1"), Identifier("str2"))
↓
Code Generator:   1. Resolve str1, str2 as LLVM Values (pointers)
                  2. Check type: isPointerTy() → yes (runtime String)
                  3. Call tryStringOperator(PLUS, str1, str2)
                  4. Switch on PLUS → case PLUS:
                  5. Generate: builder_->CreateCall(hoo_string_concat_func_, ...)
↓
LLVM IR:          %result = call i8* @hoo_string_concat(i8* %str1, i8* %str2)
↓
JIT:              Resolve @hoo_string_concat → &hoo_string_concat
↓
Execution:        Call actual C function hoo_string_concat()
```

---

## Variable Injection

### Types of Variables

#### 1. Module-Level Variables
```cpp
// In compiled .hoo code:
var globalString = new std.String("hello");

// Generated LLVM IR:
@global_string = global i8* null

// In runtime:
- Allocated via hoo_alloc()
- Initialized via hoo_string_from_cstr()
- Reference counted (hoo_retain/hoo_release)
```

#### 2. Local Variables
```cpp
// In function:
var s = new std.String("world");

// Generated LLVM IR:
%s = alloca i8*                    ; Allocate pointer on stack
store i8* %call_hoo_string_new, i8** %s  ; Store result

// Reference counting:
call void @hoo_retain(i8* %call_hoo_string_new)  ; Increment refcount
```

#### 3. Function Parameters
```cpp
func process(s: std.String) -> void { ... }

// Generated LLVM IR:
define void @process(i8* %s) {
    ; %s is the parameter (pointer to HooString)
    ; Caller is responsible for reference counting
}
```

---

## Memory Management

### Reference Counting Integration

All runtime objects use automatic reference counting (ARC):

```cpp
// Constructor creates object
Value* rawPtr = builder_->CreateCall(hoo_alloc_func_, {sizeArg, typeIdArg});
// refcount = 1 (already counted by constructor)

// When variable goes out of scope
builder_->CreateCall(hoo_release_func_, {ptr});  // refcount--
// Object freed when refcount reaches 0
```

### For Strings Specifically

```cpp
// Creating a string increments refcount
Value* str = builder_->CreateCall(hoo_string_from_cstr_func_, {cstr});
// str now has refcount = 1

// Passing to function
builder_->CreateCall(func, {str});  // No additional retain needed
// Callee doesn't own reference

// Assigning to variable
Value* alloca = createEntryBlockAlloca(func, "str_var", ptr_type);
builder_->CreateStore(str, alloca);
builder_->CreateCall(hoo_retain_func_, {str});  // Increment refcount
// str_var now owns a reference
```

---

## Adding a New Runtime Class

### Step 1: Implement C Functions

Create `src/rt/hoo_myclass.c`:

```c
typedef void* HooMyClass;

HooMyClass hoo_myclass_new(void) {
    // Allocate and initialize
    return malloc(sizeof(struct HooMyClass));
}

void hoo_myclass_destroy(HooMyClass obj) {
    if (obj) free(obj);
}

int64_t hoo_myclass_get_value(HooMyClass obj) {
    return ((struct HooMyClass*)obj)->value;
}

HooMyClass hoo_myclass_add(HooMyClass a, HooMyClass b) {
    // Implementation
}
```

### Step 2: Register in RuntimeClassRegistry.h

```cpp
#define RUNTIME_CLASSES \
    DEFINE_RUNTIME_CLASS(String, HooString, isPointerTy) \
        // ... existing String definition ... \
    DEFINE_RUNTIME_CLASS(MyClass, HooMyClass, isPointerTy) \
        BEGIN_RUNTIME_FUNCTIONS \
            RUNTIME_FUNCTION(new, HooMyClass, LLVM_PTR, ) \
            RUNTIME_FUNCTION(destroy, void, LLVM_VOID, (HooMyClass, LLVM_PTR)) \
            RUNTIME_FUNCTION(get_value, int64_t, LLVM_I64, (HooMyClass, LLVM_PTR)) \
            RUNTIME_FUNCTION(add, HooMyClass, LLVM_PTR, \
                (HooMyClass, LLVM_PTR), (HooMyClass, LLVM_PTR)) \
        END_RUNTIME_FUNCTIONS \
        BEGIN_RUNTIME_OPERATORS \
            RUNTIME_OPERATOR(PLUS, add) \
        END_RUNTIME_OPERATORS
```

### Step 3: Build

```bash
cmake --build build --config RelWithDebInfo
```

**Auto-Generated Code:**

1. **HoocJIT.cpp** automatically generates:
   ```cpp
   void HoocJIT::registerMyClassFunctions();
   ```

2. **LLVMCodeGenerator.h** automatically stores:
   ```cpp
   llvm::Function* hoo_myclass_new_func_ = nullptr;
   llvm::Function* hoo_myclass_destroy_func_ = nullptr;
   llvm::Function* hoo_myclass_get_value_func_ = nullptr;
   llvm::Function* hoo_myclass_add_func_ = nullptr;
   ```

3. **LLVMCodeGenerator.cpp** automatically generates:
   ```cpp
   void LLVMCodeGenerator::declareMyClassFunctions();
   llvm::Value* LLVMCodeGenerator::tryMyClassOperator(...);
   ```

### Step 4: Use in .hoo Code

```hoo
func main() -> void {
    var obj = new MyClass();
    var val = obj.get_value();
    var combined = obj + other_obj;
}
```

---

## Module System Integration

### How Qualified Names Map to Runtime Classes

The **ModuleRegistry** maps qualified names to runtime class exports:

```cpp
// In ModuleSystem.cpp:
void ModuleRegistry::initializeStdModule() {
    auto stdModule = std::make_unique<Module>("std");

    // Maps: std.String → HooString runtime class
    stdModule->addExport(ModuleExport(
        ModuleExport::Kind::CLASS,
        "String",           // Name in .hoo code
        "HooString",        // Runtime class name
        false               // Not generic
    ));

    // Maps: std.Array<T> → HooArray runtime class
    stdModule->addExport(ModuleExport(
        ModuleExport::Kind::CLASS,
        "Array",
        "HooArray",
        true                // Generic
    ));
}
```

**Resolution in Code Generator:**

```cpp
// In generateNewObjectExpression():
if (qualifiedName->isQualified()) {
    // Resolve "std.String" → ModuleExport
    const ModuleExport* moduleExport =
        moduleRegistry_.resolveQualifiedName(*qualifiedName);

    if (moduleExport && moduleExport->kind == ModuleExport::Kind::CLASS) {
        // Dispatch to runtime constructor
        return generateStdClassConstructor(*moduleExport, newExpr);
    }
}
```

---

## Type Resolution

### Detecting Runtime Types

Each runtime class has a **detection predicate** to identify its LLVM type:

```cpp
DEFINE_RUNTIME_CLASS(String, HooString, isPointerTy)
//                                      ^^^^^^^^^^^^^^
//                                      Detection predicate
```

Used in operator dispatch:

```cpp
llvm::Value* LLVMCodeGenerator::tryStringOperator(...) {
    // Only dispatch if BOTH operands are pointers
    if (!left->getType()->isPointerTy() ||
        !right->getType()->isPointerTy()) {
        return nullptr;  // Not a string operation
    }
    // ... proceed with string operator dispatch ...
}
```

---

## Performance Characteristics

### Compile Time
- **O(n)** where n = number of runtime classes × uses in registry
- All expansion happens at C++ compile time
- No runtime overhead

### Runtime
- **Zero overhead**: Functions are direct C calls
- No indirection layers
- Inlineable by optimizer

### Code Size
- Minimal: Only declare functions actually used
- Lazy declaration: Functions declared on-demand

---

## Debugging Tips

### 1. Check Generated Code

To see what code gets generated from the registry:

```bash
# Preprocess only (stops after C preprocessor)
clang++ -E src/HoocJIT.cpp | grep "hoo_"
```

### 2. Verify Symbol Registration

Add logging in HoocJIT:

```cpp
void HoocJIT::registerStringFunctions() {
    std::cout << "Registering String functions...\n";
    symbols[JIT->mangleAndIntern("hoo_string_new")] = ...;
    std::cout << "  ✓ hoo_string_new\n";
    // ... etc ...
}
```

### 3. Check LLVM Declarations

Print the generated LLVM module:

```cpp
auto module = codeGen->generateLLVMModule(ast);
module->print(llvm::errs(), nullptr);
```

Look for lines like:
```llvm
declare i8* @hoo_string_new()
declare i8* @hoo_string_concat(i8*, i8*)
```

### 4. Trace Operator Dispatch

Add debug output in operator methods:

```cpp
llvm::Value* LLVMCodeGenerator::tryStringOperator(...) {
    if (left->getType()->isPointerTy() && right->getType()->isPointerTy()) {
        std::cerr << "String operation: " << opName << "\n";
        // ... dispatch ...
    }
    return nullptr;
}
```

---

## Related Documentation

- **[Language Specification](01-language-specification.md)**: Language features including std.String and std.Array
- **[String Integration Guide](08-string-integration-guide.md)**: Detailed String type architecture
- **[Generics Implementation](09-generics-implementation-guide.md)**: Generic classes and type parameters
- **[Memory Management](07-memory-management-design.md)**: Reference counting and ARC
- **[Object Creation](06-object-creation-guide.md)**: How classes are instantiated

---

## Summary

The Runtime Class Injection Framework:

1. ✅ Defines runtime metadata **once** in `RuntimeClassRegistry.h`
2. ✅ Auto-generates **JIT registration** via X-Macros in `HoocJIT.cpp`
3. ✅ Auto-generates **LLVM declarations** via X-Macros in `LLVMCodeGenerator.cpp`
4. ✅ Auto-generates **operator dispatch** via X-Macros in `LLVMCodeGenerator.cpp`
5. ✅ Links to **C implementations** in `src/rt/` at compile time (HoocJIT) and JIT time (execution)
6. ✅ Enables **modular extension**: Add one registry entry, get full integration

This pattern makes it straightforward to extend Hoo with new runtime types while maintaining clean, maintainable code with zero duplication.
