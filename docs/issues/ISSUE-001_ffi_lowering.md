# ISSUE-001: Missing FFI Implementation (Native/Extern/Link)

## 1. Overview
The Hooc language is designed as a "systems language" and therefore defines robust Foreign Function Interface (FFI) capabilities. This allows Hooc code to call C/C++ functions and link against native shared libraries (`.so`, `.dylib`, `.dll`).

These capabilities are modeled in the AST via four core keywords: `native`, `extern`, `library`, and `link`.

While the `SimpleASTBuilder` correctly creates `FFIDeclaration` nodes, the `HVMCodeGenerator` completely ignores them during the `generateModule` phase.

## 2. FFI Keywords and Supported Syntax

### `library` (Alias Declaration)
**Purpose**: Maps a string literal representing a shared object to a local alias for use in other FFI declarations.
**Syntax**: `library "libname.so" as mylib;`

### `link` (Dynamic Dependency)
**Purpose**: Explicitly instructs the JIT/Linker to load a dynamic library at runtime. It supports versioning and search paths.
**Syntax**: `link dynamic foo.bar @ [1..5] ["/usr/local/lib"];`

### `extern native` (External C-ABI Functions)
**Purpose**: Declares a function signature that exists in an external library.
**Syntax**: `extern native int64 printf(format: pointer[byte], ...);`

### `native func` (Internal Native Implementations)
**Purpose**: Declares a Hooc function or method whose body is not provided in Hooc, but rather implemented natively (typically in the runtime environment).
**Syntax**: `native func:void do_something();`

### `native var` (External Variables)
**Purpose**: Binds a Hooc variable name to a memory address exported by a native library.
**Syntax**: `extern native var errno: int64;`

## 3. Technical Analysis of the Gap
Currently, `HVMCodeGenerator::generateModule` iterates through all top-level declarations:

```cpp
for (const auto& decl : compilationUnit.getDeclarations()) {
    if (auto funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(decl.get())) {
        visitFunction(*funcDecl);
    } else if (auto varDecl = dynamic_cast<const ast::VariableDeclaration*>(decl.get())) {
        // ... handles variables ...
    } else if (auto classDecl = dynamic_cast<const ast::ClassDeclaration*>(decl.get())) {
        // ... handles classes ...
    }
}
```

**The issue is that the loop ignores `compilationUnit.getFFIDeclarations()`.** Even if it iterated over them, there are no `visitFFI...` methods implemented in the generator. 

As a result, no metadata, dependencies, or undefined symbols are emitted into the compiled `HOModule`.

## 4. Requirements & Lowering Suggestions
To fix this, the backend must be updated to process these AST nodes and translate them into HVM binary constructs:

1.  **Link/Library Declarations**: Must be lowered to `SHT_DEPENDENCY` entries in the `HOModule`. This tells the HVM runtime (or JIT) to load the specified `.so`/`.dylib` files into memory before executing the module.
2.  **Extern Native Functions**: Must generate an **Undefined Symbol** in the module's symbol table. This means emitting a `Symbol` with `type = STT_FUNC` and `section_index = -1`. The linker will intercept calls to this symbol and resolve the address from the loaded dynamic libraries.
3.  **Mangle Properly**: Ensure native signatures are mangled (or specifically *not* mangled, depending on the `extern` context) according to the ABI expected by `HVMJIT`.
4.  **Native Methods**: Handle the `this` pointer (`r1`) correctly when transitioning from HVM bytecode execution to native C/C++ execution.

## 5. Status
- **Date**: 2026-05-24
- **Status**: **TODO (UNIMPLEMENTED)**
- **Priority**: High (Blocks hardware integration, standard library implementation, and system calls)