// ============================================================================
// Runtime Class Registry - X-Macro Pattern
// ============================================================================
//
// NOTE: This file intentionally does NOT use include guards (#pragma once
// or #ifndef). It must be re-included multiple times with different macro
// definitions as part of the X-Macro pattern. Do not add include guards!
//
// CENTRAL REGISTRY: This is the SINGLE SOURCE OF TRUTH for all runtime
// classes that can be injected into the HoocJIT compiler.
//
// This file uses the X-Macro pattern to allow the same metadata to be
// expanded multiple times with different macro definitions, generating:
//   - JIT symbol registration code in HoocJIT.cpp
//   - LLVM function declarations in LLVMCodeGenerator.cpp
//   - Function pointer storage in LLVMCodeGenerator.h
//   - Operator overload dispatch tables in LLVMCodeGenerator.cpp
//
// ============================================================================
// HOW TO ADD A NEW RUNTIME CLASS
// ============================================================================
//
// 1. Implement the C API in runtime/hoo_xxx.{h,c}
//    - Define a handle type: typedef void* HooXxx;
//    - Implement all functions with prefix hoo_xxx_*
//
// 2. Add class definition to RUNTIME_CLASSES macro below:
//    DEFINE_RUNTIME_CLASS(ClassName, HandleType, DetectionPredicate)
//        BEGIN_RUNTIME_FUNCTIONS
//            RUNTIME_FUNCTION(func_name, ReturnType, LLVM_TYPE, (ParamType, LLVM_TYPE)...)
//            ...
//        END_RUNTIME_FUNCTIONS
//        BEGIN_RUNTIME_OPERATORS
//            RUNTIME_OPERATOR(AST_OPERATOR_ENUM, function_name)
//            ...
//        END_RUNTIME_OPERATORS
//
// 3. Rebuild the project
//    - All JIT registration
//    - All LLVM declarations
//    - All function pointer storage
//    - All operator dispatch
//    ...is auto-generated!
//
// ============================================================================
// Type Mapping Helpers
// ============================================================================

// These macros are only used within RuntimeClassRegistry.h to help define
// LLVM types. They assume 'context_' is available as the LLVM context.

#define LLVM_VOID  llvm::Type::getVoidTy(context_)
#define LLVM_I1    llvm::Type::getInt1Ty(context_)
#define LLVM_I8    llvm::Type::getInt8Ty(context_)
#define LLVM_I16   llvm::Type::getInt16Ty(context_)
#define LLVM_I32   llvm::Type::getInt32Ty(context_)
#define LLVM_I64   llvm::Type::getInt64Ty(context_)
#define LLVM_F32   llvm::Type::getFloatTy(context_)
#define LLVM_F64   llvm::Type::getDoubleTy(context_)
#define LLVM_PTR   llvm::PointerType::get(context_, 0)

// ============================================================================
// Macro Structure Documentation
// ============================================================================
//
// DEFINE_RUNTIME_CLASS(ClassName, HandleType, DetectionPredicate)
//   - ClassName: Name of the class (e.g., String, Array)
//   - HandleType: C type for the handle (e.g., HooString, HooArray)
//   - DetectionPredicate: LLVM type check method (e.g., isPointerTy())
//
// RUNTIME_FUNCTION(FuncName, ReturnType, LLVMReturnType, (...))
//   - FuncName: Function name without class prefix (e.g., concat, not hoo_string_concat)
//   - ReturnType: C return type (for documentation)
//   - LLVMReturnType: LLVM type expression (e.g., LLVM_PTR, LLVM_I64)
//   - (...): Parameter list as (CType, LLVMType) pairs, or empty for no params
//
// RUNTIME_OPERATOR(Operator, FuncName)
//   - Operator: AST operator enum (e.g., PLUS, EQUALS, LESS)
//   - FuncName: Function to call (without hoo_xxx_ prefix)
//     The framework will create: builder_->CreateCall(hoo_xxx_FuncName, ...)
//
// BEGIN_RUNTIME_FUNCTIONS / END_RUNTIME_FUNCTIONS
//   - Delimit the function list for each class
//
// BEGIN_RUNTIME_OPERATORS / END_RUNTIME_OPERATORS
//   - Delimit the operator overload list for each class

// ============================================================================
// RUNTIME CLASSES REGISTRY
// ============================================================================

#define RUNTIME_CLASSES \
    DEFINE_RUNTIME_CLASS(String, HooString, isPointerTy) \
        BEGIN_RUNTIME_FUNCTIONS \
            /* Creation functions */ \
            RUNTIME_FUNCTION(from_cstr, HooString, LLVM_PTR, \
                (const char*, LLVM_PTR)) \
            RUNTIME_FUNCTION(new, HooString, LLVM_PTR, ) \
            RUNTIME_FUNCTION(from_bytes, HooString, LLVM_PTR, \
                (const char*, LLVM_PTR), (int64_t, LLVM_I64)) \
            RUNTIME_FUNCTION(repeat, HooString, LLVM_PTR, \
                (char, LLVM_I8), (int64_t, LLVM_I64)) \
            \
            /* Manipulation functions */ \
            RUNTIME_FUNCTION(concat, HooString, LLVM_PTR, \
                (HooString, LLVM_PTR), (HooString, LLVM_PTR)) \
            RUNTIME_FUNCTION(substring, HooString, LLVM_PTR, \
                (HooString, LLVM_PTR), (int64_t, LLVM_I64), (int64_t, LLVM_I64)) \
            RUNTIME_FUNCTION(to_upper, HooString, LLVM_PTR, \
                (HooString, LLVM_PTR)) \
            RUNTIME_FUNCTION(to_lower, HooString, LLVM_PTR, \
                (HooString, LLVM_PTR)) \
            RUNTIME_FUNCTION(trim, HooString, LLVM_PTR, \
                (HooString, LLVM_PTR)) \
            RUNTIME_FUNCTION(replace, HooString, LLVM_PTR, \
                (HooString, LLVM_PTR), (HooString, LLVM_PTR), (HooString, LLVM_PTR)) \
            \
            /* Query functions */ \
            RUNTIME_FUNCTION(length, int64_t, LLVM_I64, \
                (HooString, LLVM_PTR)) \
            RUNTIME_FUNCTION(data, const char*, LLVM_PTR, \
                (HooString, LLVM_PTR)) \
            RUNTIME_FUNCTION(byte_at, uint8_t, LLVM_I8, \
                (HooString, LLVM_PTR), (int64_t, LLVM_I64)) \
            RUNTIME_FUNCTION(is_empty, int64_t, LLVM_I64, \
                (HooString, LLVM_PTR)) \
            RUNTIME_FUNCTION(index_of, int64_t, LLVM_I64, \
                (HooString, LLVM_PTR), (HooString, LLVM_PTR)) \
            RUNTIME_FUNCTION(last_index_of, int64_t, LLVM_I64, \
                (HooString, LLVM_PTR), (HooString, LLVM_PTR)) \
            RUNTIME_FUNCTION(contains, int64_t, LLVM_I64, \
                (HooString, LLVM_PTR), (HooString, LLVM_PTR)) \
            RUNTIME_FUNCTION(starts_with, int64_t, LLVM_I64, \
                (HooString, LLVM_PTR), (HooString, LLVM_PTR)) \
            RUNTIME_FUNCTION(ends_with, int64_t, LLVM_I64, \
                (HooString, LLVM_PTR), (HooString, LLVM_PTR)) \
            \
            /* Comparison functions */ \
            RUNTIME_FUNCTION(compare, int64_t, LLVM_I64, \
                (HooString, LLVM_PTR), (HooString, LLVM_PTR)) \
            RUNTIME_FUNCTION(equals, int64_t, LLVM_I64, \
                (HooString, LLVM_PTR), (HooString, LLVM_PTR)) \
            RUNTIME_FUNCTION(equals_ignore_case, int64_t, LLVM_I64, \
                (HooString, LLVM_PTR), (HooString, LLVM_PTR)) \
            \
            /* Reference counting */ \
            RUNTIME_FUNCTION(retain, void, LLVM_VOID, \
                (HooString, LLVM_PTR)) \
            RUNTIME_FUNCTION(release, void, LLVM_VOID, \
                (HooString, LLVM_PTR)) \
            RUNTIME_FUNCTION(refcount, int64_t, LLVM_I64, \
                (HooString, LLVM_PTR)) \
            \
            /* Conversion functions */ \
            RUNTIME_FUNCTION(from_int64, HooString, LLVM_PTR, \
                (int64_t, LLVM_I64)) \
            RUNTIME_FUNCTION(from_double, HooString, LLVM_PTR, \
                (double, LLVM_F64)) \
            RUNTIME_FUNCTION(from_bool, HooString, LLVM_PTR, \
                (int64_t, LLVM_I64)) \
            RUNTIME_FUNCTION(to_int64, int64_t, LLVM_I64, \
                (HooString, LLVM_PTR)) \
            RUNTIME_FUNCTION(to_double, double, LLVM_F64, \
                (HooString, LLVM_PTR)) \
            RUNTIME_FUNCTION(format, HooString, LLVM_PTR, \
                (const char*, LLVM_PTR)) \
            \
            /* Debugging */ \
            RUNTIME_FUNCTION(print, void, LLVM_VOID, \
                (HooString, LLVM_PTR)) \
            RUNTIME_FUNCTION(println, void, LLVM_VOID, \
                (HooString, LLVM_PTR)) \
            RUNTIME_FUNCTION(debug, void, LLVM_VOID, \
                (HooString, LLVM_PTR)) \
        END_RUNTIME_FUNCTIONS \
        \
        BEGIN_RUNTIME_OPERATORS \
            RUNTIME_OPERATOR(PLUS, concat) \
            RUNTIME_OPERATOR(EQUALS, equals) \
            RUNTIME_OPERATOR(NOT_EQUALS, equals) \
            RUNTIME_OPERATOR(LESS, compare) \
            RUNTIME_OPERATOR(LESS_EQUALS, compare) \
            RUNTIME_OPERATOR(GREATER, compare) \
            RUNTIME_OPERATOR(GREATER_EQUALS, compare) \
        END_RUNTIME_OPERATORS
