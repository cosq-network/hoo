# ISSUE-001: Missing FFI Implementation (Native/Extern/Link)

## 1. Overview
The Hooc grammar defines robust Foreign Function Interface (FFI) capabilities through `native`, `extern`, `library`, and `link` keywords. While the `SimpleASTBuilder` correctly creates `FFIDeclaration` nodes, the `HVMCodeGenerator` completely ignores them during the `generateModule` phase.

## 2. Technical Analysis
The HVM backend requires these declarations to:
1.  **Register Dependencies**: Call `module_->addDependency()` with the correct library name.
2.  **Define Undefined Symbols**: Create `STT_FUNC` symbols with `section_index = -1` (Undefined) so the JIT can perform lazy symbol resolution.
3.  **Mangle Properly**: Ensure native signatures are mangled according to the ABI expected by `HVMJIT`.

## 3. Requirements & Lowering Suggestions
- **Link Declarations**: Lower to `SHT_DEPENDENCY` entries in the `HOModule`.
- **Library Imports**: Map internal alias to external binary path.
- **Extern Native Functions**: Generate an undefined symbol in the module's symbol table.
- **Native Methods**: Handle `this` pointer (r1) correctly when transitioning from HVM to C/C++.

## 4. Status
- **Date**: 2026-05-24
- **Status**: **TODO (UNIMPLEMENTED)**
- **Priority**: High (Blocks hardware integration and system calls)
