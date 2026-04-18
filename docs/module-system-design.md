# Hooc Module System Design Specification

**Status:** Design Proposal (Updated)
**Date:** April 18, 2026

## 1. Abstract

This document defines the architecture and methodology for the Hooc module system. The system is hierarchical, filesystem-mapped, and inspired by Python's module resolution logic. It supports importing entire modules or selected symbols (classes, functions, and variables) from modules, providing flexibility in namespace management while maintaining strict isolation.

## 2. Design Principles

- **Hierarchical**: Modules are organized in a tree structure matching the filesystem.
- **Flexible Scoping**: Symbols can be accessed via fully qualified names or imported into the local namespace for direct access.
- **Isolated**: Each module has its own private namespace; only explicitly exported symbols are visible to importers.
- **Consistent**: Built-in and user-defined modules follow the same logical resolution rules.

## 3. Module Hierarchy and Filesystem Mapping

The logical module path `a.b.c` corresponds to a physical file or directory on the disk.

### 3.1 Resolution Rules

When resolving a module `path.to.Module`:
1.  **File Module**: Look for `path/to/Module.hoo`.
2.  **Directory Module**: Look for `path/to/Module/mod.hoo`. (The `mod.hoo` file acts as the entry point for the directory, similar to `__init__.py`).

### 3.2 Normalization

- Directory and file names are normalized (lowercase recommended).
- Dots (`.`) in the module path are translated to path separators (`/` or `\`).

## 4. Import Syntax and Semantics

Hooc supports three primary import forms to manage namespaces.

### 4.1 Basic Module Import

```hoo
import hoo.io;
import math.vector as vec;
```

- `import hoo.io;`: Makes the module `hoo.io` available. Symbols are accessed as `hoo.io.println()`.
- `import math.vector as vec;`: Aliases the module. Symbols are accessed as `vec.Vector3()`.

### 4.2 Selective Symbol Import

```hoo
from hoo.io import println;
from math.geometry import Point, Circle as Shape;
```

- `from hoo.io import println;`: Imports the `println` function into the local scope. It can be called directly as `println("Hello")`.
- `from math.geometry import Point, Circle as Shape;`: Imports `Point` and `Circle` (aliased as `Shape`). Access them directly as `Point(x, y)` or `new Shape()`.

### 4.3 Submodule Import

```hoo
from hoo import io;
```

- Imports the `io` submodule from the `hoo` package. Symbols are accessed via the submodule name: `io.println()`.

## 5. Module Search Paths

The compiler (and JIT) searches for modules in the following order:

1.  **Built-in Modules**: The `hoo` namespace (registered in `src/runtime/llvm`).
2.  **Relative Path**: Sibling files/folders relative to the importing file.
3.  **Project Root**: The directory where the main `.hoo` file or `hooc.json` resides.
4.  **Package Manager Path**: `hooc_modules/` directory (future support for npm-style packages).

## 6. Built-in Modules (`hoo`)

The standard library is registered within the compiler via the `hoo` namespace.

| Logical Name | Definition Location | Description |
|--------------|-----------------|-------------|
| `hoo.io`     | `src/runtime/llvm/hoo_io_registration.cpp` | Input/Output operations |
| `hoo.string` | `src/runtime/llvm/hoo_string_registration.cpp` | String utilities |
| `hoo.array`  | `src/runtime/llvm/hoo_array_registration.cpp` | Array and collection helpers |

## 7. Implementation Architecture

### 7.1 `ModuleResolver`

A component responsible for:
- Mapping logical paths to physical paths.
- Tracking loaded modules to prevent duplicate compilation.
- Detecting circular dependencies.

### 7.2 `ModuleScope` and Symbol Resolution

Each `CompilationUnit` holds a `ModuleScope` containing:
- **Module Imports**: Maps of aliases/names to loaded Module ASTs.
- **Symbol Imports**: Maps of local names to specific symbols inside other modules.
- **Local Declarations**: Symbols defined within the file.

**Resolution Priority:**
1.  Local declarations.
2.  Imported symbols (from `from ... import ...`).
3.  Module aliases (from `import ... as ...`).
4.  Fully qualified paths.

### 7.3 Code Generation

The `LLVMCodeGenerator` uses the `ModuleScope` to resolve identifiers.
- When it sees `println()`, it checks if `println` was selectively imported from `hoo.io`.
- If found, it generates a call to the external symbol `hoo_io_println`.

## 8. JIT and AOT Execution Guidelines

The transition from a single-file compiler to a modular system requires specific technical strategies for symbol management and binary linking.

### 8.1 Symbol Mangling (Cross-Module Naming)

To prevent name collisions when linking multiple modules, the compiler must use a deterministic mangling scheme for all exported symbols.

**Scheme**: `_H_<module_path>_<symbol_name>`
- `hoo.io.println` → `_H_hoo_io_println`
- `math.vector.Vector3` (constructor) → `_H_math_vector_Vector3_init`

### 8.2 JIT Loading Strategy

In JIT mode, modules are compiled into `llvm::orc::ThreadSafeModule` instances on-demand.

1.  **Isolation**: Each module is loaded into its own `JITDylib` if strict isolation is required, or a shared `main` Dylib for simple scripts.
2.  **Lazy Resolution**: When a module `A` imports `B`, the JIT registers a search generator. If a symbol `_H_B_func` is requested but not found, the `ModuleResolver` triggers the compilation of `B.hoo` and adds it to the JIT engine dynamically.
3.  **Global Constructor/Destructor**: JIT must execute `__hoo_module_init` for each imported module exactly once before the `main` function executes.

### 8.3 AOT Compilation and Linking

AOT execution involves producing and consuming `.ho` (Hooc Object) files.

1.  **Object File Structure**:
    - **Header**: Magic number, version, and architecture.
    - **Exports Table**: Map of mangled symbols to offsets in the code section.
    - **Imports Table**: List of required mangled symbols and the logical module paths they belong to.
    - **Metadata**: Information about classes (field layouts, VTable).
2.  **The `hooc` Linker**:
    - Generates a static dependency graph.
    - Verifies that all symbols in `Imports` tables are satisfied by an `Exports` table in another `.ho` file or the `hoo` runtime.
    - Resolves relative jumps and absolute addresses.
3.  **Standalone Executables**: The linker can bundle the `hoo` runtime, standard library `.ho` files, and user `.ho` files into a single native binary using LLVM's `TargetMachine` and `ObjectFileWriter`.

## 9. Future: Package Management

The module system is architected to support a package management ecosystem similar to `npm`.

### 9.1 `hooc_modules` and Resolution

A project-local `hooc_modules/` directory will serve as the repository for third-party dependencies. When an import cannot be resolved built-in modules or local files, the `ModuleResolver` will search within `hooc_modules/`.

### 9.2 Package Manifest (`hooc.json`)

Projects and packages will include a `hooc.json` manifest to define metadata and dependencies.

Example `hooc.json`:
```json
{
  "name": "my-app",
  "version": "1.0.0",
  "dependencies": {
    "http": "1.2.0",
    "json-parser": "github:user/repo#v1.0"
  }
}
```

### 9.3 Scoped Packages

To prevent naming conflicts in the global package space, the system will support scoped packages:
```hoo
import @company.network.Client;
```
This maps logically to `hooc_modules/@company/network/mod.hoo`.

---
*This document serves as the official design for the Hooc Module System.*
