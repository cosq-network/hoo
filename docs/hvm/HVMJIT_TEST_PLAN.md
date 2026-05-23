# HVMJIT Detailed Test Plan

This document outlines the strategy for verifying the **`HVMJIT`** class. The goal is to ensure 100% compliance with the HVM v1.4 ISA and the correct execution of all language features defined in `src/parsing/Hooc.g4`.

---

## 1. Test Strategy: Programmatic Binary Synthesis

Testing a JIT-based binary translator using high-level source code is insufficient for verifying architectural parity. Instead, the test suite will utilize **Programmatic `HOModule` Synthesis**:
1.  Tests will manually construct `HOModule` objects and populate the `.text` section with raw `HVMInstruction` sequences.
2.  The `HVMJIT` will execute these synthetic modules.
3.  Results will be verified by checking the virtual register state and memory buffers post-execution.

---

## 2. Instruction-Level Unit Tests (ISA Parity)

Every opcode in `hvm_instruction_set.csv` must be tested in isolation.

### **2.1 Data Movement & Arithmetic**
- **`MOV`/`LUI`/`ADDI`**: Verify 64-bit constant materialization and sign-extension logic.
- **Arithmetic Family (0x10)**: 
    - `ADD`, `SUB`, `MUL`: Test boundary cases (max/min `i64`).
    - `DIV`, `REM`: Verify correct behavior for negative operands and **Division by Zero Trap**.
- **Shift Family (0x13)**: Test `SHL`, `SHR` (Logical), and `SAR` (Arithmetic) with shift counts 0, 1, 63, and 64.

### **2.2 Logic & Comparisons**
- **Bitwise**: `AND`, `OR`, `XOR`, `NOT`.
- **Comparisons (0x40)**: `CMPEQ`, `CMPNE`, `CMPLT`, `CMPLE`. Verify that `rd` is set to exactly `0` or `1`.

### **2.3 Memory Engine**
- **Load/Store Widths**: `LD.B` (signed), `LD.BU` (unsigned), `LD.W`, `LD.D`.
- **Offset Arithmetic**: Verify `addr + imm15` calculation.
- **Alignment Verification**: Test 8-byte aligned and unaligned accesses (if supported by hardware profile).

---

## 3. Language Feature Verification (Grammar Mapping)

These tests ensure that the JIT correctly executes the RISC sequences emitted by the `HVMCodeGenerator` for high-level constructs.

### **3.1 Control Flow (The "JIT-Break" Tests)**
- **Rule `ifStatement`**: Test `BEQ` branching to a "then" block and fall-through to "else".
- **Rule `whileStatement`**: Test backward `JMP` targets. Verify the JIT doesn't enter an infinite loop during IR generation.
- **Rule `forRangeStatement`**: Verify manual iterator increment and boundary condition matching.

### **3.2 Object Model & Arrays**
- **Rule `newExpression`**: 
    1. Verify call to `hoo_malloc`.
    2. Verify `r1` (this) is correctly set before the constructor call.
- **Rule `memberAccess`**: Verify `LD.D` with specific offsets calculated from `ClassLayout`.
- **Array Indexing**: Verify `SHL 3` + `ADDI 8` + `ADD` sequence for `arr[i]`.

### **3.3 Exception Handling (The Shadow Stack)**
- **Rule `tryCatchStatement`**:
    - Verify `hoo_push_handler` is called with the host address of the `catch` block.
    - **Trigger**: Synthesize a `CALL _F_hoo_throw_v_p`.
    - **Success**: JIT must resume execution at the registered catch block with `r1` holding the exception object.
- **Rule `throwStatement`**: Verify `r1` is populated with the operand before the runtime call.

---

## 4. System & Modular Linkage Tests

### **4.1 Multi-Binary JIT**
- **Scenario**: `Module A` calls `Module B`.
- **Setup**: Create two `HOModule` objects. `Module A` contains a `CALL` to a mangled symbol in `Module B`'s `SHT_EXPORT` table.
- **Verification**: `HVMJIT` must automatically load/resolve both units via `HVMModuleBundle`.

### **4.2 SYSCALL & IO**
- **Scenario**: Execution of `println("test")`.
- **Setup**: Map `SYSCALL 10` (example) to `IOProvider::writeStdout`.
- **Verification**: Verify that the `IOProvider` mock receives the correct string address and data.

### **4.3 FFI Marshalling**
- **Scenario**: Calling native `printf` from HVM.
- **Verification**: 
    - Verify register alignment (r1 -> RDI/RCX).
    - Verify host stack alignment (16-byte).
    - Verify return value propagation (RAX/X0 -> r1).

---

## 5. Stress & Boundary Testing

- **Deep Recursion**: Execute a Fibonacci sequence to verify stack frame establishment.
- **Register Spilling**: Execute a function with 30+ simultaneous temporaries (beyond `r9-r15`) to verify the generator's spilling logic.
- **Large Constants**: Verify `.rodata` spilling and `LDA` resolution for 64-bit literals.
- **Circular Imports**: Attempt to load two modules that import each other; verify that the JIT detects the cycle or handles it via deferred symbols.
