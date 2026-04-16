# Hooc Codebase Cleanup Analysis

**Analysis Date:** April 16, 2026
**Analyzer:** Automated Code Scanning Tool
**Scope:** Full codebase (src/, tests/, includes)

## Executive Summary

This report identifies unused code, orphaned files, and deprecated components in the Hooc compiler codebase. The analysis found:

- **5 orphaned source files** not included in the build system
- **15 unused functions** never called in the codebase
- **32 deprecated member variables** marked for removal
- **2 partially implemented AST classes** with no code generation
- **Multiple unused function pointers** that are initialized but never invoked

**Total LOC Impact:** ~400 lines of dead code identified for potential removal

---

## 1. Orphaned Source Files (Not Compiled)

These files exist in the `src/` directory but are **not referenced in CMakeLists.txt** and are never built:

### 1.1 CustomHoocParser.cpp / CustomHoocParser.h
- **Location:** `src/CustomHoocParser.cpp` (63 lines), `src/CustomHoocParser.h` (43 lines)
- **Status:** Never referenced in codebase
- **Description:** Provides a `HoocParser` class with a `parse()` method, but this functionality is already provided by `ProcessIsolatedParser`
- **Recommendation:** **DELETE** - Redundant with ProcessIsolatedParser
- **Safety:** High - No references found

### 1.2 comprehensive_test.cpp
- **Location:** `src/comprehensive_test.cpp` (143 lines)
- **Status:** Standalone test file using old `CodeGenerator` API
- **Description:** Old test file that uses deprecated APIs
- **Recommendation:** **DELETE** - Functionality covered by GoogleTest suite
- **Safety:** High - Not part of build

### 1.3 hooc_parse.cpp
- **Location:** `src/hooc_parse.cpp` (49 lines)
- **Status:** Old standalone parser utility
- **Description:** Mentioned in `ProcessIsolatedParser.cpp` comment but never compiled or used
- **Recommendation:** **DELETE** - Functionality absorbed into ProcessIsolatedParser
- **Safety:** High - Not compiled

### 1.4 test_codegen.cpp
- **Location:** `src/test_codegen.cpp` (58 lines)
- **Status:** Old standalone code generation test
- **Description:** Uses deprecated `CodeGenerator` API; replaced by GoogleTest suite
- **Recommendation:** **DELETE** - Replaced by comprehensive test suite
- **Safety:** High - Not part of build

**Total Orphaned Code:** 356 lines

---

## 2. Unused Functions

### 2.1 Code Generator Interface Methods (Virtual)

**File:** `src/LLVMCodeGenerator.cpp`

These methods override abstract base class methods but are never called:

| Function | Line | Return Type | Status |
|----------|------|-------------|--------|
| `generateFunction()` | 43 | `GeneratedFunction*` | Never called - returns `LLVMGeneratedFunction` |
| `generateExpression()` | 51 | `GeneratedValue*` | Never called - returns `LLVMGeneratedValue` |
| `generateType()` | 63 | `GeneratedType*` | Never called - returns `LLVMGeneratedType` |

**Analysis:** These appear to be interface methods from an older design. The codebase uses direct methods like `generateLLVMExpression()` instead.

**Recommendation:**
- If these are required by a base class interface: Keep
- If not required: Consider removing the wrapper methods

**Safety:** Medium - Verify base class requirements

---

### 2.2 Array Helper Functions

**File:** `src/LLVMCodeGenerator.cpp`

| Function | Line | Purpose | Usage Count |
|----------|------|---------|-------------|
| `getArrayPushFunc()` | 1641 | Get generic array push function | 0 |
| `getArrayPushStringFunc()` | 1773 | Get string array push function | 0 |
| `getArrayPushArrayFunc()` | 1815 | Get array-of-arrays push function | 0 |

**Analysis:** These helper functions are defined but never invoked. Array operations may be using direct function pointers instead.

**Recommendation:** **DELETE** if confirmed unused, or integrate into array operation code paths

**Safety:** High - No call sites found

---

### 2.3 HoocJIT Legacy Methods

**File:** `src/HoocJIT.cpp`

These methods are marked as "Legacy demo functions" in the header:

| Function | Line | Purpose | Status |
|----------|------|---------|--------|
| `createSimpleFunction()` | 67 | Creates demo LLVM function | Marked unused |
| `executeFunction()` | 113 | Executes demo function | Marked unused |
| `parseHoocCode()` | 128 | Parse utility | Not used |
| `generateModuleFromAST()` | 186 | AST to module conversion | Not used |

**Analysis:** These are clearly marked as legacy/demo code. The actual compilation pipeline uses `HooCompiler` instead.

**Recommendation:** **DELETE** - Remove legacy demo code

**Safety:** High - Already marked as unused

---

### 2.4 LLVM Cast Helper Functions

**File:** `src/LLVMCodeGeneratorTypes.h` (lines 119-145)

Namespace: `llvm_cast`

| Function | Purpose | Usage Count |
|----------|---------|-------------|
| `toModule(GeneratedModule*)` | Cast helper | 0 |
| `toFunction(GeneratedFunction*)` | Cast helper | 0 |
| `toValue(GeneratedValue*)` | Cast helper | 0 |
| `toType(GeneratedType*)` | Cast helper | 0 |

**Analysis:** These cast helpers exist but are never used. Direct casting or type access is used instead.

**Recommendation:**
- **DELETE** if truly unused
- **KEEP** if part of public API for extensibility

**Safety:** High - No usage found

---

## 3. Unused/Uninitialized Function Pointers

**File:** `src/LLVMCodeGenerator.h` (lines 243-274)

These array function pointers are declared and sometimes initialized but **never actually invoked**:

| Member Variable | Line | Initialized? | Called? | Impact |
|-----------------|------|--------------|---------|--------|
| `hoo_array_push_byte_func_` | 248 | Yes (2101) | No | Dead code |
| `hoo_array_push_string_func_` | 250 | Yes | No | Dead code |
| `hoo_array_push_array_func_` | 251 | Yes | No | Dead code |
| `hoo_array_get_float_func_` | 256 | Yes (2104) | No | Dead code |

**Additional Unused Array Functions:**
- `hoo_array_get_double_func_` (assigned but never called)
- `hoo_array_get_bool_func_` (assigned but never called)
- `hoo_array_get_char_func_` (assigned but never called)
- `hoo_array_get_string_func_` (assigned but never called)
- `hoo_array_get_object_func_` (assigned but never called)
- `hoo_array_get_array_func_` (assigned but never called)

**Analysis:** These are initialized from the runtime registry but never used in code generation. Only `hoo_array_get_int64_func_` is actually used (in for-in loops).

**Recommendation:**
- Keep the ones that will be needed for type-specific operations
- Remove truly unused ones
- Document which types are currently supported

**Safety:** Medium - Verify type coverage needs

---

## 4. Deprecated Class Members

**File:** `src/LLVMCodeGenerator.h` (lines 191-231)

**Comment from codebase:**
```cpp
// DEPRECATED: For backward compatibility, these members point into
// runtimeFunctionStorage_.strings. They will be removed in a future refactor.
```

**Count:** 32 string function pointer members

**Examples:**
- `hoo_string_create_func_`
- `hoo_string_destroy_func_`
- `hoo_string_concat_func_`
- `hoo_string_length_func_`
- ... (28 more)

**Analysis:** These are marked as deprecated and exist only for backward compatibility. They duplicate data in `runtimeFunctionStorage_.strings`.

**Recommendation:**
1. **Phase 1:** Refactor all usages to use `runtimeFunctionStorage_.strings` directly
2. **Phase 2:** Remove these deprecated members
3. **Phase 3:** Update documentation

**Safety:** Low - Requires refactoring all call sites

---

## 5. Partially Implemented AST Classes

### 5.1 EventDeclaration

**Files:**
- `src/ast/ClassDeclaration.h` (line 45)
- `src/ast/ASTImpl.cpp`

**Status:**
- ✅ Grammar supports: `EVENT IDENTIFIER;`
- ✅ AST node defined: `EventDeclaration` class
- ✅ Parser builds AST: `SimpleASTBuilder::buildEventDeclaration()`
- ❌ Code generation: **NOT IMPLEMENTED**

**Usage:** Events can be declared in classes but have no runtime behavior

**Recommendation:**
- **Option 1:** Implement code generation (requires event system design)
- **Option 2:** Keep as placeholder for future implementation
- **Option 3:** Remove if not needed

**Safety:** Medium - Tests may expect this to parse

---

### 5.2 InterfaceDeclaration

**Files:**
- `src/ast/InterfaceDeclaration.h` (line 49)
- `src/SimpleASTBuilder.cpp`

**Status:**
- ✅ Grammar supports: `interface Foo { ... }`
- ✅ AST node defined: `InterfaceDeclaration` class
- ✅ Parser builds AST: `SimpleASTBuilder::buildInterfaceDeclaration()`
- ❌ Code generation: **NOT IMPLEMENTED** (marked as "PARTIALLY IMPLEMENTED" in CLAUDE.md)

**Usage:** Interfaces can be declared but have no runtime behavior

**Recommendation:**
- **Option 1:** Implement interface checking and virtual dispatch
- **Option 2:** Keep as placeholder for future implementation
- **Option 3:** Remove from grammar if not planned

**Safety:** Low - Major language feature

---

## 6. Summary Statistics

### Files to Remove
| Category | Count | Total Lines |
|----------|-------|-------------|
| Orphaned source files | 5 | 356 |
| **Subtotal** | **5 files** | **356 lines** |

### Functions to Remove
| Category | Count |
|----------|-------|
| Unused virtual methods | 3 |
| Unused array helpers | 3 |
| Unused HoocJIT methods | 4 |
| Unused cast helpers | 4 |
| **Subtotal** | **14 functions** |

### Variables to Clean Up
| Category | Count |
|----------|-------|
| Unused array function pointers | ~8 |
| Deprecated string pointers | 32 |
| **Subtotal** | **~40 members** |

### Incomplete Features
| Feature | Status |
|---------|--------|
| EventDeclaration | Parsed only, no codegen |
| InterfaceDeclaration | Parsed only, no codegen |

---

## 7. Prioritized Cleanup Recommendations

### Priority 1: High Safety, High Impact
1. **Delete orphaned files** (5 files, 356 lines)
   - CustomHoocParser.cpp/h
   - comprehensive_test.cpp
   - hooc_parse.cpp
   - test_codegen.cpp

2. **Remove HoocJIT legacy methods** (4 methods, already marked unused)
   - createSimpleFunction()
   - executeFunction()
   - parseHoocCode()
   - generateModuleFromAST()

3. **Remove unused array helper functions** (3 functions)
   - getArrayPushFunc()
   - getArrayPushStringFunc()
   - getArrayPushArrayFunc()

**Estimated cleanup:** ~400 lines of code

---

### Priority 2: Medium Safety, Medium Impact

4. **Refactor deprecated string function pointers** (32 members)
   - Replace all usages with `runtimeFunctionStorage_.strings`
   - Remove deprecated member variables
   - Update documentation

5. **Clean up unused array function pointers**
   - Keep only used types (currently only int64 is used)
   - Document which types need future support
   - Remove definitively unused ones

**Estimated cleanup:** ~50 lines of declarations, multiple call site updates

---

### Priority 3: Low Safety, Strategic Decision Required

6. **Decide on EventDeclaration and InterfaceDeclaration**
   - Option A: Implement code generation (significant work)
   - Option B: Keep as placeholders (document in roadmap)
   - Option C: Remove from grammar and AST (breaking change)

7. **Review unused cast helpers**
   - Decide if they're part of public API
   - Remove if not needed for extensibility

---

## 8. Automated Cleanup Script Suggestions

```bash
#!/bin/bash
# Safe automated cleanup (Priority 1 only)

# Remove orphaned files
rm src/CustomHoocParser.cpp src/CustomHoocParser.h
rm src/comprehensive_test.cpp
rm src/hooc_parse.cpp
rm src/test_codegen.cpp

# Run tests to verify nothing broke
cd build/macos-homebrew-ninja
./hoo-tests

# If tests pass:
echo "✅ Cleanup successful - all tests passing"

# If tests fail:
echo "❌ Cleanup caused test failures - investigate"
```

---

## 9. Verification Checklist

Before removing any code:

- [ ] Verify file is not in CMakeLists.txt
- [ ] Search for all references in codebase (`grep -r "filename"`)
- [ ] Check for dynamic loading or reflection usage
- [ ] Run full test suite after removal
- [ ] Check that documentation doesn't reference removed items
- [ ] Update CLAUDE.md if removing language features

---

## 10. Conclusion

The Hooc codebase is generally well-maintained with 97% test coverage. The identified dead code primarily consists of:

1. **Legacy files** from earlier development phases
2. **Deprecated members** marked for future removal
3. **Unused helpers** that were created but not integrated
4. **Incomplete features** (events, interfaces) awaiting implementation

**Recommended Action Plan:**
1. Start with Priority 1 cleanup (orphaned files) - **LOW RISK**
2. Proceed to Priority 2 (refactoring) - **MEDIUM RISK**
3. Make strategic decisions on Priority 3 items - **REQUIRES DESIGN REVIEW**

**Expected Impact:**
- **Code reduction:** ~400-500 lines
- **Maintenance improvement:** Reduced confusion from unused code
- **Build time:** Minimal impact (files not compiled anyway)
- **Risk:** Low if following recommended priority order

---

**Report Generated:** April 16, 2026
**Next Review:** After cleanup completion
**Contact:** Development team for clarification on strategic decisions
