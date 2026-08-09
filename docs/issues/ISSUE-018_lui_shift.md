# ISSUE-018: LUI Shift Value Mismatch with Encoding

## 1. Overview
The `LUI` (Load Upper Immediate) instruction uses a shift of 49 bits in the hardware encoding, but the ISA specification and codegen may disagree on this value. This was fixed in commit `a6a748c` but the mismatch risk remains for anyone extending the ISA without cross-referencing the encoding.

## 2. Technical Analysis
- **Location**: `src/codegen/HVMCodeGenerator.cpp` lines 1725-1726
- **Location**: `docs/hvm/hvm-spec.md`
- **Issue**: The `LUI` shift must match the bit-field encoding in `HVMInstruction.h`. In the HVM instruction format, the 15-bit immediate field starts at bit 49 (in the 64-bit instruction word). The codegen uses the same shift, but the value is documented in multiple places and must stay in sync.

```cpp
// Codegen LUI emission:
writeReg(o.rd, static_cast<uint64_t>(o.imm15) << 49U);
```

## 3. Impact
- If the shift value drifts between the codegen, the JIT translator, and the spec, `LUI` instructions produce wrong upper-immediate values.
- 49 specifically is unusual (typical RISC architectures use 32 or 48), making it easy to mistype.

## 4. Suggested Fix
Define a named constant for the LUI shift in a single location:

```cpp
// In a shared header (e.g., HVMInstruction.h or HVMCodeGeneratorTypes.h):
constexpr uint64_t kLuiShift = 49U;
```

Use this constant in both the codegen emitters and the JIT interpreter. Document it in `hvm-spec.md` with a cross-reference to the constant.

## 5. Status
- **Date**: 2026-08-09
- **Status**: **FIXED**
- **Priority**: **MEDIUM**
- **Audit 2026-06-21**: Current code and spec use the corrected 49-bit shift semantics for `LUI`, but the shared named constant/documentation synchronization work remains incomplete.
- **Resolution**: `hvm::kLuiImmediateShift` is the shared source of truth used by the interpreter and LLVM JIT. The HVM specification and regression tests document and verify the same 49-bit encoding.
- **Note**: The codegen path (`emitConstant` in `HVMCodeGenerator.cpp`) does not emit `LUI` instructions (it uses `MOVZ`/`ADDI`/`.rodata`+`LD.D` instead), so there is no codegen-side consumer to synchronize. The shared constant is the interface between the interpreter and JIT IR, and `LuiRespectsSharedShiftConstant` (`tests/hvm/HVMJITInstructionSemanticsTest.cpp`) pins the result against it.
