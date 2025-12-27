#pragma once

// ============================================================================
// Runtime Class Code Generation Helpers
// ============================================================================
//
// This header provides macro patterns for code generation based on the
// RUNTIME_CLASSES registry. These patterns are used in HoocJIT.cpp and
// LLVMCodeGenerator.cpp to auto-generate:
//   - JIT symbol registration
//   - LLVM function declarations
//   - Function pointer storage
//   - Operator dispatch tables
//
// IMPORTANT: These are helper patterns, not complete implementations.
// The actual code generation happens inline in HoocJIT.cpp and
// LLVMCodeGenerator.cpp where they expand RUNTIME_CLASSES with different
// macro definitions.

// ============================================================================
// LLVM Type Conversion Helper
// ============================================================================

// When parameter count handling is needed, this checks arity
#define COUNT_RUNTIME_FUNCTION_PARAMS(...) \
    (sizeof((void*[]){__VA_ARGS__})/sizeof(void*))

// ============================================================================
// Code Generation Patterns
// ============================================================================

// Pattern 1: JIT Symbol Registration
// ============================================================================
// In HoocJIT.cpp, these patterns are used:
//
//   #define DEFINE_RUNTIME_CLASS(ClassName, ...) \
//       void HoocJIT::register##ClassName##Functions() { \
//           auto& mainJD = JIT->getMainJITDylib(); \
//           llvm::orc::SymbolMap symbols; \
//           #define CURRENT_CLASS ClassName
//
//   #define RUNTIME_FUNCTION(FuncName, ...) \
//       symbols[JIT->mangleAndIntern("hoo_" #CURRENT_CLASS "_" #FuncName)] = \
//           llvm::orc::ExecutorSymbolDef( \
//               llvm::orc::ExecutorAddr::fromPtr(&hoo_##CURRENT_CLASS##_##FuncName), \
//               JITSymbolFlags::Exported);
//
//   #define END_RUNTIME_FUNCTIONS \
//           auto Err = mainJD.define(absoluteSymbols(symbols)); \
//           if (Err) { errs() << "Failed to register symbols\n"; exit(1); } \
//           #undef CURRENT_CLASS \
//       }
//
//   RUNTIME_CLASSES  // Expands to full registrations

// Pattern 2: LLVM Function Declaration
// ============================================================================
// In LLVMCodeGenerator.cpp, these patterns are used:
//
//   #define DEFINE_RUNTIME_CLASS(ClassName, ...) \
//       void LLVMCodeGenerator::declare##ClassName##Functions() { \
//           #define CURRENT_CLASS ClassName
//
//   #define RUNTIME_FUNCTION(FuncName, RetType, LLVMRetType, ...) \
//       if (!hoo_##CURRENT_CLASS##_##FuncName##_func_) { \
//           std::vector<llvm::Type*> params = {__VA_ARGS__}; \
//           FunctionType* funcType = FunctionType::get(LLVMRetType, params, false); \
//           hoo_##CURRENT_CLASS##_##FuncName##_func_ = Function::Create( \
//               funcType, Function::ExternalLinkage, \
//               "hoo_" #CURRENT_CLASS "_" #FuncName, module_.get()); \
//       }
//
//   #define END_RUNTIME_FUNCTIONS \
//           #undef CURRENT_CLASS \
//       }
//
//   RUNTIME_CLASSES  // Expands to full declarations

// Pattern 3: Function Pointer Storage
// ============================================================================
// In LLVMCodeGenerator.h private section:
//
//   #define DEFINE_RUNTIME_CLASS(ClassName, ...)
//   #define BEGIN_RUNTIME_FUNCTIONS
//   #define END_RUNTIME_FUNCTIONS
//   #define RUNTIME_FUNCTION(FuncName, ...) \
//       llvm::Function* hoo_String_##FuncName##_func_ = nullptr;
//   #define BEGIN_RUNTIME_OPERATORS
//   #define END_RUNTIME_OPERATORS
//   #define RUNTIME_OPERATOR(...)
//
//   RUNTIME_CLASSES  // Expands to all function pointer members

// Pattern 4: Operator Dispatch
// ============================================================================
// In LLVMCodeGenerator.cpp after function declarations:
//
//   #define DEFINE_RUNTIME_CLASS(ClassName, HandleType, DetectionPredicate) \
//       llvm::Value* LLVMCodeGenerator::try##ClassName##Operator( \
//           ast::BinaryOperator op, llvm::Value* left, llvm::Value* right) \
//       { \
//           if (!left->getType()->DetectionPredicate() || \
//               !right->getType()->DetectionPredicate()) { \
//               return nullptr; \
//           } \
//           declare##ClassName##Functions(); \
//           switch (op) {
//
//   #define RUNTIME_OPERATOR(Op, FuncName) \
//       case ast::BinaryOperator::Op: { \
//           if (!hoo_##CURRENT_CLASS##_##FuncName##_func_) return nullptr; \
//           Value* result = builder_->CreateCall( \
//               hoo_##CURRENT_CLASS##_##FuncName##_func_, {left, right}, \
//               #FuncName "_result"); \
//           /* Apply type conversions if needed */ \
//           if (result->getType()->isInt64Ty()) { \
//               return builder_->CreateICmpNE(result, \
//                   ConstantInt::get(llvm::Type::getInt64Ty(context_), 0)); \
//           } \
//           return result; \
//       }
//
//   #define END_RUNTIME_OPERATORS \
//               default: return nullptr; \
//           } \
//       }
//
//   #define BEGIN_RUNTIME_FUNCTIONS
//   #define BEGIN_RUNTIME_OPERATORS /* Mark section */ \
//       #define CURRENT_CLASS String  // For each class
//   #define END_RUNTIME_OPERATORS
//
//   RUNTIME_CLASSES

// ============================================================================
// Integration Checklist
// ============================================================================
//
// When integrating with a new file:
// 1. Include "runtime/RuntimeClassRegistry.h"
// 2. Define the macro meanings for your use case
// 3. Expand RUNTIME_CLASSES to generate code
// 4. Undefine all macro names to avoid conflicts
// 5. Clean up CURRENT_CLASS and other temporaries

#endif // RUNTIME_CLASS_CODEGEN_H
