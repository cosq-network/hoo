# HVM JIT Implementation Guide: LLVM-Based System Emulation

This document serves as the normative guideline for implementing a high-performance Just-In-Time (JIT) compiler for the Hooc Virtual Machine (HVM) using the LLVM compiler infrastructure. This guide is specifically aligned with the **HVM v1.4 (Hardware Ready)** specification.

---

## 1. Architectural Philosophy

The HVM JIT is designed as a **High-Fidelity System Emulator** rather than a traditional language-specific virtual machine. This distinction is critical for the HVM v1.4 "Hardware Ready" profile.

- **Binary-to-Native Translation**: Unlike the JVM or .NET, which JIT from high-level stack-based bytecode, the HVM JIT performs **Dynamic Binary Translation**. it consumes raw, physical HVM RISC instructions from `.ho` files and translates them into host-native machine code.
- **Physical State Simulation**: The JIT maintains a rigid 1-to-1 mapping of the HVM physical state. Each virtual register file is a high-speed memory block or physical register set, and the memory model is a flat, byte-addressable space that mirrors real silicon.
- **Hardware Transparency**: The generated code must behave exactly as it would on a physical HVM processor. This includes precise two's-complement overflow, 64-bit pointer arithmetic, and strict calling conventions.
- **Zero-Abstraction Execution**: Complexity is shifted to the **Lowering Layer**. By the time the JIT sees the code, high-level constructs like "objects" have already been turned into memory offsets, and "exceptions" into shadow-stack pointers. The JIT's primary goal is to execute these primitives with absolute minimum latency.

---

## 2. Infrastructure: LLVM ORC JIT v2

The JIT uses the **LLVM On-Request Compilation (ORC) version 2** library. ORC v2 provides a highly modular, thread-safe, and scalable infrastructure for building modern execution engines.

### **2.1 Core Components & Implementation Roles**
1.  **ExecutionSession (The Orchestrator)**:
    - Acts as the central manager for the entire JIT session.
    - Manages the **Global Symbol Table** and coordinates symbol lookups across all loaded modules.
    - Handles concurrency, allowing multiple functions to be compiled in parallel across different threads.
2.  **JITDylib (The Logical Module)**:
    - Every HVM `.ho` module is mapped to exactly one `JITDylib`.
    - It provides a private symbol namespace. Symbols marked as `STB_LOCAL` in the `.ho` file are restricted to their parent `JITDylib`, while `STB_GLOBAL` symbols are exported to the session.
3.  **ObjectLinkingLayer (The Runtime Linker)**:
    - Responsible for the final "physical" step: mapping the compiled machine code into executable memory pages.
    - It handles relocations, patching `CALL` and `JMP` targets once the final host addresses are determined.
    - **Registration**: This layer integrates with `llvm::JITEventListener` to support native debuggers (GDB/LLDB).
4.  **IRCompileLayer (The Translator)**:
    - Bridges the gap between the HVM-to-LLVM IR generator and the backend machine-code generator.
    - It uses a **ThreadSafeContext** and **ThreadSafeModule** to ensure that IR generation for one HVM function doesn't interfere with another.
5.  **MangleAndInterner (The String Engine)**:
    - Efficiently manages symbol names. It converts the long, complex mangled strings from `SymbolMangler.cpp` into unique, interned pointers for high-speed lookups.

---

## 3. Symbol Management: Name Mangling & Registry

The JIT uses symbol names as the normative glue between the compiled machine code, the runtime library, and third-party FFI modules.

### **3.1 The Mangling Convention: Rationale & Structure**
HVM symbols must be flat, valid strings for the host's linker. The `SymbolMangler` (see `src/core/SymbolMangler.cpp`) ensures that every unique Hooc entity—functions, methods, types, and module state—maps to a deterministic, collision-free identifier.

#### **A. Function & Method Mangling (`_F_`)**
Format: `_F_ [ClassName] _ [BaseClassName] _ [Modifiers] _ [FunctionName] _ [ReturnType] _ [ParamTypes]`
- **Uniqueness**: By embedding the full signature (return type and parameters), the JIT supports **Function Overloading** at the binary level.
- **Inheritance Context**: The inclusion of `BaseClassName` allows the JIT to reconstruct the vtable hierarchy without needing the original source code.
- **Modifier Awareness**: Prefixes like `Pb` (Public) or `Pv` (Private) allow the JIT's `SymbolTableBuilder` to enforce access rules during the dynamic linking phase.

#### **B. Module-Level Mangling (`_H_`)**
Format: `_H_ [ModulePath] _ [SymbolName]`
- **Global State**: Used for global variables and constants. The JIT resolves these symbols to specific memory offsets within the module's `.data` or `.rodata` sections.
- **Initialization**: The reserved symbol `_F_module_init_v` is the HVM equivalent of a hardware **Reset Vector** or global constructor. The JIT executes this first to ensure the module's "Physical Environment" (globals and vtables) is ready.

#### **C. Escape Mechanisms**
To handle complex identifiers (e.g., file paths or special characters), the JIT uses the `E [hex] E` wrapping rule. This ensures that the structural delimiter `_` is never ambiguous, allowing the JIT to reliably "Demangle" symbols for the debugger's stack trace.

### **3.2 Component Encoding**
Since HVM symbol names must be flat, valid identifiers in the host's object format (e.g., ELF, Mach-O), any component containing non-alphanumeric characters (like `.`, `[`, `]`, `?`, or spaces) must be encoded.

#### **A. The Encoding Algorithm (`encodeString`)**
1.  **Safety Check**: If a character is `[a-zA-Z0-9]` or `_`, it is preserved.
2.  **Hex Escape**: Any other character is converted to `_XX_`, where `XX` is its lowercase two-digit hexadecimal ASCII value.
3.  **Wrapping**: If any character in a component was escaped, the entire component is prefixed with `E` and suffixed with `E`.
4.  **Empty Components**: An empty string (e.g., in a malformed path) is represented by a single underscore `_`.

#### **B. Concrete Examples**
| Raw Component | Mangled Representation | Note |
| :--- | :--- | :--- |
| `User` | `User` | Fully alphanumeric; no encoding. |
| `hoo.io` | `E686f6f2e696fE` | Contains `.`; hex encoded and wrapped in `E...E`. |
| `int64[]` | `E696e7436345b5dE` | Contains `[]`. |
| `String?` | `E537472696e673fE` | Contains `?`. |

#### **C. The Delimiter Rule**
Mangled symbols use the single underscore `_` as a structural delimiter between fields (e.g., between `ClassName` and `FunctionName`).
- **Conflict Resolution**: Because raw components are wrapped in `E...E` when they contain "unsafe" characters (including underscores that might look like delimiters), the JIT parser can unambiguously identify where one component ends and the next begins by scanning for the `_` delimiter outside of `E...E` blocks.

### **3.3 Function & Method Mangling: Module Scope & Demangling**
To ensure global uniqueness across a multi-binary JIT session, every callable symbol must be qualified by its module path. The JIT uses a multi-layered mangling strategy with unambiguous markers.

#### **A. The Qualified Symbol Format**
Format: `_F_ [M_ ModuleParts E_]? [ClassName?] _ [BaseClassName?] _ [Modifiers?] _ [FunctionName | CT | DT] _ [Signature]`

1.  **Module Path Marker**: If a module path is provided, it is wrapped in `M_` (Start) and `E_` (End). This allows the demangler to distinguish module components from class/function names regardless of depth.
2.  **Class Context**: If the function is a method, the `ClassName` (and `BaseClassName` for virtual resolution) follows the module end marker.
3.  **Signature**: The return type and parameters are appended to support overloading.

#### **B. Mangling Examples with Modules**
| Hooc Source | Logical Path | Mangled Symbol |
| :--- | :--- | :--- |
| `func: int add(a: int)` | `math.utils` | `_F_M_math_utils_E_add_i8_i8` |
| `constructor()` | `app.User` | `_F_M_app_E_User_CT_v` |
| `func: void save()` | `hoo.io` | `_F_M_hoo_io_E_save_v` |

#### **C. Demangling Logic (The Reversal Process)**
The JIT must be able to "Demangle" symbols for the HVM Inspector and stack traces. The process follows these technical steps:

1.  **Prefix Identification**: Strip `_F_` (Function) or `_H_` (Handle/Global).
2.  **Module Extraction**: If the first token is `M`, everything until the token `E` is extracted into the `modulePath` vector.
3.  **Name Categorization**: 
    - If 3 names remain: `className=N1, base=N2, functionName=N3`.
    - If 2 names remain: `className=N1, functionName=N2`.
    - If 1 name remains: `className=N1` (Legacy compatibility).
4.  **Type Decoding**: Translate short-codes (e.g., `i8`, `s`) to human-readable names (`int64`, `string`).

**Resulting Debug String**: `math.utils::add(int64) -> int64`

### **3.4 Type Coding Reference**
Types are compressed into deterministic short-codes:
- **Primitives**: `i1` (int8), `i8` (int64), `f` (float), `d` (double), `b` (bool), `s` (string), `v` (void).
- **Qualifiers**: `Q [hex-identifier] Z`.
    - *Example*: `hoo.Exception` -> `Q686f6f2e457863657074696f6eZ`.
- **Containers**:
    - **Array**: `A [type]`. *Example*: `int64[]` -> `Ai8`.
    - **Map**: `M [key] [value]`. *Example*: `map[string, int64]` -> `Msi8`.
    - **Nullable**: `O [type]`. *Example*: `string?` -> `Os`.

### **3.5 Global Instances & Constants**
Format: `_H_ [ModulePath] _ [SymbolName]`

#### **Examples:**
1.  **Module Constant**: `const PI = 3.14` in module `math`
    - Mangled: `_H_math_PI`
2.  **Qualified Global**: `var instance` in module `app.core`
    - Mangled: `_H_E6170702e636f7265E_instance`

### **3.6 Special Initialization Symbols**
To bootstrap the RISC environment, the JIT must identify and execute "System Hooks" emitted by the compiler. These symbols do not represent user-callable functions but are internal triggers for environmental setup.

#### **A. Module Initialization (`_F_module_init_v`)**
- **Role**: The "Reset Vector" of the module.
- **Tasks**: 
    - Allocates memory for module-level `var` and `const` fields.
    - Executes dynamic assignments (e.g., `var x = random()`).
    - Calls `_F_[ClassName]_vtable_init_v` for every class defined in the module.
- **JIT Logic**: The JIT must look up this symbol immediately after parsing the `.ho` file and before returning control to the caller.

#### **B. Class VTable Setup (`_F_[ClassName]_vtable_init_v`)**
- **Role**: Dynamically populates the Virtual Method Table.
- **Tasks**:
    1.  Allocates a host-memory block for the VTable (size derived from class metadata).
    2.  Copies parent vtable entries (if the class `EXTENDS` another).
    3.  Stores the physical host addresses of the class's methods into specific slots.
    4.  Stores the final VTable pointer in a global handle mangled as `_H_[ClassName]_vtable_ptr`.
- **HVM Implementation Example**:
```assembly
_F_User_vtable_init_v:
    # 1. Get host address of 'save' method
    LDA r1, _F_User_save_v
    # 2. Store in vtable slot 0
    LDA r2, _H_User_vtable_ptr
    ST.D r1, r2, 0
    RET
```

#### **C. Singleton Instance Placeholder (`_H_[ClassName]_instance`)**
- **Role**: A reserved memory slot for the unique instance of a `SINGLETON` class.
- **JIT Task**: The JIT ensures this slot is initialized to `NULL` during `module_init`. On the first `new` request, it populates this address.

#### **D. Execution Hierarchy**
The JIT enforces a strict "Bottom-Up" execution order for these symbols:
1.  **Dependency Initialization**: All imported modules' `_F_module_init_v` are executed.
2.  **VTable Generation**: All classes in the current module have their `_F_..._vtable_init_v` called.
3.  **Current Module Bootstrap**: Finally, the current module's `_F_module_init_v` completes the setup.

---

## 4. Module Loading: Understanding `.ho` (v1.4)

The JIT must use `src/hvm/HOModule.cpp` to parse HVM binaries.

### **4.1 Parsing Pass**
1.  **Header Verification**: Validate `MAGIC`, `VERSION_MAJOR` (1), and `VERSION_MINOR` (4).
2.  **Section Scanning**:
    - **.text**: Load raw bytecode.
    - **.symtab / .strtab**: Extract function names and offsets.
    - **.funcmeta**: Extract `local_size` and `param_count` for frame reconstruction.
    - **.rodata**: Map constant spills into a readable memory segment.

### **4.2 Module Initialization (`__hoo_init`)**
The `__hoo_init` reserved keyword (mangled as `_F_module_init_v` in HVM) is the critical bootstrap entry point for every Hooc module. Unlike the `main` function, which is the application entry point, `__hoo_init` is the **Module Entry Point**.

#### **A. Why is it required?**
Because HVM v1.4 is a pure RISC machine, it cannot "automagically" set up complex state. Many Hooc constructs require dynamic execution at load-time:
1.  **Dynamic Globals**: If a module defines `var x = Math.sqrt(2.0);`, the value of `x` cannot be statically baked into the `.data` section. It must be calculated at runtime.
2.  **VTable Registration**: Classes must register their method addresses into a global lookup table so that virtual calls (`JALR`) can find the correct implementation.
3.  **Library Handshakes**: FFI modules may need to initialize host-side handles or state.

#### **B. Example: Hooc to HVM Transition**
**Hooc Source:**
```hoo
// Module: math.core
var global_id = generateUniqueId();
const PI = 3.14159;
```

**Generated HVM Assembly for `_F_module_init_v`:**
```assembly
_F_module_init_v:
    ENTER 0
    # 1. Call generateUniqueId()
    CALL _F_generateUniqueId_i8
    # 2. Store result in global_id memory (r1 is return)
    LDA r2, _H_math_core_global_id
    ST.D r1, r2, 0
    # 3. Initialize constant (if not static)
    # ...
    LEAVE
    RET
```

#### **C. JIT Execution Workflow: Step-by-Step**
When the JIT is requested to load a primary `.ho` file, it must execute the following normative sequence to ensure a stable environment:

1.  **Stage 1: Recursive Discovery & Parsing**
    - The JIT parses the primary module using `HOModule::parse`.
    - It extracts the `SHT_IMPORT` list.
    - It recursively repeats this for every dependency until all required `.ho` files are in memory and an **Initialization DAG** (Directed Acyclic Graph) is built.

2.  **Stage 2: JITDylib & Symbol Registration**
    - For each module in the DAG, a unique `llvm::orc::JITDylib` is created.
    - The JIT scans each module's `SHT_EXPORT` section.
    - All mangled symbols are added to the respective `JITDylib`'s symbol table as **Deferred Symbols** (pointing to the HVM `.text` offset).

3.  **Stage 3: Topological Initialization (The Bottom-Up Rule)**
    - The JIT performs a **Topological Sort** of the DAG.
    - Starting from the modules with **zero imports** (the leaves), it invokes the `_F_module_init_v` function.
    - **Linking Phase**: Before calling `_F_module_init_v` for a module, all its imports must be cross-linked. If `Module A` imports `Module B`, the JIT ensures `A`'s dylib can search `B`'s dylib.

4.  **Stage 4: VTable & Global Setup**
    - Inside each `_F_module_init_v`, the JIT-compiled code:
        - Allocates memory for global variables in `.data`.
        - Calculates absolute addresses for vtable entries.
        - Stores function pointers (physical host addresses) into the vtable buffers.

5.  **Stage 5: Final Handover**
    - Once all initializers in the DAG have returned successfully, the JIT marks the primary module as "Executable".
    - The JIT is now ready to receive a lookup request for the application's `main` function (e.g., `_F_main_v`).

#### **D. Error Recovery**
If any `_F_module_init_v` triggers a hardware trap (`BREAK`) or a software exception (`hoo_exception_throw`), the JIT must:
1.  **Stop**: Immediately halt the initialization of the current module and its parents.
2.  **Unwind**: Call `hoo_release` on any globals partially initialized in the current frame.
3.  **Report**: Propagate the failure back to the CLI or host process, preventing the application from starting in an inconsistent state.

---

## 5. The Lowering Engine: HVM to LLVM IR

The JIT translates HVM instructions (using `src/hvm/HVMInstruction.cpp`) into LLVM IR.

### **5.1 State Mapping**
- **Registers**: Represent HVM `r0..r31` as an array of 32 `i64` values.
    - *Optimization*: Map frequently used registers (like `r1-r15`) to LLVM local values (SSA).
- **Memory**: Implement HVM memory as a raw `i8*` buffer. Use LLVM `load` and `store` instructions with explicit alignment checks.
- **r0 (Hardwired Zero)**: Ensure all IR generated for `r0` reads always results in a constant `0`.

### **5.2 Instruction Translation**
The JIT performs a 1-to-1 lowering of RISC primitives:
- **Arithmetic**: Map `ADD`, `SUB`, `MUL`, etc., directly to LLVM `add`, `sub`, `mul`.
- **Branches**: Map `BEQ`, `BNE`, `JMP` to LLVM `br`.
- **System Calls**: Map `SYSCALL` to a call into a native C++ "OS Wrapper" function.

### **5.3 ISA Translation Patterns (LLVM IR)**
The JIT uses the `llvm::IRBuilder` to generate SSA-form instructions. Below are the normative patterns for the HVM core:

#### **A. Register Access**
```cpp
// Mapping HVM r1 to LLVM Value
Value* reg_ptr = builder.CreateStructGEP(hvmStateStruct, hvmStatePtr, 1);
Value* r1 = builder.CreateLoad(builder.getInt64Ty(), reg_ptr, "r1");
```

#### **B. Arithmetic (Opcode 0x10)**
- **ADD**: `builder.CreateAdd(lhs, rhs)`
- **SUB**: `builder.CreateSub(lhs, rhs)`
- **MUL**: `builder.CreateMul(lhs, rhs)`
- **DIV**: `builder.CreateSDiv(lhs, rhs)` (requires zero-check trap)

#### **C. Conditional Branching (B-Format)**
```cpp
// beq r1, r2, offset
Value* cond = builder.CreateICmpEQ(r1, r2);
builder.CreateCondBr(cond, targetBlock, fallthroughBlock);
```

#### **D. Memory Operations (I-Format)**
```cpp
// ld.d rd, rs, imm15
Value* base_addr = getRegister(rs);
Value* offset = builder.getInt64(signExtend(imm15));
Value* final_addr = builder.CreateAdd(base_addr, offset);
Value* ptr = builder.CreateIntToPtr(final_addr, builder.getPtrTy());
Value* val = builder.CreateLoad(builder.getInt64Ty(), ptr);
setRegister(rd, val);
```

---

## 6. Module System & Linkage

### **6.1 Inter-Module Symbol Resolution & HVMModuleBundle**
The JIT environment utilizes the **`HVMModuleBundle`** (see `src/hvm/HVMModuleBundle.cpp`) as its central "System Registry". This component acts as the global linker and orchestrator for all loaded code units.

#### **A. The Role of `HVMModuleBundle`**
1.  **Global Symbol Table**: It maintains a thread-safe map of all exported symbols across all modules.
2.  **Cross-Dylib Bridge**: When the JIT encounters a `CALL` to an external symbol, it queries `HVMModuleBundle::findModuleBySymbolMangled()`.
3.  **Topological Orchestration**: It uses the `HoocModuleBase::resolveDependencyOrder()` logic to build the normative initialization sequence for the entire session.

#### **B. Module Sub-Types: Unifying Bytecode and Native Code**
The JIT treats all code units as subclasses of **`HOModuleBase`** (`src/hvm/HOModuleBase.cpp`), providing a unified interface for symbol resolution:

| Module Type | Implementation Class | Role in JIT |
| :--- | :--- | :--- |
| **Compiled** | `HOModule` | Loads HVM bytecode from `.ho` files. |
| **Static Runtime**| `StaticHOModule` | Wraps process-local C++ code (like `libhoort`). |
| **Dynamic Library**| `DynamicHOModule` | Wraps native OS binaries (`.so`, `.dll`) via `dlopen`. |

#### **C. Grammar Context: The Import Statement**
Consider the following Hooc source code:

```hoo
// Module: app.core
from math.geometry import calculateArea;

func: void main() {
    var area = calculateArea(10, 20);
}
```

#### **D. AST to Binary Transition**
1.  **Parsing**: The `fromImport` rule identifies `math.geometry` as the source module and `calculateArea` as the symbol.
2.  **Mangling**: `calculateArea` is mangled based on its signature (e.g., `_F_calculateArea_i8_i8_i8`).
3.  **HVM Generation**:
    - The compiler emits a `CALL` instruction with a placeholder offset.
    - An entry is added to the **`SHT_IMPORT`** section of `app.ho`:
        - `name`: `_F_calculateArea_i8_i8_i8`
        - `library`: `math.geometry`

#### **E. JIT Resolution Workflow**
When the JIT loads `app.ho`:
1.  **Dependency Scanning**: It reads `SHT_IMPORT` and identifies that it needs the symbol `_F_calculateArea_i8_i8_i8` from `math.geometry`.
2.  **Dynamic Loading**: If `math/geometry.ho` is not already in memory, the JIT uses `HOModule::parse` to load it.
3.  **Dylib Lookup**:
    - The JIT creates (or retrieves) an `llvm::orc::JITDylib` named `math.geometry`.
    - It performs a lookup: `ExecutionSession::lookup({ &math_geom_dylib }, "_F_calculateArea_i8_i8_i8")`.
4.  **Binding**: Once the physical address of the function is found, the JIT's `ObjectLinkingLayer` patches the `CALL` instruction in the machine code, replacing the placeholder with the actual relative offset or absolute address.

#### **Example: Qualified Module Call**
```hoo
import hoo.io;
func: void hello() {
    hoo.io.println("Hello");
}
```
- **Import Entry**: `name`: `_F_println_v_s`, `library`: `hoo.io`.
- **JIT Strategy**: The JIT prioritizes searching the **Runtime Dylib (`hoort`)** for any library starting with `hoo.` before searching the file system.

### **6.2 Third-Party & Foreign Function Invoke (FFI)**
The HVM JIT implements the FFI rules defined in the grammar, allowing Hooc code to consume native host libraries.

#### **A. Library Loading (`ffiImportDeclaration`)**
Syntax: `library "libname.so" (as alias)?;`
- **JIT Action**: The JIT uses `llvm::sys::DynamicLibrary::LoadLibraryPermanently(path)` to pull the native binary into the host process address space.
- **Alias Handling**: If an `alias` is provided, the JIT creates a scoped symbol table entry to prevent name collisions between different native providers.

#### **B. Dynamic Linkage (`ffiLinkDeclaration`)**
Syntax: `link dynamic modulePath (at version)? [searchPaths]?;`
- **JIT Action**: 
    1.  **Search**: Scans provided `searchPaths` for the requested module.
    2.  **Version Validation**: Checks the version metadata against the `versionRange`.
    3.  **Dylib Attachment**: Automatically creates an `llvm::orc::JITDylib` and populates it with the found symbols, ensuring they are searchable by the `ExecutionSession`.

#### **C. Native Function Mapping (`ffiNativeFunction`)**
Syntax: `extern native type identifier(ffiParams) -> type;`
- **Signature Reconstruction**: The JIT parses the `ffiType` to build an LLVM `FunctionType`.
- **Complex Type Marshalling**:
    - **`POINTER[T]`**: Map to `i8*` or the appropriate LLVM pointer type.
    - **`ARRAY[size] T`**: Passed as a pointer to the first element; the `size` metadata can be used by the JIT for bounds-check injection if requested.
    - **`FUNCTION(args) -> ret`**: Declares a native function pointer. The JIT generates a **Native-to-HVM Trampoline** if this function is passed back to a host library as a callback.

#### **D. Callback Support**
When a Hooc function is passed as a `FUNCTION` parameter to an `extern native` call:
1.  The JIT generates a "C-wrapped" version of the function.
2.  This wrapper sets up a temporary HVM register file on the stack.
3.  It invokes the JIT-compiled function.
4.  It marshals the result back to the host C return register.

### **6.3 Runtime Dependency Loading & Linkage**
The JIT environment distinguishes between **User Modules** (external `.ho` files) and **Built-in Modules** (the `hoo` standard library).

#### **A. User Module Linkage (.ho Files)**
The JIT implements a dynamic loader that consumes the `src/hvm/HOModule.cpp` API:
1.  **SHT_IMPORT Scan**: The loader reads the `SHT_IMPORT` section of the primary module.
2.  **Path Resolution**: It converts module paths (e.g., `app.network.protocol`) into file system paths (`app/network/protocol.ho`).
3.  **Dylib Mapping**: Each unique `.ho` file is mapped to its own `llvm::orc::JITDylib`. This ensures that module-level private symbols do not collide across the global session.
4.  **Recursive Bootstrap**: For every dependency, the JIT recursively executes Step 1, ensuring the entire Directed Acyclic Graph (DAG) of the application is in memory before execution.

#### **B. Built-in Module Redirection (`hoo.*`)**
HVM code often imports symbols from the standard library (e.g., `import hoo.io`). The JIT handles these via **Symbol Redirection**:
- **The "hoo" Virtual Dylib**: The JIT creates a special `JITDylib` named `hoo` that does not correspond to a `.ho` file.
- **Intrinsic Mapping**: When a symbol starting with `_F_hoo_` or an import from `hoo.*` is encountered, the JIT redirects the lookup to the statically linked **`libhoort.a`** symbols.

#### **C. Linkage Mechanics for Runtime Classes**
Based on the `src/runtime/lib` headers, the JIT must resolve specific symbol patterns to their native implementations:

| Hooc Symbol Pattern | Native `hoort` Entry Point | Implementation File |
| :--- | :--- | :--- |
| `_F_String_CT_...` | `hoo_string_from_cstr` | `hoo_string.cpp` |
| `_F_Array_CT_...` | `hoo_array_new` | `hoo_generic_array.cpp` |
| `_F_Map_CT_...` | `hoo_map_new` | `hoo_map.cpp` |
| `_F_Exception_CT_...`| `hoo_exception_create` | `hoo_exception.cpp` |

#### **D. VTable Patching for Inherited Modules**
For classes extending from different modules:
1.  The JIT identifies the `BaseClassName` from the mangled symbol (e.g., `_F_Admin_User_...`).
2.  It looks up the vtable of `User` in the parent `JITDylib`.
3.  It performs a **Recursive Merge** of the child class layout with the parent's vtable, ensuring that inherited methods point to the correct machine code addresses in the respective dependency modules.

### **6.4 Cross-Platform FFI: Marshalling & Calling Conventions**
When the JIT resolves an external `CALL` to a native third-party library, it must bridge the HVM 64-bit virtual ISA with the host OS's Application Binary Interface (ABI). This is achieved through **FFI Trampolines**.

#### **A. Register Mapping & Calling Conventions**
The JIT generator must map HVM virtual registers (`r1..r8`) to physical host registers.

| HVM Reg | System V (Linux/macOS x64) | Microsoft x64 (Windows) | ARM64 (macOS/Linux) |
| :--- | :--- | :--- | :--- |
| **r1 (Arg1/Ret)** | `RDI` / `RAX` (Ret) | `RCX` / `RAX` (Ret) | `X0` / `X0` (Ret) |
| **r2 (Arg2)** | `RSI` | `RDX` | `X1` |
| **r3 (Arg3)** | `RDX` | `R8` | `X2` |
| **r4 (Arg4)** | `RCX` | `R9` | `X3` |
| **r5 (Arg5)** | `R8` | (Stack) | `X4` |
| **r6 (Arg6)** | `R9` | (Stack) | `X5` |
| **r7 (Arg7)** | (Stack) | (Stack) | `X6` |
| **r8 (Arg8)** | (Stack) | (Stack) | `X7` |

**Host Stack Requirements**:
- **System V**: 16-byte stack alignment is mandatory before the `CALL`. For variadic functions (e.g., `printf`), the JIT must set `AL` (number of FP registers used).
- **Windows x64**: Requires 32 bytes of **Shadow Space** (spill area) allocated on the stack above the return address, even if no arguments are passed via stack.
- **ARM64**: Arguments are passed in `X0-X7`. The JIT must ensure `SP` is 16-byte aligned.

#### **B. Data Type Marshalling Details**
The JIT performs "Value Unwrapping" during the bridge sequence:

1.  **Strings (`HooString`)**:
    - **Inbound**: Host `char*` -> JIT calls `hoo_string_from_cstr` -> HVM `r1`.
    - **Outbound**: HVM `r1` -> JIT calls `hoo_string_data` (extracts raw pointer) -> Host `RDI/RCX`.
2.  **Arrays (`HooArray`)**:
    - **Outbound**: Native libraries cannot read HVM headers. The JIT passes `instance_ptr + 8` to provide a raw C-buffer.
3.  **Floating Point**:
    - Since HVM uses GP registers for doubles, the JIT must emit `MOVQ xmm0, r1` (x64) or `FMOV d0, x1` (ARM64) to move values into the host's floating-point registers.

#### **C. Struct Marshalling (The Complexity Layer)**
Native C-structs are handled based on their size and the host's "Classify" rules:

- **System V (Linux/macOS)**: 
    - Structs $\le$ 16 bytes are passed in two registers (e.g., `RDI` and `RSI`).
    - Structs $>$ 16 bytes are passed **By Reference**. The JIT allocates a temporary stack buffer, copies the HVM object fields into it, and passes the buffer address in `RDI`.
- **Windows x64**:
    - Structs of size 1, 2, 4, or 8 bytes are passed in a register.
    - All other sizes are passed **By Reference** (pointer to a copy).
- **ARM64**:
    - Small structs (Composite Types) are passed in `X0-X7` or `D0-D7` if they match the homogeneous floating-point/short-vector rules.

#### **D. Execution Protection**
The JIT must wrap FFI calls in a "Native Guard":
- **State Save**: Save all HVM callee-saved registers (`r16-r28`) and the Link Register (`r29`) to the stack.
- **Signal Shield**: On Linux/macOS, the JIT may mask certain signals (like `SIGALRM`) during the native call to prevent host-level interruptions from corrupting the virtual machine state.

### **6.5 Mapping Import Nodes to JIT Operations**
The JIT's **Module Loader** must directly translate `importStatement` nodes from the `Hooc.g4` grammar into symbol resolution strategies.

#### **A. Basic Imports (`IMPORT modulePath (AS alias)?`)**
Example: `import math.advanced as adv;`
1.  **Loader Action**: The JIT searches for `math/advanced.ho`.
2.  **Namespace Aliasing**: 
    - If `adv` is provided, the JIT creates a **Proxy Symbol Table**. 
    - Any subsequent call in HVM code like `CALL _F_adv_calculate_v` is internally redirected to `_F_math_advanced_calculate_v`.
3.  **Recursive Load**: If `math.advanced` is not yet in the `ExecutionSession`, it is parsed and registered as a new `JITDylib` immediately.

#### **B. Partial Imports (`FROM modulePath IMPORT items`)**
Example: `from math.core import PI, sqrt;`
1.  **Symbol Pinning**: Instead of making the entire `math.core` namespace available, the JIT only populates the current module's local symbol table with specific pointers for `PI` and `sqrt`.
2.  **Efficiency**: This allows the JIT to perform **Tree Shaking**. If only `sqrt` is imported, the JIT can avoid generating machine code for the rest of `math.core` if it hasn't been used elsewhere.

#### **C. Native Runtime Imports (`import hoo.*`)**
Nodes targeting the `hoo` namespace receive specialized handling:
- **Redirection**: The Loader identifies the `hoo` prefix and bypasses the file system search.
- **Process Lookup**: It uses `llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess()` to bind the import node directly to the `hoo_*` symbols already linked into the JIT host.
- **Header Synchronization**: The JIT ensures that the `type_id` and field offsets used in the HVM code match the C++ class definitions in `src/runtime/lib`.

#### **D. Initialization Ordering for Imports**
The JIT maintains an **Import Dependency Stack**. 
1.  When an `import` node is encountered, it is pushed onto the stack.
2.  The JIT ensures that the target module's `__hoo_init` is called before the current module's `import` processing completes.
3.  **Circular Import Guard**: If an import node targets a module already on the active dependency stack, the JIT must either resolve it via deferred symbols (lazy linking) or emit a "Circular Dependency" trap if the language strictly forbids it.

---

## 7. Runtime Library Integration (`hoort`)
The **Hooc Runtime Library (`hoort`)** is integrated as a core system component using a modular, object-oriented linkage model. The JIT leverages the **`HVMModuleBundle`** and **`HOModuleBase`** hierarchy to unify native code with HVM bytecode.

### **7.1 The Global Module Registry**
The JIT maintains a static instance of **`HVMModuleBundle`** (see `src/hvm/HVMModuleBundle.cpp`) which serves as the "System Bus" for all executable code units.

1.  **Unified Storage**: The `HVMModuleBundle::getModules()` instance holds a thread-safe collection of `std::shared_ptr<HOModuleBase>`.
2.  **Polymorphic Dispatch**: The bundle stores both HVM bytecode modules (`HOModule`) and specialized native modules (`StaticHOModule` derivatives).
3.  **Cross-Binary Linkage**: When a `CALL` target or `LDA` handle is not found in the local module, the JIT queries the `HVMModuleBundle` to resolve the physical address in a native runtime module.

### **7.2 Specialized Runtime Modules**
To represent the `hoort` standard library, the JIT defines a hierarchy of classes deriving from **`StaticHOModule`**. Each runtime class (String, Array, Map, etc.) is encapsulated in its own module instance.

#### **A. Implementation Pattern**
For each runtime component in `src/runtime/lib`, a dedicated bridge class is implemented:

```cpp
// Example: The String Runtime Bridge (HooStringModule.cpp)
class HooStringModule : public StaticHOModule {
public:
    HooStringModule() : StaticHOModule("hoo.String") {
        // 1. Register Native Functions
        registerFunction("new", (void*)&hoo_string_new, "_F_hoo_String_CT_p");
        registerFunction("from_cstr", (void*)&hoo_string_from_cstr, "_F_hoo_String_from_cstr_p_p");
        registerFunction("concat", (void*)&hoo_string_concat, "_F_hoo_String_concat_p_p_p");
        
        // 2. Register Global Constants
        static int64_t MAX_LEN = 0xFFFFFFFF;
        registerObject("MAX_LENGTH", &MAX_LEN, sizeof(int64_t), "i8", SymbolBinding::Global);
    }
};
```

#### **B. Injection and Bootstrap**
During JIT startup, these specialized modules are instantiated and injected into the global bundle:

```cpp
// JIT Initialization Sequence
void bootstrapRuntime() {
    auto& bundle = HVMModuleBundle::getModules();
    
    // Inject specialized hoort modules
    bundle.addModule(std::make_shared<HooStringModule>());
    bundle.addModule(std::make_shared<HooArrayModule>());
    bundle.addModule(std::make_shared<HooMapModule>());
    bundle.addModule(std::make_shared<HooExceptionModule>());
    
    // Inject the core 'hoo' system module for low-level intrinsics
    auto core = StaticHOModule::create("hoo");
    core->registerFunction("alloc", (void*)&hoo_alloc, "_F_hoo_alloc_p_i8_i8");
    core->registerFunction("retain", (void*)&hoo_retain, "_F_hoo_retain_p_p");
    core->registerFunction("release", (void*)&hoo_release, "_F_hoo_release_v_p");
    bundle.addModule(core);
}
```

### **7.3 Technical Implementation Details**
1.  **Symbol Resolution**: The JIT's IR generator uses `HVMModuleBundle::findModuleBySymbolMangled()` to obtain a `ModuleSymbol` pointer during `CALL` lowering.
2.  **Machine Code Generation**: If the `ModuleSymbol` contains a physical address (from a `StaticHOModule`), the JIT emits a direct LLVM `call` or `load` to that absolute address, bypassing standard PLT/GOT overhead.
3.  **VTable Integration**: Runtime classes that can be extended by HVM code (like `hoo.Exception`) have their base VTables pre-populated in the `StaticHOModule::registerObject` phase.

### **7.4 Memory Model & The 16-Byte Header**
All managed objects share a normative physical layout required for interoperability between JIT machine code and native `hoort` methods.

| Offset | Field | Type | Description |
| :--- | :--- | :--- | :--- |
| **-16** | `refcount` | `atomic<i64>` | Thread-safe ARC count. |
| **-8** | `type_id` | `i64` | Runtime type identifier for RTTI and GC. |
| **0** | **User Data** | (variable) | The address stored in HVM registers. |

### **7.5 Automatic Reference Counting (ARC) Rules**
The JIT must strictly adhere to the `libhoort` ownership model:
- **Assignment (`ST.D`)**:
    - `OLD_VAL = LD.D [addr]`
    - `CALL hoo_retain(NEW_VAL)`
    - `ST.D NEW_VAL, [addr]`
    - `CALL hoo_release(OLD_VAL)`
- **Parameter Passing**: HVM `r1..r8` are treated as **Borrowed References**. Native functions in `hoort` must not release parameters unless they explicitly take ownership.
- **Return Values**: Functions return an **Owned (+1)** reference in `r1`.
- **Cleanup**: The JIT parses `.funcmeta` and injects `hoo_release` calls in the function epilogue for all object-type locals.

### **7.6 Runtime Expansion & Contribution Guidelines**
When adding new features to `src/runtime/lib`:
1.  **Header Integrity**: Maintain strict `extern "C"` linkage for all entry points.
2.  **Derived Class**: Create a new class deriving from `StaticHOModule` (e.g., `HooNetModule`).
3.  **Signature Mapping**: Use the HVM mangled name format for all `registerFunction` calls.
4.  **Injection**: Add the new module to the `HVMModuleBundle` bootstrap sequence in the JIT host.

### **7.7 Type-Specific Native Bridges**
The JIT maps HVM `CALL` sequences to specific `hoort` entry points, maintaining the standard library namespaces defined in `hoo` and `hoo.io`. These bridges are categorized by the core type they manage.

#### **A. String Management (`hoo_string.h`)**
Strings in HVM are ARC-managed UTF-8 pointers.

| Hooc Operation | HVM Instruction | Native `hoort` Call | Note |
| :--- | :--- | :--- | :--- |
| `new String(s)` | `LDA`, `CALL` | `hoo_string_from_cstr(char*)` | Loads from `.rodata`. |
| `s1 + s2` | `MOV`, `CALL` | `hoo_string_concat(ptr, ptr)` | Returns a new handle. |
| `s.length()` | `MOV`, `CALL` | `hoo_string_length(ptr)` | Returns `i64`. |
| `s.toUpper()` | `MOV`, `CALL` | `hoo_string_to_upper(ptr)` | Returns a new handle. |

#### **B. Generic Arrays (`hoo_generic_array.h`)**
HVM arrays are type-agnostic and support automatic resizing.

| Hooc Operation | HVM Instruction | Native `hoort` Call | Note |
| :--- | :--- | :--- | :--- |
| `new Array()` | `CALL` | `hoo_array_new()` | Initial refcount = 1. |
| `arr.push(v)` | `MOV`, `CALL` | `hoo_array_push_int64(ptr, i64)` | Type-specialized bridge. |
| `arr.get(i)` | `MOV`, `CALL` | `hoo_array_get_int64(ptr, i64, &v)`| Out-parameter for value. |

#### **C. Map Module (`hoo_map.h`)**
Maps provide type-safe key-value storage with optimized native lookups.

| Hooc Operation | HVM Instruction | Native `hoort` Call | Note |
| :--- | :--- | :--- | :--- |
| `new Map(type)` | `MOV`, `CALL` | `hoo_map_new(i64)` | `type` is `HooMapKeyType`. |
| `m.set(k, v)` | `MOV`, `CALL` | `hoo_map_set_string_object(ptr, char*, ptr)` | For string-to-object. |

### **7.8 Runtime Intrinsic Requirements**
The JIT requires the following primitive functions to be available in the process space. The JIT generator maps register `r1..r8` to the first 8 arguments of these C-functions.

1.  **`void* hoo_alloc(i64 size, i64 type_id)`**:
    - Allocates `size + 16` bytes.
    - Sets `refcount = 1` and `type_id = type_id`.
    - Returns address of byte 16 (User Data).
2.  **`void hoo_retain(void* obj)`**:
    - Atomically increments refcount at `obj - 16`.
3.  **`void hoo_release(void* obj)`**:
    - Atomically decrements refcount.
    - If 0, calls the destructor (found via `type_id` lookup) and `free()`.

### **7.9 RTTI & Heap Segmentation**
The `type_id` stored in the object header is a unique 64-bit identifier used for **Runtime Type Information (RTTI)**.
- **JIT Usage**: When HVM code performs a `CATCH` check or an `instanceof` check, the JIT emits a load from `obj - 8` and compares it against the expected class ID.
- **Heap Safety**: The JIT can implement "Heap Zones" by checking the base address range before calling `release`, preventing ARC management of stack-allocated or static data.

---

## 8. Grammar-to-JIT Mapping: Handling High-Level Semantics

The JIT must strictly implement the semantics defined in `src/parsing/Hooc.g4`. While the backend lowers these to RISC, the JIT can leverage high-level metadata to optimize execution.

### **8.1 Class Modifiers & Lifecycle**
Class modifiers in `Hooc.g4` provide the JIT with structural metadata to apply specialized execution patterns and optimizations beyond basic RISC lowering.

#### **A. Life-Cycle Management (CT/DT)**
- **Constructors (`CT_`)**: 
    - **JIT Task**: Ensure every `newExpression` calls the correct `_F_..._CT_` symbol.
    - **Memory Setup**: The JIT-emitted code must call `hoo_alloc` first, then pass the returned pointer as the `this` (in `r1`) to the constructor.
- **Destructors (`DT_`)**:
    - **JIT Task**: The `_F_..._DT_` symbol is automatically invoked by `hoo_release` when a refcount hits zero.
    - **ARC Chain**: The destructor code must contain release calls for all object-type fields before calling `free()` on the instance.

#### **B. Structural Modifiers: Runtime Implementation Plans**

Class modifiers in `Hooc.g4` trigger specialized machine-code patterns and JIT-managed runtime structures.

---

##### **1. SINGLETON (Unique Instance Management)**
- **JIT Structure**: The JIT reserves an 8-byte slot in the module's `.data` section (mangled as `_H_[ClassName]_instance_ptr`).
- **Initialization Plan**:
    - The JIT wraps the constructor `_F_[ClassName]_CT_v` in a **Double-Checked Locking (DCL)** pattern.
    - **Step 1**: Check if `instance_ptr` is `NULL`.
    - **Step 2**: If `NULL`, enter a native mutex (synchronized via `hoort`).
    - **Step 3**: Re-check `NULL`; if still `NULL`, call `hoo_alloc`, then the `CT_` symbol, and store the result in `instance_ptr`.
- **Call-Site Optimization**: The JIT identifies any `new [Singleton]()` in HVM code and replaces the entire allocation/constructor sequence with a single `LD.D` from the `instance_ptr` address.

---

##### **2. IMMUTABLE (Write-Once Hardening)**
- **JIT Hardening**: After the constructor (`CT_`) returns, the JIT ensures the object state is frozen.
- **Implementation**:
    - **Soft Path**: The JIT-compiled code simply contains no `ST.D` (Store) instructions targeting this object outside the `CT_`.
    - **Hard Path (Page Protection)**: For large, page-aligned immutable objects, the JIT can use `mprotect(PROT_READ)` on the object's memory.
- **Optimization Plan**: The JIT marks all fields of an immutable object as **LLVM Constants**. This enables LLVM to propagate field values through complex call trees, potentially eliminating thousands of memory loads.

---

##### **3. FACTORY (Virtual Allocation Redirection)**
- **JIT Structure**: The JIT identifies that `new [FactoryClass]()` is a semantic alias for a generator.
- **Implementation**:
    - Instead of emitting a `hoo_alloc` call, the JIT redirects the instruction to a static "Generator" method (e.g., `_F_[FactoryClass]_create_v`).
    - This method is responsible for selecting the correct concrete subclass and performing the allocation.
- **Benefit**: Allows the JIT to implement **Dependency Injection (DI)** at the machine-code level without modifying the HVM bytecode.

---

##### **4. OBSERVABLE (Automatic Interception)**
- **JIT Instrumentation**: The JIT identifies "Observable" classes and instruments every `ST.D` (Store) instruction to the instance's fields.
- **Implementation Plan**:
    - For each field write, the JIT generates:
        1.  `OldValue = LD.D [field_addr]`
        2.  `ST.D NewValue, [field_addr]`
        3.  `CALL _F_[ClassName]_onPropertyChanged_v_s_i8_i8` (passes field name, old value, and new value).
- **Optimization**: If no observers are registered in the runtime, the JIT can "hot-patch" the code to remove the instrumentation and restore raw `ST.D` performance.

---

##### **5. SERVICE (System-Wide Discovery)**
- **JIT Registry**: The JIT maintains a global **Service Map** in the `ExecutionSession`.
- **Implementation**:
    - During `__hoo_init`, any class marked as `service` is instantiated and its pointer is registered under its mangled name in the map.
    - When HVM code invokes a service method, the JIT resolves the target by looking up the instance from the global map, ensuring that services remain long-lived and globally unique.

---

##### **6. STRATEGY (Dynamic Logic Swapping)**
- **JIT Structure**: Implemented using an **Indirect VTable Wrapper**.
- **Implementation Plan**:
    - The JIT generates an extra level of indirection for method calls.
    - Code: `reg = load [instance]; strategy_ptr = load [reg]; vtable = load [strategy_ptr]; call vtable[offset]`.
    - The JIT provides a native bridge (`hoo_set_strategy`) that can update the `strategy_ptr` at runtime, instantly changing the behavior of all call sites without re-JITing.

---

##### **7. ACTOR (Isolated Execution Contexts)**
- **JIT Transformation**: The JIT transforms direct method calls into **Asynchronous Message Passing**.
- **Implementation Plan**:
    - **Mailbox**: Each Actor instance is assigned a concurrent queue (provided by `hoort`).
    - **Invocation**: `actor.doWork(x)` is lowered to:
        1.  Pack `x` and the method ID into a "Message Struct".
        2.  `CALL hoo_actor_enqueue(instance_ptr, message_ptr)`.
    - **Execution**: The JIT-compiled method bodies are only ever invoked by the **Actor Scheduler Thread**, ensuring that no two methods execute simultaneously on the same instance (lock-free concurrency).

---

##### **8. FINAL (Speculative Devirtualization)**
- **JIT Optimization**: Since `final` classes cannot be extended, the JIT can perform **Direct Linking**.
- **Implementation**:
    - The JIT identifies virtual method calls (`JALR`) targeting a `final` class.
    - It replaces the vtable lookup and indirect jump with a **Direct CALL** to the known physical address of the implementation.
    - **Inlining**: By devirtualizing the call, the JIT enables the LLVM optimizer to inline the method body directly into the caller, resulting in significant performance gains for core library functions.

---

### **8.2 Block Semantics & Deterministic Cleanup**
The HVM JIT must ensure that lexical block boundaries defined in `Hooc.g4` are respected to maintain deterministic memory and resource management.

#### **A. Lexical ARC Management**
Every block `{ ... }` represents a lifecycle region.
- **Owning Locals**: The JIT tracks which registers or stack slots hold "Owning Pointers" (objects with a +1 refcount).
- **Automatic Release**: Upon reaching the closing brace `}`, the JIT must inject a sequence of `hoo_release` calls for all owning locals defined within that specific block.

#### **B. The SCOPE Statement (`scope { ... }`)**
The `scope` keyword provides a hint for aggressive local optimization.
- **Escape Analysis**: If the JIT determines that an object allocated within a `scope` block does not "escape" (is not stored in a global, passed to a non-analyzable function, or returned), it should **Stack-Allocate** the object using LLVM's `alloca` instead of calling `hoo_alloc`.
- **Eager Reclamation**: Registers used for intermediate objects within a `scope` are prioritized for reclamation immediately after their last use, even before the block ends.

#### **C. Control-Flow Cleanup (The "Unwind-on-Jump" Rule)**
When a statement breaks the linear flow of a block, the JIT must ensure all active lifecycles are terminated.
- **`break` / `continue`**: Before jumping to the loop's start or end, the JIT must inject `hoo_release` calls for all locals in the blocks being exited.
- **`return`**: Before the `LEAVE` and `RET` sequence, the JIT must release all local variables in the entire function's lexical stack.
- **Implementation**: The JIT maintains a **Cleanup Stack** during IR generation. Each entry contains the list of `hoo_release` calls for a block. When a jump occurs, the JIT "replays" the releases from the current depth up to the target depth.

#### **D. Exception Blocks (`try/catch/finally`)**
Exception handling is implemented through a combination of the **Shadow Stack** and **Cleanup Landing Pads**.
- **`try`**: Registers the catch/finally blocks via `hoo_push_handler`.
- **`finally`**: This block is treated as a "Must-Execute" landing pad.
    - **Normal Flow**: The JIT generates code to execute the `finally` block before exiting the `try` or `catch`.
    - **Exception Flow**: The `hoo_exception_throw` logic must identify the `finally` address, execute it, and then re-trigger the unwinding if no `catch` matched.
- **`catch`**: The JIT generates a comparison between the thrown exception's `type_id` and the requested type. If it matches, the exception handle is moved from `r1` into the catch variable's local slot.

### **8.3 Advanced FFI & Type Marshalling**
The JIT must support the complex `ffiType` rules:
- **POINTER[T]**: Map to a raw `i64` in HVM, but treat as `T*` when calling host functions.
- **FUNCTION(args) -> ret**: This represents a **Function Pointer**. 
    - **Inbound**: When a host library provides a callback, the JIT wraps it in a trampoline that sets up the HVM register state.
    - **Outbound**: When passing a JIT'd function to the host, the JIT must generate a native C-compatible wrapper.

### **8.4 Class Variable & Function Dispatch**
The JIT's behavior for member access and method invocation varies significantly depending on the class kind and member scope.

#### **A. Memory Layout & instance Variables**
Regardless of the class kind, the physical layout of an instance in memory (at offset 0) is:
- **Offset 0**: `VTable Pointer` (i64, physical host address).
- **Offset 8+**: Instance fields in the order defined in the grammar.

**JIT Behavior**:
- **Field Access**: Lowered to `LD.D rd, [r1 + offset]` (where `r1` is the `this` pointer).
- **Bounds Checking**: For `IMMUTABLE` classes, the JIT can elide re-loading fields into registers within a loop if it can prove no external calls modify the memory.

#### **B. Static Members (Class Variables)**
Hooc treats static variables as module-level symbols owned by the class namespace.
- **Mangling**: `_H_ [ClassName] _ [VariableName]`
- **JIT Behavior**: These are resolved as **Global Constants/Variables**. They are NOT part of the instance heap allocation. The JIT resolves these to absolute addresses in the `.data` segment.

#### **C. Function Dispatch by Class Kind**

| Class Kind | Dispatch Mechanism | Register Convention |
| :--- | :--- | :--- |
| **Normal** | **VTable Lookup** | `r1` = this, `r2-r8` = args. |
| **Final** | **Direct Link** | `r1` = this. No vtable lookup. |
| **Singleton**| **Static Pointer** | JIT loads `_H_..._instance` into `r1` automatically at call-site. |
| **Actor** | **Async Message** | `r1` = this. JIT returns control to caller immediately; work is queued. |
| **Strategy** | **Indirect VTable**| `vtable_ptr` is loaded from a "Strategy Handle" to allow runtime swapping. |

#### **D. Specialized Implementation Details**

1.  **Actor "Mailbox" Variables**:
    - For `ACTOR` classes, instance variables are effectively "private" to the actor's thread. 
    - **JIT Enforcement**: If HVM code attempts to read an actor's field from a non-actor thread, the JIT must inject a trap or a synchronous "Request-Response" message loop.

2.  **Singleton/Service Instantiation**:
    - The JIT identifies that `new Singleton()` is an idempotent operation.
    - It replaces the instantiation with a `CALL` to a "Getter" function that returns the cached instance pointer.

3.  **Inheritance & Overloading**:
    - The JIT uses the mangled signature (including `ParamTypes`) to perform **Static Overload Resolution**.
    - If a method is virtual, the JIT populates the class's VTable during `_F_[ClassName]_vtable_init_v` by inheriting parent pointers and overwriting them with overridden child implementations.

### **8.5 Exception Runtime Infrastructure**
The HVM JIT treats exception handling as a specialized "System Trap" mechanism. It must bridge the gap between physical hardware signals and high-level language recovery.

#### **A. Trap Interception & Conversion**
The JIT-generated code must be able to convert physical processor traps into managed `HooException` objects:
- **Division by Zero**: The JIT must either inject a manual check before `DIV`/`REM` instructions or install a host signal handler (`SIGFPE`). On a trap, it calls `hoo_exception_division_by_zero()`.
- **Null Reference**: The JIT can leverage the host's MMU. If an `LD.D` or `ST.D` targets address `0`, the resulting `SIGSEGV` is caught by the JIT runner and converted to a `hoo_exception_null_pointer()` throw.
- **Index Out of Bounds**: Injected IR for `LDELEM`/`STELEM` checks the array length header. If the check fails, it calls `hoo_exception_index_out_of_bounds()`.

#### **B. The Shadow Stack & Unwinding**
To support non-local jumps across function boundaries without complex DWARF tables, the JIT maintains a **Per-Thread Shadow Stack**:
1.  **Registration**: Every `try` block calls `hoo_push_handler(catch_pc)`. The JIT also pushes a snapshot of the current **Virtual Frame Pointer (`r30`)** and **Stack Pointer (`r31`)**.
2.  **Unwinding**: When `hoo_exception_throw` is called:
    - The runtime pops the shadow stack to get the nearest handler.
    - It uses the saved `r30`/`r31` to "snap" the HVM stack back to the state it was in at the start of the `try` block.
    - It sets `r1` to the exception handle and performs a `JMP` to the `catch_pc`.

#### **C. The 'Finally' Re-throw Sequence**
The JIT ensures the `finally` block runs even if an exception is unhandled in the `catch` block:
- **The Hidden Flag**: The JIT reserves a hidden local boolean `is_exception_active`.
- **Sequence**:
    1.  At the end of a matched `catch`, `is_exception_active` is cleared.
    2.  The `finally` block executes.
    3.  After `finally`, the JIT checks the flag. If still active, it calls `hoo_rethrow`.

#### **D. FFI Boundary Safety**
The JIT must provide "Safe Bridges" for native calls:
- **Outbound**: When calling a native library, the JIT wraps the call in a `try...catch` block (C++ level) to prevent native crashes or `longjmp` from corrupting the HVM state.
- **Inbound**: If a native library calls a JIT'd callback, the JIT must install a new shadow stack anchor to prevent the callback's `throw` from attempting to unwind through non-JIT'd native frames.

---

## 9. Attachable Debugger Implementation

To support professional development, the JIT must provide hooks for a debugger (either a native host debugger like LLDB or a custom HVM-specific CLI).

### **9.1 Exposing JIT Symbols to Host Debuggers**
By default, JIT'd code is "invisible" to tools like `gdb` or `lldb`. The JIT must use LLVM's **GDB Registration Listener** to expose its memory-mapped objects.
- **Implementation**: Register an `llvm::JITEventListener` (e.g., `createGDBRegistrationListener()`) with the `ObjectLinkingLayer`.
- **Result**: When the JIT compiles a function, it emits a temporary ELF/Mach-O object in memory. The listener notifies the OS, allowing a native debugger to show function names in stack traces.

### **9.2 Debug Metadata (DWARF) Generation**
For source-level debugging (stepping through `.hoo` files), the JIT must generate DWARF metadata using the `llvm::DIBuilder`.

#### **A. DIBuilder Initialization**
1.  **DICompileUnit**: The JIT creates one `DICompileUnit` per `JITDylib`, specifying the source language as `DW_LANG_C_plus_plus_11` (or a custom identifier) and the producer as "HVM-JIT".
2.  **DIFile**: Every `.ho` module's source path (from metadata) is used to create a `DIFile` node.

#### **B. Function & Scope Mapping**
1.  **DISubprogram**: For every mangled function (e.g., `_F_main_v`), the JIT creates a `DISubprogram`. 
    - **Line Numbers**: The start line is retrieved from `.funcmeta::source_line`.
    - **Flags**: Marked as `DISPFlagDefinition`.
2.  **DILexicalBlock**: Nested blocks (loops, `scope` blocks) are mapped to `DILexicalBlock` nodes to allow the debugger to restrict variable visibility.

#### **C. Variable & Register Mapping**
To show "Local Variables" in the debugger:
1.  **Registers**: The JIT generates `DIVariable` entries for HVM registers `r1..r31`.
2.  **Location Expressions**: Use `llvm::DIExpression` with `DW_OP_regX` (where `X` is the host register mapped to the HVM register) or `DW_OP_fbreg` (if the register is spilled to the stack).
3.  **DILocation**: Every generated host instruction is tagged with a `DILocation` identifying the HVM byte-offset and original Hooc line/column.

---

### **9.3 Breakpoint Mechanics**
The JIT supports both hardware-triggered and user-defined software breakpoints.

#### **A. User Software Breakpoints (Instruction Patching)**
When a user sets a breakpoint in a custom HVM debugger, the JIT performs **Dynamic Patching**:
1.  **Capture**: The JIT saves the original 4-byte HVM instruction at the target offset.
2.  **Patch**: It overwrites the instruction with a **Native Trap**:
    - **x86_64**: `INT3` ($0xCC$).
    - **ARM64**: `BRK #0`.
3.  **Trigger**: When the physical CPU hits the patch, it generates a `SIGTRAP`.
4.  **Resumption**:
    - The debugger catches the signal.
    - It restores the original instruction in memory.
    - It sets the host PC back to the start of the instruction (e.g., x86_64 `INT3` leaves PC 1 byte ahead; JIT must decrement `RIP`).
    - It executes a **Single Step** (`PTRACE_SINGLESTEP`).
    - It re-applies the patch to maintain the breakpoint.

#### **B. HVM Hardware Breakpoint (`BREAK` - 0xC1)**
The `BREAK` opcode is used for "Hardcoded" break-points (e.g., `__debugbreak()` in code).
- **JIT Lowering**: The translator always lowers Opcode `0xC1` to the host's native trap instruction.
- **State Preservation**: Before the trap, the JIT ensures all HVM registers are synchronized to their designated memory or register slots, allowing the debugger to present a consistent state.

#### **C. Conditional Breakpoints**
The JIT can optimize conditional breakpoints by generating **Sentinel Branches**:
- Instead of a trap, the JIT generates: `if (condition) { NativeTrap; }`.
- This allows high-speed execution until the specific debug condition is met, bypassing the expensive kernel signal cycle.

---

### **9.4 The HVM Inspector API**
For custom HVM debuggers, the JIT runner should expose a thread-safe "Inspector" interface:
- **`stopExecution()`**: Sets a "Stop" bit checked at every HVM backward branch or `CALL`.
- **`getRegisters(threadId)`**: Returns a snapshot of the 32 virtual registers.
- **`readVirtualMemory(addr, size)`**: Maps HVM virtual addresses to the host process buffer.

---

## 10. Performance Strategy: Surpassing JVM/CLR/Python

The HVM JIT is architected to exceed the performance of traditional virtual machines (JVM, .NET CLR, CPython) by eliminating high-level VM overhead and aligning strictly with physical hardware behaviors.

### **10.1 Hardware-Aligned ISA vs. Stack Machines**
- **The JVM/CLR Bottleneck**: Java and .NET use "Stack Machines" (push/pop). Every high-level instruction requires multiple memory/stack operations before execution.
- **The HVM Advantage**: HVM v1.4 is a **Register Machine**. The JIT maps HVM registers directly to physical CPU registers (RAX, RDI, etc.). There is **zero stack overhead** for basic arithmetic and data movement.

### **10.2 Deterministic ARC vs. Non-Deterministic GC**
- **The GC Pause Problem**: JVM and .NET suffer from "Stop-the-World" Garbage Collection pauses, which degrade latency.
- **The HVM Advantage**: By using **Automatic Reference Counting (ARC)** with **ARC Elision**, memory is reclaimed the instant an object goes out of scope. 
    - **Performance Gain**: No background collector threads competing for cache; perfect cache locality for short-lived objects.

### **10.3 Aggressive AOT/JIT Hybrid Model**
Unlike Python (interpreter-heavy) or Java (heavily dependent on warm-up), the HVM JIT uses an **AOT-First** approach:
1.  **Static Lowering**: `hooc` performs heavy lifting (offset calculation, scaling) at compile-time.
2.  **Binary Compatibility**: The `.ho` format is a pre-linked object file. The JIT's "warm-up" time is near zero because the IR generation is a simple 1-to-1 mapping of RISC primitives.

### **10.4 Instruction Elision & Inlining**
The JIT leverages LLVM's O3 optimizer with HVM-specific hints:
- **Devirtualization**: Because the JIT knows the entire class hierarchy from `.ho` metadata, it converts virtual calls into direct jumps far more aggressively than the JVM.
- **Bounds Check Elision**: For loops using `FOR IN` on arrays, the JIT proves that the index cannot exceed the header-stored length and removes the safety checks entirely from the machine code.

### **10.5 Zero-Cost FFI**
- **The Python/Java FFI Tax**: Calling C from Python or Java involves heavy marshalling and "Global Interpreter Lock" (GIL) transitions.
- **The HVM Advantage**: Since HVM follows physical calling conventions, the FFI bridge is a **Raw Jump**. The JIT generates a few `MOV` instructions to align registers and then executes a physical `CALL`. There is no "VM-to-Native" context switch.

---

## 11. Future Expansion

### **10.1 Tiered Compilation**
Implement a dual-execution model:
- **Tier 1 (Baseline)**: A non-optimizing interpreter for immediate execution.
- **Tier 2 (JIT)**: LLVM's `MCJIT` or `ORC` provides aggressive optimization for hot code paths identified via call-site profiling.

### **10.2 SIMD & Vectorization**
Future `ext.simd` instructions will map to LLVM's `fixed-width vector` types, allowing the JIT to automatically utilize AVX-512 or Neon instructions.

### **10.3 Hardware-Level Emulation (MMU/Interrupts)**
- **MMU**: Inject bounds-checking IR before memory operations to simulate protected memory spaces.
- **Interrupts**: Insert "poll" IR sequences at the start of loops to allow the JIT to handle asynchronous signals or thread preemption.

---

## 11. Implementation Roadmap

This roadmap defines a multi-phase technical execution plan to build the HVM JIT from foundational loader to a high-performance optimized engine.

### **Phase 1: Binary Loader & Environment Setup**
- [ ] **HOModuleLoader Implementation**
    - [ ] Implement `HeaderValidator` (Verify Magic `0x484F4F43`, Version 1.4, Endianness).
    - [ ] Implement `SectionExtractor` for `.text`, `.data`, and `.rodata`.
    - [ ] Map `.rodata` to a read-only memory segment for immediate `LDA` constant access.
- [ ] **Symbol & Metadata Registry**
    - [ ] Build `SymbolTableBuilder` to decode `SHT_SYMTAB` and integrate with `SymbolMangler`.
    - [ ] Implement `ExportRegistry` to expose public symbols for cross-module linkage.
    - [ ] Parse `.funcmeta` to cache `local_count` and `param_count` for every function.
- [ ] **JITDylib & Dependency Management**
    - [ ] Implement `JITDylibManager` to map each `.ho` file to an isolated ORC unit.
    - [ ] Configure `SearchOrder` (Internal -> Dependencies -> hoort -> Process).
    - [ ] Implement `TopologicalSorter` to build the `SHT_IMPORT` Directed Acyclic Graph (DAG).

### **Phase 2: Core ISA Lowering (HVM -> LLVM IR)**
- [ ] **Register File & State Mapping**
    - [ ] Define `HVMState` struct containing 32 `i64` members.
    - [ ] Implement `r0` Hard-Zero logic in `readRegister()` (all reads return constant 0).
    - [ ] Create `VirtualRegisterAllocator` to map HVM registers to LLVM SSA values.
- [ ] **Instruction Translation Engine**
    - [ ] Implement `ArithmeticTranslator` for Opcode `0x10` (ADD, SUB, MUL, DIV).
    - [ ] Implement `ShiftTranslator` for Opcode `0x13` (SHL, SHR, SAR).
    - [ ] Handle 15-bit sign-extension logic for `I` and `B` format immediates.
    - [ ] Implement `0xFE` Escape Decoder for extended opcodes (`SYSCALL`, `BREAK`).
- [ ] **Control Flow & Branching**
    - [ ] Implement `BasicBlockMap` to track machine-code offsets to LLVM BasicBlocks.
    - [ ] Map `BEQ`, `BNE`, `BLT`, `BLE` to conditional `br` instructions.
    - [ ] Handle 20-bit relative word-offsets for `JMP`, `JAL`, and `CALL`.
- [ ] **Memory Engine**
    - [ ] Create `AddressTranslator` (HVM 64-bit virtual -> host physical pointer).
    - [ ] Implement `LoadStoreHandler` for all widths (`LD.B`, `LD.H`, `LD.W`, `LD.D`).
    - [ ] Inject 8-byte alignment checks for `LD.D` and `ST.D`.

### **Phase 3: Runtime Bridge & Intrinsics**
- [ ] **Native Linkage (`hoort`)**
    - [ ] Integrate `StaticLibrarySearchGenerator` to link against `libhoort.a`.
    - [ ] Export `hoo_*` symbols into the global `ExecutionSession`.
    - [ ] Implement `RuntimeSymbolResolver` to verify presence of mandatory intrinsics.
    - [ ] Map HVM mangled names (e.g., `_F_hoo_alloc_p_i8_i8`) to raw C++ function pointers.
- [ ] **Allocation & ARC Intrinsics**
    - [ ] Implement `AllocationEmitter`: Generate IR for `hoo_alloc(size, type_id)`.
    - [ ] Implement `ARCInverter` logic for `ST.D`:
        - [ ] Emit `LD.D` to fetch existing pointer (the "to-be-overwritten" value).
        - [ ] Emit `hoo_retain(NEW_VAL)` with NULL-check guard.
        - [ ] Emit physical `store` to HVM memory.
        - [ ] Emit `hoo_release(OLD_VAL)` with NULL-check guard.
    - [ ] Implement "Autorelease" landing pads for expression-level temporaries.
- [ ] **Exception & Stack Management**
    - [ ] Implement `ShadowStackManager`: Manage a per-thread `std::vector` of frame snapshots.
    - [ ] Create `PushHandlerEmitter`:
        - [ ] Snapshot host `RBP/RSP` and virtual `r30/r31`.
        - [ ] Register the `catch` block offset on the shadow stack.
    - [ ] Implement `UnwindEmitter`:
        - [ ] Call `hoo_exception_throw`.
        - [ ] Restore host context and perform a tail-call or jump to the handler PC.
- [ ] **Core Type Infrastructure**
    - [ ] String Bridge: Link `hoo_string_from_cstr` and `hoo_string_concat`.
    - [ ] Array Bridge: Implement `ArrayIndexEmitter` with 8-byte header-skip logic.
    - [ ] Map Bridge: Link type-specialized native entry points (e.g., `hoo_map_set_string_i8`).

### **Phase 4: Module Bootstrap & Initialization**
- [ ] **Module Entry Point Executor**
    - [ ] Implement `PostLoadInitializer` to automatically invoke `_F_module_init_v`.
    - [ ] Enforce post-order execution based on the `TopologicalSorter` DAG.
- [ ] **VTable Construction**
    - [ ] Implement `VTableManager` to allocate memory for class virtual tables.
    - [ ] Populate vtable slots with physical host addresses of child implementations.
    - [ ] Perform recursive parent-vtable merging for `EXTENDS` support.
- [ ] **Static Memory Setup**
    - [ ] Allocate and zero-initialize `.data` and `.bss` segments for each module.
    - [ ] Once-Only Guards: Implement thread-safe `std::call_once` flags for every module.

### **Phase 5: FFI & Multi-Binary Linkage**
- [ ] **Dynamic Library Integration**
    - [ ] Implement `LibraryManager` to handle `LIBRARY` and `LINK DYNAMIC` nodes.
    - [ ] Configure `DynamicLibrarySearchGenerator` for host `.so`/`.dylib` files.
- [ ] **ABI Trampoline Generation**
    - [ ] Implement `TrampolineGenerator` to map HVM registers to `System V` and `Win64` ABIs.
    - [ ] Handle `XMM/D` register mapping for floating-point parameter passing.
    - [ ] Implement `WindowsShadowSpace` reservation on the host stack.
- [ ] **Callback & Type Marshalling**
    - [ ] Implement `InboundTrampoline` to wrap JIT functions for native library callbacks.
    - [ ] Inject `hoo_string_data` IR to unwrap `HooString` handles to `char*`.

### **Phase 6: Optimization & Hardening**
- [ ] **Advanced ARC Optimization**
    - [ ] Implement `ARCUseDefGraph` to elide redundant `retain/release` pairs.
    - [ ] Perform **Escape Analysis** to promote heap allocations to stack `alloca`.
- [ ] **Execution Hardening**
    - [ ] Implement **Speculative Devirtualization** for `final` and `singleton` classes.
    - [ ] TLAB Integration: Implement Thread-Local Allocation Buffers for lock-free `hoo_alloc`.
- [ ] **Tooling & Debugging**
    - [ ] Implement **DWARF Generation** using `DIBuilder` for source-level debugging.
    - [ ] Register `llvm::JITEventListener` to expose symbols to `gdb` and `lldb`.
    - [ ] HVM Inspector API: Expose register snapshots and step-by-step execution control.
