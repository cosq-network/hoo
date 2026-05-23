# HVM Implementation Analysis: Hardware-Ready Lowering

This document provides a detailed technical analysis of the `HVMCodeGenerator` implementation as of Version 1.4 (Hardware Ready). It explores how high-level Hooc language elements are resolved into pure RISC instructions and identifies the current level of completion.

## 1. ISA Alignment (Version 1.4)

The `HVMCodeGenerator` strictly adheres to the "Hardware Ready" profile. It has purged all high-level VM opcodes and now performs aggressive software-level lowering.

### **1.1 RISC Purity**
- **Instruction Formats**: Generates bit-packed 32-bit words following R, I, B, and J formats.
- **Opcode Space**: Uses the base 7-bit opcode field for core operations and the `0xFE` escape prefix for instructions >= 0x80 (e.g., `ENTER`, `SYSCALL`).
- **Register Usage**: Operates on a 32-register 64-bit file. `r0` is hardwired zero; `r1-r8` are used for the calling convention; `r9-r15` are temporaries; `r29-r31` manage control and the stack.

---

## 2. Programming Element Resolution

### **2.1 Arithmetic, Bitwise, and Logical Operations**
**AST Node**: `BinaryExpression` / `UnaryMinus` / `LogicalNot`

HVM is a 64-bit register machine. All integer types (`int8`, `byte`, `int64`) are promoted to 64-bit registers before operations. Floating-point operations use a dedicated opcode family.

#### **2.1.1 Integer Arithmetic**
Integer operations use the `ARITH` opcode family (0x10) with specific `func` sub-codes.

| Operation | Hooc Syntax | HVM Assembly (Illustrative) | Description |
| :--- | :--- | :--- | :--- |
| **Addition** | `a + b` | `add rd, rs1, rs2` | `rd = rs1 + rs2` (func 0) |
| **Subtraction** | `a - b` | `sub rd, rs1, rs2` | `rd = rs1 - rs2` (func 1) |
| **Multiplication**| `a * b` | `mul rd, rs1, rs2` | `rd = rs1 * rs2` (func 2) |
| **Division** | `a / b` | `div rd, rs1, rs2` | `rd = rs1 / rs2` (signed, func 5) |
| **Unsigned Div** | (Internal) | `divu rd, rs1, rs2`| `rd = rs1 / rs2` (unsigned, func 6) |
| **Modulo** | `a % b` | `rem rd, rs1, rs2` | `rd = rs1 % rs2` (signed, func 7) |

#### **2.1.2 Floating-Point Arithmetic**
Floating-point operations (for `float`, `double`, `f64`) use the `FLOAT_ARITH` family (0x30).

| Operation | Hooc Syntax | HVM Assembly (Illustrative) | Description |
| :--- | :--- | :--- | :--- |
| **FP Add** | `fa + fb` | `fadd rd, rs1, rs2` | `rd = rs1 + rs2` (f64, func 0) |
| **FP Sub** | `fa - fb` | `fsub rd, rs1, rs2` | `rd = rs1 - rs2` (f64, func 1) |
| **FP Mul** | `fa * fb` | `fmul rd, rs1, rs2` | `rd = rs1 * rs2` (f64, func 2) |
| **FP Div** | `fa / fb` | `fdiv rd, rs1, rs2` | `rd = rs1 / rs2` (f64, func 3) |

#### **2.1.3 Bitwise Operations**
Bitwise operations use the `LOGIC` (0x20) and `SHIFT` (0x13) opcode families.

| Operation | Hooc Syntax | HVM Assembly (Illustrative) | Description |
| :--- | :--- | :--- | :--- |
| **Bitwise AND** | `a & b` | `and rd, rs1, rs2` | `rd = rs1 & rs2` (func 0) |
| **Bitwise OR** | `a \| b` | `or rd, rs1, rs2` | `rd = rs1 \| rs2` (func 1) |
| **Bitwise XOR** | `a ^ b` | `xor rd, rs1, rs2` | `rd = rs1 ^ rs2` (func 2) |
| **Bitwise NOT** | `~a` | `not rd, rs` | `rd = ~rs` (opcode 0x21) |
| **Shift Left** | `a << b` | `shl rd, rs1, rs2` | `rd = rs1 << (rs2 & 63)` (func 0) |
| **Shift Right** | `a >> b` | `shr rd, rs1, rs2` | `rd = rs1 >> (rs2 & 63)` (logical, func 1)|
| **Arithmetic SR**| (Internal) | `sar rd, rs1, rs2` | `rd = rs1 >> (rs2 & 63)` (signed, func 2) |

#### **2.1.4 Logical Operations**
Logical operations operate on booleans (where `false=0`, `true=1`).

| Operation | Hooc Syntax | HVM Assembly (Lowering) | Description |
| :--- | :--- | :--- | :--- |
| **Logical NOT** | `!a` | `cmpeq rd, rs, r0` | `rd = (rs == 0)` (Lowered to zero-compare) |
| **Logical AND** | `a && b` | `and rd, rs1, rs2` | `rd = rs1 & rs2` (Current HVM uses bitwise) |
| **Logical OR** | `a \|\| b` | `or rd, rs1, rs2` | `rd = rs1 \| rs2` (Current HVM uses bitwise) |
| **Unary Minus** | `-a` | `sub rd, r0, rs` | `rd = 0 - rs` (Lowered to subtraction) |

*Note: The HVM backend currently lowers logical AND/OR to bitwise instructions. Short-circuiting evaluation is planned for a future update by transitioning these to conditional branch sequences.*


### **2.2 Function Calling & Convention**
**AST Node**: `FunctionDeclaration` / `FunctionCall`

HVM implements a standard RISC calling convention using registers `r1-r8` for arguments and `r1` for the return value.

#### **2.2.1 Function Prologue & Epilogue**
Every function establishes a stack frame to save the return address (LR) and frame pointer (FP).

| Stage | HVM Assembly (Illustrative) | Description |
| :--- | :--- | :--- |
| **Prologue** | `enter 32` | Decrement SP, save `r29` (LR) and `r30` (FP), set new FP. |
| **Param Save**| `st.d r1, r30, -8` | Save `arg1` to stack for local usage. |
| **Epilogue** | `leave` | Restore `r29` (LR) and `r30` (FP), increment SP to old FP. |
| **Return** | `ret` | Jump to address in `r29`. |

#### **2.2.2 Standard Call Sequence**
A function call involves moving arguments to registers and using the `call` (J-format) instruction.

**Hooc Syntax**: `result = myFunc(a, b)`

**HVM Assembly**:
```assembly
# 1. Load arguments into calling registers
mov r1, r9          # Move 'a' from temporary r9 to arg1
mov r2, r10         # Move 'b' from temporary r10 to arg2

# 2. Perform the call (saves return address in r29)
call r29, -120      # Relative word offset to 'myFunc'

# 3. Retrieve result from return register
mov r11, r1         # Move result from r1 to temporary r11
```

#### **2.2.3 Method Call Sequence (Implicit `this`)**
Methods are lowered to standard calls where the object pointer is passed as the first argument (`r1`).

**Hooc Syntax**: `obj.method(x)`

**HVM Assembly**:
```assembly
# 1. Pass 'this' pointer in r1
mov r1, r12         # Move 'obj' pointer to r1

# 2. Pass subsequent arguments in r2..r8
mov r2, r13         # Move 'x' to r2

# 3. Call method (statically resolved name)
call r29, 450       # Call 'ClassName_method'
```

#### **2.2.4 Register Convention Summary**
- **`r1`**: First argument / Return value.
- **`r2..r8`**: Arguments 2 through 8.
- **`r9..r15`**: Caller-saved temporaries (volatile across `call`).
- **`r29` (LR)**: Link register (target for `ret`).
- **`r30` (FP)**: Frame pointer.
- **`r31` (SP)**: Stack pointer.


### **2.3 Inheritance (EXTENDS)**
**AST Node**: `ClassDeclaration`

In HVM v1.4, inheritance is implemented via **Layout Prefixing**. A child class includes all fields of its parent at the start of its memory layout.

#### **2.3.1 Memory Layout Aggregation**
If `class B extends A`, the layout of `B` is constructed by:
1. Copying the entire field map and total size of `A`.
2. Appending new fields defined in `B` starting at the offset where `A` ended.

**Conceptual Layout**:
| Offset | Field Origin | Description |
| :--- | :--- | :--- |
| `0 .. 7` | `Class A` | Parent Field 1 |
| `8 .. 15`| `Class A` | Parent Field 2 |
| `16 .. 23`| `Class B` | Child Field 1 (New) |

#### **2.3.2 Constructor Chaining**
Constructors are currently resolved statically. When a child class is instantiated, its constructor (`B_init`) is called. By convention, `B_init` should call `A_init` as its first operation (lowered to a standard `call` instruction).

### **2.4 Control Flow & Loops**
**AST Node**: `IfStatement` / `WhileStatement` / `ForRangeStatement`

Control flow is implemented using conditional branches (`BEQ`, `BNE`, `BLT`, `BLE`) and unconditional jumps (`JMP`).

#### **2.4.1 If-Else Statement**
The condition is evaluated into a register, followed by a branch to the `else` block if the condition is false (zero).

**Hooc Syntax**:
```hooc
if (a > b) {
    do_thing();
} else {
    other_thing();
}
```

**HVM Assembly**:
```assembly
# 1. Evaluate condition
cmplt r9, r10, r11  # r9 = (b < a) -> effectively (a > b)

# 2. Branch if false
beq r9, r0, 12      # if r9 == 0, jump to elseLabel

# 3. Then block
call r29, 100       # call do_thing
jmp 8               # jump to endLabel

# 4. Else block (elseLabel)
call r29, 200       # call other_thing

# 5. Merge point (endLabel)
```

#### **2.4.2 While Loop**
A `while` loop checks the condition at the start and jumps back to that check from the end of the body.

**HVM Assembly Pattern**:
```assembly
# loopStartLabel:
cmplt r9, r10, r11  # Evaluate condition
beq r9, r0, 20      # if false, exit loop (to loopEndLabel)

# [loop body code here]

jmp -28             # jump back to loopStartLabel
# loopEndLabel:
```

#### **2.4.3 For-Range Loop**
The `for i in start..end by step` construct is lowered to a standard while-like pattern with a manual iterator and step increment.

**HVM Assembly Pattern**:
```assembly
# 1. Initialization
st.d r9, r30, -8    # Store 'start' into local 'i'

# 2. Condition Check (loopStartLabel)
ld.d r10, r30, -8   # Load current 'i'
cmplt r11, r10, r12 # Check if i < end
beq r11, r0, 32     # if false, exit loop

# 3. [loop body code]

# 4. Increment (stepLabel)
ld.d r10, r30, -8   # Load current 'i'
add r13, r10, r14   # i = i + step
st.d r13, r30, -8   # Save updated 'i'
jmp -44             # jump to loopStartLabel
```

*Note: `continue` statements in for-loops jump to the `stepLabel`, ensuring the iterator is updated before the next iteration.*


### **2.5 Memory Allocation & Layout**

HVM v1.4 is a load/store architecture with no hardware-level garbage collection or object semantics. All memory management is explicit and byte-addressable.

#### **2.5.1 Memory Operations (Load/Store)**
HVM supports various data widths for memory access. In the "Hardware Ready" profile, these map directly to physical bus widths.

| Instruction | HVM Assembly | Description |
| :--- | :--- | :--- |
| **Load Byte** | `ld.b rd, addr, imm` | `rd = sign_extend(mem[addr + imm]:8)` |
| **Load Unsigned**| `ld.bu rd, addr, imm`| `rd = zero_extend(mem[addr + imm]:8)` |
| **Load Word** | `ld.w rd, addr, imm` | `rd = sign_extend(mem[addr + imm]:32)` |
| **Load Double** | `ld.d rd, addr, imm` | `rd = mem[addr + imm]:64` (Standard for Hooc) |
| **Store Byte** | `st.b rs, addr, imm` | `mem[addr + imm]:8 = rs` |
| **Store Double** | `st.d rs, addr, imm` | `mem[addr + imm]:64 = rs` (Standard for Hooc) |

#### **2.5.2 Heap Allocation**
Allocating an object or array is lowered to a standard library call to `hoo_malloc`.

**Hooc Syntax**: `var p = new Point()`

**HVM Assembly**:
```assembly
# 1. Prepare size (e.g., 24 bytes for 3 fields)
movz r9, 0, 24
mov r1, r9          # Pass size in r1 (arg1)

# 2. Call allocator
call r29, 600       # call hoo_malloc

# 3. Handle return pointer
mov r10, r1         # Pointer to allocated block in r10
```

#### **2.5.3 Object Member Access**
Field access is performed via standard 64-bit load/store instructions using the base pointer of the object and a calculated immediate offset.

**Hooc Syntax**: `p.y = 42` (where `y` is at offset 16)

**HVM Assembly**:
```assembly
movz r11, 0, 42     # Prepare value
st.d r11, r10, 16   # Store 64-bit value at [object_ptr + 16]
```

#### **2.5.4 Array Layout and Operations**
HVM arrays are represented as heap-allocated buffers with a **Metadata Header** at offset 0.

**Array Memory Map**:
- `[ptr + 0]`: **Length** (64-bit integer)
- `[ptr + 8]`: **Element 0**
- `[ptr + 16]`: **Element 1** ...

**Operation: Get Array Length** (`arr.length`)
```assembly
ld.d r9, r10, 0     # Load the 64-bit length from the header
```

**Operation: Array Access** (`var x = arr[i]`)
```assembly
# 1. Scale index by 8 (SHL 3)
shl r11, r9, 3      # r11 = i * 8

# 2. Calculate address: base + element_start + scaled_index
movz r12, 0, 8      # Header size (8 bytes)
add r13, r11, r12   # r13 = (i * 8) + 8
add r14, r10, r13   # r14 = arr_ptr + total_offset

# 3. Load the element
ld.d r15, r14, 0    # Load 64-bit element into r15
```



### **2.6 Constants & Initialization**

HVM instructions use a 15-bit immediate field. The generator optimizes constant materialization based on the value's size.

#### **2.6.1 Small Constants (<= 15 bits)**
Small values are loaded directly into registers using zero-extending or sign-extending move instructions.

| Value Range | HVM Assembly | Description |
| :--- | :--- | :--- |
| `0 .. 32,767` | `movz rd, r0, imm` | Load unsigned 15-bit value. |
| `-16,384 .. 16,383`| `addi rd, r0, imm` | Load signed 15-bit value via ADD. |

#### **2.6.2 Large Constants (64-bit Spilling)**
Values exceeding 15 bits (like `1234567890`) are automatically spilled to the `.rodata` section by the generator to maintain 64-bit precision.

**HVM Assembly Sequence**:
```assembly
# 1. Load the address of the constant from .rodata
lda r9, r0, 128     # Load address relative to data segment base

# 2. Load the actual 64-bit value into the destination register
ld.d r10, r9, 0     # r10 now contains the 64-bit constant
```

#### **2.6.3 String Constants and Allocation**
Strings in Hooc are objects. The HVM backend lowers string literals into a two-step process: **Static Storage** and **Runtime Wrapping**.

1.  **Static Storage**: The raw character data is stored in the `.rodata` section, followed by a `NUL` (`\0`) terminator.
2.  **Runtime Wrapping**: A call is made to `hoo_string_from_cstr` which allocates a managed string object on the heap and copies the static data.

**HVM Assembly Sequence**:
```assembly
# 1. Get the physical address of the raw bytes in .rodata
lda r1, r0, 512     # r1 = address of "hello" in data segment

# 2. Call the runtime constructor
# Signature: HooString* hoo_string_from_cstr(const char* raw)
call r29, 800       # Target: hoo_string_from_cstr
mov r10, r1         # r10 now holds the pointer to the managed object
```

*Compliance Note: The `string` type is consistently handled as a 64-bit opaque pointer across both the HVM and LLVM backends, ensuring runtime compatibility.*


### **2.7 Debugging & Stepping**

HVM is designed for transparency, providing both hardware-level primitives and high-level metadata for tooling.

#### **2.7.1 Breakpoints**
The `break` instruction is a dedicated R-format opcode that triggers a trap to the host environment (hardware simulator or debugger).

**HVM Assembly**:
```assembly
break               # Stop execution and wait for debugger
```

#### **2.7.2 Source Mapping (Function Metadata)**
Every function in an `.ho` module includes a metadata entry (48 bytes) in the `.funcmeta` section. This allows a debugger to:
- **Stepping**: Map the current `PC` (Program Counter) to a `source_line`.
- **Introspection**: Determine `local_size` and `param_count` to reconstruct the stack frame during a pause.

### **2.8 Runtime & FFI Support**

The HVM backend achieves hardware readiness by standardizing all external interactions through a common calling convention.

#### **2.8.1 FFI (Foreign Function Interface)**
Calling a native C function is lowered to a standard `call` targeting an external symbol.

**Hooc Syntax**: `extern native void printf(string fmt, int64 val);`

**HVM Assembly**:
```assembly
mov r1, r9          # fmt string object
mov r2, r10         # val
call r29, 0         # Offset 0 (placeholder for linker fixup)
```
*The HVM loader resolves the symbol "printf" and patches the call offset at load-time.*

#### **2.8.2 System Interaction (SYSCALL)**
For operations that require Kernel/OS intervention (file I/O, network sockets), the `syscall` instruction is used. It follows the I-format where the immediate represents the syscall number.

**HVM Assembly**:
```assembly
# Invoke OS service (e.g., exit or yield)
# Format: syscall rd, rs1, imm15
syscall r1, r0, 10      # Invoke syscall #10, result in r1
```


---

## 3. Implementation Completion Status
...
### **3.1 Fully Complete [100%]**
- **Core Control Flow**: `if/else`, `while`, `break`, `continue`, `return`.
- **Arithmetic/Logic**: All binary and unary operators (Arithmetic, Logic, Comparisons).
- **Function Calling**: Full support for parameters, return values, and nested calls with `r29` saving.
- **Variable Scoping**: Local variable offset calculation and global variable allocation in `.data`.
- **Constants**: Efficient immediate handling and automatic `.rodata` spilling for 64-bit literals.

### **3.2 Substantially Complete [85%]**
- **Objects**: Basic allocation and field/method access are stable.
- **Arrays**: 1D arrays with literal initialization and dynamic indexing are functional.
- **Exceptions**: Control flow lowering is implemented; requires the `hoort` library for full system execution.

### **3.3 Partially Complete [40%]**
- **FFI**: Basic symbol resolution and calling work, but complex FFI types (e.g., nested structs/pointers) are not yet fully mapped in the HVM backend.
- **Type Inference**: Works for local variables, but complex chained member access (e.g., `a.b.c.d`) requires more robust type-tracking in the generator.

---

## 4. Missing Features & Technical Gaps

### **4.1 Stack-Based Register Spilling (CRITICAL)**
- **Current State**: The generator is limited to 7 temporary registers (`r9-r15`). If an expression tree is too deep, it emits a "Register Pressure" error.
- **Requirement**: Implement standard "Spill to Stack" logic where overflow registers are stored in the current frame and re-loaded as needed.

### **4.2 Inheritance (EXTENDS)**
- **Current State**: The grammar and AST support `class A extends B`, but the `HVMCodeGenerator` calculates `ClassLayout` in isolation.
- **Requirement**: Layout calculation must recursively aggregate field offsets from the base class to the child class.

### **4.3 String Interpolation**
- **Current State**: Handled as a constant placeholder or a simple string literal.
- **Requirement**: Lowering must emit a sequence of `CALL`s to `hoo_string_concat` and `hoo_to_string` to build the final result at runtime.

### **4.4 Multi-dimensional Arrays**
- **Current State**: Backend handles `[index]` but does not yet optimize or correctly scale for `[i][j]` without manual nested access.
- **Requirement**: Implement recursive address calculation for nested array types.

### **4.5 Advanced FFI Marshalling**
- **Current State**: Lowers to standard `CALL`.
- **Requirement**: Need logic to convert between HVM's 64-bit objects and native C ABI expectations (e.g., unboxing strings to `char*`).

---

## 5. Conclusion

The `HVMCodeGenerator` is a high-performance, physical-silicon-ready backend. By moving all complexity to the compiler and a thin C-runtime library, it achieves a level of simplicity comparable to industrial RISC processors. The immediate priority for the next phase is **Register Spilling** and **Inheritance Support** to handle production-grade Hooc applications.
