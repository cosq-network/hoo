# Runtime LLVM Integration (`src/runtime/llvm`)

This directory contains the LLVM integration layer for the Hooc runtime system. It bridges the pure C/C++ runtime library (`src/runtime/lib`) with the LLVM compilation infrastructure.

## Purpose

The LLVM integration layer provides:

- **JIT Symbol Registration**: Making runtime functions discoverable by the JIT compiler
- **LLVM IR Declaration**: Creating LLVM function prototypes for runtime calls
- **Type System Integration**: Mapping Hooc types to LLVM types
- **Method Registry**: Tracking available methods on runtime types
- **Code Generation Support**: Helpers for generating calls to runtime functions

## Contents

| File | Description |
|------|-------------|
| `RuntimeRegistry.h` / `RuntimeRegistry.cpp` | Central registry for runtime libraries |
| `RuntimeMethodRegistry.h` / `RuntimeMethodRegistry.cpp` | Runtime method descriptors and registration |
| `RuntimeFunctionStorage.h` | Function pointer storage for LLVM calls |
| `RuntimeClassRegistry.h` | Class descriptors for runtime types |
| `hoo_string_registration.cpp` | String type registration (JIT + LLVM) |
| `hoo_array_registration.cpp` | Array type registration (JIT + LLVM) |
| `hoo_io_registration.cpp` | I/O type registration (JIT + LLVM) |
| `RuntimeStringMethods.h` | String method descriptors |
| `RuntimeArrayMethods.h` | Array method descriptors |
| `RuntimeStringMethodsRegistration.cpp` | Force-linking method registry |

## What Belongs Here

**DO add files here if they:**
- Register runtime functions with LLVM or the JIT
- Define LLVM function prototypes
- Provide code generation helpers
- Track runtime type metadata (methods, classes)
- Require LLVM headers

**DO NOT add files here if they:**
- Implement runtime logic (put in `src/runtime/lib`)
- Are general compiler infrastructure (put in `src/`)
- Could work without LLVM present

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     Hooc Code                               │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│              LLVMCodeGenerator (src/)                        │
│    - Generates LLVM IR from AST                             │
│    - Uses RuntimeFunctionStorage for function pointers      │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│           Runtime LLVM Integration (here)                   │
│    - RuntimeRegistry: Manages registration callbacks       │
│    - RuntimeFunctionStorage: Stores LLVM Function*         │
│    - *Registration.cpp: Declares functions in module      │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│              Runtime Library (src/runtime/lib)              │
│    - hoo_string.cpp: String implementation                  │
│    - hoo_generic_array.cpp: Array implementation           │
│    - hoo_runtime.c: Memory management                      │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                     HoocJIT                                 │
│    - Registers runtime symbols with JIT                    │
│    - Enables JIT calls to runtime functions                │
└─────────────────────────────────────────────────────────────┘
```

## Module Initialization

The compiler supports dynamic initialization of global variables and constants via an internal `__hoo_init` function.

### Initialization Flow

1. **Generation**: `LLVMCodeGenerator` collects all global initializers that require runtime calls (strings, arrays, function calls).
2. **Implementation**: These initializers are emitted into a private `__hoo_init` function within the LLVM module.
3. **Registration**: The `__hoo_init` function is added to the `llvm.global_ctors` array with priority 65535.
4. **Execution**: The JIT (or OS loader for AOT) executes `__hoo_init` automatically before the program's entry point (`main`) runs.

This system ensures that complex global data structures are fully constructed and ready for use when the program logic begins.

## Registration Pattern

Each runtime type follows a two-phase registration pattern:

### Phase 1: Static Registration (at load time)

```cpp
// In hoo_string_registration.cpp
HOOC_REGISTER_RUNTIME(
    String,
    hoo_string_register_with_jit,      // JIT callback
    hoo_string_declare_llvm_functions // LLVM callback
)
```

This creates a static `RuntimeAutoRegister` object that registers callbacks with `RuntimeRegistry::getInstance()` during C++ static initialization.

### Phase 2a: JIT Registration (at JIT construction)

```cpp
void hoo_string_register_with_jit(LLJIT& jit, JITDylib& mainDylib) {
    SymbolMap symbols;
    symbols[jit.mangleAndIntern("hoo_string_concat")] =
        ExecutorSymbolDef(ExecutorAddr::fromPtr(&hoo_string_concat), ...);
    mainDylib.define(absoluteSymbols(symbols));
}
```

### Phase 2b: LLVM Declaration (at module creation)

```cpp
void hoo_string_declare_llvm_functions(Module& module, LLVMContext& context, void* userData) {
    auto storage = static_cast<RuntimeFunctionStorage*>(userData);
    storage->strings.hoo_string_concat_func = Function::Create(
        FunctionType::get(ptrTy, {ptrTy, ptrTy}, false),
        Function::ExternalLinkage,
        "hoo_string_concat",
        &module);
}
```

## Implementation Guidelines

### JIT Registration Callback

```cpp
void hoo_<type>_register_with_jit(llvm::orc::LLJIT& jit, llvm::orc::JITDylib& mainDylib) {
    SymbolMap symbols;

    #define REGISTER_FUNC(name) \
        symbols[jit.mangleAndIntern("hoo_<type>_" #name)] = \
            ExecutorSymbolDef(ExecutorAddr::fromPtr(&hoo_<type>_##name), \
                              JITSymbolFlags::Exported);

    // Register all functions...

    #undef REGISTER_FUNC

    auto err = mainDylib.define(absoluteSymbols(symbols));
    if (err) { /* handle error */ }
}
```

### LLVM Declaration Callback

```cpp
void hoo_<type>_declare_llvm_functions(llvm::Module& module,
                                       llvm::LLVMContext& context,
                                       void* userData) {
    auto storage = static_cast<RuntimeFunctionStorage*>(userData);

    #define DECLARE_FN(name, retType, ...) \
        storage-><type>s.hoo_<type>_##name##_func = Function::Create( \
            FunctionType::get(retType, {__VA_ARGS__}, false), \
            Function::ExternalLinkage, \
            "hoo_<type>_" #name, \
            &module);

    // Declare all functions...

    #undef DECLARE_FN
}
```

### Adding Function Pointers to Storage

When adding new runtime functions, update `RuntimeFunctionStorage.h`:

```cpp
struct <Type>FunctionStorage {
    // Existing functions...
    llvm::Function* hoo_<type>_<new_func>_func = nullptr;
};
```

### Method Registry Macros

Use the method registry macros for declaring runtime methods:

```cpp
// In Runtime<Type>Methods.h
BEGIN_RUNTIME_CLASS(<type>, "<hooc_type_name>")
    RUNTIME_METHOD(<hoocMethodName>, "hoo_<type>_<runtime_func>")
    // ... more methods
END_RUNTIME_CLASS(<type>, "<hooc_type_name>")
```

## Usage

### Including Headers

```cpp
#include "runtime/llvm/RuntimeRegistry.h"
#include "runtime/llvm/RuntimeFunctionStorage.h"
```

### Forcing Registration Linkage

The runtime registration uses static initialization, which may be stripped by the linker. Force inclusion with:

```cpp
// In LLVMCodeGenerator.cpp
extern void _hoo_runtime_methods_ensure_registration();

// Call during initialization
_hoo_runtime_methods_ensure_registration();
```

## Adding a New Runtime Type

1. **Implement the type** in `src/runtime/lib/` (see `src/runtime/lib/README.md`)

2. **Create registration file** `hoo_<type>_registration.cpp`:

   ```cpp
   #include "../lib/hoo_<type>.h"
   #include "RuntimeRegistry.h"
   #include "RuntimeFunctionStorage.h"

   void hoo_<type>_register_with_jit(LLJIT& jit, JITDylib& mainDylib) { ... }
   void hoo_<type>_declare_llvm_functions(Module& mod, LLVMContext& ctx, void* userData) { ... }

   HOOC_REGISTER_RUNTIME(<Type>, hoo_<type>_register_with_jit, hoo_<type>_declare_llvm_functions)
   ```

3. **Add function storage** in `RuntimeFunctionStorage.h`

4. **Update CMakeLists.txt**:
   ```cmake
   src/runtime/llvm/hoo_<type>_registration.cpp
   ```

5. **Add tests** in `tests/`

## Namespace

All classes and types in this directory are in the `hooc::runtime` namespace:

```cpp
namespace hooc {
namespace runtime {

class RuntimeRegistry { ... };

} // namespace runtime
} // namespace hooc
```
