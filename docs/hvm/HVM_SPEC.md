# Hooc Virtual Machine (HVM) Specification

This document is a consolidated specification covering all aspects of the HVM architecture, including registers, instruction encoding, instruction set, and implementation guidelines.

---

## Table of Contents

1. [Overview](#1-overview)
2. [Execution Model](#2-execution-model)
3. [Register Set](#3-register-set)
4. [Instruction Encoding](#4-instruction-encoding)
5. [Instruction Set Reference](#5-instruction-set-reference)
   - [5.15 String Operations](#516-string-operations-instructions)
   - [5.16 Foreign Function Interface (FFI)](#516-foreign-function-interface-ffi-instructions)
   - [5.17 Exception Handling](#517-exception-handling-instructions)
   - [5.18 Interrupt Handling](#518-interrupt-handling-instructions)
   - [5.19 Threading](#519-threading-instructions)
   - [5.20 Multi-Process](#520-multi-process-instructions)
6. [Calling Convention](#6-calling-convention)
7. [Object Model](#7-object-model)
8. [Dynamic Linking](#8-dynamic-linking)
9. [Implementation Guidelines](#9-implementation-guidelines)
10. [Appendix A: Opcode Map Summary](#appendix-a-opcode-map-summary)
11. [Appendix B: Changes from 128-bit to 64-bit](#appendix-b-changes-from-128-bit-to-64-bit)
12. [Appendix C: Assembly Syntax](#appendix-c-assembly-syntax)
13. [Appendix D: Glossary](#appendix-d-glossary)

---

## 1. Overview

The Hooc Virtual Machine (HVM) is a lightweight, register-based virtual machine designed for the Hooc programming language. It supports static compilation, dynamic linking, JIT compilation (LLVM IR friendly), and C/C++ calling conventions.

### 1.1 Design Goals

- High performance and portability
- Static compilation and dynamic linking
- Seamless integration with C/C++ and LLVM toolchains
- Support for modern language features (classes, interfaces)
- RISC-like instruction set enabling efficient JIT compilation

### 1.2 Architecture Type

HVM is a **RISC-like** architecture with:
- Fixed 32-bit instruction length
- Simple instruction formats (R, I, RI, B, J)
- Register-to-register operations
- Load/store architecture
- 64-bit general-purpose registers

---

## 2. Execution Model

### 2.1 Memory Model

- **Byte-addressable, little-endian** memory
- **Stack**: Grows downward, holds activation records
- **Heap**: Managed by a garbage collector or reference counting
- **Static data**: For constants, vtables, and module metadata

### 2.2 Data Types

| Hooc Type | Size      | Register Representation |
|-----------|-----------|------------------------|
| int64     | 64 bits   | Single register        |
| double    | 64 bits   | Single register        |
| int32     | 32 bits   | Register (sign/zero extended) |
| bool      | 8 bits    | Register (0 or 1)      |
| char      | 8 bits    | Register (0-255)       |
| string    | variable  | Pointer in register    |
| object    | variable  | Pointer in register    |
| array     | variable  | Pointer in register    |

### 2.3 Instruction Format

All instructions are **32-bit**, fixed-length, providing predictable memory access patterns and simplified instruction fetch/decode.

---

## 3. Register Set

HVM provides 32 general-purpose registers and 16 vector registers.

### 3.1 General-Purpose Registers (r0-r31)

| Register | Mnemonic   | Width (bits) | Purpose                                      |
|----------|------------|--------------|----------------------------------------------|
| r0       | zero       | 64           | Hardwired to zero. Reading returns 0.        |
| r1       | arg1/ret   | 64           | First argument and return value.             |
| r2       | arg2       | 64           | Second argument.                             |
| r3       | arg3       | 64           | Third argument.                              |
| r4       | arg4       | 64           | Fourth argument.                             |
| r5       | arg5       | 64           | Fifth argument.                              |
| r6       | arg6       | 64           | Sixth argument.                              |
| r7       | arg7       | 64           | Seventh argument.                            |
| r8       | arg8       | 64           | Eighth argument.                             |
| r9-r15   | temp1-7    | 64           | Temporary registers (caller-saved).          |
| r16-r29  | saved1-14  | 64           | Callee-saved registers.                      |
| r30      | fp         | 64           | Frame pointer (optional, for debugging).     |
| r31      | sp         | 64           | Stack pointer.                               |

### 3.2 Vector Registers (v0-v15)

| Register | Mnemonic  | Width (bits) | Purpose                       |
|----------|-----------|--------------|-------------------------------|
| v0-v7    | vec1-8    | 128          | Vector registers (caller-saved). |
| v8-v15   | vsaved1-8 | 128          | Vector registers (callee-saved). |

### 3.3 Register Conventions

| Category         | Registers                | Notes                                        |
|------------------|--------------------------|----------------------------------------------|
| Caller-saved     | r1-r15, v0-v7            | May be overwritten by called function.      |
| Callee-saved     | r16-r29, v8-v15          | Must be preserved by called function.        |
| Reserved         | r0 (zero), r31 (sp), r30 (fp) | Special-purpose registers.             |

---

## 4. Instruction Encoding

### 4.1 Instruction Formats

Five instruction formats cover all operations:

| Format | Description                              | Bit Layout                              |
|--------|------------------------------------------|-----------------------------------------|
| **R**  | Register-register operations             | opcode[7] rd[5] rs1[5] rs2[5] func[10] |
| **I**  | Register + immediate value              | opcode[7] rd[5] rs1[5] imm[15]         |
| **RI** | Two registers + small immediate         | opcode[7] rd[5] rs1[5] rs2[5] imm[10]  |
| **B**  | Branch conditions                        | opcode[7] rs1[5] rs2[5] imm[15]        |
| **J**  | Jump operations                          | opcode[7] rd[5] imm[20]                 |

### 4.2 Field Definitions

| Field   | Bits | Description                              |
|---------|------|------------------------------------------|
| opcode  | 7    | Primary operation code (0x00-0x7F)      |
| rd      | 5    | Destination register (r0-r31)            |
| rs1     | 5    | First source register                   |
| rs2     | 5    | Second source register                   |
| func    | 10   | Sub-function code for R-type variants   |
| imm     | 15/10/20 | Immediate value (sign-extended)      |

### 4.3 Encoding Table

```
| Format | 31..25 (7 bits) | 24..20 (5 bits) | 19..15 (5 bits) | 14..10 (5 bits) | 9..0 (10 bits) |
|--------|-----------------|-----------------|-----------------|-----------------|---------------|
| R      | opcode          | rd              | rs1             | rs2             | func          |
| I      | opcode          | rd              | rs1             | imm15 (15..10)  | imm15 (9..0)  |
| RI     | opcode          | rd              | rs1             | rs2             | imm10         |
| B      | opcode          | rs1             | rs2             | imm15 (15..10)  | imm15 (9..0)  |
| J      | opcode          | rd              | imm20 (19..15)  | imm20 (14..10)  | imm20 (9..0)  |
```

### 4.4 Operand Notation

In instruction references:
- `rd` = Destination register
- `rs1`, `rs2` = Source registers
- `imm` = Immediate value
- `-` = Unused field

### 4.5 Binary Encoding Examples

**Example 1: ADD r1, r2, r3 (opcode 0x10, func 0)**
```
Binary: 0010000 00001 00010 00011 0000000000
Hex:    0x20    0x01  0x02  0x03  0x000

R-type: [opcode:7][rd:5][rs1:5][rs2:5][func:10]
```

**Example 2: LD.B r5, r6, 100 (opcode 0x70)**
```
I-type: [opcode:7][rd:5][rs1:5][imm:15]
        [1110000][00101][00110][0000001100100]
Hex:    0x70    0x05  0x06  0x064
```

**Example 3: BEQ r1, r2, -8 (opcode 0x50)**
```
B-type: [opcode:7][rs1:5][rs2:5][imm:15]
        [1010000][00001][00010][111111111111000]
Hex:    0x50    0x01  0x02  0xFFF8 (sign-extended -8)
```

### 4.6 Immediate Encoding

All immediates are **sign-extended** to 64 bits before use, except:
- `MOVZ`: Zero-extended
- Branch offsets: Scaled by 4 (word-aligned)
- Jump offsets: Scaled by 4 (word-aligned)

**15-bit immediates (I-type, B-type):**
- Range: -16384 to +16383
- Encoding: bits 14..0 of the offset

**20-bit offsets (J-type):**
- Range: -524288 to +524287 (word offsets)
- Actual byte offset = offset * 4
- Range: -2MB to +2MB

**10-bit immediates (RI-type):**
- Range: -512 to +511
- Used for small offsets and lane indices

---

## 5. Instruction Set Reference

### 5.1 Data Movement Instructions

| Mnemonic | Opcode | Format | Operands          | Operation                        | Description                        |
|----------|--------|--------|-------------------|----------------------------------|------------------------------------|
| NOP      | 0x00   | R      | -, -, -, -        |                                  | No operation                       |
| MOV      | 0x01   | R      | rd, rs, -, -      | rd = rs                          | Move register                      |
| MOVI     | 0x02   | I      | rd, rs, imm15     | rd = rs + imm15                  | Add immediate (sign-extended)      |
| MOVZ     | 0x03   | I      | rd, rs, imm15     | rd = rs \| zero_extend(imm15)    | Move with zero-extended immediate  |
| LUI      | 0x04   | I      | rd, rs, imm15     | rd = rs \| (imm15 << 32)         | Load upper immediate               |
| ADDI     | 0x05   | I      | rd, rs, imm15     | rd = rs + sign_extend(imm15)     | Add immediate                      |
| SUBI     | 0x06   | I      | rd, rs, imm15     | rd = rs - sign_extend(imm15)     | Subtract immediate                 |
| NEG      | 0x07   | R      | rd, rs, -, -      | rd = -rs                         | Negate                             |
| XCHG     | 0x08   | R      | rd, rs, -, -      | swap rd, rs                      | Exchange registers                  |

### 5.2 Integer Arithmetic Instructions

| Mnemonic | Opcode | Format | Operands              | Operation                           | Description                    | Func |
|----------|--------|--------|-----------------------|-------------------------------------|--------------------------------|------|
| ADD      | 0x10   | R      | rd, rs1, rs2, -       | rd = rs1 + rs2                      | Add                            | 0    |
| SUB      | 0x10   | R      | rd, rs1, rs2, -       | rd = rs1 - rs2                      | Subtract                       | 1    |
| MUL      | 0x10   | R      | rd, rs1, rs2, -       | rd = (rs1 * rs2)[63:0]              | Multiply (low 64 bits)         | 2    |
| MULH     | 0x10   | R      | rd, rs1, rs2, -       | rd = (rs1 * rs2) >> 64 (unsigned)  | Multiply high (unsigned)       | 3    |
| MULHS    | 0x10   | R      | rd, rs1, rs2, -       | rd = (rs1 * rs2) >> 64 (signed)    | Multiply high (signed)         | 4    |
| DIV      | 0x10   | R      | rd, rs1, rs2, -       | rd = rs1 / rs2 (signed)             | Divide (signed)                | 5    |
| DIVU     | 0x10   | R      | rd, rs1, rs2, -       | rd = rs1 / rs2 (unsigned)           | Divide (unsigned)              | 6    |
| REM      | 0x10   | R      | rd, rs1, rs2, -       | rd = rs1 % rs2 (signed)             | Remainder (signed)             | 7    |
| MULI     | 0x11   | I      | rd, rs, imm15         | rd = rs * sign_extend(imm15)        | Multiply immediate             | -    |
| DIVI     | 0x12   | I      | rd, rs, imm15         | rd = rs / sign_extend(imm15)        | Divide immediate               | -    |
| SHL      | 0x13   | R      | rd, rs1, rs2, -       | rd = rs1 << (rs2 & 63)              | Shift left                     | 0    |
| SHR      | 0x13   | R      | rd, rs1, rs2, -       | rd = rs1 >> (rs2 & 63) (logical)    | Shift right (logical)          | 1    |
| SAR      | 0x13   | R      | rd, rs1, rs2, -       | rd = rs1 >> (rs2 & 63) (arith)     | Shift right (arithmetic)       | 2    |
| SHLI     | 0x14   | I      | rd, rs, imm15         | rd = rs << (imm15 & 63)             | Shift left immediate            | -    |

### 5.3 Floating-Point Arithmetic Instructions

| Mnemonic | Opcode | Format | Operands        | Operation                 | Description                    | Func |
|----------|--------|--------|-----------------|---------------------------|--------------------------------|------|
| FADD     | 0x30   | R      | rd, rs1, rs2, - | rd = rs1 + rs2 (f64)      | Floating-point add             | 0    |
| FSUB     | 0x30   | R      | rd, rs1, rs2, - | rd = rs1 - rs2 (f64)      | Floating-point subtract       | 1    |
| FMUL     | 0x30   | R      | rd, rs1, rs2, - | rd = rs1 * rs2 (f64)      | Floating-point multiply       | 2    |
| FDIV     | 0x30   | R      | rd, rs1, rs2, - | rd = rs1 / rs2 (f64)      | Floating-point divide         | 3    |
| FSQRT    | 0x31   | R      | rd, rs, -, -    | rd = sqrt(rs)             | Floating-point square root    | -    |
| FABS     | 0x32   | R      | rd, rs, -, -    | rd = abs(rs)              | Floating-point absolute value | -    |
| FNEG     | 0x33   | R      | rd, rs, -, -    | rd = -rs                  | Floating-point negate          | -    |
| FADD32   | 0x34   | R      | rd, rs1, rs2, - | rd = rs1 + rs2 (f32)      | Floating-point add (32-bit)    | 0    |
| FSUB32   | 0x34   | R      | rd, rs1, rs2, - | rd = rs1 - rs2 (f32)      | Floating-point subtract (32)   | 1    |
| FMUL32   | 0x34   | R      | rd, rs1, rs2, - | rd = rs1 * rs2 (f32)      | Floating-point multiply (32)   | 2    |
| FDIV32   | 0x34   | R      | rd, rs1, rs2, - | rd = rs1 / rs2 (f32)      | Floating-point divide (32)     | 3    |
| FCVT     | 0x35   | I      | rd, rs, imm15   | rd = convert(rs, imm15)   | Floating-point convert         | -    |

### 5.4 Logical & Bitwise Instructions

| Mnemonic | Opcode | Format | Operands        | Operation                      | Description                | Func |
|----------|--------|--------|-----------------|--------------------------------|----------------------------|------|
| AND      | 0x20   | R      | rd, rs1, rs2, - | rd = rs1 & rs2                | Bitwise AND                | 0    |
| OR       | 0x20   | R      | rd, rs1, rs2, - | rd = rs1 \| rs2               | Bitwise OR                 | 1    |
| XOR      | 0x20   | R      | rd, rs1, rs2, - | rd = rs1 ^ rs2                | Bitwise XOR                | 2    |
| NOT      | 0x21   | R      | rd, rs, -, -    | rd = ~rs                      | Bitwise NOT                | -    |
| ANDI     | 0x22   | I      | rd, rs, imm15   | rd = rs & sign_extend(imm15) | Bitwise AND immediate      | -    |
| ORI      | 0x23   | I      | rd, rs, imm15   | rd = rs \| sign_extend(imm15) | Bitwise OR immediate       | -    |
| XORI     | 0x24   | I      | rd, rs, imm15   | rd = rs ^ sign_extend(imm15) | Bitwise XOR immediate      | -    |
| CLZ      | 0x25   | R      | rd, rs, -, -    | rd = count_leading_zeros(rs)  | Count leading zeros         | -    |
| CTZ      | 0x26   | R      | rd, rs, -, -    | rd = count_trailing_zeros(rs) | Count trailing zeros       | -    |
| POPCNT   | 0x27   | R      | rd, rs, -, -    | rd = population_count(rs)     | Population count           | -    |

### 5.5 Comparison Instructions

| Mnemonic | Opcode | Format | Operands        | Operation                            | Description                        | Func |
|----------|--------|--------|-----------------|--------------------------------------|------------------------------------|------|
| CMPEQ    | 0x40   | R      | rd, rs1, rs2, - | rd = (rs1 == rs2) ? 1 : 0            | Equal                              | 0    |
| CMPNE    | 0x40   | R      | rd, rs1, rs2, - | rd = (rs1 != rs2) ? 1 : 0            | Not equal                          | 1    |
| CMPLT    | 0x40   | R      | rd, rs1, rs2, - | rd = (rs1 < rs2) ? 1 : 0 (signed)    | Less than (signed)                 | 2    |
| CMPLE    | 0x40   | R      | rd, rs1, rs2, - | rd = (rs1 <= rs2) ? 1 : 0 (signed)  | Less than or equal (signed)        | 3    |
| CMPGT    | 0x40   | R      | rd, rs1, rs2, - | rd = (rs1 > rs2) ? 1 : 0 (signed)   | Greater than (signed)              | 4    |
| CMPGE    | 0x40   | R      | rd, rs1, rs2, - | rd = (rs1 >= rs2) ? 1 : 0 (signed)  | Greater than or equal (signed)     | 5    |
| CMPLTU   | 0x40   | R      | rd, rs1, rs2, - | rd = (rs1 < rs2) ? 1 : 0 (unsigned) | Less than (unsigned)               | 6    |
| CMPLEU   | 0x40   | R      | rd, rs1, rs2, - | rd = (rs1 <= rs2) ? 1 : 0 (unsigned)| Less than or equal (unsigned)     | 7    |
| FCMPEQ   | 0x41   | R      | rd, rs1, rs2, - | rd = (rs1 == rs2) ? 1 : 0 (f64)     | Floating-point equal               | 0    |
| FCMPLT   | 0x41   | R      | rd, rs1, rs2, - | rd = (rs1 < rs2) ? 1 : 0 (f64)      | Floating-point less than           | 1    |
| FCMPLE   | 0x41   | R      | rd, rs1, rs2, - | rd = (rs1 <= rs2) ? 1 : 0 (f64)     | Floating-point less or equal       | 2    |
| FCMPGT   | 0x41   | R      | rd, rs1, rs2, - | rd = (rs1 > rs2) ? 1 : 0 (f64)      | Floating-point greater than        | 4    |
| FCMPGE   | 0x41   | R      | rd, rs1, rs2, - | rd = (rs1 >= rs2) ? 1 : 0 (f64)     | Floating-point greater or equal    | 5    |
| SET      | 0x42   | I      | rd, -, imm15    | rd = imm15 & 1                      | Set boolean from immediate         | -    |

### 5.6 Branch Instructions

| Mnemonic | Opcode | Format | Operands        | Operation                              | Description                    |
|----------|--------|--------|-----------------|----------------------------------------|--------------------------------|
| BEQ      | 0x50   | B      | rs1, rs2, imm15 | if rs1 == rs2 then pc += sign_extend(imm15) | Branch if equal         |
| BNE      | 0x51   | B      | rs1, rs2, imm15 | if rs1 != rs2 then pc += sign_extend(imm15) | Branch if not equal     |
| BLT      | 0x52   | B      | rs1, rs2, imm15 | if rs1 < rs2 (signed) then pc += sign_extend(imm15) | Branch if less than |
| BLE      | 0x53   | B      | rs1, rs2, imm15 | if rs1 <= rs2 (signed) then pc += sign_extend(imm15) | Branch if less or equal |
| BGT      | 0x54   | B      | rs1, rs2, imm15 | if rs1 > rs2 (signed) then pc += sign_extend(imm15) | Branch if greater than |
| BGE      | 0x55   | B      | rs1, rs2, imm15 | if rs1 >= rs2 (signed) then pc += sign_extend(imm15) | Branch if greater or equal |
| BLTU     | 0x56   | B      | rs1, rs2, imm15 | if rs1 < rs2 (unsigned) then pc += sign_extend(imm15) | Branch if less than unsigned |
| BGEU     | 0x57   | B      | rs1, rs2, imm15 | if rs1 >= rs2 (unsigned) then pc += sign_extend(imm15) | Branch if greater or equal unsigned |

### 5.7 Jump Instructions

| Mnemonic | Opcode | Format | Operands          | Operation                                      | Description               |
|----------|--------|--------|-------------------|------------------------------------------------|---------------------------|
| JMP      | 0x60   | J      | offset            | pc += sign_extend(offset)                      | Unconditional jump        |
| JAL      | 0x61   | J      | rd, offset        | rd = pc+4; pc += sign_extend(offset)          | Jump and link             |
| JALR     | 0x62   | I      | rd, rs, imm15     | rd = pc+4; pc = rs + sign_extend(imm15)       | Jump and link register    |
| RET      | 0x63   | R      | -, -, -, -        | pc = r1                                        | Return from subroutine    |

### 5.8 Memory Load Instructions

| Mnemonic | Opcode | Format | Operands          | Operation                                  | Description                  |
|----------|--------|--------|-------------------|--------------------------------------------|------------------------------|
| LD.B     | 0x70   | I      | rd, addr, imm15   | rd = sign_extend(mem[addr + imm15]:8)      | Load signed byte            |
| LD.BU    | 0x71   | I      | rd, addr, imm15   | rd = zero_extend(mem[addr + imm15]:8)     | Load unsigned byte          |
| LD.H     | 0x72   | I      | rd, addr, imm15   | rd = sign_extend(mem[addr + imm15]:16)    | Load signed half            |
| LD.HU    | 0x73   | I      | rd, addr, imm15   | rd = zero_extend(mem[addr + imm15]:16)    | Load unsigned half          |
| LD.W     | 0x74   | I      | rd, addr, imm15   | rd = sign_extend(mem[addr + imm15]:32)    | Load signed word            |
| LD.WU    | 0x75   | I      | rd, addr, imm15   | rd = zero_extend(mem[addr + imm15]:32)    | Load unsigned word          |
| LD.D     | 0x76   | I      | rd, addr, imm15   | rd = mem[addr + imm15]:64                  | Load double word (64-bit)  |
| LD.X     | 0x77   | RI     | rd, rd2, addr, imm | rd = mem[addr]:64; rd2 = mem[addr+8]:64  | Load 128-bit pair           |

### 5.9 Memory Store Instructions

| Mnemonic | Opcode | Format | Operands          | Operation                                  | Description                  |
|----------|--------|--------|-------------------|--------------------------------------------|------------------------------|
| ST.B     | 0x78   | I      | data, addr, imm15 | mem[addr + imm15]:8 = data                | Store byte                  |
| ST.H     | 0x79   | I      | data, addr, imm15 | mem[addr + imm15]:16 = data               | Store half                  |
| ST.W     | 0x7A   | I      | data, addr, imm15 | mem[addr + imm15]:32 = data               | Store word                  |
| ST.D     | 0x7B   | I      | data, addr, imm15 | mem[addr + imm15]:64 = data               | Store double word (64-bit)  |
| ST.X     | 0x7C   | RI     | rd2, rd, addr, imm | mem[addr]:64 = rd; mem[addr+8]:64 = rd2  | Store 128-bit pair          |

### 5.10 Stack & Address Instructions

| Mnemonic | Opcode | Format | Operands          | Operation                                  | Description                  |
|----------|--------|--------|-------------------|--------------------------------------------|------------------------------|
| LDA      | 0x7D   | I      | rd, addr, imm15   | rd = addr + sign_extend(imm15)            | Load address                |
| PUSH     | 0x7E   | R      | rs, -, -, -       | sp -= 8; mem[sp] = rs                     | Push register to stack      |
| POP      | 0x7F   | R      | rd, -, -, -       | rd = mem[sp]; sp += 8                     | Pop from stack              |
| ENTER    | 0x80   | I      | rd, -, imm15      | sp -= sign_extend(imm15); rd = old_sp     | Enter stack frame           |
| LEAVE    | 0x81   | R      | -, -, -, -        | sp = r30; r30 = mem[old_sp]               | Leave stack frame           |
| ADJSP    | 0x82   | I      | -, rs, imm15      | sp += sign_extend(imm15)                  | Adjust stack pointer        |
| FRAME    | 0x83   | I      | rd, -, imm15      | rd = r30 + sign_extend(imm15)             | Get frame address           |

### 5.11 Object & Array Instructions

| Mnemonic    | Opcode | Format | Operands              | Operation                                | Description              |
|-------------|--------|--------|-----------------------|------------------------------------------|--------------------------|
| NEW         | 0xA8   | I      | rd, -, imm15          | rd = new_object(imm15)                   | Allocate object by class ID |
| NEWA        | 0xA9   | RI     | rd, len, type, -      | rd = new_array(len, type)                 | Allocate array            |
| LDF         | 0xAA   | RI     | rd, obj, -, idx       | rd = obj.field[idx]                       | Load object field         |
| STF         | 0xAB   | RI     | val, obj, -, idx      | obj.field[idx] = val                      | Store object field        |
| LDELEM      | 0xAC   | R      | rd, arr, idx, -       | rd = arr[idx]                             | Load array element        |
| STELEM      | 0xAD   | R      | val, arr, idx, -       | arr[idx] = val                            | Store array element       |
| ARRAYLEN    | 0xAE   | R      | rd, arr, -, -         | rd = length(arr)                          | Get array length          |
| INSTANCEOF  | 0xAF   | RI     | rd, obj, -, type      | rd = (obj instanceof type) ? 1 : 0        | Type check                |
| CHECKCAST   | 0xB0   | RI     | rd, obj, -, type      | rd = cast(obj, type)                      | Type cast                 |
| MONITORENTER| 0xB1   | R      | obj, -, -, -           | monitor_enter(obj)                         | Lock object for sync      |
| MONITOREXIT | 0xB2   | R      | obj, -, -, -           | monitor_exit(obj)                          | Unlock object from sync   |
| GC          | 0xB3   | R      | -, -, -, -             | request_garbage_collection()              | Request garbage collection|

### 5.12 Call & Dynamic Linking Instructions

| Mnemonic   | Opcode | Format | Operands              | Operation                                        | Description                |
|------------|--------|--------|-----------------------|--------------------------------------------------|----------------------------|
| CALL       | 0xB4   | R      | addr, -, -, -         | call addr                                       | Call function at address   |
| CALLI      | 0xB5   | I      | rd, -, imm15          | rd = call symbol_table[imm15]                  | Call by symbol table index |
| TAILCALL   | 0xB6   | R      | addr, -, -, -         | tail_call addr                                  | Tail call                  |
| CALLVIRT   | 0xB7   | RI     | rd, obj, -, method    | rd = obj.vtable[method](); call rd              | Virtual method call        |
| CALLINTF   | 0xB8   | RI     | rd, obj, -, method    | rd = obj.intf_table[method](); call rd          | Interface method call      |
| IMPORT     | 0xB9   | RI     | rd, -, -, imm15       | rd = resolve(mod_table[imm15], sym)             | Resolve external symbol    |
| LOADMOD    | 0xBA   | I      | rd, -, imm15          | rd = load_module(imm15)                         | Load module by index       |
| RESOLVE    | 0xBB   | RI     | rd, mod, -, sym       | rd = resolve(mod, sym)                          | Resolve symbol from module |

### 5.13 Conversion & Type Handling Instructions

| Mnemonic   | Opcode | Format | Operands        | Operation                        | Description               |
|------------|--------|--------|-----------------|----------------------------------|---------------------------|
| SEXT.B     | 0xF0   | R      | rd, rs, -, -    | rd = sign_extend(rs:8)            | Sign extend byte          |
| SEXT.H     | 0xF1   | R      | rd, rs, -, -    | rd = sign_extend(rs:16)          | Sign extend half          |
| SEXT.W     | 0xF2   | R      | rd, rs, -, -    | rd = sign_extend(rs:32)          | Sign extend word          |
| ZEXT.B     | 0xF3   | R      | rd, rs, -, -    | rd = zero_extend(rs:8)           | Zero extend byte          |
| ZEXT.H     | 0xF4   | R      | rd, rs, -, -    | rd = zero_extend(rs:16)          | Zero extend half          |
| ZEXT.W     | 0xF5   | R      | rd, rs, -, -    | rd = zero_extend(rs:32)          | Zero extend word          |
| TRUNC      | 0xF6   | R      | rd, rs, bits, - | rd = truncate(rs, bits)          | Truncate to bits          |
| REINTERPRET| 0xF7   | R      | rd, rs, -, -    | rd = bitcast(rs)                  | Reinterpret bits          |

### 5.14 Vector / SIMD Instructions (16-bit opcodes 0x100-0x10A)

| Mnemonic | Opcode | Format | Operands          | Operation                                  | Description                    | Func |
|----------|--------|--------|-------------------|--------------------------------------------|--------------------------------|------|
| VADD     | 0x100  | R      | vd, vs1, vs2, -   | vd = vs1 + vs2 (vector)                   | Vector add                     | 0    |
| VSUB     | 0x100  | R      | vd, vs1, vs2, -   | vd = vs1 - vs2 (vector)                   | Vector subtract                | 1    |
| VMUL     | 0x100  | R      | vd, vs1, vs2, -   | vd = vs1 * vs2 (vector)                   | Vector multiply                | 2    |
| VDOT     | 0x101  | R      | rd, vs1, vs2, -   | rd = dot_product(vs1, vs2)                | Vector dot product             | -    |
| VLOAD    | 0x102  | I      | vd, addr, imm15   | vd = load_vector(addr, sign_extend(imm15)) | Load vector with stride        |
| VSTORE   | 0x103  | I      | vd, addr, imm15   | store_vector(vd, addr, sign_extend(imm15)) | Store vector with stride       |
| VSHUF    | 0x104  | RI     | vd, vs1, vs2, imm | vd = shuffle(vs1, vs2, imm)              | Vector shuffle                 | -    |
| VSPLAT   | 0x105  | R      | vd, rs, -, -      | vd = splat(rs)                            | Broadcast scalar to vector     | -    |
| VEXTRACT | 0x106  | I      | rd, vd, imm15     | rd = vd[imm15 & 15]                       | Extract lane from vector       | -    |
| VINSERT  | 0x107  | RI     | vd, rs, -, lane   | vd[lane] = rs                             | Insert scalar into vector lane | -    |
| VCMPEQ   | 0x108  | R      | vd, vs1, vs2, -   | vd = (vs1 == vs2)                         | Vector compare equal           | 0    |
| VCMPNE   | 0x108  | R      | vd, vs1, vs2, -   | vd = (vs1 != vs2)                         | Vector compare not equal       | 1    |
| VCMPLT   | 0x108  | R      | vd, vs1, vs2, -   | vd = (vs1 < vs2)                          | Vector compare less than       | 2    |
| VCMPLE   | 0x108  | R      | vd, vs1, vs2, -   | vd = (vs1 <= vs2)                        | Vector compare less or equal   | 3    |
| VREDUCE  | 0x109  | I      | rd, vd, imm15     | rd = reduce(vd, imm15)                    | Vector reduction              | -    |
| VFMA     | 0x10A  | R      | vd, vs1, vs2, -   | vd = vd + vs1 * vs2                      | Fused multiply-add             | 0    |
| VFMS     | 0x10A  | R      | vd, vs1, vs2, -   | vd = vd - vs1 * vs2                      | Fused multiply-sub             | 1    |

### 5.15 System & Debug Instructions

| Mnemonic | Opcode | Format | Operands        | Operation                        | Description                    |
|----------|--------|--------|-----------------|----------------------------------|--------------------------------|
| SYSCALL  | 0x130  | I      | rd, -, imm15    | rd = syscall(imm15)             | Invoke system call             |
| TRAP     | 0x131  | I      | -, -, imm15     | software_breakpoint(imm15)       | Software breakpoint            |
| DEBUG    | 0x132  | R      | rs, -, -, -     | print_register(rs)               | Print register (debug)         |
| RDCOUNT  | 0x133  | I      | rd, -, imm15    | rd = read_counter(imm15)         | Read performance counter       |
| BARRIER  | 0x134  | R      | -, -, -, -      | memory_barrier()                 | Memory barrier                 |
| BREAKPOINT| 0x135  | I      | -, -, imm15     | trigger_breakpoint(imm15)        | Source-level breakpoint        |
| SINGLESTEP| 0x136  | R      | rd, -, -, -     | rd = single_step()               | Single-step instruction        |
| GETREGS  | 0x137  | R      | addr, -, -, -   | read_registers(addr)              | Read all registers             |
| SETREGS  | 0x138  | R      | addr, -, -, -   | write_registers(addr)             | Write all registers            |
| GETFPOFF | 0x139  | R      | rd, -, -, -     | rd = get_frame_pointer_offset()  | Get frame pointer offset       |

### 5.16 String Operations Instructions

HVM provides native string manipulation instructions for efficient text processing.

#### 5.16.1 String Structure

```
String Object Layout:
+------------------+
| Vtable Pointer   | 8 bytes
+------------------+
| Length          | 8 bytes (character count)
+------------------+
| Hash Code       | 8 bytes (cached hash for equality)
+------------------+
| UTF-8 Data      | variable (null-terminated)
+------------------+
```

#### 5.16.2 String Creation & Length

| Mnemonic | Opcode | Format | Operands          | Operation                                      | Description                       |
|----------|--------|--------|-------------------|------------------------------------------------|-----------------------------------|
| STRNEW   | 0x84   | I      | rd, -, imm15       | rd = string_create(imm15)                     | Create empty string with capacity  |
| STRNEWB  | 0x85   | RI     | rd, data, len      | rd = string_from_bytes(data, len)             | Create string from byte array      |
| STRLEN   | 0x86   | R      | rd, str, -         | rd = string_length(str)                       | Get string length                 |
| STREMPTY | 0x87   | R      | rd, -, -           | rd = (string_length == 0)                    | Check if string is empty          |

#### 5.16.3 String Access & Modification

| Mnemonic | Opcode | Format | Operands          | Operation                                      | Description                       |
|----------|--------|--------|-------------------|------------------------------------------------|-----------------------------------|
| STRGET   | 0x88   | R      | rd, str, idx       | rd = string_char_at(str, idx)                  | Get character at index             |
| STRSET   | 0x89   | R      | str, idx, char     | string_set_char(str, idx, char)                | Set character at index            |
| STRAPPEND| 0x8A   | R      | rd, str, char      | rd = string_append_char(str, char)             | Append single character           |
| STRPOP   | 0x8B   | R      | rd, str, -         | rd = string_pop_back(str)                      | Remove and return last character   |

#### 5.16.4 String Comparison

| Mnemonic | Opcode | Format | Operands          | Operation                                      | Description                       |
|----------|--------|--------|-------------------|------------------------------------------------|-----------------------------------|
| STRCMP   | 0x8C   | R      | rd, str1, str2    | rd = strcmp(str1, str2)                       | Compare strings (lexicographic)   |
| STRCMPN  | 0x8D   | RI     | rd, str1, n       | rd = strncmp(str1, str2, n)                   | Compare first n characters         |
| STREQUAL | 0x8E   | R      | rd, str1, str2    | rd = (strcmp == 0)                            | Check if strings are equal        |
| STRSTART | 0x8F   | R      | rd, str, prefix   | rd = string_starts_with(str, prefix)          | Check if string starts with prefix |
| STREND   | 0x90   | R      | rd, str, suffix   | rd = string_ends_with(str, suffix)            | Check if string ends with suffix  |

#### 5.16.5 String Searching

| Mnemonic | Opcode | Format | Operands          | Operation                                      | Description                       |
|----------|--------|--------|-------------------|------------------------------------------------|-----------------------------------|
| STRCHR   | 0x91   | R      | rd, str, char     | rd = string_index_of(str, char)               | Find character (first occurrence) |
| STRRCHR  | 0x92   | R      | rd, str, char     | rd = string_last_index_of(str, char)           | Find character (last occurrence)   |
| STRFIND  | 0x93   | R      | rd, str, substr  | rd = string_find(str, substr)                  | Find substring                    |
| STRRFIND | 0x94   | R      | rd, str, substr  | rd = string_rfind(str, substr)                 | Find substring (from end)          |
| STRCONTAINS| 0x95 | R      | rd, str, substr  | rd = (find != npos)                           | Check if substring exists         |

#### 5.16.6 String Extraction & Manipulation

| Mnemonic | Opcode | Format | Operands          | Operation                                      | Description                       |
|----------|--------|--------|-------------------|------------------------------------------------|-----------------------------------|
| STRSUB   | 0x96   | RI     | rd, str, start, len| rd = string_substring(str, start, len)       | Extract substring                 |
| STRSLICE | 0x97   | R      | rd, str, start, end| rd = string_slice(str, start, end)            | Extract slice [start, end)        |
| STRSPLIT | 0x98   | RI     | rd, str, delim   | rd = string_split(str, delim)                 | Split string by delimiter         |
| STRJOIN  | 0x99   | R      | rd, str1, str2   | rd = string_concat(str1, str2)                | Concatenate two strings           |
| STREPEAT | 0x9A   | RI     | rd, str, count   | rd = string_repeat(str, count)                 | Repeat string n times             |
| STRREV   | 0x9B   | R      | rd, str, -       | rd = string_reverse(str)                       | Reverse string                    |

#### 5.16.7 String Case & Whitespace

| Mnemonic | Opcode | Format | Operands          | Operation                                      | Description                       |
|----------|--------|--------|-------------------|------------------------------------------------|-----------------------------------|
| STRUPPER | 0x9C   | R      | rd, str, -        | rd = string_to_uppercase(str)                  | Convert to uppercase              |
| STRLOWER | 0x9D   | R      | rd, str, -        | rd = string_to_lowercase(str)                  | Convert to lowercase              |
| STRTRIM  | 0x9E   | R      | rd, str, -        | rd = string_trim(str)                         | Trim leading and trailing whitespace|
| STRLTRIM | 0x9F   | R      | rd, str, -        | rd = string_trim_start(str)                    | Trim leading whitespace            |
| STRRTRIM | 0xA0   | R      | rd, str, -        | rd = string_trim_end(str)                      | Trim trailing whitespace           |
| STRPAD   | 0xA1   | RI     | rd, str, width   | rd = string_pad_right(str, width)             | Pad string to width               |

#### 5.16.8 String Conversion

| Mnemonic | Opcode | Format | Operands          | Operation                                      | Description                       |
|----------|--------|--------|-------------------|------------------------------------------------|-----------------------------------|
| STRTOI   | 0xA2   | R      | rd, str, base     | rd = parse_int64(str, base)                   | Parse string to integer           |
| STRTOD   | 0xA3   | R      | rd, str, -        | rd = parse_double(str)                         | Parse string to double            |
| ITOSTR   | 0xA4   | R      | rd, num, base     | rd = int64_to_string(num, base)                | Convert integer to string          |
| DTOSTR   | 0xA5   | R      | rd, num, -        | rd = double_to_string(num)                      | Convert double to string          |
| STRENCODE| 0xA6   | R      | rd, str, encoding | rd = string_encode(str, encoding)              | Encode string to bytes            |
| STRDECODE| 0xA7   | R      | rd, bytes, encoding| rd = string_decode(bytes, encoding)           | Decode bytes to string            |

#### 5.16.9 String Example

```asm
; String operations example
    STRNEWB r1, data, 13        ; r1 = "Hello, World"
    STRLEN  r2, r1              ; r2 = 13
    STRGET  r3, r1, 0           ; r3 = 'H' (first char)
    STRFIND r4, r1, "World"    ; r4 = 7 (index of "World")
    STREQUAL r5, r1, r1        ; r5 = 1 (true)
    STRUPPER r6, r1            ; r6 = "HELLO, WORLD"
    STRTRIM r7, r6             ; r7 = "HELLO, WORLD"
    STRSUB  r8, r1, 0, 5       ; r8 = "Hello"
    STRJOIN r9, r8, r6         ; r9 = "HelloHELLO, WORLD"

; Parse and convert
    ITOSTR  r10, r2, 10        ; r10 = "13"
    STRTOI  r11, r10, 10       ; r11 = 13
```

### 5.16 Foreign Function Interface (FFI) Instructions

The FFI instructions enable HVM to call functions from static runtimes and dynamically loaded modules. This provides seamless integration with native code, host runtime functions, and external libraries.

#### 5.16.1 Static Runtime Calls

Functions in the static runtime (e.g., HoocJIT's runtime functions like `hoo_string_from_cstr`, `hoo_alloc`) are pre-registered at VM initialization.

| Mnemonic  | Opcode | Format | Operands           | Operation                                    | Description                              |
|-----------|--------|--------|--------------------|----------------------------------------------|------------------------------------------|
| CALLHOST  | 0x120  | I      | rd, -, imm15       | rd = call_static_runtime(imm15)               | Call static runtime function by ID        |
| CALLHOSTV | 0x121  | RI     | rd, obj, -, method | rd = obj.vtable[method](); call rd          | Call virtual method via host runtime      |

#### 5.16.2 Native Function Calls

Native functions follow the C ABI and can be loaded from shared libraries or declared at compile time.

| Mnemonic   | Opcode | Format | Operands           | Operation                                    | Description                              |
|------------|--------|--------|--------------------|----------------------------------------------|------------------------------------------|
| CALLNATIVE | 0x122  | RI     | rd, addr, -, -     | rd = call_c_abi(addr)                        | Call native function with C ABI            |
| PREPCALL   | 0x123  | I      | -, -, imm15        | prepare_call_context(imm15)                  | Prepare stack frame for native call       |
| FINISHCA   | 0x124  | R      | rd, -, -, -        | rd = finish_call(); return_value             | Complete native call, retrieve return     |

#### 5.16.3 Dynamic Library Loading

These instructions enable loading shared libraries (.dll, .so, .dylib) and resolving symbols from them.

| Mnemonic | Opcode | Format | Operands           | Operation                                    | Description                              |
|----------|--------|--------|--------------------|----------------------------------------------|------------------------------------------|
| LOADLIB  | 0x125  | I      | rd, -, imm15       | rd = dlopen(imm15)                           | Load dynamic library by name/index        |
| FREELIB  | 0x126  | R      | -, lib, -, -       | dlclose(lib)                                 | Unload dynamic library                   |
| GETSYM   | 0x127  | RI     | rd, lib, -, imm10  | rd = dlsym(lib, imm10)                      | Get symbol address from library           |
| GETFUNC  | 0x128  | RI     | rd, lib, -, imm10  | rd = resolve_function(lib, imm10)             | Resolve function pointer from library     |

#### 5.16.4 Type Conversion for FFI

When calling native functions, values may need conversion between HVM types and C types.

| Mnemonic   | Opcode | Format | Operands           | Operation                                    | Description                              |
|------------|--------|--------|--------------------|----------------------------------------------|------------------------------------------|
| I2PTR      | 0x129  | R      | rd, rs, -, -       | rd = int_to_pointer(rs)                       | Convert integer to pointer               |
| PTR2I      | 0x12A  | R      | rd, rs, -, -       | rd = pointer_to_int(rs)                      | Convert pointer to integer               |
| REINTERP   | 0x12B  | R      | rd, rs, -, -       | rd = bitcast_pointer(rs)                     | Reinterpret pointer type                |
| ADDR2FUNC  | 0x12C  | R      | rd, addr, -, -     | rd = create_function_pointer(addr)           | Convert address to function pointer       |
| FUNC2ADDR  | 0x12D  | R      | rd, func, -, -     | rd = function_to_address(func)               | Extract address from function pointer     |

#### 5.16.5 FFI Calling Convention

When using `CALLNATIVE`, the following conventions apply:

| Category              | Register/Location        | Notes                                    |
|-----------------------|-------------------------|------------------------------------------|
| Integer arguments     | r1 - r8                | First 8 arguments                        |
| Pointer arguments     | r1 - r8                | Treated as integer registers               |
| Floating-point args   | v0 - v7                | First 8 float/double arguments           |
| Return (integer/ptr)  | r1                      | Default return register                  |
| Return (float/double) | v0                      | Vector register for FP return            |
| Stack                 | r31 (sp)               | 8-byte aligned, grows downward           |

#### 5.16.6 FFI Example

```asm
; Call native C function: int64_t strlen(const char* str)
; Assume string pointer is in r1

    I2PTR   r2, r1              ; Ensure r1 is treated as pointer
    GETSYM  r3, libc, strlen     ; r3 = dlsym(libc, "strlen")
    CALLNATIVE r1, r3            ; r1 = strlen(r2)
    ; r1 now contains the string length

; Load a dynamic library and call a function
    LOADLIB  r10, libmylib       ; r10 = dlopen("libmylib.so")
    GETFUNC  r11, r10, myfunc    ; r11 = dlsym(r10, "myfunc")
    MOVI     r1, 42              ; First argument = 42
    CALLNATIVE r1, r11           ; Call myfunc(42)

---

### 5.17 Exception Handling Instructions

HVM provides structured exception handling with try-catch-finally semantics.

#### 5.17.1 Exception Structure

```
Exception Record:
+------------------+
| Type ID          | 8 bytes (exception class)
+------------------+
| Message          | 8 bytes (pointer to string)
+------------------+
| Stack Trace      | 8 bytes (pointer to trace)
+------------------+
| Target PC        | 8 bytes (handler address)
+------------------+
```

#### 5.17.2 Exception Instructions

| Mnemonic | Opcode | Format | Operands          | Operation                                      | Description                       |
|----------|--------|--------|-------------------|------------------------------------------------|-----------------------------------|
| TRY      | 0x110  | I      | rd, -, imm15       | rd = handler_table.push(imm15); sp -= 16      | Begin try block, push handler    |
| THROW    | 0x111  | I      | -, type, imm15     | throw_exception(type, imm15)                  | Throw exception with type/message |
| THROWV   | 0x112  | R      | type, msg, -       | throw_exception(type, msg)                     | Throw with message register       |
| CATCH    | 0x113  | I      | rd, -, imm15        | rd = pop_handler(imm15)                        | Catch exception, pop handler      |
| FINALLY  | 0x114  | I      | -, -, imm15         | execute_finally(imm15)                          | Execute finally block            |
| RETHROW  | 0x115  | R      | -, exc, -          | rethrow(exc)                                    | Rethrow current exception        |
| EXCINFO  | 0x116  | R      | rd, exc, field     | rd = exc.field                                  | Get exception field              |
| ENDFIN   | 0x117  | R      | -, -, -            | end_finally()                                   | End finally block                |

#### 5.17.3 Exception Example

```asm
; try {
;     result = risky_function();
; } catch (Error e) {
;     print(e.message);
; } finally {
;     cleanup();
; }

    TRY     r10, .handler    ; Begin try block, save handler PC
    CALL    r1, risky_func  ; Call risky function
    MOV     r10, r1         ; result = return value
    JMP     .finally        ; Jump to finally

.handler:                    ; Exception handler
    CATCH   r11, .finally   ; Catch exception, r11 = exception record
    EXCINFO r1, r11, msg    ; r1 = exception message
    CALL    print_str, r1   ; print(e.message)

.finally:                    ; Finally block
    FINALLY .end            ; Execute cleanup
    ENDFIN                  ; End finally, restore state

.end:
```

---

### 5.18 Interrupt Handling Instructions

HVM supports hardware interrupts and software traps for responsive execution.

#### 5.18.1 Interrupt Structure

```
Interrupt Frame:
+------------------+
| PC               | 8 bytes (interrupted PC)
+------------------+
| Status Register  | 8 bytes
+------------------+
| Handler Address  | 8 bytes
+------------------+
| Interrupt ID     | 8 bytes
+------------------+
```

#### 5.18.2 Interrupt Instructions

| Mnemonic | Opcode | Format | Operands          | Operation                                      | Description                       |
|----------|--------|--------|-------------------|------------------------------------------------|-----------------------------------|
| DI       | 0x118  | R      | -, -, -, -        | disable_interrupts()                            | Disable interrupts                 |
| EI       | 0x119  | R      | -, -, -, -        | enable_interrupts()                             | Enable interrupts                  |
| INT      | 0x11A  | I      | -, intid, -        | software_interrupt(intid)                       | Trigger software interrupt        |
| IRET     | 0x11B  | R      | -, -, -, -        | return_from_interrupt()                          | Return from interrupt handler      |
| SETINT   | 0x11C  | I      | -, handler, -       | set_interrupt_handler(intid, handler)           | Set interrupt handler             |
| GETINT   | 0x11D  | R      | rd, -, -          | rd = current_interrupt_id                       | Get current interrupt ID          |
| MASKINT  | 0x11E  | I      | -, intid, -        | mask_interrupt(intid)                           | Mask specific interrupt           |
| UNMASKINT| 0x11F  | I      | -, intid, -        | unmask_interrupt(intid)                         | Unmask specific interrupt         |

#### 5.18.3 Interrupt Example

```asm
; Set up timer interrupt handler
    MOVI    r1, .timer_handler  ; r1 = handler address
    SETINT  -, r1, 4           ; Set handler for timer interrupt (ID 4)

.timer_loop:
    EI                       ; Enable interrupts
    ; ... main work ...
    JMP    .timer_loop

.timer_handler:             ; Interrupt service routine
    PUSH   r0, .save_sp    ; Save registers
    PUSH   r1, .save_sp+8
    ; ... handle timer ...
    POP    r1, .save_sp+8  ; Restore registers
    POP    r0, .save_sp
    IRET                     ; Return from interrupt
```

---

### 5.19 Threading Instructions

HVM provides native threading support with mutexes, condition variables, and thread synchronization primitives.

#### 5.19.1 Thread Structure

```
Thread Control Block (TCB):
+------------------+
| Thread ID        | 8 bytes
+------------------+
| Stack Pointer    | 8 bytes
+------------------+
| Status          | 8 bytes (running/blocked/exited)
+------------------+
| TLS Base        | 8 bytes (thread-local storage)
+------------------+
```

#### 5.19.2 Thread Management Instructions (0xC0-0xC5)

| Mnemonic | Opcode | Format | Operands          | Operation                                      | Description                       |
|----------|--------|--------|-------------------|------------------------------------------------|-----------------------------------|
| THCREATE| 0xC0   | RI     | rd, entry, -      | rd = thread_create(entry)                      | Create new thread                 |
| THJOIN  | 0xC1   | R      | rd, tid, -        | rd = thread_join(tid)                          | Wait for thread to finish         |
| THEXIT  | 0xC2   | R      | -, retval, -      | thread_exit(retval)                            | Exit current thread               |
| THID    | 0xC3   | R      | rd, -, -          | rd = thread_id()                   | Get current thread ID             |
| THYIELD | 0xC4   | R      | -, -, -           | thread_yield()                           | Yield execution to other threads  |
| THWAIT  | 0xC5   | RI     | rd, tid, timeout  | rd = thread_wait(tid, timeout)                  | Wait with timeout                 |

#### 5.19.3 Synchronization Instructions (0xC6-0xD3)

| Mnemonic | Opcode | Format | Operands          | Operation                                      | Description                       |
|----------|--------|--------|-------------------|------------------------------------------------|-----------------------------------|
| MUTEXINI| 0xC6   | R      | rd, -, -          | rd = mutex_init()                             | Initialize mutex                  |
| MUTEXLCK| 0xC7   | R      | rd, mutex, -      | rd = mutex_lock(mutex)                       | Lock mutex (returns 0 on success) |
| MUTEXULK| 0xC8   | R      | -, mutex, -       | mutex_unlock(mutex)                           | Unlock mutex                     |
| MUTEXDL | 0xC9   | R      | -, mutex, -       | mutex_destroy(mutex)                           | Destroy mutex                     |
| CONDNWI | 0xCA   | R      | rd, -, -          | rd = condvar_init()                           | Initialize condition variable     |
| CONDSIG | 0xCB   | R      | -, cond, -        | condvar_signal(cond)                          | Signal one waiting thread         |
| CONDBRO | 0xCC   | R      | -, cond, -        | condvar_broadcast(cond)                       | Signal all waiting threads        |
| CONDWT  | 0xCD   | RI     | rd, cond, mutex   | rd = condvar_wait(cond, mutex)                | Wait on condition                 |
| CONDDST | 0xCE   | R      | -, cond, -        | condvar_destroy(cond)                         | Destroy condition variable        |
| SPININIT| 0xCF   | R      | rd, -, -          | rd = spinlock_init()                         | Initialize spinlock               |
| SPINLCK | 0xD0   | R      | rd, lock, -       | rd = spinlock_acquire(lock)                   | Acquire spinlock (returns 0)      |
| SPINULK | 0xD1   | R      | -, lock, -        | spinlock_release(lock)                       | Release spinlock                  |
| BARRSET | 0xD2   | R      | rd, count, -      | rd = barrier_init(count)                      | Initialize barrier                 |
| BARRWT  | 0xD3   | R      | rd, barrier, -    | rd = barrier_wait(barrier)                    | Wait at barrier (returns phase)   |

#### 5.19.4 Atomic & TLS Instructions (0xE0-0xE8)

| Mnemonic | Opcode | Format | Operands          | Operation                                      | Description                       |
|----------|--------|--------|-------------------|------------------------------------------------|-----------------------------------|
| ATOMADD | 0xE0   | R      | rd, addr, val     | rd = atomic_add(addr, val)                    | Atomic add                        |
| ATOMSUB | 0xE1   | R      | rd, addr, val     | rd = atomic_sub(addr, val)                    | Atomic subtract                   |
| ATOMCAS | 0xE2   | R      | rd, addr, old, new| rd = atomic_cas(addr, old, new)               | Atomic compare-and-swap            |
| ATOMLD  | 0xE3   | R      | rd, addr, -       | rd = atomic_load(addr)                        | Atomic load                       |
| ATOMST  | 0xE4   | R      | -, addr, val      | atomic_store(addr, val)                       | Atomic store                      |
| TLSALLOC| 0xE5   | R      | rd, size, -       | rd = tls_allocate(size)                       | Allocate TLS slot                 |
| TLSGET  | 0xE6   | R      | rd, slot, -       | rd = tls_read(slot)                           | Read from TLS slot                |
| TLSSET  | 0xE7   | R      | -, slot, val      | tls_write(slot, val)                          | Write to TLS slot                 |
| TLSFREE | 0xE8   | R      | -, slot, -        | tls_deallocate(slot)                           | Free TLS slot                     |

#### 5.19.5 Threading Example

```asm
; Create two threads that synchronize with mutex
    MUTEXINI r10               ; r10 = mutex
    MOVI     r1, .worker      ; r1 = worker entry point
    THCREATE r11, r1          ; r11 = thread1 ID
    THCREATE r12, r1           ; r12 = thread2 ID
    THJOIN   r1, r11          ; Wait for thread1
    THJOIN   r1, r12          ; Wait for thread2
    MUTEXDL  -, r10            ; Destroy mutex

.worker:
    MUTEXLCK -, r10            ; Lock mutex
    ; ... critical section ...
    MUTEXULK -, r10            ; Unlock mutex
    THEXIT   -, r0             ; Exit thread with return value 0
```

---

### 5.20 Multi-Process Support

HVM supports multi-process execution via FFI calls to native OS APIs rather than dedicated instructions.

#### 5.20.1 Process Management via FFI

Multi-process operations are provided through native system calls:

| Category          | Native API                | Via Instruction         |
|-------------------|--------------------------|------------------------|
| Fork process      | fork()                   | CALLNATIVE             |
| Execute program   | execve()                 | CALLNATIVE             |
| Wait for child    | waitpid()                | CALLNATIVE             |
| Exit process      | _exit()                  | SYSCALL                |
| Get process ID    | getpid()                 | CALLNATIVE             |
| Get parent PID     | getppid()                | CALLNATIVE             |

#### 5.20.2 Inter-Process Communication via FFI

| Category          | Native API                | Via Instruction         |
|-------------------|--------------------------|------------------------|
| Create pipe       | pipe()                   | CALLNATIVE             |
| Send message      | send() / write()         | CALLNATIVE             |
| Receive message   | recv() / read()          | CALLNATIVE             |
| Shared memory     | shmget() / shmat()       | CALLNATIVE             |

#### 5.20.3 Multi-Process Example

```asm
; Fork a child process using native call
    GETSYM  r10, libc, fork    ; r10 = fork function address
    CALLNATIVE r1, r10         ; r1 = fork() result
    CMPNE   r2, r1, r0         ; Is this the parent?
    BNE     r2, r0, .parent    ; If parent, jump to parent code

.child:                        ; Child process
    GETSYM  r10, libc, execlp  ; Load execlp
    ; ... set up arguments ...
    CALLNATIVE r1, r10         ; execve("/bin/echo", args)
    THEXIT   -, r0             ; Exit if exec fails

.parent:                      ; Parent process
    MOVI    r1, "Hello from parent"
    ; ... send message to child ...
    THWAIT  r2, r1, -1         ; Wait for child (blocking)
```
```

---

## 6. Calling Convention

### 6.1 Argument Passing

| Type                | Registers                  | Notes                        |
|---------------------|----------------------------|------------------------------|
| Integer/pointer     | r1 - r8                    | First 8 arguments            |
| Floating-point      | v0 - v7 (or r1 - r8)       | First 8 scalar floats         |
| Vector              | v0 - v7                    | First 8 vector arguments     |

### 6.2 Return Values

| Type                | Register | Notes                        |
|---------------------|----------|------------------------------|
| Integer/pointer     | r1       | Default return register       |
| Floating-point      | v0       | For scalar floats            |
| Vector              | v0       | For vector returns           |

### 6.3 Stack

- **Stack pointer**: r31 (sp)
- **Frame pointer**: r30 (fp, optional for debugging)
- **Alignment**: 8-byte required at call boundary (16-byte recommended)

---

## 7. Object Model

### 7.1 Object Header

Every object has a header containing:
- Size (in bytes)
- Vtable pointer
- Lock bits (for threading)

```
Object Layout (64-bit):
+------------------+
| Vtable Pointer   | 8 bytes
+------------------+
| Size             | 8 bytes
+------------------+
| Lock/Monitor     | 8 bytes (optional)
+------------------+
| Field 0          | 8 bytes
| Field 1          | 8 bytes
| ...              |
+------------------+
```

### 7.2 Vtable Structure

```
Vtable:
+------------------+
| Type ID          | 8 bytes (for type checking)
+------------------+
| Method Count     | 8 bytes
+------------------+
| Method 0 ptr     | 8 bytes
| Method 1 ptr     | 8 bytes
| ...              |
+------------------+
```

### 7.3 Array Layout

```
Array Layout:
+------------------+
| Vtable Pointer   | 8 bytes (points to array vtable)
+------------------+
| Length           | 8 bytes
+------------------+
| Element 0        | 8 bytes
| Element 1        | 8 bytes
| ...              |
+------------------+
```

### 7.4 Method Dispatch

- Methods are invoked via vtables
- Virtual method call (`CALLVIRT`): Look up method in object's vtable
- Interface call (`CALLINTF`): Use interface table for dispatch
- Virtual method dispatch via vtables

### 7.5 String Representation

Strings are objects with special layout:
```
String Layout:
+------------------+
| Vtable Pointer   | 8 bytes
+------------------+
| Length           | 8 bytes (character count)
+------------------+
| Hash Code        | 8 bytes (cached hash)
+------------------+
| UTF-8 Data       | variable
+------------------+
```

---

---

## 8. Dynamic Linking

### 8.1 Module Format

- Modules are compiled to `.hobj` files
- Contains: code, data, symbol tables, relocation info

### 8.2 Import Resolution

- `IMPORT` instruction generates import stubs
- VM resolver binds symbols at load time
- Lazy binding: resolve on first use

### 8.3 Public APIs

```c
hvm_load_module("path");  // Load a module
hvm_resolve("func");      // Resolve a symbol
```

---

## 9. Implementation Guidelines

### 9.1 Interpreter vs. JIT

- **Interpreter**: Easier to implement, useful for debugging
- **JIT**: Use LLVM's JIT (OrcV2) for production performance

### 9.2 Memory Management

- **Garbage Collection**: Mark-and-sweep or generational GC
- **Reference Counting**: Alternative for deterministic cleanup

### 9.3 Security Considerations

- Memory bounds checking for all memory accesses
- Sandboxing for system calls and file access

### 9.4 Debugging & Profiling

- Breakpoints and single-stepping support
- Instruction execution counters
- Memory usage and GC activity monitoring

### 9.5 Exception Handling

**Trap Types (SYSCALL numbers):**

| Number | Name           | Description                          |
|--------|----------------|--------------------------------------|
| 0      | exit           | Terminate program                    |
| 1      | print_int      | Print integer to stdout              |
| 2      | print_float    | Print float to stdout                |
| 3      | print_string   | Print string to stdout               |
| 4      | read_int       | Read integer from stdin              |
| 5      | read_float     | Read float from stdin                |
| 6      | read_string    | Read string from stdin               |
| 7      | alloc          | Allocate memory                      |
| 8      | free           | Free memory                          |
| 9      | get_time_ns    | Get time in nanoseconds               |
| 10     | gc_collect     | Force garbage collection             |

**Trap Handling:**

When `TRAP` instruction is executed:
1. Save current PC to trap handler
2. Jump to trap handler address (stored in special register)
3. Handler can inspect faulting instruction via memory read

**Exception Flow:**

```
Exception Raised
       |
       v
Save PC to exception link
       |
       v
Jump to current handler
       |
       v
Handler executes (may catch, rethrow, or unwind)
       |
       v
If uncaught: unwind stack, run finally blocks
       |
       v
Terminate or return to caller
```

### 9.6 Floating-Point Details

**Rounding Modes:**
- Round to nearest (default)
- Round toward zero
- Round toward +infinity
- Round toward -infinity

**NaN Handling:**
- Quiet NaN propagates through operations
- Signaling NaN raises exception

**Floating-Point Exception Flags:**
| Flag | Name          | Triggered By              |
|------|---------------|---------------------------|
| NV   | Invalid       | 0/0, sqrt(-1), etc.       |
| DZ   | Divide by zero| 1.0/0.0                   |
| OF   | Overflow      | Result too large          |
| UF   | Underflow     | Result too small          |
| IX   | Inexact       | Result rounded           |

### 9.7 Vector Lane Semantics

**128-bit Vector Registers (v0-v15):**

Vector registers contain 2 x 64-bit lanes, 4 x 32-bit lanes, 8 x 16-bit lanes, or 16 x 8-bit lanes:

| Element Size | Lanes per Vector |
|--------------|------------------|
| 64-bit (f64) | 2                |
| 32-bit (f32) | 4                |
| 16-bit       | 8                |
| 8-bit        | 16               |

**Lane Operations:**

- `VSPLAT`: Broadcast scalar to all lanes
- `VEXTRACT`: Read single lane (rd = vd[lane])
- `VINSERT`: Write single lane (vd[lane] = rs)
- `VSHUF`: Permute lanes using shuffle control

### 9.8 Memory Ordering

**Memory Barrier (BARRIER):**
- All loads/stores before barrier complete before those after
- Ensures visibility across threads

**Atomics (future extension):**
- LD.X/ST.X provide atomic pair operations
- Used for implementing locks and concurrent data structures

---

---

## Appendix A: Opcode Map Summary

| Opcode Range | Category                     |
|--------------|------------------------------|
| 0x00         | NOP                          |
| 0x01-0x08    | Data movement                |
| 0x10         | Integer ALU (add/sub/mul/div)|
| 0x11-0x14    | Immediate arithmetic/shifts  |
| 0x20-0x27    | Bitwise operations           |
| 0x30-0x35    | Floating-point operations     |
| 0x40-0x42    | Comparison operations        |
| 0x50-0x57    | Branch operations            |
| 0x60-0x63    | Jump operations              |
| 0x70-0x7F    | Memory load/store            |
| 0x80-0x83    | Stack management              |
| 0x84-0xA7    | String operations            |
| 0xA8-0xBB    | Object/Array/Call operations |
| 0xC0-0xCE    | Thread management/Sync       |
| 0xCF-0xD3    | Spinlock/Barrier             |
| 0xE0-0xE8    | Atomics & TLS                |
| 0xF0-0xF7    | Conversion/Type handling      |
| 0x100-0x10A  | Vector/SIMD (16-bit ops)     |
| 0x110-0x117  | Exception handling           |
| 0x118-0x11F  | Interrupt handling           |
| 0x120-0x12D  | FFI instructions             |
| 0x130-0x134  | System instructions           |
| 0x135-0x139  | Debug instructions            |

---

## Appendix B: Changes from 128-bit to 64-bit

| Change | Description |
|--------|-------------|
| Register width | All GPRs changed from 128-bit to 64-bit |
| LD.Q/ST.Q | Removed 128-bit load/store instructions |
| LD.X/ST.X | Added pair load/store for 128-bit data (uses two consecutive registers) |
| CP | Removed (redundant with MOV for 64-bit) |
| MULH | Changed from `>> 128` to `>> 64` |
| MUL | Result now `(rs1 * rs2)[63:0]` instead of `[127:0]` |
| LUI | Immediate shift changed from `<< 48` to `<< 32` |
| PUSH/POP | Stack size changed from 16 bytes to 8 bytes |
| SHL/SHR/SAR | Shift mask changed from `& 127` to `& 63` |
| SHLI | Shift mask changed from `& 127` to `& 63` |
| Vector registers | Unchanged (still 128-bit for SIMD) |

---

## Appendix C: Assembly Syntax

HVM assembly uses the following syntax:

```
; Comments start with semicolon
ADD   r1, r2, r3      ; R-type: r1 = r2 + r3
ADDI  r1, r2, 100     ; I-type: r1 = r2 + 100
BEQ   r1, r2, label   ; B-type: branch if r1 == r2
JMP   .loop           ; J-type: unconditional jump
LD.B  r1, r2, 0       ; Load byte from [r2+0] into r1
ST.D  r3, sp, 0       ; Store r3 to [sp+0]

; Labels
.loop:
    ADDI r1, r1, -1
    BNE  r1, r0, .loop
```

**Example: Factorial Function**

```asm
; int64 factorial(int64 n)
; r1 = n (input and return value)
; r2 = temporary
factorial:
    ENTER r1, 0          ; Save old sp, allocate 0 bytes
    CMPLT r2, r1, 2     ; Is n < 2?
    BNE   r2, r0, .base ; If false, goto base case

.recursive:
    ADDI  r2, r1, -1    ; r2 = n - 1
    CALL  factorial, r2 ; Recursive call
    MUL   r1, r1, r2    ; return n * factorial(n-1)
    JMP   .end

.base:
    MOVI  r1, 1          ; return 1

.end:
    LEAVE                ; Restore sp and fp
    RET                  ; Return to caller
```

**Example: Array Sum**

```asm
; int64 sum_array(int64* arr, int64 len)
; r1 = arr pointer
; r2 = len
; r3 = accumulator
; r4 = temp (loop counter)
sum_array:
    ENTER r1, 0
    MOV   r3, r0         ; acc = 0
    MOV   r4, r0         ; i = 0

.loop:
    CMPLT r5, r4, r2     ; i < len?
    BEQ   r5, r0, .end   ; if false, break

    MULI  r6, r4, 8      ; offset = i * 8
    ADD   r6, r1, r6     ; addr = arr + offset
    LD.D  r7, r6, 0     ; r7 = arr[i]
    ADD   r3, r3, r7     ; acc += arr[i]
    ADDI  r4, r4, 1      ; i++
    JMP   .loop

.end:
    MOV   r1, r3         ; return acc
    LEAVE
    RET
```

---

## Appendix D: Undefined and Implementation-Defined Behaviors

### D.1 Arithmetic Operations

| Operation | Behavior when Undefined |
|-----------|------------------------|
| **DIV, DIVI** (signed) | If divisor is 0, result is 0 and exception thrown |
| **DIVU** (unsigned) | If divisor is 0, result is all-ones (0xFFFFFFFFFFFFFFFF) |
| **REM, REMU** | If divisor is 0, result is dividend value |
| **Shift by >= 64** | Shift amount masked to 63 (bits 0-5 of shift amount used) |
| **Shift by negative** | Treated as shift by 0 (no shift) |
| **MUL overflow** | Low 64 bits stored (wrapping) |
| **ADD/SUB overflow** | Wrapping behavior (two's complement) |

### D.2 Floating-Point Operations

| Operation | Behavior |
|-----------|----------|
| **Division by zero** | Returns ±Infinity (IEEE 754 compliant) |
| **Square root of negative** | Returns NaN |
| **Overflow** | Returns ±Infinity or rounded value based on rounding mode |
| **Underflow** | Returns denormalized number or zero |
| **NaN operand** | Result is NaN (specific NaN implementation-defined) |
| **Infinity - Infinity** | Returns NaN |
| **Infinity / Infinity** | Returns NaN |
| **0 / 0** | Returns NaN |

### D.3 Memory Operations

| Operation | Behavior |
|-----------|----------|
| **Load from unmapped memory** | Implementation may: (1) return 0, (2) trap, (3) return garbage |
| **Store to unmapped memory** | Implementation may: (1) silently fail, (2) grow heap, (3) trap |
| **Misaligned access** | Implementation may: (1) split into multiple accesses, (2) trap |
| **LD.X/ST.X alignment** | Must be 16-byte aligned; misaligned access traps |

### D.4 Control Flow

| Operation | Behavior |
|-----------|----------|
| **Jump to non-instruction address** | Implementation-defined; may trap or interpret as NOPs |
| **RET with no matching CALL** | Undefined; may trap or continue execution |
| **Divide by zero** | Traps to exception handler (exception type 1: `DIVISION_BY_ZERO`) |

---

## Appendix E: Standard Exception Types

HVM defines the following standard exception types for structured exception handling:

| Type ID | Mnemonic | Description |
|---------|----------|-------------|
| `0x01` | `DIVISION_BY_ZERO` | Integer division by zero |
| `0x02` | `NULL_POINTER` | Null pointer dereference |
| `0x03` | `INDEX_OUT_OF_BOUNDS` | Array/string index out of valid range |
| `0x04` | `INVALID_CAST` | Type cast failed (CHECKCAST) |
| `0x05` | `STACK_OVERFLOW` | Stack pointer exceeds limit |
| `0x06` | `STACK_UNDERFLOW` | Stack pointer below valid range |
| `0x07` | `HEAP_OVERFLOW` | Heap allocation failed |
| `0x08` | `TYPE_MISMATCH` | Operation on incompatible types |
| `0x09` | | |

---

*Document Version: 1.2*
*Last Updated: April 2026*
