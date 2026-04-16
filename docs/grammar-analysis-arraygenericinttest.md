# Grammar Analysis for ArrayGenericIntegrationTest

**Date:** April 16, 2026
**Analyzed By:** Claude (AI Assistant)

## Executive Summary

The Hooc grammar **already supports** the syntax required by the failing `ArrayGenericIntegrationTest` tests. The test failures are **not due to grammar issues** but rather due to **implementation gaps** in the AST builder or code generator.

## Test Requirements

The failing tests in `ArrayGenericIntegrationTest` are:

1. **ArrayWithForInLoop** (line 381-398)
2. **ArrayWithForRangeLoop** (line 401-418)

### Required Syntax

#### Test 1: For-In Loop
```hooc
for item in arr {
    sum = sum + item;
}
```

#### Test 2: For-Range Loop
```hooc
for i in 0..5 {
    sum = sum + arr[i];
}
```

## Grammar Analysis

### Current Grammar Rules (src/Hooc.g4)

The grammar defines for-statement syntax at lines 215-218:

```antlr
forStatement
    : FOR IDENTIFIER IN expression block                    # forInStatement
    | FOR IDENTIFIER IN expression RANGE expression block   # forRangeStatement
    ;
```

This is properly included in the statement rule (line 203):

```antlr
statement
    : block
    | variableDeclarationStatement
    | expressionStatement
    | returnStatement
    | ifStatement
    | whileStatement
    | forStatement      // ← For loops are included
    | scopeStatement
    ;
```

### Syntax Matching

#### For-In Statement

**Grammar Rule:**
```
FOR IDENTIFIER IN expression block
```

**Test Code:**
```
for item in arr { sum = sum + item; }
```

**Token Mapping:**
- `FOR` → `for`
- `IDENTIFIER` → `item`
- `IN` → `in`
- `expression` → `arr`
- `block` → `{ sum = sum + item; }`

**Result:** ✅ **MATCHES PERFECTLY**

#### For-Range Statement

**Grammar Rule:**
```
FOR IDENTIFIER IN expression RANGE expression block
```

**Test Code:**
```
for i in 0..5 { sum = sum + arr[i]; }
```

**Token Mapping:**
- `FOR` → `for`
- `IDENTIFIER` → `i`
- `IN` → `in`
- `expression` → `0`
- `RANGE` → `..`
- `expression` → `5`
- `block` → `{ sum = sum + arr[i]; }`

**Result:** ✅ **MATCHES PERFECTLY**

## Why Tests Are Failing

Since the grammar correctly supports the required syntax, the test failures must be due to:

### 1. AST Builder Implementation

**Status:** ✅ **IMPLEMENTED**

The AST builder has methods for both for-loop types:
- `buildForInStatement()` (line 308-313 in SimpleASTBuilder.cpp)
- `buildForRangeStatement()` (line 315-321 in SimpleASTBuilder.cpp)

### 2. AST Node Definitions

**Status:** ✅ **DEFINED**

AST nodes exist for both types:
- `class ForInStatement` (ast/Statement.h:69)
- `class ForRangeStatement` (ast/Statement.h:89)

### 3. Code Generator

**Status:** ⚠️ **PARTIAL IMPLEMENTATION**

Code generation methods exist but have limitations:

**For-In Loop Issues:**
- Hardcoded array length of 10 (LLVMCodeGenerator.cpp:1118)
- Assumes int64 element type (line 1097)
- Cannot get actual array length from array object
- Cannot determine actual element type dynamically

**For-Range Loop Issues:**
- Implementation appears more complete (lines 1154-1203)
- Should work for simple integer ranges

### 4. Runtime Support

**Status:** ❌ **MISSING**

For-in loops over arrays require runtime support to:
- Query array length
- Query array element type
- Iterate over array elements type-safely

The current generic array system (`hoo_generic_array.cpp`) uses `std::any` for type erasure, making it difficult to:
- Get element type at codegen time
- Generate type-correct iteration code

## Root Cause

The for-in loop over arrays **cannot be fully implemented** with the current architecture because:

1. Arrays are opaque pointers at the LLVM IR level
2. Element type information is erased (stored in `std::any`)
3. Array length is not accessible without calling a runtime function
4. The codegen hardcodes assumptions (10 elements, int64 type)

## Recommendations

### Option 1: Fix Code Generator (Immediate)

Update `generateForInStatement()` in `LLVMCodeGenerator.cpp` to:

1. Call runtime function to get array length:
   ```cpp
   Function* getLengthFunc = module_->getFunction("hoo_array_length");
   Value* arrayLength = builder_->CreateCall(getLengthFunc, {iterableValue});
   ```

2. Call runtime function to get element at index:
   ```cpp
   Function* getElemFunc = module_->getFunction("hoo_array_get_int64");  // or type-specific
   Value* element = builder_->CreateCall(getElemFunc, {iterableValue, currentIndex});
   ```

3. Add runtime functions to `hoo_generic_array.cpp`:
   - `hoo_array_length(HooArray* arr) -> int64`
   - `hoo_array_get_int64(HooArray* arr, int64 idx) -> int64`
   - Similar functions for other types

### Option 2: Add Type Information to Arrays (Long-term)

Modify the array system to:
1. Store element type ID in array header
2. Provide runtime type queries
3. Generate element access code based on runtime type

### Option 3: Compile-Time Array Specialization (Best)

Generate specialized array types at compile time:
- `int64_array` with typed operations
- `double_array` with typed operations
- etc.

This approach:
- Eliminates runtime type erasure
- Allows direct memory access
- Enables efficient iteration
- Matches how generic classes currently work

## Grammar Changes Attempted

### Attempted Change: Generic Function Call Support

I attempted to add support for explicit type arguments in function calls:

```antlr
postfixSuffix
    : DOT IDENTIFIER
    | LBRACKET expression RBRACKET
    | typeArgumentList? LPAREN argumentList? RPAREN  // Added type arguments
    ;
```

This would enable syntax like:
```hooc
identity<int64>(42)
pair<int64, string>(1, "hello")
```

### Result: ❌ **FAILED**

The change introduced severe ambiguity:
- Parser couldn't distinguish `f<10` (comparison) from `f<Type>` (generic call)
- Broke ALL parsing, including simple array literals
- Caused 100% test failure rate

### Why It Failed

The `<` and `>` tokens are used for both:
1. Comparison operators (LESS, GREATER)
2. Type argument delimiters

ANTLR cannot disambiguate without:
- Complex lookahead predicates
- Semantic analysis
- Or different syntax (like Rust's `::<>` turbofish)

### Alternative Approaches for Generic Functions

1. **Use different delimiters:**
   ```hooc
   identity.[int64](42)    // Square brackets
   identity::<int64>(42)   // Turbofish (Rust-style)
   ```

2. **Require whitespace:**
   ```hooc
   identity <int64> (42)   // Spaces around <Type>
   ```

3. **Type inference only:**
   ```hooc
   identity(42)           // Infer int64 from argument
   ```

4. **Semantic predicates:**
   Add ANTLR predicates to check if `<` starts a type argument based on following tokens

## Conclusion

**For the ArrayGenericIntegrationTest tests:**
- ✅ Grammar is correct and complete
- ✅ AST building is implemented
- ⚠️ Code generation is partially implemented with hardcoded assumptions
- ❌ Runtime support for array iteration is incomplete

**Next Steps:**
1. DO NOT modify grammar - it's already correct
2. Fix code generator to call array runtime functions
3. Add array query functions to runtime (length, element access)
4. Update test expectations or implementation

**For generic function calls (different issue):**
- Grammar change creates unacceptable ambiguity
- Needs different syntax or complex disambiguation
- Should be addressed separately from for-loop issue

## Files Analyzed

- `/Users/benoybose/Projects/hooc/src/Hooc.g4` - Grammar file
- `/Users/benoybose/Projects/hooc/src/SimpleASTBuilder.h/cpp` - AST builder
- `/Users/benoybose/Projects/hooc/src/LLVMCodeGenerator.cpp` - Code generator
- `/Users/benoybose/Projects/hooc/src/ast/Statement.h` - AST node definitions
- `/Users/benoybose/Projects/hooc/tests/ArrayGenericIntegrationTest.cpp` - Tests

## See Also

- [Test Status Report](test-status-report-2026-04-16.md) - Full test analysis
- [CLAUDE.md](../CLAUDE.md) - Developer guide
- [Implementation Status](implementation-status.md) - Feature status
