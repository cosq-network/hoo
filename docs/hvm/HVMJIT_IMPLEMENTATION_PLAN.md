# HVMJIT Technical Implementation Plan

This document defines the technical strategy for implementing the **`HVMJIT`** class, a high-fidelity Dynamic Binary Translator that executes **HVM v1.4 (Hardware Ready)** binaries using the LLVM ORC JIT v2 infrastructure.

---

## 1. Architectural Role

Unlike `HoocJIT` (which compiles Hooc source), **`HVMJIT`** is a **System Emulator**. It operates exclusively on physical HVM primitives:
1.  **Input**: `HOModule` (parsed from `.ho` binaries).
2.  **Process**: Dynamic translation of RISC instructions to host-native machine code via LLVM IR.
3.  **State**: Explicit management of a 32-register 64-bit file and a flat memory model.
4.  **Goal**: Execute HVM binaries with near-native performance while maintaining 100% architectural parity with physical silicon.

---

## 2. Core Components & Dependencies

| Component | Role | File Reference |
| :--- | :--- | :--- |
| **`HOModule`** | Binary container and metadata source. | `src/hvm/HOModule.h` |
| **`HVMInstruction`** | Instruction decoder (Opcodes/Operands). | `src/hvm/HVMInstruction.h` |
| **`HVMModuleBundle`** | Global registry for cross-binary linkage. | `src/hvm/HVMModuleBundle.h` |
| **`IOProvider`** | Abstraction for `SYSCALL` and system services. | `src/core/IOProvider.h` |
| **`llvm::orc::LLJIT`** | Underlying ORC JIT engine. | LLVM 15+ |

---

## 3. The `HVMJIT` Class Structure

```cpp
namespace hooc {

class HVMJIT {
public:
    HVMJIT(IOProvider& io);
    ~HVMJIT();

    // High-Level Execution
    bool loadModule(const std::string& path);
    bool loadModule(std::unique_ptr<hvm::HOModule> module);
    int64_t run(const std::string& entryPoint = "_F_main_v");

    // Symbol Management
    void* getSymbolAddress(const std::string& mangledName);

private:
    // Internal Translation Logic
    llvm::Expected<llvm::orc::ThreadSafeModule> translateModule(hvm::HOModule& hvmModule);
    void translateFunction(hvm::HOModule& hvmModule, const hvm::Symbol& sym, llvm::IRBuilder<>& builder);
    
    // Physical State Mapping
    struct HVMState {
        int64_t regs[32];      // Virtual Register File
        uint8_t* memory;       // Flat memory buffer
        IOProvider* io;        // System services
    };

    // LLVM Infrastructure
    std::unique_ptr<llvm::orc::LLJIT> jit_;
    llvm::orc::ThreadSafeContext tsc_;
    IOProvider& io_;
    hvm::HVMModuleBundle bundle_;
};

} // namespace hooc
```

---

## 4. Implementation Strategy: The Lowering Engine

### **4.1 State Mapping (Physical-to-Virtual)**
- **Registers**: `r0..r31` are mapped to a `StructType` in LLVM. The JIT maintains a physical pointer to the `HVMState` for each thread.
- **r0 Logic**: The `readRegister(0)` helper always returns an `llvm::ConstantInt(0)`. Writes to `r0` are ignored.
- **Program Counter (PC)**: Not explicitly a register. The JIT uses LLVM's control flow (Basic Blocks) to represent jumps.

### **4.2 Instruction Translation Loop**
For every function in the `.text` section:
1.  **Block Mapping**: Scan the instruction stream to identify jump targets and create corresponding `llvm::BasicBlock` nodes.
2.  **Decoding**: Iterate through instructions using `HVMInstruction::decode`.
3.  **IR Generation**:
    - **Arithmetic**: Map `ADD`, `SUB`, `MUL` to `builder.CreateAdd`, etc.
    - **Memory**: Map `LD.D`/`ST.D` to `builder.CreateLoad`/`Store` with a GEP (GetElementPtr) based on the HVM memory pointer.
    - **Branching**: Map `BEQ`, `BNE` to `builder.CreateCondBr`.
    - **System Calls**: Map `SYSCALL` to a native C function call: `int64_t hvm_handle_syscall(HVMState* state, int16_t id)`.

### **4.3 Multi-Module Linkage**
- **Import Handling**: For every entry in `SHT_IMPORT`, the JIT registers a **Deferred Symbol**.
- **Resolution**: When a `CALL` targets an external symbol, the JIT queries the `HVMModuleBundle`.
- **Topological Bootstrapping**: `HVMJIT` must execute `_F_module_init_v` for all dependencies in post-order before executing the primary entry point.

---

## 5. Phase-by-Phase Implementation

### **Phase 1: Binary Loader (Week 1)**
- Implement `loadModule` using `HOModule::parse`.
- Integrate with `HVMModuleBundle` to track multiple `.ho` units.
- Implement the topological sort for `SHT_IMPORT` sections.

### **Phase 2: Translation Foundation (Week 2)**
- Build the `HVMState` IR mapping.
- Implement translation for Data Movement (`MOV`, `LUI`, `ADDI`) and Integer Arithmetic (`ADD`, `SUB`, `MUL`).
- Verify with "Leaf Functions" (no calls or branches).

### **Phase 3: Control Flow & Memory (Week 3)**
- Implement the BasicBlock mapper for `PC` relative jumps.
- Implement Memory Load/Store (`LD.*`, `ST.*`) with alignment checks.
- Implement `CALL`/`RET` using native host stack management.

### **Phase 4: System & FFI (Week 4)**
- Implement `SYSCALL` bridge to `IOProvider`.
- Implement `StaticHOModule` bridge for `libhoort` linkage.
- Implement `BREAK` trap to a host debugger or CLI.

---

## 6. Optimization Priorities

1.  **Register Promotion**: Detect registers that don't escape a basic block and map them to LLVM SSA values instead of memory-backed `regs[i]`.
2.  **Dead Store Elimination**: Elide `ST.D` instructions to the virtual stack if the lifetime of the local doesn't exceed the JIT'd function.
3.  **Tail Call Optimization**: Map HVM `TAILCALL` to LLVM `musttail call`.
