# Hooc Virtual Machine (HVM) Specification

This document is a consolidated specification covering all aspects of the HVM architecture, including registers, instruction encoding, instruction set, and implementation guidelines.

---

## Table of Contents

1. [Overview](#1-overview)
2. [Execution Model](#2-execution-model)
3. [Register Set](#3-register-set)
4. [Instruction Encoding](#4-instruction-encoding)
5. [Instruction Set Reference](#5-instruction-set-reference)
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
- Support for modern language features (classes, generics, interfaces)
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
| MOV      | 0x01   | R      | rd, rs, -, -      | rd = rs                          | Move register                      |
| MOVI     | 0x02   | I      | rd, rs, imm15     | rd = rs + imm15                  | Add immediate (sign-extended)      |
| MOVZ     | 0x03   | I      | rd, rs, imm15     | rd = rs \| zero_extend(imm15)    | Move with zero-extended immediate  |
| LUI      | 0x04   | I      | rd, rs, imm15     | rd = rs \| (imm15 << 32)         | Load upper immediate               |
| NEG      | 0x08   | R      | rd, rs, -, -      | rd = -rs                         | Negate                             |
| XCHG     | 0x0A   | R      | rd, rs, -, -      | swap rd, rs                      | Exchange registers                  |
| SWAP     | 0x0A   | R      | rd, rs, -, -      | swap rd, rs                      | Alias for XCHG                     |

### 5.2 Integer Arithmetic Instructions

| Mnemonic | Opcode | Format | Operands              | Operation                           | Description                    | Func |
|----------|--------|--------|-----------------------|-------------------------------------|--------------------------------|------|
| ADDI     | 0x06   | I      | rd, rs, imm15         | rd = rs + sign_extend(imm15)        | Add immediate                  | -    |
| SUB      | 0x07   | R      | rd, rs1, rs2, -       | rd = rs1 - rs2                      | Subtract                       | 0    |
| ADD      | 0x10   | R      | rd, rs1, rs2, -       | rd = rs1 + rs2                      | Add                            | 0    |
| SUB      | 0x10   | R      | rd, rs1, rs2, -       | rd = rs1 - rs2                      | Subtract                       | 1    |
| MUL      | 0x10   | R      | rd, rs1, rs2, -       | rd = (rs1 * rs2)[63:0]              | Multiply (low 64 bits)         | 2    |
| MULH     | 0x10   | R      | rd, rs1, rs2, -       | rd = (rs1 * rs2) >> 64 (unsigned)   | Multiply high (unsigned)        | 3    |
| MULHS    | 0x10   | R      | rd, rs1, rs2, -       | rd = (rs1 * rs2) >> 64 (signed)     | Multiply high (signed)         | -    |
| DIV      | 0x10   | R      | rd, rs1, rs2, -       | rd = rs1 / rs2 (signed)             | Divide (signed)                | 4    |
| DIVU     | 0x10   | R      | rd, rs1, rs2, -       | rd = rs1 / rs2 (unsigned)           | Divide (unsigned)              | 5    |
| REM      | 0x10   | R      | rd, rs1, rs2, -       | rd = rs1 % rs2 (signed)             | Remainder (signed)             | 6    |
| REMU     | 0x10   | R      | rd, rs1, rs2, -       | rd = rs1 % rs2 (unsigned)           | Remainder (unsigned)           | 7    |
| SUBI     | 0x12   | I      | rd, rs, imm15         | rd = rs - sign_extend(imm15)       | Subtract immediate             | -    |
| MULI     | 0x13   | I      | rd, rs, imm15         | rd = rs * sign_extend(imm15)        | Multiply immediate             | -    |
| DIVI     | 0x14   | I      | rd, rs, imm15         | rd = rs / sign_extend(imm15)        | Divide immediate               | -    |
| SHL      | 0x15   | R      | rd, rs1, rs2, -       | rd = rs1 << (rs2 & 63)              | Shift left                     | 0    |
| SHR      | 0x15   | R      | rd, rs1, rs2, -       | rd = rs1 >> (rs2 & 63) (logical)    | Shift right (logical)          | 1    |
| SAR      | 0x15   | R      | rd, rs1, rs2, -       | rd = rs1 >> (rs2 & 63) (arith)     | Shift right (arithmetic)        | 2    |
| SHLI     | 0x16   | I      | rd, rs, imm15         | rd = rs << (imm15 & 63)             | Shift left immediate            | -    |

### 5.3 Floating-Point Arithmetic Instructions

| Mnemonic | Opcode | Format | Operands        | Operation                 | Description                    | Func |
|----------|--------|--------|-----------------|---------------------------|--------------------------------|------|
| FADD     | 0x20   | R      | rd, rs1, rs2, - | rd = rs1 + rs2 (f64)      | Floating-point add             | 0    |
| FSUB     | 0x20   | R      | rd, rs1, rs2, - | rd = rs1 - rs2 (f64)      | Floating-point subtract       | 1    |
| FMUL     | 0x20   | R      | rd, rs1, rs2, - | rd = rs1 * rs2 (f64)      | Floating-point multiply       | 2    |
| FDIV     | 0x20   | R      | rd, rs1, rs2, - | rd = rs1 / rs2 (f64)      | Floating-point divide         | 3    |
| FSQRT    | 0x21   | R      | rd, rs, -, -    | rd = sqrt(rs)             | Floating-point square root    | -    |
| FABS     | 0x22   | R      | rd, rs, -, -    | rd = abs(rs)              | Floating-point absolute value | -    |
| FNEG     | 0x23   | R      | rd, rs, -, -    | rd = -rs                  | Floating-point negate          | -    |
| FADD32   | 0x24   | R      | rd, rs1, rs2, - | rd = rs1 + rs2 (f32)      | Floating-point add (32-bit)    | 0    |
| FSUB32   | 0x24   | R      | rd, rs1, rs2, - | rd = rs1 - rs2 (f32)      | Floating-point subtract (32)   | 1    |
| FMUL32   | 0x24   | R      | rd, rs1, rs2, - | rd = rs1 * rs2 (f32)      | Floating-point multiply (32)   | 2    |
| FDIV32   | 0x24   | R      | rd, rs1, rs2, - | rd = rs1 / rs2 (f32)      | Floating-point divide (32)     | 3    |
| FCVT     | 0x25   | I      | rd, rs, imm15   | rd = convert(rs, imm15)   | Floating-point convert         | -    |

### 5.4 Logical & Bitwise Instructions

| Mnemonic | Opcode | Format | Operands        | Operation                      | Description                | Func |
|----------|--------|--------|-----------------|--------------------------------|----------------------------|------|
| AND      | 0x30   | R      | rd, rs1, rs2, - | rd = rs1 & rs2                | Bitwise AND                | 0    |
| OR       | 0x30   | R      | rd, rs1, rs2, - | rd = rs1 \| rs2               | Bitwise OR                 | 1    |
| XOR      | 0x30   | R      | rd, rs1, rs2, - | rd = rs1 ^ rs2                | Bitwise XOR                | 2    |
| NOT      | 0x31   | R      | rd, rs, -, -    | rd = ~rs                      | Bitwise NOT                | -    |
| ANDI     | 0x32   | I      | rd, rs, imm15   | rd = rs & sign_extend(imm15) | Bitwise AND immediate      | -    |
| ORI      | 0x33   | I      | rd, rs, imm15   | rd = rs \| sign_extend(imm15) | Bitwise OR immediate       | -    |
| XORI     | 0x34   | I      | rd, rs, imm15   | rd = rs ^ sign_extend(imm15) | Bitwise XOR immediate      | -    |
| CLZ      | 0x35   | R      | rd, rs, -, -    | rd = count_leading_zeros(rs)  | Count leading zeros         | -    |
| CTZ      | 0x36   | R      | rd, rs, -, -    | rd = count_trailing_zeros(rs) | Count trailing zeros       | -    |
| POPCNT   | 0x37   | R      | rd, rs, -, -    | rd = population_count(rs)     | Population count           | -    |

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
| NEW         | 0x90   | I      | rd, -, imm15          | rd = new_object(imm15)                   | Allocate object by class ID |
| NEWA        | 0x91   | RI     | rd, len, type, -      | rd = new_array(len, type)                 | Allocate array            |
| LDF         | 0x92   | RI     | rd, obj, -, idx       | rd = obj.field[idx]                       | Load object field         |
| STF         | 0x93   | RI     | val, obj, -, idx      | obj.field[idx] = val                      | Store object field        |
| LDELEM      | 0x94   | R      | rd, arr, idx, -       | rd = arr[idx]                             | Load array element        |
| STELEM      | 0x95   | R      | val, arr, idx, -       | arr[idx] = val                            | Store array element       |
| ARRAYLEN    | 0x96   | R      | rd, arr, -, -         | rd = length(arr)                          | Get array length          |
| INSTANCEOF  | 0x97   | RI     | rd, obj, -, type      | rd = (obj instanceof type) ? 1 : 0        | Type check                |
| CHECKCAST   | 0x98   | RI     | rd, obj, -, type      | rd = cast(obj, type)                      | Type cast                 |
| MONITORENTER| 0x99   | R      | obj, -, -, -          | lock(obj)                                  | Lock object               |
| MONITOREXIT | 0x9A   | R      | obj, -, -, -          | unlock(obj)                               | Unlock object             |
| GC          | 0x9B   | R      | -, -, -, -            | request_garbage_collection()               | Request garbage collection|

### 5.12 Call & Dynamic Linking Instructions

| Mnemonic   | Opcode | Format | Operands              | Operation                                        | Description                |
|------------|--------|--------|-----------------------|--------------------------------------------------|----------------------------|
| CALL       | 0xA0   | R      | addr, -, -, -         | call addr                                       | Call function at address   |
| CALLI      | 0xA1   | I      | rd, -, imm15          | rd = call symbol_table[imm15]                  | Call by symbol table index |
| TAILCALL   | 0xA2   | R      | addr, -, -, -         | tail_call addr                                  | Tail call                  |
| CALLVIRT   | 0xA3   | RI     | rd, obj, -, method    | rd = obj.vtable[method](); call rd              | Virtual method call        |
| CALLINTF   | 0xA4   | RI     | rd, obj, -, method    | rd = obj.intf_table[method](); call rd          | Interface method call      |
| IMPORT     | 0xA5   | RI     | rd, -, -, imm10       | rd = resolve(mod_table[imm10], sym)             | Resolve external symbol    |
| LOADMOD    | 0xA6   | I      | rd, -, imm15          | rd = load_module(imm15)                         | Load module by index       |
| RESOLVE    | 0xA7   | RI     | rd, mod, -, sym       | rd = resolve(mod, sym)                          | Resolve symbol from module |

### 5.13 Conversion & Type Handling Instructions

| Mnemonic   | Opcode | Format | Operands        | Operation                        | Description               |
|------------|--------|--------|-----------------|----------------------------------|---------------------------|
| SEXT.B     | 0xB0   | R      | rd, rs, -, -    | rd = sign_extend(rs:8)            | Sign extend byte          |
| SEXT.H     | 0xB1   | R      | rd, rs, -, -    | rd = sign_extend(rs:16)          | Sign extend half          |
| SEXT.W     | 0xB2   | R      | rd, rs, -, -    | rd = sign_extend(rs:32)          | Sign extend word          |
| ZEXT.B     | 0xB3   | R      | rd, rs, -, -    | rd = zero_extend(rs:8)           | Zero extend byte          |
| ZEXT.H     | 0xB4   | R      | rd, rs, -, -    | rd = zero_extend(rs:16)          | Zero extend half          |
| ZEXT.W     | 0xB5   | R      | rd, rs, -, -    | rd = zero_extend(rs:32)          | Zero extend word          |
| TRUNC      | 0xB6   | R      | rd, rs, bits, - | rd = truncate(rs, bits)          | Truncate to bits          |
| REINTERPRET| 0xB7   | R      | rd, rs, -, -    | rd = bitcast(rs)                  | Reinterpret bits          |

### 5.14 Vector / SIMD Instructions

| Mnemonic | Opcode | Format | Operands          | Operation                                  | Description                    | Func |
|----------|--------|--------|-------------------|--------------------------------------------|--------------------------------|------|
| VADD     | 0xC0   | R      | vd, vs1, vs2, -   | vd = vs1 + vs2 (vector)                   | Vector add                     | 0    |
| VSUB     | 0xC0   | R      | vd, vs1, vs2, -   | vd = vs1 - vs2 (vector)                   | Vector subtract                | 1    |
| VMUL     | 0xC0   | R      | vd, vs1, vs2, -   | vd = vs1 * vs2 (vector)                   | Vector multiply                | 2    |
| VDOT     | 0xC1   | R      | rd, vs1, vs2, -   | rd = dot_product(vs1, vs2)                | Vector dot product             | -    |
| VLOAD    | 0xC2   | I      | vd, addr, imm15   | vd = load_vector(addr, sign_extend(imm15)) | Load vector with stride        |
| VSTORE   | 0xC3   | I      | vd, addr, imm15   | store_vector(vd, addr, sign_extend(imm15)) | Store vector with stride       |
| VSHUF    | 0xC4   | RI     | vd, vs1, vs2, imm | vd = shuffle(vs1, vs2, imm)              | Vector shuffle                 | -    |
| VSPLAT   | 0xC5   | R      | vd, rs, -, -      | vd = splat(rs)                            | Broadcast scalar to vector     | -    |
| VEXTRACT | 0xC6   | I      | rd, vd, imm15     | rd = vd[imm15 & 15]                       | Extract lane from vector       | -    |
| VINSERT  | 0xC7   | RI     | vd, rs, -, lane   | vd[lane] = rs                             | Insert scalar into vector lane | -    |
| VCMPEQ   | 0xC8   | R      | vd, vs1, vs2, -   | vd = (vs1 == vs2)                         | Vector compare equal           | 0    |
| VCMPNE   | 0xC8   | R      | vd, vs1, vs2, -   | vd = (vs1 != vs2)                         | Vector compare not equal       | 1    |
| VCMPLT   | 0xC8   | R      | vd, vs1, vs2, -   | vd = (vs1 < vs2)                          | Vector compare less than       | 2    |
| VCMPLE   | 0xC8   | R      | vd, vs1, vs2, -   | vd = (vs1 <= vs2)                        | Vector compare less or equal   | 3    |
| VREDUCE  | 0xC9   | I      | rd, vd, imm15     | rd = reduce(vd, imm15)                    | Vector reduction              | -    |
| VFMA     | 0xCA   | R      | vd, vs1, vs2, -   | vd = vd + vs1 * vs2                      | Fused multiply-add             | 0    |
| VFMS     | 0xCA   | R      | vd, vs1, vs2, -   | vd = vd - vs1 * vs2                      | Fused multiply-sub             | 1    |

### 5.15 System & Debug Instructions

| Mnemonic | Opcode | Format | Operands        | Operation                        | Description                    |
|----------|--------|--------|-----------------|----------------------------------|--------------------------------|
| SYSCALL  | 0xF0   | I      | rd, -, imm15    | rd = syscall(imm15)             | Invoke system call             |
| TRAP     | 0xF1   | I      | -, -, imm15     | software_breakpoint(imm15)       | Software breakpoint            |
| DEBUG    | 0xF2   | R      | rs, -, -, -     | print_register(rs)               | Print register (debug)         |
| RDCOUNT  | 0xF3   | I      | rd, -, imm15    | rd = read_counter(imm15)         | Read performance counter       |
| BARRIER  | 0xFE   | R      | -, -, -, -      | memory_barrier()                 | Memory barrier                 |
| NOP      | 0xFF   | R      | -, -, -, -      |                                  | No operation                    |

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
- Generics are monomorphized at compile time

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
| 0x01-0x0F    | Data movement & basic ALU    |
| 0x10-0x1F    | Extended integer arithmetic  |
| 0x20-0x2F    | Floating-point operations    |
| 0x30-0x3F    | Logical & bitwise operations |
| 0x40-0x4F    | Comparison operations        |
| 0x50-0x5F    | Branch operations            |
| 0x60-0x6F    | Jump operations              |
| 0x70-0x7F    | Memory load/store            |
| 0x80-0x8F    | Stack management              |
| 0x90-0x9F    | Object & array operations    |
| 0xA0-0xAF    | Call & dynamic linking       |
| 0xB0-0xBF    | Conversion & type handling   |
| 0xC0-0xCF    | Vector/SIMD operations       |
| 0xF0-0xFF    | System & debug operations    |

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

## Appendix D: Glossary

| Term | Definition |
|------|------------|
| **GPR** | General Purpose Register |
| **Vtable** | Virtual method table for dynamic dispatch |
| **ABI** | Application Binary Interface |
| **SSA** | Static Single Assignment (IR form) |
| **JIT** | Just-In-Time compilation |
| **AOT** | Ahead-Of-Time compilation |
| **GC** | Garbage Collector |
| **SIMD** | Single Instruction, Multiple Data |
| **Lane** | Single element within a vector register |
| **Sign-extend** | Extend value with sign bit to fill higher bits |
| **Zero-extend** | Fill higher bits with zeros |
| **PC** | Program Counter (instruction pointer) |
| **SP** | Stack Pointer |
| **FP** | Frame Pointer |

---

*Document Version: 1.2*
*Last Updated: April 2026*
