# HVM Instruction Reference Manual

**Version:** 1.0
**Document ID:** HVM-REF-001
**Status:** Normative

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [Data Movement Instructions](#2-data-movement-instructions)
3. [Integer Arithmetic Instructions](#3-integer-arithmetic-instructions)
4. [Shift Instructions](#4-shift-instructions)
5. [Bitwise Logic Instructions](#5-bitwise-logic-instructions)
6. [Bit Manipulation Instructions](#6-bit-manipulation-instructions)
7. [Floating-Point Instructions](#7-floating-point-instructions)
8. [Single-Precision FP Instructions](#8-single-precision-floating-point-instructions)
9. [Integer Comparison Instructions](#9-integer-comparison-instructions)
10. [Floating-Point Comparison Instructions](#10-floating-point-comparison-instructions)
11. [Set Instructions](#11-set-instructions)
12. [Branch Instructions](#12-branch-instructions)
13. [Jump Instructions](#13-jump-instructions)
14. [Load Instructions](#14-load-instructions)
15. [Store Instructions](#15-store-instructions)
16. [Stack Operations](#16-stack-operations)
17. [String Operations](#17-string-operations)
18. [Object and Array Operations](#18-object-and-array-operations)
19. [Function Call Instructions](#19-function-call-instructions)
20. [Threading and Synchronization](#20-threading-and-synchronization)
21. [Atomic Operations](#21-atomic-operations)
22. [Type Conversion](#22-type-conversion)
23. [Vector Instructions](#23-vector-instructions)
24. [Exception Handling](#24-exception-handling)
25. [Interrupt Instructions](#25-interrupt-instructions)
26. [FFI and Native Calls](#26-ffi-and-native-calls)
27. [System and Debug Instructions](#27-system-and-debug-instructions)
28. [Packed SIMD Instructions](#28-packed-simd-instructions)
29. [Extended Floating-Point Operations](#29-extended-floating-point-operations)
30. [Extended Atomic Operations](#30-extended-atomic-operations)
A. [Instruction Encoding Formats](#a-instruction-encoding-formats)
B. [Opcode Map Summary](#b-opcode-map-summary)

---

## 1. Introduction

This document provides comprehensive reference documentation for the HVM (Hooc Virtual Machine) instruction set. Each instruction is documented with syntax, semantics, encoding details, and usage examples.

### 1.1 Instruction Format Notation

Instructions are documented using the following notation:

| Symbol | Meaning |
|--------|---------|
| `rd` | Destination register |
| `rs`, `rs1`, `rs2` | Source register(s) |
| `imm`, `imm15` | Immediate value (15-bit signed unless specified) |
| `addr` | Memory address register |
| `-` | Unused field |

### 1.2 Register Conventions

| Register | Purpose |
|----------|---------|
| `r0` | Hardwired zero |
| `r1` | First argument / return value |
| `r2-r8` | Additional arguments |
| `r30` | Frame pointer (optional) |
| `r31` | Stack pointer |
| `v0-v15` | Vector registers (128-bit) |

---

## 2. Data Movement Instructions

### 2.1 NOP — No Operation

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x00` |
| **Format** | R |
| **Operands** | — |

**Semantics:** Performs no operation. Execution continues with the next sequential instruction.

**Usage:** Instruction `NOP` is typically employed for:
- Software timing delays
- Alignment padding
- Conditional instruction placeholders

**Encoding:**
```
[  0x00  ][  rd  ][  rs1 ][  rs2 ][   func   ]
   7 bits   5 bits  5 bits  5 bits   10 bits
```

---

### 2.2 MOV — Move Register

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x01` |
| **Format** | R |
| **Operands** | `rd, rs` |

**Semantics:** Copies the value from source register `rs` into destination register `rd`.

**Operation:** `rd ← rs`

**Pseudocode:**
```c
rd = rs;
```

**Example:**
```asm
; Copy r2 to r1
MOV   r1, r2          ; r1 ← r2
```

---

### 2.3 MOVI — Move Immediate

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x02` |
| **Format** | I |
| **Operands** | `rd, rs, imm15` |

**Semantics:** Adds a sign-extended 15-bit immediate value to register `rs` and stores the result in `rd`.

**Operation:** `rd ← rs + sext(imm15)`

**Pseudocode:**
```c
rd = rs + sign_extend(imm15);
```

**Example:**
```asm
; Add 42 to r0 (which is 0), result in r1
MOVI  r1, r0, 42      ; r1 ← 0 + 42 = 42

; Add 100 to existing value in r2
MOVI  r3, r2, 100     ; r3 ← r2 + 100
```

---

### 2.4 MOVZ — Move with Zero-Extended Immediate

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x03` |
| **Format** | I |
| **Operands** | `rd, rs, imm15` |

**Semantics:** Zero-extends the 15-bit immediate and ORs it with `rs`, storing the result in `rd`. Used when the immediate value should not be sign-extended.

**Operation:** `rd ← rs | zext(imm15)`

**Pseudocode:**
```c
rd = rs | zero_extend(imm15);
```

**Example:**
```asm
; Load unsigned byte value 0xFF
MOVZ  r1, r0, 0xFF    ; r1 ← 0 | 0xFF = 255

; Construct value with specific lower bits
MOVZ  r2, r1, 0x0F    ; r2 ← r1 | 0x0F
```

---

### 2.5 LUI — Load Upper Immediate

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x04` |
| **Format** | I |
| **Operands** | `rd, rs, imm15` |

**Semantics:** Loads the upper 32 bits of a 64-bit register by ORing `rs` with `imm15` shifted left by 32 bits. Used in combination with `MOVI` to construct 64-bit constants.

**Operation:** `rd ← rs | (imm15 << 32)`

**Pseudocode:**
```c
rd = rs | ((uint64_t)imm15 << 32);
```

**Example:**
```asm
; Construct 64-bit constant 0x123456789ABCDEF0
LUI   r1, r0, 0x12345   ; r1 ← 0x123450000000000
MOVI  r1, r1, 0x6789ABC ; r1 ← 0x123456789ABC000
MOVI  r1, r1, 0xDEF0    ; r1 ← 0x123456789ABCDEF0
```

---

### 2.6 ADDI — Add Immediate

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x05` |
| **Format** | I |
| **Operands** | `rd, rs, imm15` |

**Semantics:** Adds a sign-extended 15-bit immediate to register `rs` and stores the result in `rd`.

**Operation:** `rd ← rs + sext(imm15)`

**Pseudocode:**
```c
rd = rs + sign_extend(imm15);
```

**Example:**
```asm
; Add 10 to r2, result in r1
ADDI  r1, r2, 10        ; r1 ← r2 + 10
```

---

### 2.7 SUBI — Subtract Immediate

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x06` |
| **Format** | I |
| **Operands** | `rd, rs, imm15` |

**Semantics:** Subtracts a sign-extended 15-bit immediate from register `rs` and stores the result in `rd`.

**Operation:** `rd ← rs - sext(imm15)`

**Pseudocode:**
```c
rd = rs - sign_extend(imm15);
```

**Example:**
```asm
; Subtract 5 from r2
SUBI  r1, r2, 5         ; r1 ← r2 - 5
```

---

### 2.8 NEG — Negate

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x07` |
| **Format** | R |
| **Operands** | `rd, rs` |

**Semantics:** Negates the value in register `rs` using two's complement arithmetic and stores the result in `rd`.

**Operation:** `rd ← -rs`

**Pseudocode:**
```c
rd = -rs;
```

**Example:**
```asm
; Negate r2
NEG   r1, r2             ; r1 ← -r2
```

---

### 2.9 XCHG — Exchange Registers

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x08` |
| **Format** | R |
| **Operands** | `rd, rs` |

**Semantics:** Atomically swaps the values in registers `rd` and `rs`.

**Operation:** `swap(rd, rs)`

**Pseudocode:**
```c
temp = rd;
rd = rs;
rs = temp;
```

**Example:**
```asm
; Swap values of r1 and r2
XCHG  r1, r2             ; r1 ↔ r2
```

---

## 3. Integer Arithmetic Instructions

### 3.1 ADD — Add Registers

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x10` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `0` |

**Semantics:** Adds the values in registers `rs1` and `rs2`, storing the result in `rd`. Integer overflow wraps around.

**Operation:** `rd ← rs1 + rs2`

**Pseudocode:**
```c
rd = rs1 + rs2;
```

**Example:**
```asm
; Add r2 and r3, store result in r1
ADD   r1, r2, r3         ; r1 ← r2 + r3
```

---

### 3.2 SUB — Subtract Registers

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x10` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `1` |

**Semantics:** Subtracts the value in `rs2` from `rs1`, storing the result in `rd`.

**Operation:** `rd ← rs1 - rs2`

**Pseudocode:**
```c
rd = rs1 - rs2;
```

**Example:**
```asm
; Subtract r3 from r2, store in r1
SUB   r1, r2, r3         ; r1 ← r2 - r3
```

---

### 3.3 MUL — Multiply

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x10` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `2` |

**Semantics:** Multiplies `rs1` by `rs2`, storing the low 64 bits of the result in `rd`.

**Operation:** `rd ← (rs1 × rs2)[63:0]`

**Pseudocode:**
```c
rd = (rs1 * rs2) & 0xFFFFFFFFFFFFFFFF;
```

**Example:**
```asm
; Multiply r2 by r3, store low 64 bits in r1
MUL   r1, r2, r3         ; r1 ← r2 × r3 (low bits)
```

---

### 3.4 MULH — Multiply High Unsigned

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x10` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `3` |

**Semantics:** Multiplies `rs1` by `rs2` as unsigned values, storing the high 64 bits of the 128-bit result in `rd`. Used for implementing unsigned 128-bit multiplication.

**Operation:** `rd ← (rs1 × rs2) >> 64` (unsigned)

**Pseudocode:**
```c
rd = (unsigned __int128(rs1) * unsigned __int128(rs2)) >> 64;
```

**Example:**
```asm
; Get high 64 bits of unsigned product
MULH  r1, r2, r3         ; r1 ← upper 64 bits of r2 × r3
```

---

### 3.5 MULHS — Multiply High Signed

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x10` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `4` |

**Semantics:** Multiplies `rs1` by `rs2` as signed values, storing the high 64 bits of the result in `rd`.

**Operation:** `rd ← (rs1 × rs2)[127:64]` (signed)

**Pseudocode:**
```c
rd = (signed __int128(rs1) * signed __int128(rs2)) >> 64;
```

**Example:**
```asm
; Get high 64 bits of signed product
MULHS r1, r2, r3         ; r1 ← upper 64 bits of r2 × r3 (signed)
```

---

### 3.6 DIV — Divide Signed

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x10` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `5` |

**Semantics:** Divides `rs1` by `rs2` using signed integer division. If `rs2` is zero, the result is undefined.

**Operation:** `rd ← rs1 / rs2` (signed)

**Pseudocode:**
```c
rd = rs1 / rs2;  // signed division
```

**Example:**
```asm
; Signed division: r2 / r3
DIV   r1, r2, r3         ; r1 ← r2 ÷ r3 (signed)
```

---

### 3.7 DIVU — Divide Unsigned

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x10` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `6` |

**Semantics:** Divides `rs1` by `rs2` using unsigned integer division.

**Operation:** `rd ← rs1 / rs2` (unsigned)

**Pseudocode:**
```c
rd = (unsigned)rs1 / (unsigned)rs2;
```

**Example:**
```asm
; Unsigned division
DIVU  r1, r2, r3         ; r1 ← r2 ÷ r3 (unsigned)
```

---

### 3.8 REM — Remainder Signed

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x10` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `7` |

**Semantics:** Computes the signed remainder of `rs1` divided by `rs2`.

**Operation:** `rd ← rs1 % rs2` (signed)

**Pseudocode:**
```c
rd = rs1 % rs2;  // signed remainder
```

**Example:**
```asm
; Signed remainder: r2 % r3
REM   r1, r2, r3         ; r1 ← r2 % r3 (signed)
```

---

### 3.9 MULI — Multiply Immediate

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x11` |
| **Format** | I |
| **Operands** | `rd, rs, imm15` |

**Semantics:** Multiplies `rs` by a sign-extended immediate value.

**Operation:** `rd ← rs × sext(imm15)`

**Pseudocode:**
```c
rd = rs * sign_extend(imm15);
```

**Example:**
```asm
; Multiply r2 by 10
MULI  r1, r2, 10         ; r1 ← r2 × 10
```

---

### 3.10 DIVI — Divide Immediate

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x12` |
| **Format** | I |
| **Operands** | `rd, rs, imm15` |

**Semantics:** Divides `rs` by a sign-extended immediate value using signed division.

**Operation:** `rd ← rs / sext(imm15)` (signed)

**Pseudocode:**
```c
rd = rs / sign_extend(imm15);  // signed division
```

**Example:**
```asm
; Divide r2 by 10
DIVI  r1, r2, 10         ; r1 ← r2 / 10
```

---

## 4. Shift Instructions

### 4.1 SHL — Shift Left Logical

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x13` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `0` |

**Semantics:** Logically shifts the value in `rs1` left by the number of bits specified in `rs2[5:0]`. Low-order bits are zero-filled.

**Operation:** `rd ← rs1 << (rs2 & 63)`

**Pseudocode:**
```c
rd = rs1 << (rs2 & 63);
```

**Example:**
```asm
; Shift left by 4 bits
SHL   r1, r2, r3         ; r1 ← r2 << (r3 & 63)
```

---

### 4.2 SHR — Shift Right Logical

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x13` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `1` |

**Semantics:** Logically shifts the value in `rs1` right by the number of bits specified in `rs2[5:0]`. High-order bits are zero-filled.

**Operation:** `rd ← rs1 >> (rs2 & 63)` (zero-fill)

**Pseudocode:**
```c
rd = rs1 >> (rs2 & 63);  // zero-fill
```

**Example:**
```asm
; Shift right logical by 4 bits
SHR   r1, r2, r3         ; r1 ← r2 >> (r3 & 63)
```

---

### 4.3 SAR — Shift Right Arithmetic

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x13` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `2` |

**Semantics:** Arithmetically shifts the value in `rs1` right by the number of bits specified in `rs2[5:0]`. High-order bits are filled with the sign bit.

**Operation:** `rd ← rs1 >> (rs2 & 63)` (sign-extend)

**Pseudocode:**
```c
rd = ((int64_t)rs1) >> (rs2 & 63);  // arithmetic shift
```

**Example:**
```asm
; Shift right arithmetic by 4 bits
SAR   r1, r2, r3         ; r1 ← r2 >> (r3 & 63) (sign-extend)
```

---

### 4.4 SHLI — Shift Left Immediate

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x14` |
| **Format** | I |
| **Operands** | `rd, rs, imm15` |

**Semantics:** Logically shifts the value in `rs` left by the number of bits specified by the immediate value.

**Operation:** `rd ← rs << (imm15 & 63)`

**Pseudocode:**
```c
rd = rs << (imm15 & 63);
```

**Example:**
```asm
; Shift left by 8 bits
SHLI  r1, r2, 8          ; r1 ← r2 << 8
```

---

## 5. Bitwise Logic Instructions

### 5.1 AND — Bitwise AND

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x20` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `0` |

**Semantics:** Performs bitwise AND between `rs1` and `rs2`, storing the result in `rd`.

**Operation:** `rd ← rs1 & rs2`

**Example:**
```asm
; Bitwise AND
AND   r1, r2, r3         ; r1 ← r2 & r3
```

---

### 5.2 OR — Bitwise OR

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x20` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `1` |

**Semantics:** Performs bitwise OR between `rs1` and `rs2`, storing the result in `rd`.

**Operation:** `rd ← rs1 | rs2`

**Example:**
```asm
; Bitwise OR
OR    r1, r2, r3         ; r1 ← r2 | r3
```

---

### 5.3 XOR — Bitwise XOR

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x20` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `2` |

**Semantics:** Performs bitwise XOR between `rs1` and `rs2`, storing the result in `rd`.

**Operation:** `rd ← rs1 ^ rs2`

**Example:**
```asm
; Bitwise XOR
XOR   r1, r2, r3         ; r1 ← r2 ^ r3
```

---

### 5.4 NOT — Bitwise NOT

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x21` |
| **Format** | R |
| **Operands** | `rd, rs` |

**Semantics:** Performs bitwise NOT on `rs`, storing the result in `rd`.

**Operation:** `rd ← ~rs`

**Example:**
```asm
; Bitwise NOT
NOT   r1, r2             ; r1 ← ~r2
```

---

### 5.5 ANDI — Bitwise AND Immediate

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x22` |
| **Format** | I |
| **Operands** | `rd, rs, imm15` |

**Semantics:** Performs bitwise AND between `rs` and a sign-extended immediate.

**Operation:** `rd ← rs & sext(imm15)`

**Example:**
```asm
; AND with mask 0xFF
ANDI  r1, r2, 0xFF       ; r1 ← r2 & 0xFF
```

---

### 5.6 ORI — Bitwise OR Immediate

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x23` |
| **Format** | I |
| **Operands** | `rd, rs, imm15` |

**Semantics:** Performs bitwise OR between `rs` and a sign-extended immediate.

**Operation:** `rd ← rs | sext(imm15)`

**Example:**
```asm
; OR with mask
ORI   r1, r2, 0xF0       ; r1 ← r2 | 0xF0
```

---

### 5.7 XORI — Bitwise XOR Immediate

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x24` |
| **Format** | I |
| **Operands** | `rd, rs, imm15` |

**Semantics:** Performs bitwise XOR between `rs` and a sign-extended immediate.

**Operation:** `rd ← rs ^ sext(imm15)`

**Example:**
```asm
; XOR with mask
XORI  r1, r2, 0xFF       ; r1 ← r2 ^ 0xFF
```

---

## 6. Bit Manipulation Instructions

### 6.1 CLZ — Count Leading Zeros

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x25` |
| **Format** | R |
| **Operands** | `rd, rs` |

**Semantics:** Counts the number of leading zero bits in `rs` and stores the result in `rd`. For a 64-bit value of zero, the result is 64.

**Operation:** `rd ← count_leading_zeros(rs)`

**Example:**
```asm
; Count leading zeros in r2
CLZ   r1, r2             ; r1 ← number of leading zeros in r2
```

---

### 6.2 CTZ — Count Trailing Zeros

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x26` |
| **Format** | R |
| **Operands** | `rd, rs` |

**Semantics:** Counts the number of trailing zero bits in `rs` and stores the result in `rd`. For a 64-bit value of zero, the result is 64.

**Operation:** `rd ← count_trailing_zeros(rs)`

**Example:**
```asm
; Count trailing zeros in r2
CTZ   r1, r2             ; r1 ← number of trailing zeros in r2
```

---

### 6.3 POPCNT — Population Count

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x27` |
| **Format** | R |
| **Operands** | `rd, rs` |

**Semantics:** Counts the number of set (1) bits in `rs` and stores the result in `rd`.

**Operation:** `rd ← population_count(rs)`

**Example:**
```asm
; Count set bits in r2
POPCNT r1, r2            ; r1 ← number of 1-bits in r2
```

---

## 7. Floating-Point Instructions (f64)

### 7.1 FADD — Floating-Point Add

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x30` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `0` |

**Semantics:** Adds the floating-point values in `rs1` and `rs2`, storing the result in `rd`.

**Operation:** `rd ← rs1 + rs2` (f64)

**Example:**
```asm
; Add floating-point values
FADD  f1, f2, f3         ; f1 ← f2 + f3
```

---

### 7.2 FSUB — Floating-Point Subtract

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x30` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `1` |

**Semantics:** Subtracts the floating-point value in `rs2` from `rs1`, storing the result in `rd`.

**Operation:** `rd ← rs1 - rs2` (f64)

**Example:**
```asm
; Subtract floating-point values
FSUB  f1, f2, f3         ; f1 ← f2 - f3
```

---

### 7.3 FMUL — Floating-Point Multiply

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x30` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `2` |

**Semantics:** Multiplies the floating-point values in `rs1` and `rs2`, storing the result in `rd`.

**Operation:** `rd ← rs1 × rs2` (f64)

**Example:**
```asm
; Multiply floating-point values
FMUL  f1, f2, f3         ; f1 ← f2 × f3
```

---

### 7.4 FDIV — Floating-Point Divide

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x30` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `3` |

**Semantics:** Divides the floating-point value in `rs1` by `rs2`, storing the result in `rd`.

**Operation:** `rd ← rs1 / rs2` (f64)

**Example:**
```asm
; Divide floating-point values
FDIV  f1, f2, f3         ; f1 ← f2 / f3
```

---

### 7.5 FSQRT — Floating-Point Square Root

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x31` |
| **Format** | R |
| **Operands** | `rd, rs` |

**Semantics:** Computes the square root of the floating-point value in `rs` and stores the result in `rd`.

**Operation:** `rd ← sqrt(rs)`

**Example:**
```asm
; Compute square root
FSQRT f1, f2             ; f1 ← sqrt(f2)
```

---

### 7.6 FABS — Floating-Point Absolute Value

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x32` |
| **Format** | R |
| **Operands** | `rd, rs` |

**Semantics:** Computes the absolute value of the floating-point value in `rs` and stores the result in `rd`.

**Operation:** `rd ← abs(rs)`

**Example:**
```asm
; Absolute value
FABS  f1, f2             ; f1 ← abs(f2)
```

---

### 7.7 FNEG — Floating-Point Negate

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x33` |
| **Format** | R |
| **Operands** | `rd, rs` |

**Semantics:** Negates the floating-point value in `rs` and stores the result in `rd.

**Operation:** `rd ← -rs`

**Example:**
```asm
; Negate floating-point value
FNEG  f1, f2             ; f1 ← -f2
```

---

## 8. Single-Precision Floating-Point Instructions (f32)

### 8.1 FADD32 — Floating-Point Add (32-bit)

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x34` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `0` |

**Semantics:** Adds the 32-bit floating-point values in `rs1` and `rs2`, storing the result in `rd`.

**Operation:** `rd ← rs1 + rs2` (f32)

**Example:**
```asm
; Add 32-bit floating-point values
FADD32 f1, f2, f3        ; f1 ← f2 + f3 (32-bit)
```

---

### 8.2 FSUB32 — Floating-Point Subtract (32-bit)

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x34` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `1` |

**Semantics:** Subtracts the 32-bit floating-point value in `rs2` from `rs1`.

**Operation:** `rd ← rs1 - rs2` (f32)

**Example:**
```asm
; Subtract 32-bit floating-point values
FSUB32 f1, f2, f3        ; f1 ← f2 - f3 (32-bit)
```

---

### 8.3 FMUL32 — Floating-Point Multiply (32-bit)

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x34` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `2` |

**Semantics:** Multiplies the 32-bit floating-point values in `rs1` and `rs2`.

**Operation:** `rd ← rs1 × rs2` (f32)

**Example:**
```asm
; Multiply 32-bit floating-point values
FMUL32 f1, f2, f3        ; f1 ← f2 × f3 (32-bit)
```

---

### 8.4 FDIV32 — Floating-Point Divide (32-bit)

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x34` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `3` |

**Semantics:** Divides the 32-bit floating-point value in `rs1` by `rs2`.

**Operation:** `rd ← rs1 / rs2` (f32)

**Example:**
```asm
; Divide 32-bit floating-point values
FDIV32 f1, f2, f3        ; f1 ← f2 / f3 (32-bit)
```

---

### 8.5 FCVT — Floating-Point Convert

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x35` |
| **Format** | I |
| **Operands** | `rd, rs, imm15` |

**Semantics:** Converts the value in `rs` to the type specified by `imm15`.

**Conversion Types (imm15):**
| Value | Conversion |
|-------|------------|
| 0 | i64 → f64 |
| 1 | u64 → f64 |
| 2 | f64 → i64 |
| 3 | f64 → u64 |
| 4 | f32 → f64 |
| 5 | f64 → f32 |

**Example:**
```asm
; Convert integer to double
FCVT  f1, r2, 0          ; f1 ← (double) r2
```

---

## 9. Integer Comparison Instructions

### 9.1 CMPEQ — Compare Equal

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x40` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `0` |

**Semantics:** Sets `rd` to 1 if `rs1` equals `rs2`, otherwise sets `rd` to 0.

**Operation:** `rd ← (rs1 == rs2) ? 1 : 0`

**Example:**
```asm
; Compare r2 and r3 for equality
CMPEQ r1, r2, r3         ; r1 ← 1 if r2 == r3, else 0
```

---

### 9.2 CMPNE — Compare Not Equal

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x40` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `1` |

**Semantics:** Sets `rd` to 1 if `rs1` does not equal `rs2`, otherwise sets `rd` to 0.

**Operation:** `rd ← (rs1 != rs2) ? 1 : 0`

**Example:**
```asm
; Compare r2 and r3 for inequality
CMPNE r1, r2, r3         ; r1 ← 1 if r2 != r3, else 0
```

---

### 9.3 CMPLT — Compare Less Than (Signed)

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x40` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `2` |

**Semantics:** Sets `rd` to 1 if `rs1` is less than `rs2` (signed comparison), otherwise sets `rd` to 0.

**Operation:** `rd ← (rs1 < rs2) ? 1 : 0` (signed)

**Example:**
```asm
; Signed comparison: r2 < r3
CMPLT r1, r2, r3         ; r1 ← 1 if r2 < r3 (signed), else 0
```

---

### 9.4 CMPLE — Compare Less or Equal (Signed)

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x40` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `3` |

**Semantics:** Sets `rd` to 1 if `rs1` is less than or equal to `rs2` (signed comparison).

**Operation:** `rd ← (rs1 <= rs2) ? 1 : 0` (signed)

**Example:**
```asm
; Signed comparison: r2 <= r3
CMPLE r1, r2, r3         ; r1 ← 1 if r2 ≤ r3 (signed), else 0
```

---

### 9.5 CMPGT — Compare Greater Than (Signed)

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x40` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `4` |

**Semantics:** Sets `rd` to 1 if `rs1` is greater than `rs2` (signed comparison).

**Operation:** `rd ← (rs1 > rs2) ? 1 : 0` (signed)

**Example:**
```asm
; Signed comparison: r2 > r3
CMPGT r1, r2, r3         ; r1 ← 1 if r2 > r3 (signed), else 0
```

---

### 9.6 CMPGE — Compare Greater or Equal (Signed)

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x40` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `5` |

**Semantics:** Sets `rd` to 1 if `rs1` is greater than or equal to `rs2` (signed comparison).

**Operation:** `rd ← (rs1 >= rs2) ? 1 : 0` (signed)

**Example:**
```asm
; Signed comparison: r2 >= r3
CMPGE r1, r2, r3         ; r1 ← 1 if r2 ≥ r3 (signed), else 0
```

---

### 9.7 CMPLTU — Compare Less Than (Unsigned)

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x40` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `6` |

**Semantics:** Sets `rd` to 1 if `rs1` is less than `rs2` (unsigned comparison).

**Operation:** `rd ← (rs1 < rs2) ? 1 : 0` (unsigned)

**Example:**
```asm
; Unsigned comparison: r2 < r3
CMPLTU r1, r2, r3        ; r1 ← 1 if r2 < r3 (unsigned), else 0
```

---

### 9.8 CMPLEU — Compare Less or Equal (Unsigned)

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x40` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `7` |

**Semantics:** Sets `rd` to 1 if `rs1` is less than or equal to `rs2` (unsigned comparison).

**Operation:** `rd ← (rs1 <= rs2) ? 1 : 0` (unsigned)

**Example:**
```asm
; Unsigned comparison: r2 <= r3
CMPLEU r1, r2, r3        ; r1 ← 1 if r2 ≤ r3 (unsigned), else 0
```

---

## 10. Floating-Point Comparison Instructions

### 10.1 FCMPEQ — Floating-Point Compare Equal

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x41` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `0` |

**Semantics:** Sets `rd` to 1 if `rs1` equals `rs2` (floating-point comparison), otherwise sets `rd` to 0.

**Operation:** `rd ← (rs1 == rs2) ? 1 : 0` (f64)

**Example:**
```asm
; Compare floating-point values for equality
FCMPEQ r1, f2, f3        ; r1 ← 1 if f2 == f3, else 0
```

---

### 10.2 FCMPLT — Floating-Point Compare Less Than

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x41` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `1` |

**Semantics:** Sets `rd` to 1 if `rs1` is less than `rs2` (floating-point comparison).

**Operation:** `rd ← (rs1 < rs2) ? 1 : 0` (f64)

**Example:**
```asm
; Compare floating-point values
FCMPLT r1, f2, f3        ; r1 ← 1 if f2 < f3, else 0
```

---

### 10.3 FCMPLE — Floating-Point Compare Less or Equal

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x41` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `2` |

**Semantics:** Sets `rd` to 1 if `rs1` is less than or equal to `rs2` (floating-point comparison).

**Operation:** `rd ← (rs1 <= rs2) ? 1 : 0` (f64)

**Example:**
```asm
; Compare floating-point values
FCMPLE r1, f2, f3        ; r1 ← 1 if f2 ≤ f3, else 0
```

---

### 10.4 FCMPGT — Floating-Point Compare Greater Than

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x41` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `4` |

**Semantics:** Sets `rd` to 1 if `rs1` is greater than `rs2` (floating-point comparison).

**Operation:** `rd ← (rs1 > rs2) ? 1 : 0` (f64)

**Example:**
```asm
; Compare floating-point values
FCMPGT r1, f2, f3        ; r1 ← 1 if f2 > f3, else 0
```

---

### 10.5 FCMPGE — Floating-Point Compare Greater or Equal

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x41` |
| **Format** | R |
| **Operands** | `rd, rs1, rs2` |
| **Func** | `5` |

**Semantics:** Sets `rd` to 1 if `rs1` is greater than or equal to `rs2` (floating-point comparison).

**Operation:** `rd ← (rs1 >= rs2) ? 1 : 0` (f64)

**Example:**
```asm
; Compare floating-point values
FCMPGE r1, f2, f3        ; r1 ← 1 if f2 ≥ f3, else 0
```

---

## 11. Set Instructions

### 11.1 SET — Set from Immediate

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x42` |
| **Format** | I |
| **Operands** | `rd, imm15` |

**Semantics:** Sets `rd` to the value of `imm15` AND 1 (extracts the least significant bit).

**Operation:** `rd ← imm15 & 1`

**Example:**
```asm
; Set r1 to 1
SET   r1, 1               ; r1 ← 1

; Set r2 to 0
SET   r2, 0               ; r2 ← 0
```

---

## 12. Branch Instructions

### 12.1 BEQ — Branch if Equal

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x50` |
| **Format** | B |
| **Operands** | `rs1, rs2, imm15` |

**Semantics:** If `rs1` equals `rs2`, branches to the target address calculated as `PC + sext(imm15) * 2`.

**Operation:** `if (rs1 == rs2) pc += sext(imm15) * 2`

**Example:**
```asm
; Branch if r1 equals r2
BEQ   r1, r2, .label      ; if r1 == r2, jump to .label
```

---

### 12.2 BNE — Branch if Not Equal

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x51` |
| **Format** | B |
| **Operands** | `rs1, rs2, imm15` |

**Semantics:** If `rs1` does not equal `rs2`, branches to the target address.

**Operation:** `if (rs1 != rs2) pc += sext(imm15) * 2`

**Example:**
```asm
; Branch if r1 not equal to r2
BNE   r1, r2, .label      ; if r1 != r2, jump to .label
```

---

### 12.3 BLT — Branch if Less Than (Signed)

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x52` |
| **Format** | B |
| **Operands** | `rs1, rs2, imm15` |

**Semantics:** If `rs1` is less than `rs2` (signed comparison), branches to the target address.

**Operation:** `if (rs1 < rs2) pc += sext(imm15) * 2` (signed)

**Example:**
```asm
; Branch if r1 < r2 (signed)
BLT   r1, r2, .label      ; if r1 < r2, jump to .label
```

---

### 12.4 BLE — Branch if Less or Equal (Signed)

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x53` |
| **Format** | B |
| **Operands** | `rs1, rs2, imm15` |

**Semantics:** If `rs1` is less than or equal to `rs2` (signed comparison), branches to the target address.

**Operation:** `if (rs1 <= rs2) pc += sext(imm15) * 2` (signed)

**Example:**
```asm
; Branch if r1 <= r2 (signed)
BLE   r1, r2, .label      ; if r1 ≤ r2, jump to .label
```

---

### 12.5 BGT — Branch if Greater Than (Signed)

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x54` |
| **Format** | B |
| **Operands** | `rs1, rs2, imm15` |

**Semantics:** If `rs1` is greater than `rs2` (signed comparison), branches to the target address.

**Operation:** `if (rs1 > rs2) pc += sext(imm15) * 2` (signed)

**Example:**
```asm
; Branch if r1 > r2 (signed)
BGT   r1, r2, .label      ; if r1 > r2, jump to .label
```

---

### 12.6 BGE — Branch if Greater or Equal (Signed)

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x55` |
| **Format** | B |
| **Operands** | `rs1, rs2, imm15` |

**Semantics:** If `rs1` is greater than or equal to `rs2` (signed comparison), branches to the target address.

**Operation:** `if (rs1 >= rs2) pc += sext(imm15) * 2` (signed)

**Example:**
```asm
; Branch if r1 >= r2 (signed)
BGE   r1, r2, .label      ; if r1 ≥ r2, jump to .label
```

---

### 12.7 BLTU — Branch if Less Than (Unsigned)

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x56` |
| **Format** | B |
| **Operands** | `rs1, rs2, imm15` |

**Semantics:** If `rs1` is less than `rs2` (unsigned comparison), branches to the target address.

**Operation:** `if (rs1 < rs2) pc += sext(imm15) * 2` (unsigned)

**Example:**
```asm
; Branch if r1 < r2 (unsigned)
BLTU  r1, r2, .label      ; if r1 < r2, jump to .label
```

---

### 12.8 BGEU — Branch if Greater or Equal (Unsigned)

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x57` |
| **Format** | B |
| **Operands** | `rs1, rs2, imm15` |

**Semantics:** If `rs1` is greater than or equal to `rs2` (unsigned comparison), branches to the target address.

**Operation:** `if (rs1 >= rs2) pc += sext(imm15) * 2` (unsigned)

**Example:**
```asm
; Branch if r1 >= r2 (unsigned)
BGEU  r1, r2, .label      ; if r1 ≥ r2, jump to .label
```

---

## 13. Jump Instructions

### 13.1 JMP — Unconditional Jump

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x60` |
| **Format** | J |
| **Operands** | `offset` |

**Semantics:** Unconditionally jumps to the target address calculated as `PC + sext(offset) * 2`.

**Operation:** `pc += sext(offset) * 2`

**Example:**
```asm
; Unconditional jump
JMP   .end               ; Jump to .end label
MOVI  r1, r0, 99        ; This instruction is skipped
.end:
```

---

### 13.2 JAL — Jump and Link

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x61` |
| **Format** | J |
| **Operands** | `rd, offset` |

**Semantics:** Jumps to the target address and saves the return address (`PC + 4`) in `rd`. Used for function calls.

**Operation:** `rd ← pc + 4; pc += sext(offset) * 2`

**Example:**
```asm
; Function call
JAL   r1, .my_function   ; r1 = return address, jump to .my_function
; Return address is saved in r1
```

---

### 13.3 JALR — Jump and Link Register

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x62` |
| **Format** | I |
| **Operands** | `rd, rs, imm15` |

**Semantics:** Jumps to the address in `rs` plus the immediate offset, and saves the return address in `rd`.

**Operation:** `rd ← pc + 4; pc ← rs + sext(imm15)`

**Usage:** Indirect function calls, virtual method dispatch, case/switch statements.

**Example:**
```asm
; Indirect call via register
MOVI  r10, r0, .func_addr ; r10 = address of function
JALR  r1, r10, 0         ; Call function at r10, save return in r1
```

---

### 13.4 RET — Return from Subroutine

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x63` |
| **Format** | R |
| **Operands** | — |

**Semantics:** Returns from a subroutine by jumping to the address in `r1`. Assumes `r1` contains the return address saved by `JAL` or `JALR`.

**Operation:** `pc ← r1`

**Example:**
```asm
; Function epilogue
RET                    ; Return to caller (pc ← r1)
```

---

## 14. Load Instructions

### 14.1 LD.B — Load Signed Byte

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x70` |
| **Format** | I |
| **Operands** | `rd, addr, imm15` |

**Semantics:** Loads an 8-bit byte from memory, sign-extends it to 64 bits, and stores the result in `rd`.

**Operation:** `rd ← sext(mem[addr + sext(imm15)]:8)`

**Example:**
```asm
; Load signed byte
MOVI  r2, r0, 0         ; r2 = base address
LD.B  r1, r2, 10        ; r1 = sign_extend(mem[r2 + 10])
```

---

### 14.2 LD.BU — Load Unsigned Byte

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x71` |
| **Format** | I |
| **Operands** | `rd, addr, imm15` |

**Semantics:** Loads an 8-bit byte from memory, zero-extends it to 64 bits, and stores the result in `rd`.

**Operation:** `rd ← zext(mem[addr + sext(imm15)]:8)`

**Example:**
```asm
; Load unsigned byte
LD.BU r1, r2, 0         ; r1 = zero_extend(mem[r2])
```

---

### 14.3 LD.H — Load Signed Halfword

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x72` |
| **Format** | I |
| **Operands** | `rd, addr, imm15` |

**Semantics:** Loads a 16-bit halfword from memory, sign-extends it to 64 bits, and stores the result in `rd`.

**Operation:** `rd ← sext(mem[addr + sext(imm15)]:16)`

**Example:**
```asm
; Load signed 16-bit value
LD.H  r1, r2, 4         ; r1 = sign_extend(mem[r2 + 4])
```

---

### 14.4 LD.HU — Load Unsigned Halfword

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x73` |
| **Format** | I |
| **Operands** | `rd, addr, imm15` |

**Semantics:** Loads a 16-bit halfword from memory, zero-extends it to 64 bits, and stores the result in `rd`.

**Operation:** `rd ← zext(mem[addr + sext(imm15)]:16)`

**Example:**
```asm
; Load unsigned 16-bit value
LD.HU r1, r2, 4         ; r1 = zero_extend(mem[r2 + 4])
```

---

### 14.5 LD.W — Load Signed Word

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x74` |
| **Format** | I |
| **Operands** | `rd, addr, imm15` |

**Semantics:** Loads a 32-bit word from memory, sign-extends it to 64 bits, and stores the result in `rd`.

**Operation:** `rd ← sext(mem[addr + sext(imm15)]:32)`

**Example:**
```asm
; Load signed 32-bit value
LD.W  r1, r2, 8         ; r1 = sign_extend(mem[r2 + 8])
```

---

### 14.6 LD.WU — Load Unsigned Word

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x75` |
| **Format** | I |
| **Operands** | `rd, addr, imm15` |

**Semantics:** Loads a 32-bit word from memory, zero-extends it to 64 bits, and stores the result in `rd`.

**Operation:** `rd ← zext(mem[addr + sext(imm15)]:32)`

**Example:**
```asm
; Load unsigned 32-bit value
LD.WU r1, r2, 8         ; r1 = zero_extend(mem[r2 + 8])
```

---

### 14.7 LD.D — Load Doubleword

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x76` |
| **Format** | I |
| **Operands** | `rd, addr, imm15` |

**Semantics:** Loads a 64-bit doubleword from memory and stores it in `rd`.

**Operation:** `rd ← mem[addr + sext(imm15)]:64`

**Example:**
```asm
; Load 64-bit value
LD.D  r1, r2, 16        ; r1 = mem[r2 + 16]
```

---

### 14.8 LD.X — Load 128-bit Pair

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x77` |
| **Format** | RI |
| **Operands** | `rd, rd2, addr, imm` |

**Semantics:** Loads a 128-bit value from memory as two consecutive 64-bit doublewords. The first doubleword is stored in `rd`, the second in `rd2`.

**Operation:** `rd ← mem[addr]:64; rd2 ← mem[addr + 8]:64`

**Note:** The loaded value must be aligned to 16 bytes for correct behavior.

**Example:**
```asm
; Load 128-bit value
MOVI  r3, r0, 0x100     ; r3 = base address
LD.X  r1, r2, r3, 0     ; r1 = mem[0x100], r2 = mem[0x108]
```

---

## 15. Store Instructions

### 15.1 ST.B — Store Byte

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x78` |
| **Format** | I |
| **Operands** | `rs, addr, imm15` |

**Semantics:** Stores the low 8 bits of `rs` to memory at the address computed as `addr + sext(imm15)`.

**Operation:** `mem[addr + sext(imm15)]:8 ← rs[7:0]`

**Example:**
```asm
; Store byte
MOVI  r2, r0, 0x100     ; r2 = base address
MOVI  r3, r0, 0xAB      ; r3 = value to store
ST.B  r3, r2, 0         ; mem[0x100] = 0xAB
```

---

### 15.2 ST.H — Store Halfword

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x79` |
| **Format** | I |
| **Operands** | `rs, addr, imm15` |

**Semantics:** Stores the low 16 bits of `rs` to memory at the address computed as `addr + sext(imm15)`.

**Operation:** `mem[addr + sext(imm15)]:16 ← rs[15:0]`

**Example:**
```asm
; Store 16-bit value
MOVI  r2, r0, 0x100     ; r2 = base address
MOVI  r3, r0, 0x1234    ; r3 = value to store
ST.H  r3, r2, 4         ; mem[0x104] = 0x1234
```

---

### 15.3 ST.W — Store Word

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x7A` |
| **Format** | I |
| **Operands** | `rs, addr, imm15` |

**Semantics:** Stores the low 32 bits of `rs` to memory at the address computed as `addr + sext(imm15)`.

**Operation:** `mem[addr + sext(imm15)]:32 ← rs[31:0]`

**Example:**
```asm
; Store 32-bit value
MOVI  r2, r0, 0x100     ; r2 = base address
MOVI  r3, r0, 0xDEADBEEF ; r3 = value to store
ST.W  r3, r2, 8         ; mem[0x108] = 0xDEADBEEF
```

---

### 15.4 ST.D — Store Doubleword

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x7B` |
| **Format** | I |
| **Operands** | `rs, addr, imm15` |

**Semantics:** Stores the full 64-bit value of `rs` to memory at the address computed as `addr + sext(imm15)`.

**Operation:** `mem[addr + sext(imm15)]:64 ← rs`

**Example:**
```asm
; Store 64-bit value
MOVI  r2, r0, 0x100     ; r2 = base address
MOVI  r3, r0, 0x123456789ABCDEF0 ; r3 = value to store
ST.D  r3, r2, 16        ; mem[0x110] = 0x123456789ABCDEF0
```

---

### 15.5 ST.X — Store 128-bit Pair

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x7C` |
| **Format** | RI |
| **Operands** | `rd2, rd, addr, imm` |

**Semantics:** Stores a 128-bit value to memory as two consecutive 64-bit doublewords. The value in `rd` is stored at the address, and the value in `rd2` is stored at address + 8.

**Operation:** `mem[addr]:64 ← rd; mem[addr + 8]:64 ← rd2`

**Note:** The address must be 16-byte aligned for correct behavior.

**Example:**
```asm
; Store 128-bit value
MOVI  r3, r0, 0x100     ; r3 = base address
MOVI  r1, r0, 0x11111111 ; r1 = low 64 bits
MOVI  r2, r0, 0x22222222 ; r2 = high 64 bits
ST.X  r2, r1, r3, 0     ; mem[0x100] = r1, mem[0x108] = r2
```

---

### 15.6 LDA — Load Address

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x7D` |
| **Format** | I |
| **Operands** | `rd, addr, imm15` |

**Semantics:** Computes the effective address by adding the sign-extended immediate to the base address and stores the result in `rd`. Unlike a load instruction, LDA does not access memory.

**Operation:** `rd ← addr + sext(imm15)`

**Usage:** Computing addresses for subsequent memory operations, pointer arithmetic.

**Example:**
```asm
; Compute address of struct field
MOVI  r2, r0, 0x1000    ; r2 = struct base address
LDA   r3, r2, 24        ; r3 = address of field at offset 24
LD.D  r1, r3, 0         ; Load the field value
```

---

## 16. Stack Operations

### 16.1 PUSH — Push Register to Stack

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x7E` |
| **Format** | R |
| **Operands** | `rs` |

**Semantics:** Decrements the stack pointer by 8 and stores the value of `rs` at the new stack top.

**Operation:** `sp ← sp - 8; mem[sp] ← rs`

**Example:**
```asm
; Push registers before function call
PUSH  r1                 ; Save return address
PUSH  r2                 ; Save callee-saved register
```

---

### 16.2 POP — Pop from Stack

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x7F` |
| **Format** | R |
| **Operands** | `rd` |

**Semantics:** Loads a 64-bit value from the top of the stack into `rd` and increments the stack pointer by 8.

**Operation:** `rd ← mem[sp]; sp ← sp + 8`

**Example:**
```asm
; Restore registers after function call
POP   r2                 ; Restore callee-saved register
POP   r1                 ; Restore return address
```

---

### 16.3 ENTER — Enter Stack Frame

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x80` |
| **Format** | I |
| **Operands** | `rd, imm15` |

**Semantics:** Creates a new stack frame by saving the current stack pointer and allocating space for local variables. The old stack pointer (frame pointer) is saved in `rd`.

**Operation:** `rd ← sp; sp ← sp - sext(imm15)`

**Usage:** Function prologue for establishing stack frame with local variable space.

**Example:**
```asm
; Function prologue - allocate 32 bytes for locals
ENTER r30, 32           ; r30 = old sp, sp -= 32
; Frame pointer now in r30, locals at r30-8 through r30-32
```

---

### 16.4 LEAVE — Leave Stack Frame

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x81` |
| **Format** | R |
| **Operands** | — |

**Semantics:** Restores the stack pointer from the frame pointer and restores the saved frame pointer from memory. Used to exit a function.

**Operation:** `sp ← r30; r30 ← mem[r30]`

**Prerequisite:** `r30` must contain the saved frame pointer.

**Example:**
```asm
; Function epilogue
LEAVE                   ; Restore sp and r30
RET                     ; Return to caller
```

---

### 16.5 ADJSP — Adjust Stack Pointer

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x82` |
| **Format** | I |
| **Operands** | `rs, imm15` |

**Semantics:** Adjusts the stack pointer by adding the sign-extended immediate value. Positive values deallocate space; negative values allocate space.

**Operation:** `sp ← sp + sext(imm15)`

**Usage:** Allocating/deallocating variable-size stack objects, stack alignment.

**Example:**
```asm
; Allocate space for 64-byte buffer on stack
ADJSP r0, -64           ; sp -= 64
; ... use buffer ...
ADJSP r0, 64            ; sp += 64 (deallocate)
```

---

### 16.6 FRAME — Get Frame Address

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x83` |
| **Format** | I |
| **Operands** | `rd, imm15` |

**Semantics:** Computes an address within the current stack frame by adding the immediate offset to the frame pointer and stores it in `rd`.

**Operation:** `rd ← r30 + sext(imm15)`

**Usage:** Accessing stack-allocated local variables.

**Example:**
```asm
; Get address of local variable at offset 16
FRAME r1, 16            ; r1 = address of local var
LD.D  r2, r1, 0         ; Load the local variable
```

---

## 17. String Operations

### 17.1 STRNEW — Create Empty String

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x84` |
| **Format** | I |
| **Operands** | `rd, imm15` |

**Semantics:** Creates a new empty string with the specified initial capacity and stores the string handle in `rd`.

**Operation:** `rd ← string_create(sext(imm15))`

**Example:**
```asm
; Create string with capacity 64
STRNEW r1, 64           ; r1 = new empty string
```

---

### 17.2 STRLEN — Get String Length

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x86` |
| **Format** | R |
| **Operands** | `rd, str` |

**Semantics:** Returns the length of the string in `rd`.

**Operation:** `rd ← string_length(str)`

**Example:**
```asm
; Get string length
STRLEN r2, r1           ; r2 = length of string in r1
```

---

### 17.3 STRGET — Get Character at Index

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x88` |
| **Format** | R |
| **Operands** | `rd, str, idx` |

**Semantics:** Returns the character at the specified index in `rd`. Index is 0-based.

**Operation:** `rd ← string_char_at(str, idx)`

**Example:**
```asm
; Get first character
STRGET r3, r1, r0        ; r3 = first character of r1
```

---

### 17.4 STRSET — Set Character at Index

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x89` |
| **Format** | R |
| **Operands** | `str, idx, char` |

**Semantics:** Sets the character at the specified index to the given value.

**Operation:** `string_set_char(str, idx, char)`

**Example:**
```asm
; Set character at index 0 to 'H'
MOVI  r2, r0, 'H'       ; r2 = 'H'
STRSET r1, r0, r2        ; str[0] = 'H'
```

---

### 17.5 STRCMP — Compare Strings

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x8C` |
| **Format** | R |
| **Operands** | `rd, str1, str2` |

**Semantics:** Compares two strings lexicographically. Returns 0 if equal, negative if str1 < str2, positive if str1 > str2.

**Operation:** `rd ← strcmp(str1, str2)`

**Example:**
```asm
; Compare two strings
STRCMP r3, r1, r2        ; r3 = comparison result
```

---

### 17.6 STRJOIN — Concatenate Strings

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x99` |
| **Format** | R |
| **Operands** | `rd, str1, str2` |

**Semantics:** Concatenates two strings and stores the result in `rd`.

**Operation:** `rd ← string_concat(str1, str2)`

**Example:**
```asm
; Concatenate two strings
STRJOIN r3, r1, r2       ; r3 = r1 + r2
```

---

### 17.7 STRREV — Reverse String

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x9B` |
| **Format** | R |
| **Operands** | `rd, str` |

**Semantics:** Returns a reversed copy of the string in `rd`.

**Operation:** `rd ← string_reverse(str)`

**Example:**
```asm
; Reverse string
STRREV r2, r1            ; r2 = reversed(r1)
```

---

### 17.8 STRTRIM — Trim Whitespace

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x9E` |
| **Format** | R |
| **Operands** | `rd, str` |

**Semantics:** Returns a copy of the string with leading and trailing whitespace removed.

**Operation:** `rd ← string_trim(str)`

**Example:**
```asm
; Trim whitespace
STRTRIM r2, r1           ; r2 = trimmed(r1)
```

---

### 17.9 STRTOI — Parse String to Integer

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xA2` |
| **Format** | R |
| **Operands** | `rd, str, base` |

**Semantics:** Parses the string as an integer in the specified base (2-36) and stores the result in `rd`.

**Operation:** `rd ← parse_int64(str, base)`

**Example:**
```asm
; Parse hex string
STRTOI r2, r1, 16        ; r2 = parse "1A" as hex = 26
```

---

### 17.10 ITOSTR — Convert Integer to String

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xA4` |
| **Format** | R |
| **Operands** | `rd, num, base` |

**Semantics:** Converts the integer to a string in the specified base and stores the result in `rd`.

**Operation:** `rd ← int64_to_string(num, base)`

**Example:**
```asm
; Convert 255 to hex string
ITOSTR r2, r1, 16        ; r2 = "FF"
```

---

## 18. Object and Array Operations

### 18.1 NEW — Allocate Object

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xA8` |
| **Format** | I |
| **Operands** | `rd, imm15` |

**Semantics:** Allocates a new object of the class identified by `imm15` and stores the object reference in `rd`.

**Operation:** `rd ← new_object(sext(imm15))`

**Example:**
```asm
; Create instance of class ID 5
NEW   r1, 5              ; r1 = new Object(5)
```

---

### 18.2 NEWA — Allocate Array

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xA9` |
| **Format** | RI |
| **Operands** | `rd, len, type` |

**Semantics:** Allocates a new array of the specified element type with `len` elements and stores the array reference in `rd`.

**Operation:** `rd ← new_array(len, type)`

**Example:**
```asm
; Allocate array of 10 int64 elements
MOVI  r2, r0, 10         ; r2 = length
MOVI  r3, r0, 1          ; r3 = int64 type ID
NEWA  r1, r2, r3         ; r1 = new int64[10]
```

---

### 18.3 LDF — Load Object Field

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xAA` |
| **Format** | RI |
| **Operands** | `rd, obj, idx` |

**Semantics:** Loads the value of field `idx` from the object and stores it in `rd`.

**Operation:** `rd ← obj.field[sext(idx)]`

**Example:**
```asm
; Load field 3 from object
LDF   r2, r1, 3           ; r2 = r1.field[3]
```

---

### 18.4 STF — Store Object Field

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xAB` |
| **Format** | RI |
| **Operands** | `val, obj, idx` |

**Semantics:** Stores `val` into field `idx` of the object.

**Operation:** `obj.field[sext(idx)] ← val`

**Example:**
```asm
; Store value in field 3
STF   r3, r1, 3           ; r1.field[3] = r3
```

---

### 18.5 LDELEM — Load Array Element

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xAC` |
| **Format** | R |
| **Operands** | `rd, arr, idx` |

**Semantics:** Loads the element at index `idx` from the array and stores it in `rd`.

**Operation:** `rd ← arr[idx]`

**Example:**
```asm
; Load element at index
LDELEM r3, r1, r2        ; r3 = r1[r2]
```

---

### 18.6 STELEM — Store Array Element

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xAD` |
| **Format** | R |
| **Operands** | `val, arr, idx` |

**Semantics:** Stores `val` into the element at index `idx` of the array.

**Operation:** `arr[idx] ← val`

**Example:**
```asm
; Store value at index
STELEM r3, r1, r2        ; r1[r2] = r3
```

---

### 18.7 ARRAYLEN — Get Array Length

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xAE` |
| **Format** | R |
| **Operands** | `rd, arr` |

**Semantics:** Returns the length of the array in `rd`.

**Operation:** `rd ← length(arr)`

**Example:**
```asm
; Get array length
ARRAYLEN r2, r1          ; r2 = length of r1
```

---

### 18.8 INSTANCEOF — Type Check

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xAF` |
| **Format** | RI |
| **Operands** | `rd, obj, type` |

**Semantics:** Returns 1 if `obj` is an instance of `type` or a subtype, 0 otherwise.

**Operation:** `rd ← instanceof(obj, sext(type)) ? 1 : 0`

**Example:**
```asm
; Check if object is instance of String
INSTANCEOF r2, r1, 3     ; r2 = 1 if r1 instanceof String
```

---

### 18.9 CHECKCAST — Type Cast

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xB0` |
| **Format** | RI |
| **Operands** | `rd, obj, type` |

**Semantics:** Casts `obj` to the specified type. Returns null if the cast is invalid (throws exception).

**Operation:** `rd ← cast(obj, sext(type))`

**Example:**
```asm
; Cast object to String
CHECKCAST r2, r1, 3      ; r2 = (String) r1
```

---

## 19. Function Call Instructions

### 19.1 CALL — Call Function at Address

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xB4` |
| **Format** | R |
| **Operands** | `addr` |

**Semantics:** Calls the function at the specified address. The return address is saved in `r1`.

**Operation:** `r1 ← pc + instruction_size; pc ← addr`

**Example:**
```asm
; Call function at address in r10
CALL  r10                ; Call function
; Return here, return value in r1
```

---

### 19.2 CALLI — Call by Symbol Index

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xB5` |
| **Format** | I |
| **Operands** | `rd, imm15` |

**Semantics:** Calls the function identified by the symbol table index `imm15`. The return address is saved in `rd`.

**Operation:** `rd ← pc + instruction_size; pc ← symbol_table[sext(imm15)]`

**Example:**
```asm
; Call function by symbol index
CALLI r1, 42             ; r1 = return addr, call symbol[42]
```

---

### 19.3 TAILCALL — Tail Call

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xB6` |
| **Format** | R |
| **Operands** | `addr` |

**Semantics:** Performs a tail call to the function at the specified address. Does not save a return address; reuses the current stack frame.

**Operation:** `pc ← addr`

**Example:**
```asm
; Tail call to another function
TAILCALL r10             ; Jump to function
```

---

### 19.4 CALLVIRT — Virtual Method Call

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xB7` |
| **Format** | RI |
| **Operands** | `rd, obj, method` |

**Semantics:** Calls a virtual method on an object. The method is looked up in the object's vtable.

**Operation:** `rd ← call(obj.vtable[sext(method)])`

**Example:**
```asm
; Call virtual method 2 on object
CALLVIRT r1, r2, 2      ; r1 = result of obj.method2()
```

---

## 20. Threading and Synchronization

### 20.1 THCREATE — Create Thread

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xC0` |
| **Format** | RI |
| **Operands** | `rd, entry` |

**Semantics:** Creates a new thread starting at the `entry` address and stores the thread ID in `rd`.

**Operation:** `rd ← thread_create(entry)`

**Example:**
```asm
; Create thread
MOVI  r2, r0, thread_func ; r2 = thread entry point
THCREATE r1, r2          ; r1 = thread ID
```

---

### 20.2 THJOIN — Join Thread

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xC1` |
| **Format** | R |
| **Operands** | `rd, tid` |

**Semantics:** Waits for the thread identified by `tid` to complete. Stores the exit value in `rd`.

**Operation:** `rd ← thread_join(tid)`

**Example:**
```asm
; Wait for thread
THJOIN r2, r1            ; r2 = thread exit value
```

---

### 20.3 THEXIT — Exit Thread

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xC2` |
| **Format** | R |
| **Operands** | `retval` |

**Semantics:** Exits the current thread with the specified return value.

**Operation:** `thread_exit(retval)`

**Example:**
```asm
; Exit thread with value 0
MOVI  r1, r0, 0          ; r1 = exit value
THEXIT r1                ; Terminate current thread
```

---

### 20.4 THID — Get Thread ID

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xC3` |
| **Format** | R |
| **Operands** | `rd` |

**Semantics:** Returns the unique identifier of the current thread in `rd`.

**Operation:** `rd ← thread_id()`

**Example:**
```asm
; Get current thread ID
THID   r1                ; r1 = current thread ID
```

---

### 20.5 MUTEXINI — Initialize Mutex

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xC6` |
| **Format** | R |
| **Operands** | `rd` |

**Semantics:** Creates and initializes a mutex. Returns the mutex handle in `rd`.

**Operation:** `rd ← mutex_init()`

**Example:**
```asm
; Create mutex
MUTEXINI r1               ; r1 = mutex handle
```

---

### 20.6 MUTEXLCK — Lock Mutex

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xC7` |
| **Format** | R |
| **Operands** | `rd, mutex` |

**Semantics:** Attempts to acquire the mutex lock. Returns 0 on success, non-zero if already locked.

**Operation:** `rd ← mutex_lock(mutex)`

**Example:**
```asm
; Acquire mutex
MUTEXLCK r2, r1           ; r2 = 0 on success
```

---

### 20.7 MUTEXULK — Unlock Mutex

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xC8` |
| **Format** | R |
| **Operands** | `mutex` |

**Semantics:** Releases the mutex lock.

**Operation:** `mutex_unlock(mutex)`

**Example:**
```asm
; Release mutex
MUTEXULK r1               ; Unlock mutex in r1
```

---

## 21. Atomic Operations

### 21.1 ATOMADD — Atomic Add

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xE0` |
| **Format** | R |
| **Operands** | `rd, addr, val` |

**Semantics:** Atomically adds `val` to the value at `addr`. Returns the original value at `addr` in `rd`.

**Operation:** `rd ← atomic_add(addr, val)`

**Example:**
```asm
; Atomically add 1 to counter
ATOMADD r2, r1, r3       ; r2 = old value, mem[r1] += r3
```

---

### 21.2 ATOMCAS — Atomic Compare and Swap

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xE2` |
| **Format** | R |
| **Operands** | `rd, addr, old, new` |

**Semantics:** Compares the value at `addr` with `old`. If equal, stores `new` at `addr`. Returns the original value at `addr` in `rd` (whether swapped or not).

**Operation:** `rd ← atomic_cas(addr, old, new)`

**Example:**
```asm
; Compare-and-swap
ATOMCAS r3, r1, r2, r4   ; if mem[r1] == r2: mem[r1] = r4; r3 = old value
```

---

### 21.3 ATOMLD — Atomic Load

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xE3` |
| **Format** | R |
| **Operands** | `rd, addr` |

**Semantics:** Atomically loads the value at `addr` into `rd`.

**Operation:** `rd ← atomic_load(addr)`

**Example:**
```asm
; Atomic load
ATOMLD r2, r1            ; r2 = atomic load from r1
```

---

### 21.4 ATOMST — Atomic Store

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xE4` |
| **Format** | R |
| **Operands** | `addr, val` |

**Semantics:** Atomically stores `val` at `addr`.

**Operation:** `atomic_store(addr, val)`

**Example:**
```asm
; Atomic store
MOVI  r2, r0, 42         ; r2 = value to store
ATOMST r1, r2            ; atomic store to address in r1
```

---

### 21.5 TLSALLOC — Allocate TLS Slot

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xE5` |
| **Format** | R |
| **Operands** | `rd, size` |

**Semantics:** Allocates a thread-local storage slot of the specified size. Returns the slot index in `rd`.

**Operation:** `rd ← tls_allocate(size)`

**Example:**
```asm
; Allocate TLS slot for 8 bytes
MOVI  r2, r0, 8          ; r2 = slot size
TLSALLOC r1, r2          ; r1 = slot index
```

---

### 21.6 TLSGET — Read from TLS

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xE6` |
| **Format** | R |
| **Operands** | `rd, slot` |

**Semantics:** Reads the value from the thread-local storage slot.

**Operation:** `rd ← tls_read(slot)`

**Example:**
```asm
; Read TLS slot
TLSGET r2, r1            ; r2 = TLS value at slot r1
```

---

### 21.7 TLSSET — Write to TLS

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xE7` |
| **Format** | R |
| **Operands** | `slot, val` |

**Semantics:** Writes `val` to the thread-local storage slot.

**Operation:** `tls_write(slot, val)`

**Example:**
```asm
; Write to TLS slot
MOVI  r2, r0, 100        ; r2 = value
TLSSET r1, r2            ; TLS[r1] = r2
```

---

## 22. Type Conversion

### 22.1 SEXT.B — Sign Extend Byte

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xF0` |
| **Format** | R |
| **Operands** | `rd, rs` |

**Semantics:** Sign-extends the low 8 bits of `rs` to 64 bits and stores the result in `rd`.

**Operation:** `rd ← sext(rs[7:0])`

**Example:**
```asm
; Sign extend byte
SEXT.B r2, r1            ; r2 = sign_extend(r1[7:0])
```

---

### 22.2 SEXT.H — Sign Extend Halfword

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xF1` |
| **Format** | R |
| **Operands** | `rd, rs` |

**Semantics:** Sign-extends the low 16 bits of `rs` to 64 bits and stores the result in `rd`.

**Operation:** `rd ← sext(rs[15:0])`

**Example:**
```asm
; Sign extend halfword
SEXT.H r2, r1            ; r2 = sign_extend(r1[15:0])
```

---

### 22.3 SEXT.W — Sign Extend Word

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xF2` |
| **Format** | R |
| **Operands** | `rd, rs` |

**Semantics:** Sign-extends the low 32 bits of `rs` to 64 bits and stores the result in `rd`.

**Operation:** `rd ← sext(rs[31:0])`

**Example:**
```asm
; Sign extend word
SEXT.W r2, r1            ; r2 = sign_extend(r1[31:0])
```

---

### 22.4 ZEXT.B — Zero Extend Byte

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xF3` |
| **Format** | R |
| **Operands** | `rd, rs` |

**Semantics:** Zero-extends the low 8 bits of `rs` to 64 bits and stores the result in `rd`.

**Operation:** `rd ← zext(rs[7:0])`

**Example:**
```asm
; Zero extend byte
ZEXT.B r2, r1             ; r2 = zero_extend(r1[7:0])
```

---

### 22.5 ZEXT.H — Zero Extend Halfword

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xF4` |
| **Format** | R |
| **Operands** | `rd, rs` |

**Semantics:** Zero-extends the low 16 bits of `rs` to 64 bits and stores the result in `rd`.

**Operation:** `rd ← zext(rs[15:0])`

**Example:**
```asm
; Zero extend halfword
ZEXT.H r2, r1             ; r2 = zero_extend(r1[15:0])
```

---

### 22.6 ZEXT.W — Zero Extend Word

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xF5` |
| **Format** | R |
| **Operands** | `rd, rs` |

**Semantics:** Zero-extends the low 32 bits of `rs` to 64 bits and stores the result in `rd`.

**Operation:** `rd ← zext(rs[31:0])`

**Example:**
```asm
; Zero extend word
ZEXT.W r2, r1             ; r2 = zero_extend(r1[31:0])
```

---

### 22.7 TRUNC — Truncate Value

| Attribute | Value |
|----------|-------|
| **Opcode** | `0xF6` |
| **Format** | R |
| **Operands** | `rd, rs, bits` |

**Semantics:** Truncates the value in `rs` to the specified number of bits.

**Operation:** `rd ← truncate(rs, bits)`

**Example:**
```asm
; Truncate to 16 bits
TRUNC  r2, r1, r0        ; r2 = r1 truncated to r0 bits
```

---

## 23. Vector Instructions

### Vector Lane Configuration

HVM vector registers (v0-v15) are 128 bits wide and support multiple lane configurations:

| Lane Size | Lanes per Register | Use Case |
|-----------|-------------------|----------|
| 64-bit (i64/f64) | 2 lanes | Double-precision FP, 64-bit integers |
| 32-bit (i32/f32) | 4 lanes | Single-precision FP, 32-bit integers |
| 16-bit (i16) | 8 lanes | Half-precision FP, 16-bit integers |
| 8-bit (i8) | 16 lanes | Bytes, characters |

**Lane Indexing:**
- Lane 0: bits [0-63] or [0-31], [0-15], [0-7] depending on size
- Lane 1: bits [64-127] or [32-63], [16-31], [8-15]
- etc.

**Default Lane Size:** Operations default to 64-bit lanes unless specified.

### 23.1 VADD — Vector Add

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x100` |
| **Format** | R |
| **Operands** | `vd, vs1, vs2` |
| **Func** | `0` |

**Semantics:** Performs element-wise addition of two 128-bit vector registers. Each 64-bit lane is added independently.

**Operation:** `vd ← vs1 + vs2` (element-wise)

**Example:**
```asm
; Add two vectors
VADD  v0, v1, v2         ; v0 = v1 + v2 (element-wise)
```

---

### 23.2 VSUB — Vector Subtract

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x100` |
| **Format** | R |
| **Operands** | `vd, vs1, vs2` |
| **Func** | `1` |

**Semantics:** Performs element-wise subtraction of two 128-bit vector registers.

**Operation:** `vd ← vs1 - vs2` (element-wise)

**Example:**
```asm
; Subtract vectors
VSUB  v0, v1, v2         ; v0 = v1 - v2 (element-wise)
```

---

### 23.3 VMUL — Vector Multiply

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x100` |
| **Format** | R |
| **Operands** | `vd, vs1, vs2` |
| **Func** | `2` |

**Semantics:** Performs element-wise multiplication of two 128-bit vector registers.

**Operation:** `vd ← vs1 * vs2` (element-wise)

**Example:**
```asm
; Multiply vectors
VMUL  v0, v1, v2         ; v0 = v1 * v2 (element-wise)
```

---

### 23.4 VDOT — Vector Dot Product

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x101` |
| **Format** | R |
| **Operands** | `rd, vs1, vs2` |

**Semantics:** Computes the dot product of two 128-bit vector registers, storing the scalar result in `rd`.

**Operation:** `rd ← vs1[0]*vs2[0] + vs1[1]*vs2[1] + ...`

**Example:**
```asm
; Dot product
VDOT  r1, v0, v1         ; r1 = dot(v0, v1)
```

---

### 23.5 VLOAD — Load Vector

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x102` |
| **Format** | I |
| **Operands** | `vd, addr, imm15` |

**Semantics:** Loads a 128-bit vector from memory at the address with stride specified by the immediate.

**Operation:** `vd ← load_vector(addr, sext(imm15))`

**Example:**
```asm
; Load vector with stride 16
VLOAD v0, r1, 16         ; v0 = load vector from r1 with stride 16
```

---

### 23.6 VSTORE — Store Vector

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x103` |
| **Format** | I |
| **Operands** | `vd, addr, imm15` |

**Semantics:** Stores a 128-bit vector to memory at the address with stride specified by the immediate.

**Operation:** `store_vector(vd, addr, sext(imm15))`

**Example:**
```asm
; Store vector with stride 16
VSTORE v0, r1, 16        ; store v0 to r1 with stride 16
```

---

### 23.7 VSPLAT — Broadcast Scalar to Vector

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x105` |
| **Format** | R |
| **Operands** | `vd, rs` |

**Semantics:** Broadcasts the scalar value in `rs` to all lanes of the vector register `vd`.

**Operation:** `vd ← splat(rs)`

**Example:**
```asm
; Broadcast to vector
VSPLAT v0, r1            ; v0 = [r1, r1, r1, r1]
```

---

### 23.8 VFMA — Fused Multiply-Add

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x10A` |
| **Format** | R |
| **Operands** | `vd, vs1, vs2` |
| **Func** | `0` |

**Semantics:** Performs fused multiply-add: `vd = vd + vs1 * vs2`

**Operation:** `vd ← vd + (vs1 * vs2)`

**Example:**
```asm
; Fused multiply-add
VFMA  v0, v1, v2          ; v0 = v0 + v1 * v2
```

---

## 24. Exception Handling

### 24.1 TRY — Begin Try Block

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x110` |
| **Format** | I |
| **Operands** | `rd, imm15` |

**Semantics:** Begins a try block and pushes an exception handler to the stack. The handler index is stored in `rd`.

**Operation:** `rd ← handler_table.push(sext(imm15)); sp ← sp - 16`

**Example:**
```asm
; Begin try block
TRY   r1, 0               ; r1 = handler ID
```

---

### 24.2 THROW — Throw Exception

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x111` |
| **Format** | I |
| **Operands** | `type, imm15` |

**Semantics:** Throws an exception with the specified type and message index.

**Operation:** `throw_exception(type, sext(imm15))`

**Example:**
```asm
; Throw exception
MOVI  r1, r0, 5           ; r1 = exception type
THROW r1, 42              ; throw type 5 with message 42
```

---

### 24.3 CATCH — Catch Exception

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x113` |
| **Format** | I |
| **Operands** | `rd, imm15` |

**Semantics:** Pops an exception handler from the stack for the specified type. Returns the exception object in `rd` if caught.

**Operation:** `rd ← pop_handler(sext(imm15))`

**Example:**
```asm
; Catch exception
CATCH  r1, 5              ; r1 = exception if caught
```

---

## 25. Interrupt Instructions

### 25.1 DI — Disable Interrupts

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x118` |
| **Format** | R |
| **Operands** | — |

**Semantics:** Disables all maskable interrupts.

**Operation:** `disable_interrupts()`

**Example:**
```asm
; Disable interrupts
DI                        ; interrupts disabled
```

---

### 25.2 EI — Enable Interrupts

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x119` |
| **Format** | R |
| **Operands** | — |

**Semantics:** Enables all maskable interrupts.

**Operation:** `enable_interrupts()`

**Example:**
```asm
; Enable interrupts
EI                        ; interrupts enabled
```

---

### 25.3 INT — Trigger Software Interrupt

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x11A` |
| **Format** | I |
| **Operands** | `intid` |

**Semantics:** Triggers a software interrupt with the specified ID.

**Operation:** `software_interrupt(sext(intid))`

**Example:**
```asm
; Trigger interrupt 5
INT   r0, 5               ; software interrupt 5
```

---

### 25.4 IRET — Return from Interrupt

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x11B` |
| **Format** | R |
| **Operands** | — |

**Semantics:** Returns from an interrupt handler, restoring the previous interrupt state.

**Operation:** `return_from_interrupt()`

**Example:**
```asm
; Return from interrupt
IRET                      ; return from ISR
```

---

## 26. FFI and Native Calls

### 26.1 CALLHOST — Call Static Runtime Function

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x120` |
| **Format** | I |
| **Operands** | `rd, imm15` |

**Semantics:** Calls a static runtime function identified by the specified ID.

**Operation:** `rd ← call_static_runtime(sext(imm15))`

**Example:**
```asm
; Call runtime function 5
CALLHOST r1, 5           ; r1 = runtime function result
```

---

### 26.2 CALLNATIVE — Call Native Function

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x122` |
| **Format** | RI |
| **Operands** | `rd, addr` |

**Semantics:** Calls a native function at the specified address with C ABI.

**Operation:** `rd ← call_c_abi(addr)`

**Example:**
```asm
; Call native function
CALLNATIVE r1, r2        ; r1 = native_function(r2)
```

---

### 26.3 LOADLIB — Load Dynamic Library

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x125` |
| **Format** | I |
| **Operands** | `rd, imm15` |

**Semantics:** Loads a dynamic library by name/index. Returns the library handle.

**Operation:** `rd ← dlopen(sext(imm15))`

**Example:**
```asm
; Load library
LOADLIB r1, 0             ; r1 = library handle
```

---

### 26.4 I2PTR — Integer to Pointer

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x129` |
| **Format** | R |
| **Operands** | `rd, rs` |

**Semantics:** Converts an integer to a pointer type.

**Operation:** `rd ← int_to_pointer(rs)`

**Example:**
```asm
; Convert to pointer
I2PTR  r2, r1             ; r2 = (void*) r1
```

---

## 27. System and Debug Instructions

### 27.1 SYSCALL — System Call

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x130` |
| **Format** | I |
| **Operands** | `rd, imm15` |

**Semantics:** Invokes a system call with the specified number. The system call number and arguments are passed in registers following OS conventions.

**Operation:** `rd ← syscall(sext(imm15))`

**Example:**
```asm
; Make system call
SYSCALL r1, 0             ; syscall 0
```

---

### 27.2 TRAP — Software Breakpoint

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x131` |
| **Format** | I |
| **Operands** | `imm15` |

**Semantics:** Triggers a software breakpoint for debugging.

**Operation:** `software_breakpoint(sext(imm15))`

**Example:**
```asm
; Breakpoint
TRAP   r0, 0              ; trigger breakpoint 0
```

---

### 27.3 DEBUG — Print Register (Debug)

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x132` |
| **Format** | R |
| **Operands** | `rs` |

**Semantics:** Prints the value of the specified register. This is a debugging aid.

**Operation:** `print_register(rs)`

**Example:**
```asm
; Debug print
DEBUG  r1                 ; print r1
```

---

### 27.4 RDCOUNT — Read Performance Counter

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x133` |
| **Format** | I |
| **Operands** | `rd, imm15` |

**Semantics:** Reads a performance counter identified by the immediate value.

**Operation:** `rd ← read_counter(sext(imm15))`

**Performance Counter IDs (imm15 values):**

| Counter ID | Name | Description |
|------------|------|-------------|
| `0` | `CYCLES` | Total CPU cycles elapsed |
| `1` | `INSTRUCTIONS` | Total instructions executed |
| `2` | `BRANCHES` | Total branch instructions |
| `3` | `BRANCH_MISPREDICT` | Mispredicted branches |
| `4` | `CACHE_LOADS` | L1 cache loads |
| `5` | `CACHE_MISSES` | L1 cache misses |
| `6` | `MEMORY_READS` | Memory read operations |
| `7` | `MEMORY_WRITES` | Memory write operations |
| `8` | `DIVIDES` | Division operations |
| `9` | `FLOATS` | Floating-point operations |
| `10` | `VECTOR_OPS` | SIMD/vector operations |
| `11` | `STACK_POINTER` | Current stack pointer value |
| `12` | `THREAD_COUNT` | Current thread count |
| `13` | `HEAP_USED` | Heap memory in use (bytes) |
| `14` | `GC_PAUSE` | Total GC pause time (cycles) |
| `15` | `CUSTOM_BASE` | First custom counter (offset for implementation-specific) |

**Example:**
```asm
; Read cycle counter
RDCOUNT r1, 0             ; r1 = cycle count

; Read instruction count
RDCOUNT r2, 1             ; r2 = instruction count

; Read L1 cache miss count
RDCOUNT r3, 5             ; r3 = cache misses
```

---

### 27.5 BARRIER — Memory Barrier (Full Fence)

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x134` |
| **Format** | R |
| **Operands** | — |

**Semantics:** Creates a full memory barrier. All load and store operations before the barrier must complete before any load or store operations after the barrier begin. This ensures all memory operations are globally visible.

**Operation:** `memory_barrier()`

**Memory Ordering Guarantees:**
- All preceding loads appear before all subsequent loads
- All preceding stores appear before all subsequent stores
- All preceding loads appear before all subsequent stores
- All preceding stores are globally visible before any subsequent loads

**Example:**
```asm
; Full memory barrier
BARRIER                   ; full memory fence
```

---

### 27.5.1 FENCE — Parameterized Memory Fence

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x134` |
| **Format** | I |
| **Operands** | `imm15` |

**Semantics:** Creates a memory fence for specific memory operation types. The immediate value specifies which operations to synchronize.

**Fence Type Flags (imm15):**

| Bit | Flag | Meaning |
|-----|------|---------|
| 0 | `LOAD` | Order load operations |
| 1 | `STORE` | Order store operations |
| 2 | `IO` | I/O operations (implies full barrier) |

**Fence Combinations:**
| imm15 | Name | Description |
|-------|------|-------------|
| `0x01` | `LOAD` | Loads ordered with subsequent loads |
| `0x02` | `STORE` | Stores ordered with subsequent stores |
| `0x03` | `LDST` | Both loads and stores ordered |
| `0x04` | `IO` | Full barrier including I/O |
| `0x07` | `FULL` | Full fence (equivalent to BARRIER) |

**Example:**
```asm
; Order stores only
FENCE  r0, 0x02            ; store barrier only

; Order loads only
FENCE  r0, 0x01            ; load barrier only

; Full fence
FENCE  r0, 0x07            ; full memory fence
```

---

### 27.5.2 FENCE.TSO — Total Store Order Fence

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x134` (func=1) |
| **Format** | R |
| **Operands** | — |

**Semantics:** Creates a TSO (Total Store Order) fence. All stores before the fence are ordered before all stores after the fence. Loads may be reordered with respect to earlier stores (unlike full fence).

**Operation:** `fence_tso()`

**Use Case:** x86/x64 processor model where stores are naturally ordered but loads can be reordered.

**Example:**
```asm
; TSO fence (store ordering only)
FENCE.TSO                 ; stores ordered, loads may reorder
```

---

### 27.5.3 FENCE.I — Instruction Fence

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x134` (func=2) |
| **Format** | R |
| **Operands** | — |

**Semantics:** Ensures that all instruction fetches observe all previous memory stores. Used for self-modifying code where code is written to memory and then executed.

**Operation:** `fence_instruction()`

**Use Cases:**
- Dynamic code generation
- JIT compilation
- Self-modifying executables
- Memory-mapped executable code

**Example:**
```asm
; Instruction fence for self-modifying code
; (write new code to memory)
FENCE.I                   ; synchronize instruction fetches
```

---

### 27.6 GETREGS — Read All Registers

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x137` |
| **Format** | R |
| **Operands** | `addr` |

**Semantics:** Reads all register values into memory at the specified address.

**Operation:** `read_registers(addr)`

**Example:**
```asm
; Read registers to memory
MOVI  r1, r0, buffer      ; r1 = buffer address
GETREGS r1                ; dump registers to buffer
```

---

### 27.7 SETREGS — Write All Registers

| Attribute | Value |
|----------|-------|
| **Opcode** | `0x138` |
| **Format** | R |
| **Operands** | `addr` |

**Semantics:** Writes all register values from memory at the specified address.

**Operation:** `write_registers(addr)`

**Example:**
```asm
; Write registers from memory
MOVI  r1, r0, buffer      ; r1 = buffer address
SETREGS r1                ; restore registers from buffer
```

---

## A. Instruction Encoding Formats

### A.1 R-Type (Register)

```
31     25 24    20 19    15 14    10 9      0
┌───────┬────────┬────────┬────────┬──────────────────┐
│ opcode │   rd   │  rs1   │  rs2   │      func        │
│  7bit  │  5bit  │  5bit  │  5bit  │     10bit        │
└───────┴────────┴────────┴────────┴──────────────────┘
```

### A.2 I-Type (Immediate)

```
31     25 24    20 19    15 14                  0
┌───────┬────────┬────────┬──────────────────────────────┐
│ opcode │   rd   │  rs1   │          imm15              │
│  7bit  │  5bit  │  5bit  │          15bit             │
└───────┴────────┴────────┴──────────────────────────────┘
```

### A.3 B-Type (Branch)

```
31     25 24    20 19    15 14                  0
┌───────┬────────┬────────┬──────────────────────────────┐
│ opcode │  rs1   │  rs2   │          imm15              │
│  7bit  │  5bit  │  5bit  │          15bit             │
└───────┴────────┴────────┴──────────────────────────────┘
```

### A.4 J-Type (Jump)

```
31     25 24    20 19                                  0
┌───────┬────────┬────────────────────────────────────────┐
│ opcode │   rd   │              offset                    │
│  7bit  │  5bit  │              20bit                   │
└───────┴────────┴────────────────────────────────────────┘
```

---

## B. Opcode Map Summary

| Range | Category | Notes |
|-------|----------|-------|
| `0x00` | Control | NOP |
| `0x01-0x08` | Data Movement | MOV, MOVI, MOVZ, LUI, ADDI, SUBI, NEG, XCHG |
| `0x10` | Integer ALU | ADD, SUB, MUL, MULH, MULHS, DIV, DIVU, REM |
| `0x11-0x12` | Immediate Arithmetic | MULI, DIVI |
| `0x13-0x14` | Shift | SHL, SHR, SAR, SHLI |
| `0x20-0x24` | Bitwise | AND, OR, XOR, NOT, ANDI, ORI, XORI |
| `0x25-0x27` | Bit Manipulation | CLZ, CTZ, POPCNT |
| `0x30-0x33` | Floating-Point (f64) | FADD, FSUB, FMUL, FDIV, FSQRT, FABS, FNEG |
| `0x34-0x35` | FP Convert/32-bit | FADD32, FSUB32, FMUL32, FDIV32, FCVT |
| `0x40` | Integer Comparison | CMPEQ, CMPNE, CMPLT, CMPLE, CMPGT, CMPGE, CMPLTU, CMPLEU |
| `0x41` | FP Comparison | FCMPEQ, FCMPLT, FCMPLE, FCMPGT, FCMPGE |
| `0x42` | Set | SET |
| `0x50-0x57` | Branch | BEQ, BNE, BLT, BLE, BGT, BGE, BLTU, BGEU |
| `0x60-0x63` | Jump | JMP, JAL, JALR, RET |
| `0x70-0x77` | Load | LD.B, LD.BU, LD.H, LD.HU, LD.W, LD.WU, LD.D, LD.X |
| `0x78-0x7D` | Store | ST.B, ST.H, ST.W, ST.D, ST.X, LDA |
| `0x7E-0x83` | Stack Operations | PUSH, POP, ENTER, LEAVE, ADJSP, FRAME |
| `0x84-0xA7` | String Operations | STRNEW through STRDECODE |
| `0xA8-0xB3` | Object/Array | NEW, NEWA, LDF, STF, LDELEM, STELEM, ARRAYLEN, INSTANCEOF, CHECKCAST, MONITORENTER, MONITOREXIT, GC |
| `0xB4-0xBB` | Function Call | CALL, CALLI, TAILCALL, CALLVIRT, IMPORT, LOADMOD, RESOLVE |
| `0xC0-0xD3` | Threading/Sync | THCREATE, THJOIN, THEXIT, THID, THYIELD, THWAIT, MUTEX*, COND*, SPIN*, BARRIER* |
| `0xE0-0xE8` | Atomics/TLS | ATOMADD, ATOMSUB, ATOMCAS, ATOMLD, ATOMST, TLSALLOC, TLSGET, TLSSET, TLSFREE |
| `0xF0-0xF7` | Type Conversion | SEXT.B/H/W, ZEXT.B/H/W, TRUNC, REINTERPRET |
| `0x100-0x10A` | Vector Operations | VADD, VSUB, VMUL, VDOT, VLOAD, VSTORE, VSHUF, VSPLAT, VEXTRACT, VINSERT, VCMPEQ, VREDUCE, VFMA |
| `0x110-0x117` | Exception Handling | TRY, THROW, THROWV, CATCH, FINALLY, RETHROW, EXCINFO, ENDFIN |
| `0x118-0x11F` | Interrupts | DI, EI, INT, IRET, SETINT, GETINT, MASKINT, UNMASKINT |
| `0x120-0x12D` | FFI/Native Calls | CALLHOST, CALLHOSTV, CALLNATIVE, PREPCALL, FINISHCA, LOADLIB, FREELIB, GETSYM, GETFUNC, I2PTR, PTR2I, REINTERP, ADDR2FUNC, FUNC2ADDR |
| `0x130-0x139` | System/Debug | SYSCALL, TRAP, DEBUG, RDCOUNT, BARRIER, BREAKPOINT, SINGLESTEP, GETREGS, SETREGS, GETFPOFF |

---

*Document Version: 1.0*
*Last Updated: April 2026*
*Document Owner: HVM Specification Team*
