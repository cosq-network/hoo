# Runtime Class Injection Framework

This document explains how runtime modules, classes, functions, and variables from **hoort** (the Hoo runtime library) are injected into **HoocJIT** and **LLVMCodeGenerator** using a **callback-based registration system**.

## Overview

The **Runtime Class Injection Framework** is a callback-based system that enables:
- **Full developer control**: Runtime authors write callbacks to register their types
- **Zero compiler coupling**: HoocJIT and LLVMCodeGenerator don't know about specific runtime types
- **Distributed registration**: Each runtime library self-registers via static initialization
- **Automatic JIT registration** of runtime functions via callbacks
- **Automatic LLVM function declarations** via callbacks
- **Extensible operator dispatch** for runtime types
- **Minimal boilerplate** when adding new runtime classes

### Key Components

| Component | Purpose |
|-----------|---------|
| `src/runtime/RuntimeRegistry.h` | Central singleton registry that collects and invokes callbacks |
| `src/runtime/RuntimeFunctionStorage.h` | Storage for runtime function pointers |
| `src/HoocJIT.cpp` | Invokes JIT registration callbacks |
| `src/LLVMCodeGenerator.cpp` | Invokes LLVM declaration callbacks |
| `src/rt/hoo_string_registration.cpp` | String runtime self-registration (example) |
| `src/rt/` | C/C++ implementation of runtime classes (hoo_string, hoo_array, etc.) |

---

## The Callback-Based Registration System

Runtime libraries register themselves through **two callbacks**:

1. **JIT Registration Callback**: Register symbols with LLVM ORC JIT
2. **LLVM Declaration Callback**: Declare function prototypes in LLVM modules

### How It Works

1. **Runtime library defines callbacks**:
   ```cpp
   // src/rt/hoo_string_registration.cpp

   void hoo_string_register_with_jit(
       llvm::orc::LLJIT& jit,
       llvm::orc::JITDylib& mainDylib) {
       // Register hoo_string_* functions as JIT symbols
       // Full control over how symbols are registered
   }

   void hoo_string_declare_llvm_functions(
       llvm::Module& module,
       llvm::LLVMContext& context,
       void* userData) {
       // Declare hoo_string_* functions in LLVM module
       // Populate function pointers in userData storage
   }
   ```

2. **Runtime library self-registers using macro**:
   ```cpp
   // At end of hoo_string_registration.cpp
   HOOC_REGISTER_RUNTIME(
       String,                                // Runtime name
       hoo_string_register_with_jit,          // JIT callback
       hoo_string_declare_llvm_functions      // LLVM callback
   )
   ```

3. **HoocJIT invokes JIT callbacks**:
   ```cpp
   // In HoocJIT constructor
   auto& registry = runtime::RuntimeRegistry::getInstance();
   auto& mainJD = JIT->getMainJITDylib();
   registry.registerAllWithJIT(*JIT, mainJD);  // Invokes all registered JIT callbacks
   ```

4. **LLVMCodeGenerator invokes LLVM callbacks**:
   ```cpp
   // In LLVMCodeGenerator::declareRuntimeFunctions()
   auto& registry = runtime::RuntimeRegistry::getInstance();
   registry.declareAllFunctions(*module_, context_, &runtimeFunctionStorage_);
   // Invokes all registered LLVM callbacks, populating function pointers
   ```

### Benefits

✅ **Full Control**: Runtime developers have direct access to LLVM and JIT APIs
✅ **Zero Coupling**: Compiler infrastructure doesn't know about specific runtime types
✅ **Extensibility**: Add new runtime types without modifying compiler code
✅ **Distributed**: Each runtime manages its own registration logic
✅ **Type Safety**: Callbacks receive concrete LLVM types, not void pointers

---

## Central Registry (RuntimeRegistry)

### File Location
`src/runtime/RuntimeRegistry.h` and `src/runtime/RuntimeRegistry.cpp`

### Structure

The `RuntimeRegistry` is a singleton that collects callbacks from all runtime libraries:

```cpp
class RuntimeRegistry {
public:
    static RuntimeRegistry& getInstance();  // Singleton accessor

    void registerRuntime(const RuntimeRegistrationEntry& entry);

    void registerAllWithJIT(
        llvm::orc::LLJIT& jit,
        llvm::orc::JITDylib& mainDylib);

    void declareAllFunctions(
        llvm::Module& module,
        llvm::LLVMContext& context,
        void* userData);

    const std::vector<RuntimeRegistrationEntry>& getRegisteredRuntimes() const;

private:
    std::vector<RuntimeRegistrationEntry> runtimes_;
};
```

### Registration Entry

Each runtime library registers itself with:

```cpp
struct RuntimeRegistrationEntry {
    const char* runtimeName;
    RuntimeJITRegistrationCallback jitCallback;
    RuntimeLLVMDeclarationCallback llvmCallback;
};
```

Where callbacks are:

```cpp
// JIT symbol registration callback
using RuntimeJITRegistrationCallback = void(*)(
    llvm::orc::LLJIT& jit,
    llvm::orc::JITDylib& mainDylib
);

// LLVM function declaration callback
using RuntimeLLVMDeclarationCallback = void(*)(
    llvm::Module& module,
    llvm::LLVMContext& context,
    void* userData  // Pointer to RuntimeFunctionStorage
);
```

### Registration Macro

Runtime libraries use this macro to self-register:

```cpp
#define HOOC_REGISTER_RUNTIME(Name, JitFn, LlvmFn) \
    static ::hooc::runtime::RuntimeAutoRegister \
        __hooc_runtime_auto_register_##Name({ \
            #Name, \
            JitFn, \
            LlvmFn \
        });
```

This creates a static `RuntimeAutoRegister` object that registers the callbacks during C++ static initialization (before main()).

### Current Runtime Classes

#### 1. String (HooString)

**Self-registration in `src/rt/hoo_string_registration.cpp`:**

```cpp
// JIT Registration Callback
void hoo_string_register_with_jit(
    llvm::orc::LLJIT& jit,
    llvm::orc::JITDylib& mainDylib) {
    SymbolMap symbols;
    // Register all String functions with JIT
    symbols[jit.mangleAndIntern("hoo_string_from_cstr")] = ...;
    symbols[jit.mangleAndIntern("hoo_string_concat")] = ...;
    // ... 30+ functions ...
    mainDylib.define(absoluteSymbols(symbols));
}

// LLVM Declaration Callback
void hoo_string_declare_llvm_functions(
    llvm::Module& module,
    llvm::LLVMContext& context,
    void* userData) {
    auto storage = static_cast<RuntimeFunctionStorage*>(userData);
    auto& stringStorage = storage->strings;

    // Declare all String functions in LLVM module
    stringStorage.hoo_string_from_cstr_func = Function::Create(
        FunctionType::get(ptrTy, {ptrTy}, false),
        Function::ExternalLinkage,
        "hoo_string_from_cstr",
        &module);
    // ... 30+ functions ...
}

// Self-register
HOOC_REGISTER_RUNTIME(
    String,
    hoo_string_register_with_jit,
    hoo_string_declare_llvm_functions
)
```

Functions include: `new`, `from_cstr`, `concat`, `equals`, `length`, `compare`, `substring`, `to_upper`, `to_lower`, etc. (30+ total)

#### 2. Array (HooArray)

Similar callback-based structure:
- JIT callback registers 20+ array functions
- LLVM callback declares function prototypes and populates storage
- Self-registers using same `HOOC_REGISTER_RUNTIME` macro

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
│  - Self-registration callbacks                              │
└─────────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────────┐
│  2. CALLBACK REGISTRATION (hoo_*_registration.cpp)          │
│  ────────────────────────────────────────────────────────── │
│  • JIT callback: Register symbols with LLJIT               │
│  • LLVM callback: Declare functions in module              │
│  • HOOC_REGISTER_RUNTIME macro: Auto-registers             │
└─────────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────────┐
│  3. CENTRAL REGISTRY (RuntimeRegistry singleton)            │
│  ────────────────────────────────────────────────────────── │
│  • Collects all registered callbacks                        │
│  • Invoked by HoocJIT and LLVMCodeGenerator                 │
└─────────────────────────────────────────────────────────────┘
                          ↓
              ┌───────────┴───────────┐
              ↓                       ↓
    ┌──────────────────────┐  ┌──────────────────────┐
    │  4A. HoocJIT         │  │  4B. LLVMCodeGen     │
    │  ─────────────────   │  │  ──────────────────  │
    │  • Invoke JIT        │  │  • Invoke LLVM       │
    │    callbacks         │  │    callbacks         │
    │  • Register symbols  │  │  • Declare functions │
    │  • Enable JIT exec   │  │  • Populate storage  │
    └──────────────────────┘  └──────────────────────┘
              ↓                       ↓
    ┌──────────────────────────────────────┐
    │  5. LLVM IR GENERATION                │
    │  ──────────────────────────           │
    │  %0 = call i8* @hoo_string_new()     │
    │  %1 = call i8* @hoo_string_concat(   │
    │        i8* %0, i8* %str_lit)         │
    │  ... etc ...                          │
    └──────────────────────────────────────┘
              ↓
    ┌──────────────────────────────────────┐
    │  6. JIT EXECUTION                     │
    │  ──────────────────────              │
    │  • Link LLVM module with JIT         │
    │  • Resolve hoo_string_* symbols      │
    │  • Execute compiled code              │
    └──────────────────────────────────────┘
```

---

## HoocJIT: Symbol Registration

### Purpose
Invoke runtime registration callbacks to register runtime C functions as symbols in the LLVM ORC JIT dylib.

### Location
`src/HoocJIT.cpp` (constructor)

### How It Works

```cpp
// In HoocJIT constructor
HoocJIT::HoocJIT() {
    // ... initialize LLVM ...

    // Invoke all registered runtime callbacks
    auto& registry = runtime::RuntimeRegistry::getInstance();
    auto& mainJD = JIT->getMainJITDylib();

    registry.registerAllWithJIT(*JIT, mainJD);  // Call all JIT callbacks
}
```

**What happens inside the callback:**

```cpp
// In hoo_string_registration.cpp
void hoo_string_register_with_jit(
    llvm::orc::LLJIT& jit,
    llvm::orc::JITDylib& mainDylib) {

    llvm::orc::SymbolMap symbols;

    // Register each function with JIT
    symbols[jit.mangleAndIntern("hoo_string_new")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_new),
            JITSymbolFlags::Exported);

    symbols[jit.mangleAndIntern("hoo_string_concat")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_concat),
            JITSymbolFlags::Exported);

    // ... 28+ more functions ...

    auto Err = mainDylib.define(absoluteSymbols(symbols));
    if (Err) {
        llvm::errs() << "Failed to register String functions\\n";
        exit(1);
    }
}
```

**What This Does:**
1. HoocJIT invokes `registerAllWithJIT()` on the registry
2. Registry invokes JIT callback for each registered runtime (e.g., String)
3. Runtime callback creates a map of symbol names → function pointers
4. For each runtime function, adds an entry mapping:
   - **Symbol name**: `hoo_string_new` (for linker resolution)
   - **Function pointer**: Address of actual C implementation `&hoo_string_new`
5. Callback registers all symbols with the JIT
6. Runtime developer has **full control** over symbol registration

**Result:** When LLVM generates `call @hoo_string_new()`, the JIT can resolve it to the actual C function.

---

## LLVMCodeGenerator: Function Declaration & Dispatch

### Purpose
1. **Invoke callbacks** to declare runtime functions in LLVM modules
2. **Store** function pointers from callbacks for code generation
3. **Dispatch** operators to correct runtime implementations

### Location
`src/LLVMCodeGenerator.cpp` and `src/LLVMCodeGenerator.h`

### 1. Function Declaration via Callbacks

In `declareRuntimeFunctions()` method:

```cpp
void LLVMCodeGenerator::declareRuntimeFunctions() {
    // Force String runtime to be linked (works around linker optimization)
    extern void _hoo_string_ensure_registration();
    _hoo_string_ensure_registration();

    // Invoke all registered LLVM declaration callbacks
    auto& registry = runtime::RuntimeRegistry::getInstance();
    registry.declareAllFunctions(*module_, context_, &runtimeFunctionStorage_);

    // Sync function pointers to legacy members (for backward compatibility)
    auto& stringStorage = runtimeFunctionStorage_.strings;
    hoo_string_from_cstr_func_ = stringStorage.hoo_string_from_cstr_func;
    hoo_string_concat_func_ = stringStorage.hoo_string_concat_func;
    // ... sync 28+ more functions ...
}
```

**What happens inside the callback:**

```cpp
// In hoo_string_registration.cpp
void hoo_string_declare_llvm_functions(
    llvm::Module& module,
    llvm::LLVMContext& context,
    void* userData) {

    auto fullStorage = static_cast<RuntimeFunctionStorage*>(userData);
    auto& storage = fullStorage->strings;

    // Create LLVM types
    auto ptrTy = PointerType::get(context, 0);  // void*
    auto i64Ty = Type::getInt64Ty(context);     // int64_t

    // Declare hoo_string_new: void* hoo_string_new()
    std::vector<Type*> params;  // No parameters
    auto funcType = FunctionType::get(ptrTy, params, false);
    storage.hoo_string_new_func = Function::Create(
        funcType,
        Function::ExternalLinkage,
        "hoo_string_new",  // Must match C function name
        &module);

    // Declare hoo_string_concat: void* hoo_string_concat(void*, void*)
    params = {ptrTy, ptrTy};
    funcType = FunctionType::get(ptrTy, params, false);
    storage.hoo_string_concat_func = Function::Create(
        funcType,
        Function::ExternalLinkage,
        "hoo_string_concat",
        &module);

    // ... declare 28+ more functions ...
}
```

**What This Does:**
1. LLVMCodeGenerator invokes `declareAllFunctions()` on registry
2. Registry invokes LLVM callback for each registered runtime (e.g., String)
3. Runtime callback creates LLVM function types matching C signatures
4. Creates LLVM Function declarations (not definitions)
5. Stores pointers in `RuntimeFunctionStorage` passed via `userData`
6. LLVMCodeGenerator syncs pointers to legacy members for backward compatibility
7. Runtime developer has **full control** over how functions are declared

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

## Adding a New Runtime Type (Dict Example)

To add a new runtime type (e.g., Dict/HashMap):

1. **Implement C API** in `src/rt/hoo_dict.{h,cpp}`
   ```c
   typedef void* HooDict;
   HooDict hoo_dict_new(void);
   void hoo_dict_set(HooDict d, const char* key, void* value);
   void* hoo_dict_get(HooDict d, const char* key);
   // ... etc ...
   ```

2. **Create registration** in `src/rt/hoo_dict_registration.cpp`
   ```cpp
   void hoo_dict_register_with_jit(llvm::orc::LLJIT& jit, llvm::orc::JITDylib& mainDylib) {
       SymbolMap symbols;
       symbols[jit.mangleAndIntern("hoo_dict_new")] = ...;
       symbols[jit.mangleAndIntern("hoo_dict_set")] = ...;
       symbols[jit.mangleAndIntern("hoo_dict_get")] = ...;
       mainDylib.define(absoluteSymbols(symbols));
   }

   void hoo_dict_declare_llvm_functions(
       llvm::Module& module,
       llvm::LLVMContext& context,
       void* userData) {
       auto storage = static_cast<RuntimeFunctionStorage*>(userData);
       auto& dictStorage = storage->dicts;  // Access Dict function storage

       // Declare LLVM functions and populate storage
       dictStorage.hoo_dict_new_func = Function::Create(...);
       dictStorage.hoo_dict_set_func = Function::Create(...);
       dictStorage.hoo_dict_get_func = Function::Create(...);
   }

   HOOC_REGISTER_RUNTIME(Dict, hoo_dict_register_with_jit, hoo_dict_declare_llvm_functions)
   ```

3. **Add to CMakeLists.txt**
   ```cmake
   add_library(hoo-compiler
       ...
       src/rt/hoo_dict_registration.cpp
   )
   ```

4. **Update RuntimeFunctionStorage.h** with Dict storage:
   ```cpp
   struct DictFunctionStorage {
       llvm::Function* hoo_dict_new_func = nullptr;
       llvm::Function* hoo_dict_set_func = nullptr;
       llvm::Function* hoo_dict_get_func = nullptr;
       // ... etc ...
   };

   struct RuntimeFunctionStorage {
       StringFunctionStorage strings;
       ArrayFunctionStorage arrays;
       DictFunctionStorage dicts;  // ← Add this
   };
   ```

5. **Done!** No changes needed to HoocJIT or LLVMCodeGenerator

That's it! The callback system handles everything else automatically.

---

## Summary

The Runtime Class Injection Framework uses a **callback-based registration system**:

1. ✅ **Full Control**: Runtime developers write callbacks with complete control
2. ✅ **Zero Coupling**: Compiler infrastructure has no knowledge of specific runtimes
3. ✅ **Distributed Registration**: Each runtime manages its own registration logic
4. ✅ **Extensible**: Add new runtime types without modifying compiler code
5. ✅ **Type Safe**: Callbacks receive concrete LLVM types, not void pointers
6. ✅ **Self-registering**: Uses static initialization and HOOC_REGISTER_RUNTIME macro
7. ✅ **Links to C implementations** in `src/rt/` at compile time and JIT time

This pattern makes it straightforward to extend Hoo with new runtime types while giving runtime developers complete control over registration and maintaining clean, maintainable code with zero compiler coupling.
