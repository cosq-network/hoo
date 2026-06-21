# ISSUE-024: Character Literals Passed as Raw Char Parameters Create Object Pointers

## 1. Overview
When a `CharacterLiteral` (`'A'`) is used as an argument to a function that expects a raw `char` parameter (e.g., `map.setCharInt64('A', 65)`), the codegen emits a `Character.fromCodepoint()` call to construct a heap-allocated Character object and passes the object pointer. The JIT wrapper then casts this pointer to `char`, getting the low byte of the pointer address instead of the character's codepoint.

## 2. Technical Analysis

### 2.1 CharacterLiteral codegen
- **Location**: `src/codegen/HVMCodeGenerator.cpp` lines 744-751
- **Issue**: When visiting a `CharacterLiteral` node, the codegen emits:
  ```cpp
  uint8_t cpReg = emitConstant(static_cast<int64_t>(charLit->getValue())); // loads 65
  emit(Opcode::MOV, OperandsR{1, cpReg, 0, 0});                            // r1 = 65
  emitCall(Opcode::CALL, "_F_hoo_Character_from_codepoint_p_i8");           // creates Character object
  uint8_t dest = allocateRegister();
  emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});                              // save object pointer
  ```
  This always creates a heap-allocated Character object, regardless of how the value will be used.

### 2.2 JIT wrapper expectation
- **Location**: `src/hvm/HVMJIT.cpp` lines 1018-1027
- **Issue**: JIT wrappers for char-keyed operations cast the register directly to `char`:
  ```cpp
  hoo_map_set_char_int64(..., static_cast<char>(state->regs[2]), ...);
  ```
  This treats the register value as a raw integer codepoint, not a Character object pointer.

### 2.3 Fix applied
- **Location**: `src/hvm/HVMJIT.cpp` lines 1018-1040
- **Fix**: A `jit_char_from_reg()` helper was added that checks `hoo_get_type_id(ptr) == HOO_TYPE_CHARACTER` and extracts the codepoint via `hoo_character_codepoint()`:
  ```cpp
  static char jit_char_from_reg(uint64_t reg) {
      void* ptr = reinterpret_cast<void*>(reg);
      if (ptr && hoo_get_type_id(ptr) == HOO_TYPE_CHARACTER) {
          return static_cast<char>(hoo_character_codepoint(ptr));
      }
      return static_cast<char>(reg);
  }
  ```
  All char-keyed Map JIT wrappers now use this helper.

## 3. Impact
- **Before fix**: `m.setCharInt64('A', 65)` stores value under a garbage key (low byte of Character object pointer). `m.getCharInt64('A')` looks up with a different garbage key and returns 0.
- **After fix**: Character objects are transparently unwrapped to their codepoint value, and raw integer values also work unchanged.
- **Code budget**: Each char-keyed JIT wrapper pays the cost of a type-ID check and potential function call.

## 4. Resolution
**Decision**: Remove Hoo-level support for character-keyed Map operations entirely. The C-level char key type remains available for C API callers, but the Hoo language does not expose `setCharInt64`, `getCharInt64`, `setCharDouble`, `getCharDouble` or any char-keyed Map methods.

### Changes made:
1. **HVMJIT.cpp**: Removed `jit_char_from_reg()` helper and all 4 char-keyed JIT wrappers (`jit_map_set_char_int64`, `jit_map_get_char_int64`, `jit_map_set_char_double`, `jit_map_get_char_double`) and their symbol table entries.
2. **HVMCodeGenerator.cpp**: Removed `containsChar`, `getCharInt64`, `getCharBool`, `getCharDouble`, `getCharString` from Map type inference.
3. **HooMapJitTest.cpp**: Removed `SetGetCharInt64` and `SetGetCharDouble` tests.

## 5. Status
- **Date**: 2026-06-11
- **Status**: **FIXED** (char key support removed from Hoo language layer)
- **Priority**: **LOW**
- **Audit 2026-06-21**: Verified char-keyed map/JIT wrapper paths remain removed from the active implementation; no raw-char argument mismatch path was found.
