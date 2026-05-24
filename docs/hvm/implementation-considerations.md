# HVM Implementation Considerations (v1.4 Hardware Ready)

This guide provides normative implementation details for HVM-compliant systems (Interpreters, JITs, and physical Soft-Cores). It is strictly aligned with the **v1.4 "Hardware Ready" profile**.

## 1. System Philosophy: Hardware Purity

The HVM v1.4 architecture has transitioned from a high-level Virtual Machine to a **Physical RISC ISA**. 
- **No VM "Magic"**: The hardware has no knowledge of objects, strings, or exception stacks.
- **Aggressive Lowering**: Complexity is moved to the compiler (`hoo`).
- **Software Runtime**: Features like heap allocation and exception unwinding are provided by a thin C/C++ library (`hoort`).

---

## 2. Instruction Decoding (32/64-bit Hybrid)

Implementations must handle the tiered fetch/decode logic for the extended opcode space.

### 2.1 The 0xFE Escape Prefix
- **Standard (4 Bytes)**: Opcodes `0x00` through `0x7F`. Decoded directly from a 32-bit word.
- **Extended (8 Bytes)**: Opcodes `>= 0x80`.
    1. **Byte 0**: Must be `0xFE`.
    2. **Byte 1-3**: ULEB128-encoded opcode.
    3. **Byte 4-7**: Standard 32-bit instruction payload (padded for alignment).

### 2.2 Immediate Range Handling
- **15-bit Immediates**: Used in `I` and `B` formats. Must be **sign-extended** to 64 bits unless explicitly noted (e.g., `MOVZ` uses zero-extension).
- **20-bit Offsets**: Used in `J` format (branches/calls). These are **word offsets**. The physical branch target is calculated as `PC + (sign_extend(offset) * 4)`.

---

## 3. Register & Calling Convention (ABI)

All registers are 64 bits wide. Little-endian byte ordering is mandatory for memory transfers.

### 3.1 Hardwired Zero (r0)
- Any read from `r0` must return `0`.
- Any write to `r0` must be silently ignored or trapped (depending on hardware implementation).

### 3.2 The Standard Frame (`ENTER` / `LEAVE`)
The `ENTER imm15` instruction performs the following atomical sequence:
1. `mem[sp - 8] = r29` (Save LR)
2. `mem[sp - 16] = r30` (Save FP)
3. `r30 = sp - 16` (New FP)
4. `sp = sp - 16 - imm15` (Adjust SP)

The `LEAVE` instruction restores these values and resets the SP, effectively "popping" the entire local frame in one cycle.

### 3.3 Argument Passing
- **r1 - r8**: Used for the first 8 arguments.
- **r1**: Shared register for the first argument and the return value.
- **r29 (LR)**: Must be used for the return address in `CALL` and `JAL`.

---

## 4. Object & Array Lowering (The 8-Byte Rule)

Since the ISA is pure RISC, the implementation must strictly follow these software conventions:

### 4.1 Heap Layout
- **Objects**: Base address points to the start of the field data.
- **Arrays**: Base address points to a **64-bit Length Header**. The first element is at `base + 8`.

### 4.2 Lowering Sequences
Implementations verifying compiler output should expect these patterns:
- **Element Access**: `SHL 3` (scale) -> `ADDI 8` (header) -> `ADD` (base) -> `LD.D`.
- **Field Access**: `LD.D` with an immediate offset (assuming the offset is < 32KB).

---

## 5. Exception Model: The Shadow Stack

Hardware does not implement `TRY` blocks. Instead:
1. **Registration**: The compiler emits `LDA` to calculate the address of the `catch` block and passes it to `hoo_push_handler`.
2. **Throwing**: `hoo_throw` is a standard runtime call.
3. **Unwinding**: The runtime library maintains a "shadow stack" of handler PCs. On a throw, the runtime restores the caller's stack frame (using FP) and jumps to the handler.

---

## 6. System & Debug Interface

### 6.1 SYSCALL
The `SYSCALL rd, rs1, imm15` instruction is the normative way to trigger OS services.
- **rd**: Destination for syscall result.
- **rs1**: Pointer to argument block (optional).
- **imm15**: Syscall ID.

### 6.2 BREAK
The `BREAK` opcode should trigger a processor trap. In a simulator, this should launch the CLI debugger. In physical silicon, this should trigger a high-priority interrupt.

---

## 7. Testing & Compliance Invariants

Every HVM-compliant target must pass these round-trip checks:
1. **Opcode Parity**: Every instruction in `hvm_instruction_set.csv` must be uniquely identifiable.
2. **Precision**: 64-bit integer overflow/underflow must follow standard two's-complement wrap-around.
3. **Alignment**: Memory accesses (`LD.D`/`ST.D`) should ideally be 8-byte aligned; implementations must define behavior for unaligned access (either support via multiple cycles or trap).

---

## 8. Module Format Alignment

Binary `.ho` modules (v1.4) carry metadata that guides the implementation:
- **.funcmeta**: Must be parsed to provide the debugger with `source_line` and `local_size` information.
- **.rodata**: Used for constant spilling. Implementations must ensure the `LDA` (Load Address) instruction can correctly reference this segment relative to the instruction pointer.
